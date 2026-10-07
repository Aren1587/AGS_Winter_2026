#pragma once

// ================================================================
//  MeshCut : 三角形メッシュを平面で切断する処理
// ================================================================
//
//  使い方:
//      MeshCut::Mesh pos, neg;
//      MeshCut::Cut(mesh, 平面上の点, 平面の法線, pos, neg, true);
//
//  仕様:
//    - 平面の法線が向いている側を「正側 (pos)」、反対側を「負側 (neg)」とする
//    - 切断でできた頂点(交点)は隣り合う三角形どうしで共有されるので、
//      切り口に隙間ができない
//    - cap = true なら切断面にフタ(穴をふさぐ三角形)を付ける。
//      フタは「重心からのファン分割」なので、断面が凸のときだけ正しい
//    - 入力メッシュは「閉じていて、三角形の向き(巻き方向)が揃っている」こと
//
//  DxLib 以外のプロジェクトで使う場合は、VECTOR / VGet / VAdd / VSub / VScale /
//  VDot / VCross / VSize / VNorm を、使う環境のベクトル型・関数に置き換えること。
//  この置き換え以外に DxLib への依存はない (MV1 関連の関数は FromMV1 の中だけ)。
// ================================================================

#include "DxLib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MeshCut
{
    // ============================================================
    // メッシュ
    // ============================================================

    struct Mesh
    {
        std::vector<VECTOR> verts;
        std::vector<std::array<int, 3>> faces;

        std::vector<float> u;
        std::vector<float> v;

        int textureHandle = -1;

        // 頂点ごとの色 (MV1 のディフューズカラー、つまり元のテクスチャ・マテリアルの
        // 色をそのまま持ってきたもの)。verts と同じ数だけあるか、空(色情報なし)のどちらか。
        std::vector<COLOR_U8> colors;

        // 追加: 描画用の頂点 (3頂点 × 面数。破片のローカル座標)
        std::vector<VERTEX3D> drawVerts;
    };


    // ============================================================
    // 重複頂点をまとめる
    // ============================================================

    inline Mesh Weld(
        const Mesh& src,
        float tol = 1e-4f)
    {
        using Key =
            std::tuple<long long, long long, long long>;

        std::map<Key, int> table;

        std::vector<int> remap(
            src.verts.size());

        Mesh out;

        bool hasColor =
            !src.colors.empty();

        out.textureHandle = src.textureHandle;

        for (size_t i = 0;
            i < src.verts.size();
            ++i)
        {
            const VECTOR& p =
                src.verts[i];

            Key key =
            {
                std::llround(p.x / tol),
                std::llround(p.y / tol),
                std::llround(p.z / tol)
            };

            auto it =
                table.find(key);

            if (it == table.end())
            {
                it =
                    table.emplace(
                        key,
                        (int)out.verts.size()
                    ).first;

                out.verts.push_back(p);

                out.u.push_back(src.u[i]);
                out.v.push_back(src.v[i]);

                if (hasColor)
                {
                    out.colors.push_back(src.colors[i]);
                }
            }

            remap[i] =
                it->second;
        }

        for (const auto& face :
            src.faces)
        {
            std::array<int, 3> f =
            {
                remap[face[0]],
                remap[face[1]],
                remap[face[2]]
            };

            if (f[0] != f[1] &&
                f[1] != f[2] &&
                f[2] != f[0])
            {
                out.faces.push_back(f);
            }
        }

        return out;
    }


    // ============================================================
    // MV1 → Mesh
    // ============================================================

    inline Mesh FromMV1(
        int modelHandle)
    {
        MV1SetupReferenceMesh(
            modelHandle,
            -1,
            TRUE);

        MV1_REF_POLYGONLIST polygonList =
            MV1GetReferenceMesh(
                modelHandle,
                -1,
                TRUE);

        Mesh raw;

        raw.verts.reserve(
            polygonList.VertexNum);

        raw.colors.reserve(
            polygonList.VertexNum);

        raw.u.reserve(
            polygonList.VertexNum);

        raw.v.reserve(
            polygonList.VertexNum);

        raw.faces.reserve(
            polygonList.PolygonNum);

        raw.textureHandle = MV1GetTextureGraphHandle(modelHandle, 0);

        // 頂点 (座標と色)
        // DiffuseColor は、頂点カラーが無いモデルでは
        // そのままマテリアル(テクスチャ)の色が入っている
        for (int i = 0;
            i < polygonList.VertexNum;
            ++i)
        {
            raw.verts.push_back(
                polygonList.Vertexs[i].Position);

            raw.colors.push_back(
                polygonList.Vertexs[i].DiffuseColor);

            raw.u.push_back(
                polygonList.Vertexs[i].TexCoord->u);

            raw.v.push_back(
                polygonList.Vertexs[i].TexCoord->v);
        }

        // ポリゴン
        for (int i = 0;
            i < polygonList.PolygonNum;
            ++i)
        {
            const auto& polygon =
                polygonList.Polygons[i];

            raw.faces.push_back(
                {
                    polygon.VIndex[0],
                    polygon.VIndex[1],
                    polygon.VIndex[2]
                });
        }

        MV1TerminateReferenceMesh(
            modelHandle,
            -1,
            TRUE);

        return Weld(raw);
    }

    namespace detail
    {
        // ========================================================
        // 切断線をつなげてループにする
        // ========================================================

        inline std::vector<std::vector<int>> Chain(
            const std::vector<std::pair<int, int>>& edges)
        {
            std::unordered_map<int, int> next;

            for (const auto& edge : edges)
            {
                next[edge.first] =
                    edge.second;
            }

            std::unordered_map<int, bool> visited;

            std::vector<std::vector<int>> loops;

            for (const auto& pair : next)
            {
                int start =
                    pair.first;

                if (visited[start])
                    continue;

                std::vector<int> loop;

                loop.push_back(start);

                visited[start] = true;

                int current =
                    pair.second;

                while (
                    current != start &&
                    !visited[current])
                {
                    loop.push_back(current);

                    visited[current] = true;

                    auto it =
                        next.find(current);

                    if (it == next.end())
                    {
                        current = -1;
                        break;
                    }

                    current =
                        it->second;
                }

                if (
                    current == start &&
                    loop.size() >= 3)
                {
                    loops.push_back(
                        std::move(loop));
                }
            }

            return loops;
        }


        // ========================================================
        // 使用されていない頂点を削除
        // ========================================================

        inline Mesh Compact(
            const std::vector<VECTOR>& verts,
            const std::vector<COLOR_U8>& colors,
            const std::vector<float>& u,
            const std::vector<float>& v,
            const std::vector<std::array<int, 3>>& faces,
            int textureHandle)
        {
            Mesh out;

            bool hasColor =
                !colors.empty();

            out.textureHandle =
                textureHandle;

            std::vector<int> remap(
                verts.size(),
                -1);

            for (const auto& face : faces)
            {
                std::array<int, 3> newFace;

                for (int i = 0;
                    i < 3;
                    ++i)
                {
                    int index =
                        face[i];

                    if (remap[index] < 0)
                    {
                        remap[index] =
                            (int)out.verts.size();

                        out.verts.push_back(
                            verts[index]);

                        // UV
                        out.u.push_back(
                            u[index]);

                        out.v.push_back(
                            v[index]);

                        // 色
                        if (hasColor)
                        {
                            out.colors.push_back(
                                colors[index]);
                        }
                    }

                    newFace[i] =
                        remap[index];
                }

                out.faces.push_back(
                    newFace);
            }

            return out;
        }
    }


    // ============================================================
    // メッシュを平面で切断
    //
    // origin : 切断平面上の点
    // normal : 切断平面の法線
    //
    // outPos : 法線方向側
    // outNeg : 法線と反対側
    // ============================================================

    inline void Cut(
        const Mesh& src,
        VECTOR origin,
        VECTOR normal,
        Mesh& outPos,
        Mesh& outNeg,
        bool cap = true,
        float eps = 1e-6f)
    {
        normal =
            VNorm(normal);

        // --------------------------------------------------------
        // 各頂点が平面のどちら側にあるか
        // --------------------------------------------------------

        std::vector<float> distance(
            src.verts.size());

        for (size_t i = 0;
            i < distance.size();
            ++i)
        {
            distance[i] =
                VDot(
                    VSub(
                        src.verts[i],
                        origin),
                    normal);

            if (std::fabs(distance[i]) < eps)
            {
                distance[i] = eps;
            }
        }

        // --------------------------------------------------------
        // 頂点 (色は src に無ければ空のまま)
        // --------------------------------------------------------

        std::vector<VECTOR> verts =
            src.verts;

        std::vector<COLOR_U8> colors =
            src.colors;

        std::vector<float> u =
            src.u;

        std::vector<float> v =
            src.v;

        bool hasColor =
            !colors.empty();


        // 2色を t (0〜1) で線形補間する
        auto LerpColor =
            [](COLOR_U8 a, COLOR_U8 b, float t)
            {
                COLOR_U8 c;

                c.r = (unsigned char)(a.r + (b.r - a.r) * t);
                c.g = (unsigned char)(a.g + (b.g - a.g) * t);
                c.b = (unsigned char)(a.b + (b.b - a.b) * t);
                c.a = (unsigned char)(a.a + (b.a - a.a) * t);

                return c;
            };


        // --------------------------------------------------------
        // 辺と平面の交点キャッシュ
        // --------------------------------------------------------

        std::map<
            std::pair<int, int>,
            int> intersectionCache;


        auto GetIntersection =
            [&](int i, int j)
            {
                std::pair<int, int> key;

                if (i < j)
                {
                    key =
                    {
                        i,
                        j
                    };
                }
                else
                {
                    key =
                    {
                        j,
                        i
                    };
                }


                auto it =
                    intersectionCache.find(key);

                if (it !=
                    intersectionCache.end())
                {
                    return it->second;
                }


                int a =
                    key.first;

                int b =
                    key.second;


                float t =
                    distance[a] /
                    (distance[a] -
                        distance[b]);


                VECTOR position =
                    VAdd(
                        src.verts[a],
                        VScale(
                            VSub(
                                src.verts[b],
                                src.verts[a]),
                            t));


                verts.push_back(
                    position);

                // UVを補間
                float newU =
                    src.u[a] +
                    (src.u[b] - src.u[a]) * t;

                float newV =
                    src.v[a] +
                    (src.v[b] - src.v[a]) * t;

                u.push_back(newU);
                v.push_back(newV);

                if (hasColor)
                {
                    colors.push_back(
                        LerpColor(
                            src.colors[a],
                            src.colors[b],
                            t));
                }


                int index =
                    (int)verts.size() - 1;


                intersectionCache[key] =
                    index;


                return index;
            };


        // --------------------------------------------------------
        // 切断後の面
        // --------------------------------------------------------

        std::vector<std::array<int, 3>>
            positiveFaces;

        std::vector<std::array<int, 3>>
            negativeFaces;


        // 切断面の辺
        std::vector<std::pair<int, int>>
            cutEdges;


        // --------------------------------------------------------
        // 全三角形を処理
        // --------------------------------------------------------

        for (const auto& triangle :
            src.faces)
        {
            bool side[3] =
            {
                distance[triangle[0]] > 0,
                distance[triangle[1]] > 0,
                distance[triangle[2]] > 0
            };


            int positiveCount =
                (int)side[0] +
                (int)side[1] +
                (int)side[2];


            // 全部正側
            if (positiveCount == 3)
            {
                positiveFaces.push_back(
                    triangle);

                continue;
            }


            // 全部負側
            if (positiveCount == 0)
            {
                negativeFaces.push_back(
                    triangle);

                continue;
            }


            // ----------------------------------------------------
            // 平面をまたいでいる三角形
            // ----------------------------------------------------

            int loneIndex = 0;

            for (int i = 0;
                i < 3;
                ++i)
            {
                if (
                    side[i] ==
                    (positiveCount == 1))
                {
                    loneIndex = i;
                    break;
                }
            }


            int a =
                triangle[loneIndex];

            int b =
                triangle[
                    (loneIndex + 1) % 3];

            int c =
                triangle[
                    (loneIndex + 2) % 3];


            int ab =
                GetIntersection(a, b);

            int ca =
                GetIntersection(c, a);


            std::array<int, 3> lone =
            {
                a,
                ab,
                ca
            };


            std::array<int, 3> triangle0 =
            {
                ab,
                b,
                c
            };


            std::array<int, 3> triangle1 =
            {
                ab,
                c,
                ca
            };


            if (distance[a] > 0)
            {
                positiveFaces.push_back(
                    lone);

                negativeFaces.push_back(
                    triangle0);

                negativeFaces.push_back(
                    triangle1);


                cutEdges.emplace_back(
                    ab,
                    ca);
            }
            else
            {
                negativeFaces.push_back(
                    lone);

                positiveFaces.push_back(
                    triangle0);

                positiveFaces.push_back(
                    triangle1);


                cutEdges.emplace_back(
                    ca,
                    ab);
            }
        }


        // --------------------------------------------------------
        // 切断面を作る
        // --------------------------------------------------------

        if (
            cap &&
            !cutEdges.empty())
        {
            auto AddCaps =
                [&](const std::vector<std::vector<int>>& loops,
                    std::vector<std::array<int, 3>>& output)
                {
                    for (const auto& loop :
                        loops)
                    {
                        VECTOR center =
                            VGet(0, 0, 0);

                        float centerU = 0.0f;
                        float centerV = 0.0f;

                        for (int index : loop)
                        {
                            center =
                                VAdd(
                                    center,
                                    verts[index]);

                            centerU += u[index];
                            centerV += v[index];
                        }


                        center =
                            VScale(
                                center,
                                1.0f /
                                (float)loop.size());

                        centerU /=
                            (float)loop.size();

                        centerV /=
                            (float)loop.size();

                        verts.push_back(
                            center);

                        u.push_back(centerU);
                        v.push_back(centerV);

                        if (hasColor)
                        {
                            int r = 0, g = 0, b = 0, a = 0;

                            for (int index : loop)
                            {
                                r += colors[index].r;
                                g += colors[index].g;
                                b += colors[index].b;
                                a += colors[index].a;
                            }

                            COLOR_U8 avg;

                            avg.r = (unsigned char)(r / (int)loop.size());
                            avg.g = (unsigned char)(g / (int)loop.size());
                            avg.b = (unsigned char)(b / (int)loop.size());
                            avg.a = (unsigned char)(a / (int)loop.size());

                            colors.push_back(avg);
                        }


                        int centerIndex =
                            (int)verts.size() - 1;


                        for (size_t i = 0;
                            i < loop.size();
                            ++i)
                        {
                            output.push_back(
                                {
                                    centerIndex,
                                    loop[i],
                                    loop[
                                        (i + 1) %
                                        loop.size()]
                                });
                        }
                    }
                };


            std::vector<std::pair<int, int>>
                reverseEdges;


            for (const auto& edge :
                cutEdges)
            {
                reverseEdges.emplace_back(
                    edge.second,
                    edge.first);
            }


            AddCaps(
                detail::Chain(reverseEdges),
                positiveFaces);


            AddCaps(
                detail::Chain(cutEdges),
                negativeFaces);
        }


        // --------------------------------------------------------
        // 余分な頂点を整理
        // --------------------------------------------------------

        outPos =
            detail::Compact(
                verts,
                colors,
                u,
                v,
                positiveFaces,
                src.textureHandle);

        outNeg =
            detail::Compact(
                verts,
                colors,
                u,
                v,
                negativeFaces,
                src.textureHandle);
    }


    // ============================================================
    // メッシュの体積
    // ============================================================

    inline float SignedVolume(
        const Mesh& mesh)
    {
        double volume = 0.0;

        for (const auto& face :
            mesh.faces)
        {
            volume +=
                VDot(
                    mesh.verts[face[0]],
                    VCross(
                        mesh.verts[face[1]],
                        mesh.verts[face[2]]));
        }

        return (float)(
            volume / 6.0);
    }
}