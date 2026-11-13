#include <math.h>
#include <DxLib.h>
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"
#include "../Manager/Camera.h"
#include "TitleScene.h"

TitleScene::TitleScene(void)
{
	imgTitle_ = -1;
	fontSub_ = -1;
	timer_ = 0;
	select_ = 0;
}

TitleScene::~TitleScene(void)
{
}

void TitleScene::Init(void)
{
	imgTitle_ = LoadGraph("Data/Image/Title.png");
	fontSub_ = CreateFontToHandle(NULL, 28, 1, DX_FONTTYPE_ANTIALIASING, -1);

	mainCamera.ChangeMode(Camera::MODE::FIXED_POINT);
}

void TitleScene::Update(void)
{
	timer_++;

	InputManager& ins = InputManager::GetInstance();

	using PAD = InputManager::JOYPAD_NO;
	using DIR = InputManager::JOYPAD_DIR;

	// 下:S / ↓ / 十字キー下 / 左スティック下
	if (ins.IsTrgDown(KEY_INPUT_S) || ins.IsTrgDown(KEY_INPUT_DOWN) ||
		ins.IsPadDirTrgDown(PAD::PAD1, DIR::DOWN))
	{
		select_ = (select_ + 1) % 2;
	}
	// 上:W / ↑ / 十字キー上 / 左スティック上
	if (ins.IsTrgDown(KEY_INPUT_W) || ins.IsTrgDown(KEY_INPUT_UP) ||
		ins.IsPadDirTrgDown(PAD::PAD1, DIR::UP))
	{
		select_ = (select_ + 1) % 2;
	}

	if (ins.IsTrgDown(KEY_INPUT_SPACE) || ins.IsTrgDown(KEY_INPUT_RETURN) ||
		ins.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN))
	{
		if (select_ == 0)
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::GAME);
		else
			PostQuitMessage(0);
	}
}

void TitleScene::Draw(void)
{
	const int CX = Application::SCREEN_SIZE_X / 2;
	const int SX = Application::SCREEN_SIZE_X;
	const int SY = Application::SCREEN_SIZE_Y;

	// 背景
	DrawBox(0, 0, SX, SY, GetColor(3, 5, 18), TRUE);

	// スキャンライン
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 20);
	for (int y = 0; y < SY; y += 4)
	{
		DrawLine(0, y, SX, y, GetColor(0, 80, 160));
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// 中央の水平ラインアクセント
	float pulse = sinf(timer_ * 0.04f);
	int lineAlpha = (int)(160 + pulse * 60);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, lineAlpha);
	DrawBox(CX - 350, 290, CX + 350, 293, GetColor(0, 180, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	if (imgTitle_ != -1)
	{
		int imgW, imgH;
		GetGraphSize(imgTitle_, &imgW, &imgH);
		int drawW = imgW / 2;
		int drawH = imgH / 2;
		DrawExtendGraph(CX - drawW / 2, 60, CX + drawW / 2, 60 + drawH, imgTitle_, TRUE);
	}

	// 点滅
	// Menu
	const char* items[2] = { "GAME START", "GAME EXIT" };
	const int menuY = 400;
	const int menuStep = 65;

	for (int i = 0; i < 2; i++)
	{
		bool sel = (select_ == i);
		unsigned int col = sel ? GetColor(200, 240, 255) : GetColor(55, 95, 140);
		int iw = GetDrawStringWidthToHandle(items[i], (int)strlen(items[i]), fontSub_);
		int ix = CX - iw / 2;
		int iy = menuY + i * menuStep;

		if (sel)
		{
			SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
			DrawStringToHandle(ix - 2, iy, items[i], GetColor(0, 180, 255), fontSub_);
			DrawStringToHandle(ix + 2, iy, items[i], GetColor(0, 180, 255), fontSub_);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawStringToHandle(ix - 32, iy, ">", GetColor(0, 220, 255), fontSub_);
		}

		DrawStringToHandle(ix, iy, items[i], col, fontSub_);
	}
}