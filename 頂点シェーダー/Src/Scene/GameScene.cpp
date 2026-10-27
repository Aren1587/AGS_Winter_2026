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
#include "../Object/CutEffect.h"
#include "../Renderer/PixelMaterial.h"
#include "../Renderer/PixelRenderer.h"
#include "GameScene.h"

namespace
{
	constexpr int ENEMY_NUM = 5;

	constexpr float MAX_GAGE_POS = 150.0f;

	constexpr float LINE_HIT_RANGE = 3000.0f;

	constexpr float BODY_HEIGHT = 80.0f;   // 要調整

	constexpr float LINE_HIT_RADIUS = 50.0f;

	constexpr VECTOR ENEMY_START_POS = { -400.0f, -10.0f, 600.0f };
	constexpr float  ENEMY_SPACING = 200.0f;
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

		VECTOR pos = ENEMY_START_POS;
		pos.x += ENEMY_SPACING * i;

		e->Init(pos);
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

	cutEffect_ = std::make_unique<CutEffect>();

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

	bool isEnd = true;
	for (const auto& enemy : enemys_)
	{
		if (!enemy->GetIsCut())
		{
			isEnd = false;
			break;
		}
	}

	if (isEnd)
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::CLEAR);
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

	// ヒットストップ中はプレイヤーと敵を止める(演出だけ進める)
	if (hitStopFrame_ > 0)
	{
		hitStopFrame_--;
		cutEffect_->Update();
		return;
	}

	// 肩越しの度合いに合わせて、プレイヤー以外の時間を遅くする
	constexpr float SLOW_SCALE = 0.2f;
	const float worldTimeScale =
		1.0f + (SLOW_SCALE - 1.0f) * mainCamera.GetShoulderRate();

	// プレイヤーは常に等速
	player_->Update();

	// それ以外はスケール分だけ進める
	worldTimeAccum_ += worldTimeScale;
	while (worldTimeAccum_ >= 1.0f)
	{
		worldTimeAccum_ -= 1.0f;
		skyDome_->Update();
		stage_->Update();
		for (const auto& enemy : enemys_)
		{
			enemy->Update();
		}
	}

	// HIT時の斬る処理
	if (player_->IsSlashing() && !alreadyCutThisSwing_)
	{
		// ラインモードなら、ラインから切断面を作る
		VECTOR lineOrigin = VGet(0, 0, 0), lineNormal = VGet(0, 0, 0);
		const bool useLine =
			player_->IsLineAttack() &&
			player_->GetSlashPlane(lineOrigin, lineNormal);

		VECTOR tip = player_->GetSword()->GetTip();
		VECTOR camFront = GetCameraFrontVector();
		VECTOR playerPos = player_->GetTransform().pos;

		for (const auto& enemy : enemys_)
		{
			VECTOR center = enemy->GetTransform().pos;
			center.y += BODY_HEIGHT;
			float radius = 100.0f;

			VECTOR origin, normal;
			bool hit = false;

			if (useLine)
			{
				// 敵の中心が切断面から radius 以内で、カメラの前方かつ射程内
				VECTOR toCenter = VSub(center, lineOrigin);
				float planeDist = fabsf(VDot(toCenter, lineNormal));
				bool inFront = VDot(toCenter, camFront) > 0.0f;
				bool inRange = VSize(VSub(center, playerPos)) < LINE_HIT_RANGE;

				hit = planeDist < LINE_HIT_RADIUS && inFront && inRange;
				origin = lineOrigin;
				normal = lineNormal;
			}
			else
			{
				// 通常攻撃は今までどおり剣の軌跡
				hit = VSize(VSub(tip, center)) < radius &&
					player_->GetSword()->ComputeSwingPlane(origin, normal);
			}

			if (!hit) continue;

			enemy->Cut(origin, normal);
			player_->HitProcess();
			alreadyCutThisSwing_ = true;

			// 演出の中心 = 敵の中心を切断平面へ射影した点
			VECTOR toCenter = VSub(center, origin);
			VECTOR onPlane = VSub(center, VScale(normal, VDot(toCenter, normal)));

			// 演出の向き: ラインモードならライン方向、通常なら刃の向き
			VECTOR bladeDir = useLine
				? VNorm(VCross(normal, camFront))
				: VSub(tip, player_->GetSword()->GetHilt());

			cutEffect_->Start(onPlane, normal, bladeDir);
			hitStopFrame_ = 6;
		}
	}

	if (!player_->IsSlashing())
	{
		alreadyCutThisSwing_ = false;  // 振り終わったらリセット
	}
	cutEffect_->Update();
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

	cutEffect_->Draw();

	DrawUI();
	player_->Draw();

	cutEffect_->DrawScreen();
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
