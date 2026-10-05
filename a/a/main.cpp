// ============================================================================
//  main.cpp : ドラッグした線でモデルを切断するデモ (DxLib + MeshCutter)
// ============================================================================
//  model.mv1 があればそれを、なければ立方体を切断する。
//
//  操作:
//    左ボタンを押したまま動かして離す ... その線でモデルを切断
//    ←→ キー ... カメラ回転
//    R キー   ... 切る前の状態に戻す
//    H キー   ... 重力の ON/OFF を切り替え
//    ESC キー ... 終了
//
//  切断・破片の移動・重力・描画は、すべて MeshCutter (MeshCutter.h/.cpp) が
//  行う。このファイルは「入力を受け取って MeshCutter に渡すだけ」の薄い層に
//  なっているので、他のプロジェクトでは MeshCut.h / MeshCutter.h / .cpp の
//  3ファイルをコピーし、このファイルの WinMain を参考に組み込めばよい。
//
//  プロジェクト設定: リンカー -> システム -> サブシステム を Windows にすること
// ============================================================================
#include "DxLib.h"
#include "MeshCut.h"
#include "MeshCutter.h"

#include <cmath>

using MeshCut::Mesh;

// ----------------------------------------------------------------------------
//  MakeCube : 一辺 s の立方体メッシュを作る (model.mv1 がないとき用)
// ----------------------------------------------------------------------------
static Mesh MakeCube(
    float s)
{
    Mesh m;

    const float h = s * 0.5f;

    for (int i = 0; i < 8; ++i)
    {
        m.verts.push_back(VGet(
            (i & 1) ? h : -h,
            (i & 2) ? h : -h,
            (i & 4) ? h : -h));
    }

    m.faces =
    {
        {0, 2, 3}, {0, 3, 1}, {4, 5, 7}, {4, 7, 6}, {0, 1, 5}, {0, 5, 4},
        {2, 6, 7}, {2, 7, 3}, {0, 4, 6}, {0, 6, 2}, {1, 3, 7}, {1, 7, 5}
    };

    return m;
}


// ----------------------------------------------------------------------------
//  WinMain : エントリポイント
// ----------------------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    // ---- DxLib の初期化 ----
    ChangeWindowMode(TRUE);
    SetGraphMode(1024, 768, 32);

    if (DxLib_Init() == -1)
        return -1;

    SetDrawScreen(DX_SCREEN_BACK);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseBackCulling(DX_CULLING_NONE);  // 両面描画 (切断面のフタも見えるように)
    SetBackgroundColor(255, 255, 255);
    // ---- メッシュの読み込み (失敗したら立方体) ----
    Mesh mesh;

    int handle = MV1LoadModel("model.mv1");

    if (handle != -1)
        mesh = MeshCut::FromMV1(handle);
    else
        mesh = MakeCube(100.0f);

    // ---- MeshCutter の初期化 ----
    //   中心・半径は SetMesh の時点で自動計算される (GetCenter/GetRadius)。
    //   カメラの位置決めや、重力・押し出し速度の大きさにそのまま使える。
    //   色は SetMesh の時点でモデルの色が自動的に取り込まれるので、
    //   ここで追加の呼び出しは不要。
    MeshCutter cutter;

    cutter.SetMesh(mesh);

    const VECTOR center = cutter.GetCenter();
    const float radius = cutter.GetRadius();

    cutter.SetPushSpeed(radius * 0.02f);
    cutter.SetGravity(radius * 0.0015f);
    cutter.SetFloorFromMesh();   // 元モデルの一番低い場所を床にする
    cutter.SetBounce(0.25f);
    cutter.SetFriction(0.85f);

    SetCameraNearFar(radius * 0.05f, radius * 30.0f);

    float camAngle = 0.0f;       // カメラがモデルの周りを回る角度
    bool dragging = false;       // ドラッグ中か
    int startX = 0, startY = 0;  // ドラッグを始めた画面座標
    int prevMouse = 0;           // 前フレームの左ボタンの状態
    int prevH = 0;                // 前フレームの H キーの状態 (押した瞬間だけ切り替えるため)

    // ---- メインループ ----
    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        if (CheckHitKey(KEY_INPUT_LEFT))  camAngle -= 0.02f;
        if (CheckHitKey(KEY_INPUT_RIGHT)) camAngle += 0.02f;
        if (CheckHitKey(KEY_INPUT_R)) cutter.Reset();

        // H キーを押した瞬間だけ、重力の ON/OFF を切り替える
        // (CheckHitKey は押している間ずっと true になるので、前フレームと比べて
        //  「今押された瞬間」かどうかを判定する)
        int h = CheckHitKey(KEY_INPUT_H);

        if (h && !prevH)
            cutter.SetGravityEnabled(!cutter.IsGravityEnabled());

        prevH = h;

        // カメラを設定する。
        // 画面座標をワールド座標に変換する処理(下)がこのカメラを使うので、
        // マウス処理よりも先に設定しておくこと
        SetCameraPositionAndTarget_UpVecY(
            VGet(
                center.x + std::sin(camAngle) * radius * 3.0f,
                center.y + radius * 0.8f,
                center.z - std::cos(camAngle) * radius * 3.0f),
            center);

        // ---- マウス処理: 押した位置から離した位置までを切断線とする ----
        int mouseX = 0, mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);

        int mouse = GetMouseInput() & MOUSE_INPUT_LEFT;

        if (mouse && !prevMouse)
        {
            dragging = true;
            startX = mouseX;
            startY = mouseY;
        }

        if (!mouse && prevMouse && dragging)
        {
            dragging = false;

            float dx = (float)(mouseX - startX);
            float dy = (float)(mouseY - startY);

            // ドラッグの長さが10ピクセル未満なら、ただのクリックとみなして無視する
            if (dx * dx + dy * dy > 100.0f)
            {
                VECTOR origin, normal;

                if (MeshCutter::ComputePlaneFromDrag(startX, startY, mouseX, mouseY, origin, normal))
                    cutter.Cut(origin, normal);
            }
        }

        prevMouse = mouse;

        // ---- 破片の更新(重力・床判定・移動)と描画 ----
        cutter.Update();

        ClearDrawScreen();

        cutter.Draw();

        // ドラッグ中は、押した位置から現在位置まで白い線を引く
        if (dragging)
            DrawLine(startX, startY, mouseX, mouseY, GetColor(255, 255, 255), 2);

        DrawFormatString(10, 10, GetColor(255, 255, 255),
            "左ドラッグで切断 / R:リセット / H:重力%s / ←→:カメラ回転   破片数: %d",
            cutter.IsGravityEnabled() ? "OFF" : "ON",
            cutter.GetPieceCount());

        ScreenFlip();
    }

    DxLib_End();

    return 0;
}