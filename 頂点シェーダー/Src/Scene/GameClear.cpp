#include "GameClear.h"
#include <DxLib.h>
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"

GameClear::GameClear(void)
//:
//imgClear_(-1)
{
}

GameClear::~GameClear(void)
{
	//DeleteGraph(imgClear_);
}

void GameClear::Init(void)
{
	//imgClear_ = LoadGraph((Application::PATH_IMAGE + "Clear.png").c_str());
}

void GameClear::Update(void)
{
	// Space / パッドボタンでタイトルへ
	auto& input = InputManager::GetInstance();
	if (input.IsTrgDown(KEY_INPUT_SPACE) ||
		input.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void GameClear::Draw(void)
{
	// クリア画像(今は未使用)
	//DrawRotaGraph(
	//	Application::SCREEN_SIZE_X / 2,
	//	Application::SCREEN_SIZE_Y / 2,
	//	0.5, 0.0, imgClear_, true);

	// 画像の代わりに文字で表示
	DrawString(Application::SCREEN_SIZE_X / 2 - 40, Application::SCREEN_SIZE_Y / 2, "GAME CLEAR", 0xffffff);
	DrawString(0, 0, "Space/Aで戻る", 0xffffff);
}