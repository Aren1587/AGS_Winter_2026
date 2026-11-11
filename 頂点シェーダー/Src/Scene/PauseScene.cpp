#include <DxLib.h>
#include "../Application.h"
#include "../Manager/InputManager.h"
#include "PauseScene.h"

PauseScene::PauseScene()
	: selectItem_(MENU_ITEM::RESUME)
	, isResume_(false), isGotoTitle_(false), isExit_(false)
	, isGameScene_(true), showTitleOption_(true)
{
}

PauseScene::~PauseScene()
{
}

void PauseScene::Init(void)
{
	selectItem_ = MENU_ITEM::RESUME;
	isResume_ = false;
	isGotoTitle_ = false;
	isExit_ = false;
}

void PauseScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	using PAD = InputManager::JOYPAD_NO;
	using BTN = InputManager::JOYPAD_BTN;
	using DIR = InputManager::JOYPAD_DIR;

	// キャンセル:ESC / パッド右ボタン(B / 〇)
	if (ins.IsTrgDown(KEY_INPUT_ESCAPE) ||
		ins.IsPadBtnTrgDown(PAD::PAD1, BTN::RIGHT))
	{
		isResume_ = true;
		return;
	}

	const int menuCount = static_cast<int>(MENU_ITEM::MAX);

	auto skipItem = [&](int idx) -> bool {
		return !showTitleOption_ && static_cast<MENU_ITEM>(idx) == MENU_ITEM::TITLE;
		};

	// 上:↑ / 十字キー上 / 左スティック上
	if (ins.IsTrgDown(KEY_INPUT_UP) || ins.IsPadDirTrgDown(PAD::PAD1, DIR::UP))
	{
		int idx = static_cast<int>(selectItem_) - 1;
		if (idx < 0) idx = menuCount - 1;
		if (skipItem(idx)) { idx--; if (idx < 0) idx = menuCount - 1; }
		selectItem_ = static_cast<MENU_ITEM>(idx);
	}
	// 下:↓ / 十字キー下 / 左スティック下
	if (ins.IsTrgDown(KEY_INPUT_DOWN) || ins.IsPadDirTrgDown(PAD::PAD1, DIR::DOWN))
	{
		int idx = static_cast<int>(selectItem_) + 1;
		if (idx >= menuCount) idx = 0;
		if (skipItem(idx)) { idx++; if (idx >= menuCount) idx = 0; }
		selectItem_ = static_cast<MENU_ITEM>(idx);
	}

	// 決定:Enter / パッド下ボタン(A / ×)
	if (ins.IsTrgDown(KEY_INPUT_RETURN) ||
		ins.IsPadBtnTrgDown(PAD::PAD1, BTN::DOWN))
	{
		switch (selectItem_)
		{
		case MENU_ITEM::RESUME: isResume_ = true; break;
		case MENU_ITEM::TITLE:  if (showTitleOption_) isGotoTitle_ = true; break;
		case MENU_ITEM::EXIT:   isExit_ = true; break;
		}
	}
}

void PauseScene::Draw(void)
{
	const int W = Application::SCREEN_SIZE_X;
	const int H = Application::SCREEN_SIZE_Y;
	const int cx = W / 2;
	const int cy = H / 2;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	DrawBox(0, 0, W, H, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawBox(cx - 220, cy - 150, cx + 220, cy + 150, GetColor(40, 40, 40), TRUE);
	DrawBox(cx - 220, cy - 150, cx + 220, cy + 150, GetColor(200, 200, 200), FALSE);

	SetFontSize(32);
	DrawString(cx - 56, cy - 130, "PAUSE", GetColor(255, 255, 255));
	SetFontSize(20);

	const char* labels[] = {
		isGameScene_ ? "ゲームに戻る" : "戻る",
		"タイトルに戻る",
		"ゲーム終了",
	};

	const int menuCount = static_cast<int>(MENU_ITEM::MAX);
	int visibleCount = menuCount - (showTitleOption_ ? 0 : 1);
	int visibleIdx = 0;

	for (int i = 0; i < menuCount; i++)
	{
		if (!showTitleOption_ && static_cast<MENU_ITEM>(i) == MENU_ITEM::TITLE) continue;

		int itemY = cy - 35 * (visibleCount - 1) + visibleIdx * 70;
		unsigned int color = GetColor(180, 180, 180);
		if (static_cast<int>(selectItem_) == i)
		{
			color = GetColor(255, 220, 0);
			DrawString(cx - 115, itemY, ">", color);
		}
		DrawString(cx - 90, itemY, labels[i], color);
		visibleIdx++;
	}

	SetFontSize(16);
}