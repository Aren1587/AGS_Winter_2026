#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/Camera.h"
#include "../Manager/InputManager.h"
#include "../Object/Common/Capsule.h"
#include "../Object/Common/Collider.h"
#include "../Object/SkyDome.h"
#include "../Object/Stage.h"
#include "../Object/Player.h"
#include "../Object/Planet.h"
#include "../Object/Enemy.h"
#include "../Object/Sword.h"
#include "../Renderer/PixelMaterial.h"
#include "../Renderer/PixelRenderer.h"
#include "GameScene.h"

namespace
{
	constexpr int ENEMY_NUM = 1;

	constexpr float MAX_GAGE_POS = 150.0f;
}

GameScene::GameScene(void)
	:
	player_(nullptr),
	skyDome_(nullptr),
	stage_(nullptr),
	mode_(MODE::MAIN)
{
}

GameScene::~GameScene(void)
{
	//DeleteGraph(postEffectScreen_);
}

void GameScene::Init(void)
{
	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Init();

	// 敵の作成
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		auto e = std::make_unique<Enemy>(*player_);
		e->Init();
		enemys_.emplace_back(std::move(e));
	}

	// ステージ
	stage_ = std::make_unique<Stage>(*player_);
	stage_->Init();

	// ステージの初期設定
	stage_->ChangeStage(Stage::NAME::MAIN_PLANET);

	// スカイドーム
	skyDome_ = std::make_unique<SkyDome>(player_->GetTransform());
	skyDome_->Init();

	mainCamera.SetFollow(&player_->GetTransform());
	mainCamera.ChangeMode(Camera::MODE::FOLLOW);
}

void GameScene::Update(void)
{
	// シーン遷移
	InputManager& ins = InputManager::GetInstance();
	if (ins.IsTrgDown(KEY_INPUT_SPACE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}

	// モード切替
	if (ins.IsTrgDown(KEY_INPUT_BACKSLASH))
	{
		int tmp = (int)mode_;
		tmp++;
		if (tmp >= (int)MODE::MAX)
		{
			tmp = 0;
		}
		mode_ = (MODE)tmp;
	}

	skyDome_->Update();

	stage_->Update();

	player_->Update();
	
	for (const auto& enemy : enemys_)
	{
		enemy->Update();
	}

	// HIT時の斬る処理
	if (player_->IsSlashing() && !alreadyCutThisSwing_)
	{
		VECTOR tip = player_->GetSword()->GetTip();
		for (const auto& enemy : enemys_)
		{
			VECTOR center = enemy->GetTransform().pos;//moon_->GetCenter();

			VECTOR toTip = VSub(tip, center);

			float distance = VSize(toTip);
			float radius = 100.0f;//moon_->GetRadius();

			// 月の当たり判定を可視化
			DrawSphere3D(
				center,
				radius,
				16,
				GetColor(255, 0, 0),
				GetColor(255, 0, 0),
				FALSE
			);

			// 月の中心 → 剣先を線で表示
			DrawLine3D(
				center,
				tip,
				GetColor(0, 255, 0)
			);

			// 剣先を小さい球で表示
			DrawSphere3D(
				tip,
				10.0f,
				8,
				GetColor(0, 0, 255),
				GetColor(0, 0, 255),
				TRUE
			);

			// 判定
			if (distance < radius)
			{
				VECTOR origin, normal;
				if (player_->GetSword()->ComputeSwingPlane(origin, normal))
				{
					enemy->Cut(origin, normal);
					alreadyCutThisSwing_ = true;  // 1回の振りで何度も切らないようにする
				}
			}
		}
	}

	if (!player_->IsSlashing())
	{
		alreadyCutThisSwing_ = false;  // 振り終わったらリセット
	}
}

void GameScene::Draw(void)
{
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);

	// 背景
	skyDome_->Draw();
	stage_->Draw();
	
	for (const auto& enemy : enemys_)
	{
		enemy->Draw();
	}

	DrawUI();
	player_->Draw();
}

void GameScene::DrawUI(void)const
{
	const float gageX = 10.0f;
	const auto backColor = GetColor(255, 255, 255);
	const auto hpColor = GetColor(160, 0, 0);
	const auto energyColor = GetColor(160, 160, 0);

	const auto& status = player_->GetStatus();

	float hp_rate = (float)status.hp_ / Player::MAX_HP;                 // 0.0〜1.0
	hp_rate = std::clamp(hp_rate, 0.0f, 1.0f);
	DrawBoxAA(gageX, 5, gageX + MAX_GAGE_POS,	15, backColor, TRUE);
	DrawBoxAA(gageX, 5, gageX + MAX_GAGE_POS * hp_rate, 15, hpColor, TRUE);

	float energy_rate = (float)status.energy_ / Player::MAX_ENERGY;                 // 0.0〜1.0
	energy_rate = std::clamp(energy_rate, 0.0f, 1.0f);
	DrawBoxAA(gageX, 20, gageX + MAX_GAGE_POS,	30, backColor, TRUE);
	DrawBoxAA(gageX, 20, gageX + MAX_GAGE_POS * energy_rate, 30, energyColor, TRUE);

	// ヘルプ
	DrawFormatString(840, 20, 0x000000, "移動　　 ： WASD");
	DrawFormatString(840, 40, 0x000000, "カメラ　 ： 矢印キー");
	DrawFormatString(840, 60, 0x000000, "ダッシュ ： 右Shift");
	DrawFormatString(840, 80, 0x000000, "ジャンプ ： ＼(バクスラ)");
}
