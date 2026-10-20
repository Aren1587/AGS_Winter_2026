#include "MeshCutter.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    // axis(単位ベクトル)まわりに angle 回すロドリゲスの回転を MATRIX にする。
    // 外積を使う重力トルクの計算と同じ向きの規約にそろえるため、
    // MGetRotAxis には頼らず自前で組み立てる。
    // (行ベクトル規約: 行 i = 基底ベクトル e_i を回した結果)
    MATRIX MakeAxisRotation(
        VECTOR axis,
        float angle)
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        MATRIX m = MGetIdent();

        for (int i = 0; i < 3; i++)
        {
            VECTOR e = VGet(i == 0 ? 1.0f : 0.0f,
                i == 1 ? 1.0f : 0.0f,
                i == 2 ? 1.0f : 0.0f);

            VECTOR r = VAdd(
                VAdd(VScale(e, c), VScale(VCross(axis, e), s)),
                VScale(axis, VDot(axis, e) * (1.0f - c)));

            m.m[i][0] = r.x;
            m.m[i][1] = r.y;
            m.m[i][2] = r.z;
        }

        return m;
    }
}


// ================================================================
// メッシュを設定
// ================================================================

void MeshCutter::SetMesh(
    const MeshCut::Mesh& mesh)
{
    originalMesh_ = mesh;

    // バウンディングボックスを求め直す (GetCenter/GetRadius/SetFloorFromMesh が使う)
    boundsMin_ = mesh.verts[0];
    boundsMax_ = mesh.verts[0];

    for (const auto& v : mesh.verts)
    {
        boundsMin_ = VGet(
            std::fmin(boundsMin_.x, v.x),
            std::fmin(boundsMin_.y, v.y),
            std::fmin(boundsMin_.z, v.z));

        boundsMax_ = VGet(
            std::fmax(boundsMax_.x, v.x),
            std::fmax(boundsMax_.y, v.y),
            std::fmax(boundsMax_.z, v.z));
    }

    pieces_.clear();

    colorCounter_ = 0;

    pieces_.push_back(
        MakePiece(MeshCut::Mesh(mesh), VGet(0, 0, 0), VGet(0, 0, 0)));
}


// ================================================================
// 元の状態に戻す
// ================================================================

void MeshCutter::Reset()
{
    pieces_.clear();

    colorCounter_ = 0;

    pieces_.push_back(
        MakePiece(MeshCut::Mesh(originalMesh_), VGet(0, 0, 0), VGet(0, 0, 0)));
}


// ================================================================
// 切断 (押し出し速度は pushSpeed_ を使う)
// ================================================================

void MeshCutter::Cut(
    VECTOR origin,
    VECTOR normal)
{
    Cut(
        origin,
        normal,
        pushSpeed_);
}


// ================================================================
// 切断
// ================================================================

void MeshCutter::Cut(
    VECTOR origin,
    VECTOR normal,
    float pushSpeed)
{
    if (VSize(normal) < 1e-6f)
        return;

    normal = VNorm(normal);

    // 切断面から離れる側へ倒れ込む回転軸。
    // +normal 側の破片は +軸、-normal 側の破片は -軸で回すと、
    // それぞれ上端が自分の側へ傾いていく
    const VECTOR spinAxis = rotationEnabled_
        ? VScale(VCross(VGet(0, 1, 0), normal), spin_)
        : VGet(0, 0, 0);

    std::vector<Piece> next;

    // 現在存在する全破片を切る
    for (auto& piece : pieces_)
    {
        // 現在の位置と回転をメッシュに反映してから切る
        // (そうしないと、見えている位置と切断平面の位置がずれてしまう)
        for (auto& vertex : piece.mesh.verts)
            vertex = VAdd(VTransform(vertex, piece.rot), piece.pos);

        MeshCut::Mesh positive;
        MeshCut::Mesh negative;

        MeshCut::Cut(
            piece.mesh,
            origin,
            normal,
            positive,
            negative,
            capEnabled_);

        // 平面をまたいでいなかった場合はそのまま残す
        // (頂点はワールド座標に直したので、重心基準へ組み直す。
        //  速度・角速度・色は引き継ぐ)
        if (positive.faces.empty() || negative.faces.empty())
        {
            next.push_back(MakePiece(
                std::move(piece.mesh), piece.vel, piece.angVel, piece.color));
            continue;
        }

        next.push_back(MakePiece(std::move(positive), VScale(normal, pushSpeed), spinAxis));
        next.push_back(MakePiece(std::move(negative), VScale(normal, -pushSpeed), VScale(spinAxis, -1.0f)));
    }

    pieces_ = std::move(next);
}


