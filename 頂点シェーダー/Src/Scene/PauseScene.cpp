#include <DxLib.h>
#include "../Application.h"
#include "PauseScene.h"

PauseScene::PauseScene()
{
}

PauseScene::~PauseScene()
{
}

void PauseScene::Init(void)
{
}

void PauseScene::Update(void)
{
}

void PauseScene::Draw(void)
{
	// ”¼“§–¾ŽlŠp
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
	DrawBox(0, 0, Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y, GetColor(255, 255, 255), true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	auto centerX = Application::SCREEN_SIZE_X / 2;
	auto centerY = Application::SCREEN_SIZE_Y / 2;

	DrawBox(centerX - 200, centerY - 80, centerX + 200, centerY + 80, GetColor(255, 255, 255), true);
}
