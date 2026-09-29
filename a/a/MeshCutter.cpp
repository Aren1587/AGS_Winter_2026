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
    for (auto& piece : pieces_)
    {
        // 重力
        piece.vel.y -= gravity_ * deltaTime;

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
        if (std::fabs(piece.vel.y) < gravity_ * deltaTime * 2.0f + 1e-5f)
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

void MeshCutter::DrawPiece(
    const Piece& piece)
{
    const auto& color = palette_[piece.color % palette_.size()];

    for (const auto& face : piece.mesh.faces)
    {
        VECTOR p0 = VAdd(piece.mesh.verts[face[0]], piece.pos);
        VECTOR p1 = VAdd(piece.mesh.verts[face[1]], piece.pos);
        VECTOR p2 = VAdd(piece.mesh.verts[face[2]], piece.pos);

        VECTOR normal = VCross(VSub(p1, p0), VSub(p2, p0));

        float length = VSize(normal);

        float brightness = 0.35f;

        if (length > 0.0f)
        {
            brightness +=
                0.65f * std::fabs(VDot(normal, light_)) / length;
        }

        DrawTriangle3D(
            p0,
            p1,
            p2,
            GetColor(
                (int)(color[0] * brightness),
                (int)(color[1] * brightness),
                (int)(color[2] * brightness)),
            TRUE);
    }
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
    gravity_ = gravity;
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