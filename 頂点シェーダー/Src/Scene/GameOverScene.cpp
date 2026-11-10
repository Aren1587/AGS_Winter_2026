#include <algorithm>
#include <cstring>
#include "GameOverScene.h"
#include <DxLib.h>
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"

GameOverScene::GameOverScene(void)
    : timer_(0.0f), fontLarge_(-1), fontSmall_(-1)
{
}

GameOverScene::~GameOverScene(void)
{
    if (fontLarge_ != -1) DeleteFontToHandle(fontLarge_);
    if (fontSmall_ != -1) DeleteFontToHandle(fontSmall_);
}

void GameOverScene::Init(void)
{
    timer_     = 0.0f;
    fontLarge_ = CreateFontToHandle(NULL, 80, -1, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 3);
    fontSmall_ = CreateFontToHandle(NULL, 22, -1, DX_FONTTYPE_ANTIALIASING);
}

void GameOverScene::Update(void)
{
    timer_ += 1.0f / 60.0f;

    auto& input = InputManager::GetInstance();
    if (input.IsTrgDown(KEY_INPUT_SPACE) ||
        input.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN))
    {
        SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
    }
}

void GameOverScene::Draw(void)
{
    const int sw = Application::SCREEN_SIZE_X;
    const int sh = Application::SCREEN_SIZE_Y;
    const int cx = sw / 2;
    const int cy = sh / 2;

    // 1. 黒背景
    DrawBox(0, 0, sw, sh, 0x000000, TRUE);

    // 2. 赤フラッシュ (0～2 秒でフェードアウト)
    float flashT = (std::max)(0.0f, 1.0f - timer_ / 2.0f);
    if (flashT > 0.0f)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(flashT * 230));
        DrawBox(0, 0, sw, sh, 0xcc0000, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 3. GAME OVER (0.8 秒後にフェードイン)
    float showT = (std::max)(0.0f, (std::min)(1.0f, (timer_ - 0.8f) / 1.0f));
    float pulse = 0.5f + 0.5f * sinf(timer_ * 2.5f);

    if (showT > 0.0f && fontLarge_ != -1)
    {
        const char* goText = "GAME OVER";
        int tw = GetDrawStringWidthToHandle(goText, (int)strlen(goText), fontLarge_);
        int tx = cx - tw / 2;
        int ty = cy - 55;

        // クロマティックアベレーション グロー (加算合成)
        int glowA = (int)(showT * 160.0f * pulse);
        SetDrawBlendMode(DX_BLENDMODE_ADD, glowA);
        DrawStringToHandle(tx - 6, ty, goText, 0xff1111, fontLarge_);  // R
        DrawStringToHandle(tx + 6, ty, goText, 0x1111ff, fontLarge_);  // B
        DrawStringToHandle(tx,     ty, goText, 0x11ff11, fontLarge_);  // G
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 本体テキスト
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(showT * 255));
        DrawStringToHandle(tx, ty, goText, GetColor(230, 30, 30), fontLarge_);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 上下アクセントライン
        int lineA = (int)(showT * 160.0f * pulse);
        SetDrawBlendMode(DX_BLENDMODE_ADD, lineA);
        DrawLine(0, ty - 10, sw, ty - 10, 0xff0000);
        DrawLine(0, ty -  8, sw, ty -  8, 0xff4444);
        DrawLine(0, ty + 93, sw, ty + 93, 0xff4444);
        DrawLine(0, ty + 95, sw, ty + 95, 0xff0000);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 4. スキャンライン
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 22);
    for (int y = 0; y < sh; y += 3)
        DrawLine(0, y, sw, y, 0x000000);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 5. ビネット (四辺を暗く)
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(0,           0,  sw / 5,       sh,      0x000000, TRUE);
    DrawBox(sw * 4 / 5,  0,  sw,            sh,      0x000000, TRUE);
    DrawBox(0,           0,  sw,            sh / 6,  0x000000, TRUE);
    DrawBox(0,  sh * 5 / 6,  sw,            sh,      0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 6. "PRESS SPACE" ブリンク (2 秒後から)
    if (timer_ > 2.0f && fontSmall_ != -1)
    {
        bool blink = (int)(timer_ * 2.0f) % 2 == 0;
        if (blink)
        {
            const char* prompt = "- PRESS SPACE TO RETURN -";
            int pw = GetDrawStringWidthToHandle(prompt, (int)strlen(prompt), fontSmall_);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
            DrawStringToHandle(cx - pw / 2, cy + 65, prompt, 0x999999, fontSmall_);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
}
