#include "MeshCutter.h"

#include <cmath>
#include <utility>

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
        MakePiece(MeshCut::Mesh(mesh), VGet(0, 0, 0)));
}


// ================================================================
// 元の状態に戻す
// ================================================================

void MeshCutter::Reset()
{
    pieces_.clear();

    colorCounter_ = 0;

    pieces_.push_back(
        MakePiece(MeshCut::Mesh(originalMesh_), VGet(0, 0, 0)));
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

    std::vector<Piece> next;

    // 現在存在する全破片を切る
    for (auto& piece : pieces_)
    {
        // 現在の移動量をメッシュに反映してから切る
        // (そうしないと、見えている位置と切断平面の位置がずれてしまう)
        // minY も同じ分だけ動かしておかないと、次の床判定が過去の高さを
        // 見てしまい、落下中の破片を再度切ったときに床をすり抜ける原因になる
        piece.minY += piece.pos.y;

        for (auto& vertex : piece.mesh.verts)
            vertex = VAdd(vertex, piece.pos);

        piece.pos = VGet(0, 0, 0);

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
        if (positive.faces.empty() || negative.faces.empty())
        {
            next.push_back(std::move(piece));
            continue;
        }

        next.push_back(MakePiece(std::move(positive), VScale(normal, pushSpeed)));
        next.push_back(MakePiece(std::move(negative), VScale(normal, -pushSpeed)));
    }

    pieces_ = std::move(next);
}


// ================================================================
// 更新 (重力・床判定・移動)
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

        if (!floorEnabled_)
            continue;

        // 床との当たり判定: 破片の一番低い点が床より下に来たら押し戻す
        float worldMinY = piece.minY + piece.pos.y;

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
    }
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
        v.pos = VAdd(m.verts[i], piece.pos);
        v.norm = VSize(normals[i]) > 0 ? VNorm(normals[i]) : VGet(0, 1, 0);
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
            piece.vel = VGet(0, 0, 0);
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
// mesh の最小 Y 座標
// ================================================================

float MeshCutter::ComputeMinY(
    const MeshCut::Mesh& mesh)
{
    float minY = mesh.verts[0].y;

    for (const auto& v : mesh.verts)
        minY = std::fmin(minY, v.y);

    return minY;
}


// ================================================================
// Piece を1個組み立てる
// ================================================================

MeshCutter::Piece MeshCutter::MakePiece(
    MeshCut::Mesh&& mesh,
    VECTOR vel)
{
    Piece piece;

    piece.minY = ComputeMinY(mesh);
    piece.mesh = std::move(mesh);
    piece.pos = VGet(0, 0, 0);
    piece.vel = vel;
    piece.color = colorCounter_++;

    return piece;
}