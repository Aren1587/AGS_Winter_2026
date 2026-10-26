#include "GameClear.h"
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"

GameClear::GameClear(void)
	:
	imgTitle_(-1)
{
}

GameClear::~GameClear(void)
{
}

void GameClear::Init(void)
{
	imgTitle_ = LoadGraph((Application::PATH_IMAGE + "Clear.png").c_str());
}

void GameClear::Update(void)
{
	// ÉVÅ[ÉìëJà⁄
	InputManager& ins = InputManager::GetInstance();
	if (ins.IsTrgDown(KEY_INPUT_SPACE) || ins.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}
}

void GameClear::Draw(void)
{
	DrawRotaGraph(
		Application::SCREEN_SIZE_X / 2,
		Application::SCREEN_SIZE_Y / 2,
		0.5f, 0.0, imgTitle_, true);

	DrawFormatString(0, 0, 0xffffff, "Space/AÇ≈ñﬂÇÈ");
}