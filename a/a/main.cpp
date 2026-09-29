// ============================================================================
// main.cpp
//
// ドラッグした線で3Dモデルを切断するデモ
//
// 操作:
//   左ドラッグ       : 切断
//   ← →             : カメラ回転
//   R               : リセット
//   ESC             : 終了
// ============================================================================

#include "DxLib.h"
#include <EffekseerForDXLib.h>
#include "MeshCut.h"
#include "MeshCutter.h"

#include <cmath>


// ================================================================
// 立方体を作る
// ================================================================

static MeshCut::Mesh MakeCube(
    float size)
{
    MeshCut::Mesh mesh;

    float h =
        size * 0.5f;


    // 頂点
    for (int i = 0;
        i < 8;
        ++i)
    {
        mesh.verts.push_back(
            VGet(
                (i & 1)
                ? h
                : -h,

                (i & 2)
                ? h
                : -h,

                (i & 4)
                ? h
                : -h));
    }


    // 三角形
    mesh.faces =
    {
        {0, 2, 3},
        {0, 3, 1},

        {4, 5, 7},
        {4, 7, 6},

        {0, 1, 5},
        {0, 5, 4},

        {2, 6, 7},
        {2, 7, 3},

        {0, 4, 6},
        {0, 6, 2},

        {1, 3, 7},
        {1, 7, 5}
    };


    return mesh;
}


// ================================================================
// メイン
// ================================================================