// ================================================================
// 更新 (重力・回転・床判定・移動)
// ================================================================

void MeshCutter::Update(
    float deltaTime)
{
    const float gravity = gravityEnabled_ ? gravityValue_ : 0.0f;

    for (auto& piece : pieces_)
    {
        // 重力
        piece.vel.y -= gravity * deltaTime;

        // 減衰 (damping_ が 1.0 なら何もしない)
        piece.vel = VScale(piece.vel, damping_);

        // 移動
        piece.pos = VAdd(piece.pos, VScale(piece.vel, deltaTime));

        // 回転 (ワールド軸まわりの回転を後ろから掛ける)
        if (rotationEnabled_)
        {
            piece.angVel = VScale(piece.angVel, damping_);

            const float w = VSize(piece.angVel);

            if (w > 1e-6f)
            {
                piece.rot = MMult(
                    piece.rot,
                    MakeAxisRotation(VScale(piece.angVel, 1.0f / w), w * deltaTime));

                Orthonormalize(piece.rot);
            }
        }

        if (!floorEnabled_)
            continue;

        // 床との当たり判定: 回転後の一番低い点が床より下に来たら押し戻す
        float lowest = 1e30f;

        for (const auto& v : piece.mesh.verts)
            lowest = std::fmin(lowest, VTransform(v, piece.rot).y);

        const float worldMinY = piece.pos.y + lowest;

        if (worldMinY >= floorY_)
            continue;

        piece.pos.y += floorY_ - worldMinY;

        if (piece.vel.y < 0.0f)
            piece.vel.y = -piece.vel.y * bounce_;

        piece.vel.x *= friction_;
        piece.vel.z *= friction_;

        // 跳ね返りが十分小さくなったら、完全に止める
        if (std::fabs(piece.vel.y) < gravity * deltaTime * 2.0f + 1e-5f)
            piece.vel.y = 0.0f;

        // 重心が支えの外にあれば倒れる
        if (rotationEnabled_ && gravity > 0.0f)
            ApplyTipping(piece, lowest, gravity, deltaTime);
    }
}


// ================================================================
// 床に着いている破片の倒れ込み
//
//   床に触れている頂点 (一番低い点から eps 以内) の平均を支点とし、
//   支点から重心へのベクトルに重力が掛かったときのトルクで回す。
//   接触点が3つ以上あって、重心が接触点の広がりの内側にある場合は
//   安定しているとみなして倒さない
//   (底が平らな破片や、足で立っている破片がゆっくり倒れるのを防ぐ)。
// ================================================================

void MeshCutter::ApplyTipping(
    Piece& piece,
    float lowest,
    float gravity,
    float deltaTime) const
{
    const float eps = contactEps_ > 0.0f ? contactEps_ : GetRadius() * 0.02f;

    // 接触点の平均
    VECTOR sum = VGet(0, 0, 0);
    int count = 0;

    for (const auto& v : piece.mesh.verts)
    {
        VECTOR p = VTransform(v, piece.rot);

        if (p.y <= lowest + eps)
        {
            sum = VAdd(sum, p);
            count++;
        }
    }

    if (count == 0)
        return;

    const VECTOR contact = VScale(sum, 1.0f / count);

    // 接触点の水平方向の広がり (支えの半径)
    float spread2 = 0.0f;

    for (const auto& v : piece.mesh.verts)
    {
        VECTOR p = VTransform(v, piece.rot);

        if (p.y <= lowest + eps)
        {
            const float dx = p.x - contact.x;
            const float dz = p.z - contact.z;
            spread2 = std::fmax(spread2, dx * dx + dz * dz);
        }
    }

    // 支点 → 重心 (重心はローカル原点なので -contact)
    const VECTOR toCom = VScale(contact, -1.0f);
    const float offset2 = toCom.x * toCom.x + toCom.z * toCom.z;

    const bool stable = count >= 3 && offset2 < spread2 * 0.25f;

    if (!stable)
    {
        const VECTOR torque = VCross(toCom, VGet(0.0f, -gravity, 0.0f));

        piece.angVel = VAdd(
            piece.angVel,
            VScale(torque, deltaTime / std::fmax(piece.radius2, 1e-6f)));
    }

    // 接地中は回転を減衰させて、最後には止める
    piece.angVel = VScale(piece.angVel, std::pow(angularFriction_, deltaTime));
}


