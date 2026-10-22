#pragma once

#include "DxLib.h"
#include "MeshCut.h"

#include <array>
#include <vector>

// ================================================================
//  MeshCutter : 切断・破片の物理・描画をまとめて管理するクラス
// ================================================================
//
//  他のプロジェクトで使うときの基本の流れ:
//
//      MeshCutter cutter;
//      cutter.SetMesh(MeshCut::FromMV1(handle));   // モデルを設定
//      cutter.SetGravity(0.5f);                    // 重力を使うなら設定
//      cutter.SetFloorFromMesh();                  // 床を自動設定
//
//      // 毎フレーム:
//      cutter.Cut(origin, normal);  // 切りたいときだけ呼ぶ
//      cutter.Update();
//      cutter.Draw();
//
//  破片は回転する。切った瞬間に切断面から離れる向きの回転が付き、
//  床に着いたあとは重心が支えの外にあれば重力で倒れる。
//  回転が不要なら SetRotationEnabled(false) で従来どおりの
//  平行移動だけの挙動になる。
//
//  画面をドラッグして切る操作を作りたい場合は、静的関数
//  ComputePlaneFromDrag() が使える(main.cpp を参照)。
// ================================================================
class MeshCutter
{
public:

    // 切断された破片
    //
    // mesh.verts は「破片の重心を原点としたローカル座標」で持つ。
    // ワールド座標の頂点は  VAdd(VTransform(v, rot), pos)  で求める。
    struct Piece
    {
        MeshCut::Mesh mesh;

        VECTOR pos;
        VECTOR vel;

        MATRIX rot;
        VECTOR angVel;

        float radius2;

        int color;

        // 描画用キャッシュ (MakePiece / SetPalette で作る。毎フレームは作り直さない)
        //   頂点はローカル座標のまま持ち、描画時にワールド行列で動かす。
        //   drawIdx が空のときは drawVerts が三角形リスト
        //   (頂点数が 65535 を超える破片用)
        std::vector<VERTEX3D> drawVerts;
        std::vector<unsigned short> drawIdx;
    };

public:

    // メッシュを設定 (以後はこの状態が「元の形」になる)
    void SetMesh(const MeshCut::Mesh& mesh);

    // 元の状態に戻す
    void Reset();

    // ============================================================
    // 切断 (SetPushSpeed で指定した速度を使う)
    //
    // origin : 切断平面上の点
    // normal : 切断平面の法線
    // ============================================================

    void Cut(
        VECTOR origin,
        VECTOR normal);


    // ============================================================
    // 押し出し速度を指定して切断
    // ============================================================

    void Cut(
        VECTOR origin,
        VECTOR normal,
        float pushSpeed);


    // ============================================================
    // 更新 (重力・回転・床判定・移動をまとめて行う)
    //
    // deltaTime : 1フレーム分の時間。60fps 固定で毎フレーム1回呼ぶだけなら
    //             既定の 1.0f のままでよい
    // ============================================================

    void Update(
        float deltaTime = 1.0f);


    // ============================================================
    // 描画 (既定の見た目。色や光を変えたい場合は SetLight / SetPalette を使う)
    // ============================================================

    void Draw();


    // ============================================================
    // 破片数
    // ============================================================

    int GetPieceCount() const;


    // ============================================================
    // 破片一覧 (自前の描画・当たり判定をしたいときに使う)
    //   頂点のワールド座標は Piece のコメントを参照
    // ============================================================

    const std::vector<Piece>& GetPieces() const;
    std::vector<Piece>& GetPieces();


    // ============================================================
    // 元メッシュのバウンディングボックスから求めた中心・半径
    //   カメラの位置決めなど、呼び出し側の初期化にそのまま使える
    // ============================================================

    VECTOR GetCenter() const;
    float GetRadius() const;


    // ============================================================
    // 切断時の設定
    // ============================================================

    // 押し出し速度 (Cut(origin, normal) の2引数版が使う既定値)
    void SetPushSpeed(
        float speed);

    // 切断面にフタをするか (既定 true)
    void SetCapEnabled(
        bool enabled);


    // ============================================================
    // 回転の設定
    // ============================================================

    // 破片の回転・倒れる挙動を使うか (既定 true)。
    // false にすると、回転なしの平行移動だけになる
    void SetRotationEnabled(
        bool enabled);

    // 切った瞬間に破片へ与える回転の強さ (ラジアン/フレーム。既定 0.05)。
    // 0 なら初期回転なし(床に着いたあとの倒れ込みだけが残る)
    void SetSpin(
        float spin);

    // 床に着いている間の回転の減衰率 (既定 0.9。小さいほど早く止まる)
    void SetAngularFriction(
        float friction);

    // 床との接触とみなす高さの幅。
    // 0 以下なら元メッシュの半径の 2% を自動で使う (既定 0)
    void SetContactEps(
        float eps);


    // ============================================================
    // 重力・床の設定
    //   何も呼ばなければ重力・床とも無効のまま
    // ============================================================

