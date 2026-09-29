#include "MeshCutter.h"

#include <cmath>
#include <utility>


// ================================================================
// ÉÅÉbÉVÉÖÇê›íË
// ================================================================

void MeshCutter::SetMesh(
    const MeshCut::Mesh& mesh)
{
    // å≥ÉfÅ[É^Çï€ë∂
    originalMesh_ = mesh;

    // åªç›ÇÃîjï–Çè¡Ç∑
    pieces_.clear();


    // ç≈èâÇÃîjï–ÇçÏÇÈ
    Piece piece;

    piece.mesh = mesh;

    piece.pos =
        VGet(0, 0, 0);

    piece.vel =
        VGet(0, 0, 0);

    piece.color = 0;


    pieces_.push_back(
        std::move(piece));


    colorCounter_ = 1;
}


// ================================================================
// å≥ÇÃèÛë‘Ç…ñﬂÇ∑
// ================================================================

void MeshCutter::Reset()
{
    pieces_.clear();


    Piece piece;

    piece.mesh =
        originalMesh_;

    piece.pos =
        VGet(0, 0, 0);

    piece.vel =
        VGet(0, 0, 0);

    piece.color = 0;


    pieces_.push_back(
        std::move(piece));


    colorCounter_ = 1;
}


// ================================================================
// êÿíf
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
// êÿíf
// ================================================================

void MeshCutter::Cut(
    VECTOR origin,
    VECTOR normal,
    float pushSpeed)
{
    if (VSize(normal) < 1e-6f)
        return;


    normal =
        VNorm(normal);


    std::vector<Piece> next;

    // åªç›ë∂ç›Ç∑ÇÈëSîjï–ÇêÿÇÈ
    for (auto& piece :
        pieces_)
    {
        // --------------------------------------------------------
        // åªç›ÇÃà⁄ìÆó ÇÉÅÉbÉVÉÖÇ…îΩâf
        // --------------------------------------------------------

        for (auto& vertex :
            piece.mesh.verts)
        {
            vertex =
                VAdd(
                    vertex,
                    piece.pos);
        }


        piece.pos =
            VGet(0, 0, 0);


        // --------------------------------------------------------
        // êÿíf
        // --------------------------------------------------------

        MeshCut::Mesh positive;
        MeshCut::Mesh negative;


        MeshCut::Cut(
            piece.mesh,
            origin,
            normal,
            positive,
            negative,
            true);


        // --------------------------------------------------------
        // êÿífÇ≥ÇÍÇ»Ç©Ç¡ÇΩèÍçá
        // --------------------------------------------------------

        if (
            positive.faces.empty() ||
            negative.faces.empty())
        {
            next.push_back(
                std::move(piece));

            continue;
        }


        // --------------------------------------------------------
        // ê≥ë§ÇÃîjï–
        // --------------------------------------------------------

        Piece positivePiece;

        positivePiece.mesh =
            std::move(positive);

        positivePiece.pos =
            VGet(0, 0, 0);

        positivePiece.vel =
            VScale(
                normal,
                pushSpeed);

        positivePiece.color =
            colorCounter_++;


        // --------------------------------------------------------
        // ïâë§ÇÃîjï–
        // --------------------------------------------------------

        Piece negativePiece;

        negativePiece.mesh =
            std::move(negative);

        negativePiece.pos =
            VGet(0, 0, 0);

        negativePiece.vel =
            VScale(
                normal,
                -pushSpeed);

        negativePiece.color =
            colorCounter_++;


        // --------------------------------------------------------
        // åãâ Ç…í«â¡
        // --------------------------------------------------------

        next.push_back(
            std::move(
                positivePiece));

        next.push_back(
            std::move(
                negativePiece));
    }


    // ì¸ÇÍë÷Ç¶ÇÈ
    pieces_ =
        std::move(next);
}


// ================================================================
// çXêV
// ================================================================

void MeshCutter::Update(
    float damping)
{
    for (auto& piece :
        pieces_)
    {
        // à⁄ìÆ
        piece.pos =
            VAdd(
                piece.pos,
                piece.vel);


        // å∏ë¨
        piece.vel =
            VScale(
                piece.vel,
                damping);
    }
}


// ================================================================
// ï`âÊ
// ================================================================

void MeshCutter::Draw()
{
    // ä»íPÇ»ïΩçsåı
    const VECTOR light =
        VNorm(
            VGet(
                0.4f,
                0.8f,
                -0.5f));


    for (const auto& piece :
        pieces_)
    {
        DrawPiece(
            piece,
            light);
    }
}


// ================================================================
// îjï–1å¬Çï`âÊ
// ================================================================

void MeshCutter::DrawPiece(
    const Piece& piece,
    VECTOR light)
{
    const int* color =
        Palette[
            piece.color % 6];


    for (const auto& face :
        piece.mesh.faces)
    {
        // --------------------------------------------------------
        // ÉèÅ[ÉãÉhç¿ïW
        // --------------------------------------------------------

        VECTOR p0 =
            VAdd(
                piece.mesh.verts[face[0]],
                piece.pos);

        VECTOR p1 =
            VAdd(
                piece.mesh.verts[face[1]],
                piece.pos);

        VECTOR p2 =
            VAdd(
                piece.mesh.verts[face[2]],
                piece.pos);


        // --------------------------------------------------------
        // ñ@ê¸
        // --------------------------------------------------------

        VECTOR normal =
            VCross(
                VSub(p1, p0),
                VSub(p2, p0));


        float length =
            VSize(normal);


        // --------------------------------------------------------
        // ñæÇÈÇ≥
        // --------------------------------------------------------

        float brightness =
            0.35f;


        if (length > 0.0f)
        {
            brightness +=
                0.65f *
                std::fabs(
                    VDot(
                        normal,
                        light))
                / length;
        }


        // --------------------------------------------------------
        // ï`âÊ
        // --------------------------------------------------------

        DrawTriangle3D(
            p0,
            p1,
            p2,
            GetColor(
                (int)(
                    color[0] *
                    brightness),

                (int)(
                    color[1] *
                    brightness),

                (int)(
                    color[2] *
                    brightness)),

            TRUE);
    }
}


// ================================================================
// îjï–êî
// ================================================================

int MeshCutter::GetPieceCount() const
{
    return static_cast<int>(
        pieces_.size());
}


// ================================================================
// îjï–éÊìæ
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
// âüÇµèoÇµë¨ìx
// ================================================================

void MeshCutter::SetPushSpeed(
    float speed)
{
    pushSpeed_ = speed;
}