int WINAPI WinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    // ------------------------------------------------------------
    // DxLib 初期化
    // ------------------------------------------------------------

    // アプリケーションの初期設定
    SetWindowText("3DAction");

    // ウィンドウサイズ
    SetGraphMode(1024, 768, 32);
    ChangeWindowMode(TRUE);

    SetUseDirect3DVersion(DX_DIRECT3D_11);
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    //InitEffekseer();

    SetDrawScreen(
        DX_SCREEN_BACK);

    SetUseZBuffer3D(TRUE);

    SetWriteZBuffer3D(TRUE);

    SetUseBackCulling(
        DX_CULLING_NONE);


    // ------------------------------------------------------------
    // モデル読み込み
    // ------------------------------------------------------------

    MeshCut::Mesh mesh;


    int modelHandle =
        MV1LoadModel(
            "model.mv1");


    if (modelHandle != -1)
    {
        mesh =
            MeshCut::FromMV1(
                modelHandle);
    }
    else
    {
        mesh =
            MakeCube(100.0f);
    }


    // ------------------------------------------------------------
    // モデルの大きさを計算
    // ------------------------------------------------------------

    if (mesh.verts.empty())
    {
        DxLib_End();
        return -1;
    }


    VECTOR min =
        mesh.verts[0];

    VECTOR max =
        mesh.verts[0];


    for (const auto& vertex :
        mesh.verts)
    {
        min =
            VGet(
                std::fmin(
                    min.x,
                    vertex.x),

                std::fmin(
                    min.y,
                    vertex.y),

                std::fmin(
                    min.z,
                    vertex.z));


        max =
            VGet(
                std::fmax(
                    max.x,
                    vertex.x),

                std::fmax(
                    max.y,
                    vertex.y),

                std::fmax(
                    max.z,
                    vertex.z));
    }


    VECTOR center =
        VScale(
            VAdd(min, max),
            0.5f);


    float radius =
        VSize(
            VSub(max, min))
        * 0.5f;


    SetCameraNearFar(
        radius * 0.05f,
        radius * 30.0f);


    // ------------------------------------------------------------
    // 切断クラス
    // ------------------------------------------------------------

    MeshCutter cutter;

    cutter.SetMesh(mesh);

    cutter.SetPushSpeed(
        radius * 0.02f);


    // ------------------------------------------------------------
    // カメラ
    // ------------------------------------------------------------

    float cameraAngle =
        0.0f;


    // ------------------------------------------------------------
    // マウス
    // ------------------------------------------------------------

    bool dragging =
        false;


    int startX = 0;
    int startY = 0;


    int mouseX = 0;
    int mouseY = 0;


    int previousMouse =
        0;


    // ------------------------------------------------------------
    // メインループ
    // ------------------------------------------------------------

    while (
        ProcessMessage() == 0 &&
        CheckHitKey(
            KEY_INPUT_ESCAPE) == 0)
    {
        // ========================================================
        // 入力
        // ========================================================

        if (CheckHitKey(
            KEY_INPUT_LEFT))
        {
            cameraAngle -=
                0.02f;
        }


        if (CheckHitKey(
            KEY_INPUT_RIGHT))
        {
            cameraAngle +=
                0.02f;
        }


        if (CheckHitKey(
            KEY_INPUT_R))
        {
            cutter.Reset();
        }


        // ========================================================
        // カメラ
        // ========================================================

        VECTOR cameraPosition =
            VGet(
                center.x +
                std::sin(cameraAngle)
                * radius
                * 3.0f,

                center.y +
                radius * 0.8f,

                center.z -
                std::cos(cameraAngle)
                * radius
                * 3.0f);


        SetCameraPositionAndTarget_UpVecY(
            cameraPosition,
            center);


        // ========================================================
        // マウス
        // ========================================================

        GetMousePoint(
            &mouseX,
            &mouseY);


        int mouse =
            GetMouseInput() &
            MOUSE_INPUT_LEFT;


        // --------------------------------------------------------
        // 押した瞬間
        // --------------------------------------------------------

        if (
            mouse &&
            !previousMouse)
        {
            dragging = true;

            startX = mouseX;
            startY = mouseY;
        }


        // --------------------------------------------------------
        // 離した瞬間
        // --------------------------------------------------------

        if (
            !mouse &&
            previousMouse &&
            dragging)
        {
            dragging = false;


            float dx =
                (float)(
                    mouseX -
                    startX);


            float dy =
                (float)(
                    mouseY -
                    startY);


            // 短すぎるドラッグは無視
            if (
                dx * dx +
                dy * dy >
                100.0f)
            {
                // =================================================
                // 画面上の2点から切断平面を作る
                // =================================================

                VECTOR camera =
                    GetCameraPosition();


                VECTOR screenStart =
                    ConvScreenPosToWorldPos(
                        VGet(
                            (float)startX,
                            (float)startY,
                            0.0f));


                VECTOR screenEnd =
                    ConvScreenPosToWorldPos(
                        VGet(
                            (float)mouseX,
                            (float)mouseY,
                            0.0f));


                // カメラから画面上の点への方向
                VECTOR directionA =
                    VNorm(
                        VSub(
                            screenStart,
                            camera));


                VECTOR directionB =
                    VNorm(
                        VSub(
                            screenEnd,
                            camera));


                // 2方向を含む平面の法線
                VECTOR normal =
                    VCross(
                        directionA,
                        directionB);


                // ------------------------------------------------
                // 切断
                // ------------------------------------------------

                if (VSize(normal) >
                    1e-6f)
                {
                    cutter.Cut(
                        camera,
                        VNorm(normal));
                }
            }
        }


        previousMouse =
            mouse;


        // ========================================================
        // 更新
        // ========================================================

        cutter.Update();


        // ========================================================
        // 描画
        // ========================================================

        ClearDrawScreen();


        cutter.Draw();


        // --------------------------------------------------------
        // ドラッグ中の線
        // --------------------------------------------------------

        if (dragging)
        {
            DrawLine(
                startX,
                startY,
                mouseX,
                mouseY,
                GetColor(
                    255,
                    255,
                    255),
                2);
        }


        // --------------------------------------------------------
        // UI
        // --------------------------------------------------------

        DrawFormatString(
            10,
            10,
            GetColor(
                255,
                255,
                255),

            "左ドラッグで切断 / "
            "R:リセット / "
            "<- ->:カメラ回転  "
            "破片数: %d",

            cutter.GetPieceCount());


        ScreenFlip();
    }


    // ------------------------------------------------------------
    // 終了
    // ------------------------------------------------------------

    DxLib_End();

    return 0;
}