    // 1フレームあたりの重力加速度。0 なら重力なし (既定 0)
    void SetGravity(
        float gravity);

    // 重力の ON/OFF だけを切り替える。
    //   OFF にしても SetGravity で設定した値は覚えたままなので、
    //   再び ON にすれば同じ強さの重力に戻る。
    //   OFF にした瞬間、全破片の速度と角速度を 0 にして空中で静止させる。
    void SetGravityEnabled(
        bool enabled);

    bool IsGravityEnabled() const;

    // 空中・地上を問わず毎フレームかかる速度・角速度の減衰率。
    // 1.0 なら減衰なし (既定 1.0)。
    // 重力を使わずに「切った破片が飛び散って自然に止まる」挙動にしたいときは
    // 0.9 前後を指定するとよい
    void SetDamping(
        float damping);

    // 床を有効にして高さを指定する
    void SetFloor(
        float floorY);

    // 床を有効にして、元メッシュの一番低い場所を高さにする
    void SetFloorFromMesh();

    // 床を無効にする
    void DisableFloor();

    // 床に着いたときの跳ね返り (0=跳ねない 〜 1=完全に跳ね返す。既定 0.3)
    void SetBounce(
        float bounce);

    // 床に着いている間の横方向の減速 (既定 0.85)
    void SetFriction(
        float friction);


    // ============================================================
    // 描画の見た目を変える
    // ============================================================

    // 光の向き (既定は (0.4, 0.8, -0.5) を正規化したもの)
    void SetLight(
        VECTOR light);

    // 破片の色のフォールバック。
    // MeshCut::Mesh が色情報 (MV1 の元のテクスチャ・マテリアルの色) を持っている
    // 場合はそれが優先して使われ、これは色情報を持たないメッシュ (立方体などの
    // テスト用図形) にだけ使われる。palette の最初の要素が使われる。
    void SetPalette(
        const std::vector<std::array<int, 3>>& palette);


    // ============================================================
    // ドラッグした線 (画面座標2点) から切断平面を作る
    //   カメラの位置と、画面上の2点を通る平面を返す。
    //   ドラッグ量がほぼ0で平面が作れない場合は false を返す。
    //
    //   例:
    //     VECTOR origin, normal;
    //     if (MeshCutter::ComputePlaneFromDrag(sx, sy, ex, ey, origin, normal))
    //         cutter.Cut(origin, normal);
    // ============================================================

    static bool ComputePlaneFromDrag(
        int startX,
        int startY,
        int endX,
        int endY,
        VECTOR& outOrigin,
        VECTOR& outNormal);


private:

    // 元のメッシュ (ワールド座標のまま)
    MeshCut::Mesh originalMesh_;

    // 元メッシュのバウンディングボックス (SetMesh のたびに更新)
    VECTOR boundsMin_ = VGet(0, 0, 0);
    VECTOR boundsMax_ = VGet(0, 0, 0);

    // 現在の破片
    std::vector<Piece> pieces_;

    // 次に使う色番号
    int colorCounter_ = 0;

    // 切断時の押し出し速度
    float pushSpeed_ = 1.0f;

    // 切断面にフタをするか
    bool capEnabled_ = true;

    // 回転
    bool rotationEnabled_ = true;
    float spin_ = 0.05f;
    float angularFriction_ = 0.9f;
    float contactEps_ = 0.0f;     // 0 以下なら半径の 2% を自動で使う

    // 重力・床
    float gravityValue_ = 0.0f;   // SetGravity で設定した値 (ON/OFF に関わらず覚えておく)
    bool gravityEnabled_ = true;  // 現在 ON か (OFF の間は Update で 0 として扱う)
    float damping_ = 1.0f;
    bool floorEnabled_ = false;
    float floorY_ = 0.0f;
    float bounce_ = 0.3f;
    float friction_ = 0.85f;

    // 描画
    VECTOR light_ = VGet(0.4f, 0.8f, -0.5f);

    std::vector<std::array<int, 3>> palette_ =
    {
        {230, 110, 90},
        { 90, 150, 230},
        {110, 200, 120},
        {230, 190,  80},
        {190, 110, 220},
        { 90, 200, 200}
    };

    // mesh (ワールド座標) と速度から Piece を1個組み立てる。
    // 頂点を重心基準のローカル座標へ直し、pos に重心を入れる。
    // color が負なら色番号を自動で割り当て、0 以上ならその色を引き継ぐ
    Piece MakePiece(
        MeshCut::Mesh&& mesh,
        VECTOR vel,
        VECTOR angVel,
        int color = -1);

    // 床に着いている破片に、重力による倒れ込みの回転を加える
    void ApplyTipping(
        Piece& piece,
        float lowest,
        float gravity,
        float deltaTime) const;

    // 回転行列の誤差を取り除く (毎フレーム回し続けると崩れるため)
    static void Orthonormalize(MATRIX& m);

    // 描画用キャッシュ (drawVerts / drawIdx) を mesh から作る
    void BuildDrawCache(
        Piece& piece) const;

    // 破片1個描画
    void DrawPiece(const Piece& piece);
};