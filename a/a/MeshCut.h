#pragma once

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

        raw.faces.reserve(
            polygonList.PolygonNum);

        // 頂点
        for (int i = 0;
            i < polygonList.VertexNum;
            ++i)
        {
            raw.verts.push_back(
                polygonList.Vertexs[i].Position);
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
            const std::vector<std::array<int, 3>>& faces)
        {
            Mesh out;

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
        // 頂点
        // --------------------------------------------------------

        std::vector<VECTOR> verts =
            src.verts;


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


                        for (int index :
                        loop)
                        {
                            center =
                                VAdd(
                                    center,
                                    verts[index]);
                        }


                        center =
                            VScale(
                                center,
                                1.0f /
                                (float)loop.size());


                        verts.push_back(
                            center);


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
                positiveFaces);


        outNeg =
            detail::Compact(
                verts,
                negativeFaces);
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