// ================================================================
// 回転行列の誤差除去 (グラム・シュミット)
// ================================================================

void MeshCutter::Orthonormalize(
    MATRIX& m)
{
    VECTOR x = VNorm(VGet(m.m[0][0], m.m[0][1], m.m[0][2]));

    VECTOR y = VGet(m.m[1][0], m.m[1][1], m.m[1][2]);
    y = VNorm(VSub(y, VScale(x, VDot(x, y))));

    VECTOR z = VCross(x, y);

    m.m[0][0] = x.x;  m.m[0][1] = x.y;  m.m[0][2] = x.z;
    m.m[1][0] = y.x;  m.m[1][1] = y.y;  m.m[1][2] = y.z;
    m.m[2][0] = z.x;  m.m[2][1] = z.y;  m.m[2][2] = z.z;
}


// ================================================================
// 描画
// ================================================================

void MeshCutter::Draw()
{
    for (const auto& piece : pieces_)
        DrawPiece(piece);
}


// ================================================================
// 破片1個描画
// ================================================================

void MeshCutter::DrawPiece(const Piece& piece)
{
    const auto& m = piece.mesh;
    bool hasColor = !m.colors.empty();
    const auto& fb = palette_[0];

    // 頂点法線(Mesh に無ければ面法線を頂点に足して作る)
    // ローカル座標で求めて、あとで回転だけ掛ける
    std::vector<VECTOR> normals(m.verts.size(), VGet(0, 0, 0));
    for (const auto& f : m.faces)
    {
        VECTOR n = VCross(VSub(m.verts[f[1]], m.verts[f[0]]),
            VSub(m.verts[f[2]], m.verts[f[0]]));
        for (int k = 0; k < 3; k++)
            normals[f[k]] = VAdd(normals[f[k]], n);   // 面積重み付き
    }

    std::vector<VERTEX3D> vs(m.verts.size());
    for (size_t i = 0; i < vs.size(); i++)
    {
        VERTEX3D& v = vs[i];
        v.pos = VAdd(VTransform(m.verts[i], piece.rot), piece.pos);

        VECTOR n = VSize(normals[i]) > 0 ? VNorm(normals[i]) : VGet(0, 1, 0);
        v.norm = VTransformSR(n, piece.rot);

        v.u = m.u[i];  v.v = m.v[i];
        v.su = v.sv = 0.0f;
        v.dif = hasColor ? GetColorU8(m.colors[i].r, m.colors[i].g, m.colors[i].b, 255)
            : GetColorU8(fb[0], fb[1], fb[2], 255);
        v.spc = GetColorU8(0, 0, 0, 0);   // ハイライトなし
    }

    std::vector<unsigned short> idx;
    idx.reserve(m.faces.size() * 3);
    for (const auto& f : m.faces)
        for (int k = 0; k < 3; k++)
            idx.push_back((unsigned short)f[k]);

    DrawPolygonIndexed3D(vs.data(), (int)vs.size(), idx.data(),
        (int)m.faces.size(), m.textureHandle, TRUE);
}


// ================================================================
// 破片数
// ================================================================

int MeshCutter::GetPieceCount() const
{
    return static_cast<int>(pieces_.size());
}


// ================================================================
// 破片一覧
// ================================================================

const std::vector<MeshCutter::Piece>&
MeshCutter::GetPieces() const
{
    return pieces_;
}


std::vector<MeshCutter::Piece>&
MeshCutter::GetPieces()
{
    return pieces_;
}


// ================================================================
// 元メッシュの中心・半径
// ================================================================

VECTOR MeshCutter::GetCenter() const
{
    return VScale(VAdd(boundsMin_, boundsMax_), 0.5f);
}


float MeshCutter::GetRadius() const
{
    return VSize(VSub(boundsMax_, boundsMin_)) * 0.5f;
}


// ================================================================
// 切断時の設定
// ================================================================

void MeshCutter::SetPushSpeed(
    float speed)
{
    pushSpeed_ = speed;
}


void MeshCutter::SetCapEnabled(
    bool enabled)
{
    capEnabled_ = enabled;
}


// ================================================================
// 回転の設定
// ================================================================

void MeshCutter::SetRotationEnabled(
    bool enabled)
{
    rotationEnabled_ = enabled;
}


void MeshCutter::SetSpin(
    float spin)
{
    spin_ = spin;
}


void MeshCutter::SetAngularFriction(
    float friction)
{
    angularFriction_ = friction;
}


void MeshCutter::SetContactEps(
    float eps)
{
    contactEps_ = eps;
}


// ================================================================
// 重力・床の設定
// ================================================================

void MeshCutter::SetGravity(
    float gravity)
{
    gravityValue_ = gravity;
}


void MeshCutter::SetGravityEnabled(
    bool enabled)
{
    gravityEnabled_ = enabled;

    if (!enabled)
    {
        // OFF にした瞬間、空中で静止させる (落下中だった速度を残さない)
        for (auto& piece : pieces_)
        {
            piece.vel = VGet(0, 0, 0);
            piece.angVel = VGet(0, 0, 0);
        }
    }
}


bool MeshCutter::IsGravityEnabled() const
{
    return gravityEnabled_;
}


void MeshCutter::SetDamping(
    float damping)
{
    damping_ = damping;
}


void MeshCutter::SetFloor(
    float floorY)
{
    floorEnabled_ = true;
    floorY_ = floorY;
}


void MeshCutter::SetFloorFromMesh()
{
    SetFloor(boundsMin_.y);
}


void MeshCutter::DisableFloor()
{
    floorEnabled_ = false;
}


void MeshCutter::SetBounce(
    float bounce)
{
    bounce_ = bounce;
}


void MeshCutter::SetFriction(
    float friction)
{
    friction_ = friction;
}


// ================================================================
// 描画の見た目
// ================================================================

void MeshCutter::SetLight(
    VECTOR light)
{
    light_ = VNorm(light);
}


void MeshCutter::SetPalette(
    const std::vector<std::array<int, 3>>& palette)
{
    if (!palette.empty())
        palette_ = palette;
}


// ================================================================
// ドラッグした線から切断平面を作る
// ================================================================

bool MeshCutter::ComputePlaneFromDrag(
    int startX,
    int startY,
    int endX,
    int endY,
    VECTOR& outOrigin,
    VECTOR& outNormal)
{
    VECTOR camPos = GetCameraPosition();

    VECTOR a = VSub(
        ConvScreenPosToWorldPos(VGet((float)startX, (float)startY, 0.0f)),
        camPos);

    VECTOR b = VSub(
        ConvScreenPosToWorldPos(VGet((float)endX, (float)endY, 0.0f)),
        camPos);

    if (VSize(a) < 1e-6f || VSize(b) < 1e-6f)
        return false;

    VECTOR normal = VCross(VNorm(a), VNorm(b));

    if (VSize(normal) < 1e-6f)
        return false;

    outOrigin = camPos;
    outNormal = VNorm(normal);

    return true;
}


// ================================================================
// Piece を1個組み立てる
//   mesh はワールド座標。重心 (バウンディングボックスの中心) を原点とした
//   ローカル座標へ直し、重心の位置を pos に入れる
// ================================================================

MeshCutter::Piece MeshCutter::MakePiece(
    MeshCut::Mesh&& mesh,
    VECTOR vel,
    VECTOR angVel,
    int color)
{
    VECTOR mn = mesh.verts[0];
    VECTOR mx = mesh.verts[0];

    for (const auto& v : mesh.verts)
    {
        mn = VGet(std::fmin(mn.x, v.x), std::fmin(mn.y, v.y), std::fmin(mn.z, v.z));
        mx = VGet(std::fmax(mx.x, v.x), std::fmax(mx.y, v.y), std::fmax(mx.z, v.z));
    }

    const VECTOR center = VScale(VAdd(mn, mx), 0.5f);

    float r2 = 0.0f;

    for (auto& v : mesh.verts)
    {
        v = VSub(v, center);
        r2 = std::fmax(r2, VSquareSize(v));
    }

    Piece piece;

    piece.mesh = std::move(mesh);
    piece.pos = center;
    piece.vel = vel;
    piece.rot = MGetIdent();
    piece.angVel = angVel;
    piece.radius2 = r2;
    piece.color = color >= 0 ? color : colorCounter_++;

    return piece;
}