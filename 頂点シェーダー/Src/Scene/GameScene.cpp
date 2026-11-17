#include <algorithm>
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

	// 斬撃
	constexpr float SLASH_HIT_RADIUS = 100.0f;   // 通常攻撃の当たり半径
	constexpr float CORE_HIT_RADIUS = 40.0f;     // 弱点の当たり半径
	constexpr int   HIT_STOP_CUT = 6;            // 切断時のヒットストップ
	constexpr int   HIT_STOP_DAMAGE = 3;         // ダメージ時のヒットストップ

	// 被弾
	constexpr int HIT_EFFECT_FRAME = 20;
	constexpr int HIT_STOP_DAMAGED = 4;

	// スロー
	constexpr float SLOW_SCALE = 0.2f;

	// 敵の押しのけ
	constexpr float ENEMY_SEP_RADIUS = 80.0f;

	// ゴールメッセージ
	constexpr int GOAL_MSG_FRAME = 240;
	constexpr int GOAL_MSG_FADE = 30;

	// シャドウマップ
	constexpr int SHADOW_MAP_SIZE = 8192;
	constexpr VECTOR SHADOW_LIGHT_DIR = { 0.3f, -0.7f, -0.8f };
	constexpr VECTOR SHADOW_AREA_MIN = { -5000.0f, -1000.0f, -5000.0f };
	constexpr VECTOR SHADOW_AREA_MAX = { 5000.0f,  1000.0f,  5000.0f };
}

GameScene::GameScene(void)
	:
	player_(nullptr),
	skyDome_(nullptr),
	stage_(nullptr),
	mode_(MODE::MAIN),
	shadowMapHandle_(-1)
{
}

GameScene::~GameScene(void)
{
	if (goalFontHandle_ != -1) DeleteFontToHandle(goalFontHandle_);

	// シャドウマップ削除
	if (shadowMapHandle_ != -1)
	{
		DeleteShadowMap(shadowMapHandle_);
		shadowMapHandle_ = -1;
	}
}

// ============================================================
// 初期化
// ============================================================

void GameScene::Init(void)
{
	InitPlayer();
	InitEnemies();
	InitStage();

	cutEffect_ = std::make_unique<CutEffect>();

	InitCamera();

	InitShadow();

	goalFontHandle_ = CreateFontToHandle(nullptr, 60, 3, DX_FONTTYPE_ANTIALIASING);
	goalMessageFrame_ = 0;

	SetLightDirection(VGet(0.0f, -1.0f, 0.0f));
	SetShadowMapLightDirection(shadowMapHandle_, VGet(0.0f, -1.0f, 0.0f));
	SetLightAmbColor(GetColorF(0.0f, 0.0f, 0.0f, 0.0f));

	fillLight_ = CreateDirLightHandle(VGet(-0.3f, -0.3f, 1.0f));      // カメラ側から奥へ向かう光
	SetLightDifColorHandle(fillLight_, GetColorF(0.4f, 0.4f, 0.4f, 1.0f)); // 弱め
	SetLightSpcColorHandle(fillLight_, GetColorF(0.0f, 0.0f, 0.0f, 0.0f));
	SetLightAmbColorHandle(fillLight_, GetColorF(0.0f, 0.0f, 0.0f, 0.0f));
}

void GameScene::InitPlayer(void)
{
	player_ = std::make_unique<Player>();
	player_->Init();
}

void GameScene::InitEnemies(void)
{
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		auto e = std::make_unique<Enemy>(*player_);

		VECTOR pos = ENEMY_START_POS;
		pos.x += ENEMY_SPACING * i;

		e->Init(pos);
		enemys_.emplace_back(std::move(e));
	}
}

void GameScene::InitStage(void)
{
	// ステージ
	stage_ = std::make_unique<Stage>(*player_);
	stage_->Init();
	stage_->ChangeStage(Stage::NAME::MAIN_PLANET);

	// スカイドーム
	skyDome_ = std::make_unique<SkyDome>(player_->GetTransform());
	skyDome_->Init();
}

void GameScene::InitCamera(void)
{
	mainCamera.SetFollow(&player_->GetTransform());
	mainCamera.ChangeMode(Camera::MODE::FOLLOW);
}

void GameScene::InitShadow(void)
{
	// シャドウマップ作成
	shadowMapHandle_ = MakeShadowMap(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	if (shadowMapHandle_ == -1) { printfDx("ShadowMap 作成失敗\n"); }

	// ライトの方向 
	SetShadowMapLightDirection(shadowMapHandle_, SHADOW_LIGHT_DIR);

	// 影を描画する空間範囲
	SetShadowMapDrawArea(shadowMapHandle_, SHADOW_AREA_MIN, SHADOW_AREA_MAX);
}

// ============================================================
// 更新
// ============================================================

void GameScene::Update(void)
{
	InputManager& ins = InputManager::GetInstance();

	UpdateDebugInput(ins);
	CheckGameEnd();
	UpdateModeChange(ins);

	// ヒットストップ中はここで終了(演出だけ進める)
	if (UpdateHitStop()) return;

	UpdateWorld();
	UpdateTimers();
	CheckPlayerDamage();
	SeparateEnemies();
	UpdateSlash();
	RemoveEnemies();

	cutEffect_->Update();
}

void GameScene::UpdateDebugInput(InputManager& ins)
{
#ifdef _DEBUG
	// シーン遷移
	if (ins.IsTrgDown(KEY_INPUT_SPACE))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
	}

	if (ins.IsTrgDown(KEY_INPUT_V))
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::CLEAR);
	}
#endif // _DEBUG
}

void GameScene::CheckGameEnd(void)
{
	// 敵を全滅させたら終了
	if (enemys_.empty())
	{
		SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::CLEAR);
	}
}

void GameScene::UpdateModeChange(InputManager& ins)
{
	if (!ins.IsTrgDown(KEY_INPUT_BACKSLASH)) return;

	int tmp = (int)mode_ + 1;
	if (tmp >= (int)MODE::MAX)
	{
		tmp = 0;
	}
	mode_ = (MODE)tmp;
}

bool GameScene::UpdateHitStop(void)
{
	// ヒットストップ中はプレイヤーと敵を止める(演出だけ進める)
	if (hitStopFrame_ <= 0) return false;

	hitStopFrame_--;
	cutEffect_->Update();
	return true;
}

void GameScene::UpdateWorld(void)
{
	// 肩越しの度合いに合わせて、プレイヤー以外の時間を遅くする
	const bool isSlow = SceneManager::GetInstance().GetTimeScale() < 1.0f;
	const float worldTimeScale = isSlow
		? SLOW_SCALE
		: 1.0f + (SLOW_SCALE - 1.0f) * mainCamera.GetShoulderRate();

	// スロー中はプレイヤーも worldTimeAccum_ 経由。通常時はプレイヤー常に等速
	if (!isSlow)
	{
		player_->Update();
	}

	// それ以外はスケール分だけ進める
	worldTimeAccum_ += worldTimeScale;
	while (worldTimeAccum_ >= 1.0f)
	{
		worldTimeAccum_ -= 1.0f;
		if (isSlow) player_->Update();
		skyDome_->Update();
		stage_->Update();
		for (const auto& enemy : enemys_)
		{
			enemy->Update();
		}
	}
}

void GameScene::UpdateTimers(void)
{
	// ゴールメッセージタイマー
	if (goalMessageFrame_ < GOAL_MSG_FRAME) ++goalMessageFrame_;

	// プレイヤー被弾エフェクト
	if (playerHitEffectFrame_ > 0) --playerHitEffectFrame_;
}

void GameScene::CheckPlayerDamage(void)
{
	// HPが減っていたら被弾演出
	int curHp = player_->GetStatus().hp_;
	if (curHp < prevPlayerHp_)
	{
		playerHitEffectFrame_ = HIT_EFFECT_FRAME;
		hitStopFrame_ = HIT_STOP_DAMAGED;
	}
	prevPlayerHp_ = curHp;
}

void GameScene::SeparateEnemies(void)
{
	// 敵同士が重ならないよう押しのける
	for (int si = 0; si < (int)enemys_.size(); si++)
	{
		for (int sj = si + 1; sj < (int)enemys_.size(); sj++)
		{
			VECTOR diff = VSub(enemys_[si]->GetTransform().pos, enemys_[sj]->GetTransform().pos);
			diff.y = 0.0f;
			float dist = VSize(diff);
			if (dist < ENEMY_SEP_RADIUS && dist > 0.001f)
			{
				float push = (ENEMY_SEP_RADIUS - dist) * 0.5f;
				VECTOR pushVec = VScale(VNorm(diff), push);
				enemys_[si]->Separate(pushVec);
				enemys_[sj]->Separate(VScale(pushVec, -1.0f));
			}
		}
	}
}

// 斬撃
void GameScene::UpdateSlash(void)
{
	if (!player_->IsSlashing())
	{
		alreadyCutThisSwing_ = false;  // 振り終わったらリセット
		return;
	}

	if (alreadyCutThisSwing_) return;

	const SlashInfo info = MakeSlashInfo();

	for (const auto& enemy : enemys_)
	{
		VECTOR center = enemy->GetTransform().pos;
		center.y += BODY_HEIGHT;

		VECTOR origin, normal;
		if (!CheckSlashHit(*enemy, info, center, origin, normal)) continue;

		ApplySlashHit(*enemy, info, center, origin, normal);
	}
}

GameScene::SlashInfo GameScene::MakeSlashInfo(void) const
{
	SlashInfo info{};

	// ラインモードなら、ラインから切断面を作る
	info.lineOrigin = VGet(0, 0, 0);
	info.lineNormal = VGet(0, 0, 0);
	info.useLine =
		player_->IsLineAttack() &&
		player_->GetSlashPlane(info.lineOrigin, info.lineNormal);

	// ※元コードでは敵ループ内で useLine = false にしていたため、
	//   現状はラインモードを無効化している(挙動を合わせる)
	info.useLine = false;

	info.tip = player_->GetSword()->GetTip();
	info.hilt = player_->GetSword()->GetHilt();
	info.camFront = GetCameraFrontVector();
	info.playerPos = player_->GetTransform().pos;

	return info;
}

bool GameScene::CheckSlashHit(Enemy& enemy, const SlashInfo& info,
	const VECTOR& center, VECTOR& origin, VECTOR& normal) const
{
	if (info.useLine)
	{
		// 敵の中心が切断面から一定距離以内で、カメラの前方かつ射程内
		VECTOR toCenter = VSub(center, info.lineOrigin);
		float planeDist = fabsf(VDot(toCenter, info.lineNormal));
		bool inFront = VDot(toCenter, info.camFront) > 0.0f;
		bool inRange = VSize(VSub(center, info.playerPos)) < LINE_HIT_RANGE;

		origin = info.lineOrigin;
		normal = info.lineNormal;
		return planeDist < LINE_HIT_RADIUS && inFront && inRange;
	}

	// 通常攻撃は剣の軌跡
	return VSize(VSub(info.tip, center)) < SLASH_HIT_RADIUS &&
		player_->GetSword()->ComputeSwingPlane(origin, normal);
}

void GameScene::ApplySlashHit(Enemy& enemy, const SlashInfo& info,
	const VECTOR& center, const VECTOR& origin, const VECTOR& normal)
{
	// 弱点(コア)への命中判定
	const VECTOR corePos = enemy.GetCorePos();
	const float coreDist = info.useLine
		? fabsf(VDot(VSub(corePos, info.lineOrigin), info.lineNormal))
		: VSize(VSub(info.tip, corePos));
	const bool hitCore = coreDist < CORE_HIT_RADIUS;

	player_->HitProcess();
	alreadyCutThisSwing_ = true;

	// 演出の中心 = 敵の中心を切断平面へ射影した点
	VECTOR toCenter = VSub(center, origin);
	VECTOR onPlane = VSub(center, VScale(normal, VDot(toCenter, normal)));

	// 演出の向き: ラインモードならライン方向、通常なら刃の向き
	VECTOR bladeDir = info.useLine
		? VNorm(VCross(normal, info.camFront))
		: VSub(info.tip, info.hilt);

	if (hitCore)
	{
		// 弱点に当たったら即切断
		enemy.Cut(origin, normal);
		hitStopFrame_ = HIT_STOP_CUT;
	}
	else
	{
		enemy.TakeDamage(1);

		if (enemy.IsDead())
		{
			enemy.Cut(origin, normal);
			hitStopFrame_ = HIT_STOP_CUT;
		}
		else
		{
			hitStopFrame_ = HIT_STOP_DAMAGE;
		}
	}

	cutEffect_->Start(onPlane, normal, bladeDir);
}

void GameScene::RemoveEnemies(void)
{
	// 消えてよい敵を配列から取り除く
	enemys_.erase(
		std::remove_if(enemys_.begin(), enemys_.end(),
			[](const std::unique_ptr<Enemy>& e) { return e->IsRemovable(); }),
		enemys_.end());
}

// ============================================================
// 描画
// ============================================================

void GameScene::Draw(void)
{
	// ---- シャドウマップへ書き込み ----
	ShadowMap_DrawSetup(shadowMapHandle_);
	player_->Draw();
	stage_->Draw();
	for (const auto& enemy : enemys_) enemy->Draw();
	ShadowMap_DrawEnd();

	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);

	// スカイドームは影なし
	skyDome_->Draw();

	// ---- 影を受けるもの ----
	SetUseShadowMap(0, shadowMapHandle_);
	stage_->Draw();
	for (const auto& enemy : enemys_) enemy->Draw();
	player_->Draw();
	SetUseShadowMap(0, -1);   // 必ず解除

	cutEffect_->Draw();

	// ---- 2D ----
	DrawUI();
	if (player_->IsBladeMode()) DrawWeakPoints();
	cutEffect_->DrawScreen();
	DrawHitFlash();

#ifdef _DEBUG
	// シャドウマップの中身を左上に表示（何も映っていなければ書き込みの段階で失敗）
	TestDrawShadowMap(shadowMapHandle_, 0, 0, 256, 256);
#endif
}

void GameScene::DrawWorld(void)
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
}

void GameScene::DrawWeakPoints(void) const
{
	constexpr int X_SIZE = 10;
	constexpr int Y_SIZE = 10;

	for (const auto& enemy : enemys_)
	{
		auto weakPos = ConvWorldPosToScreenPos(enemy->GetCorePos());
		const auto x1 = weakPos.x - X_SIZE / 2;
		const auto x2 = weakPos.x + X_SIZE / 2;
		const auto y1 = weakPos.y - Y_SIZE / 2;
		const auto y2 = weakPos.y + Y_SIZE / 2;
		DrawBoxAA(x1, y1, x2, y2, 0xffffff, true);
	}
}

void GameScene::DrawHitFlash(void) const
{
	// 被弾赤フラッシュ
	if (playerHitEffectFrame_ <= 0) return;

	int alpha = (int)(180.0f * playerHitEffectFrame_ / HIT_EFFECT_FRAME);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawBox(0, 0, Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y, GetColor(255, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// UI
void GameScene::DrawUI(void)const
{
	DrawGauges();
	DrawGoalMessage();
	DrawHelp();
}

void GameScene::DrawGauges(void) const
{
	const float gageX = 10.0f;
	const auto backColor = GetColor(255, 255, 255);
	const auto hpColor = GetColor(160, 0, 0);
	const auto energyColor = GetColor(160, 160, 0);

	const auto& status = player_->GetStatus();

	// HP
	float hp_rate = (float)status.hp_ / Player::MAX_HP;                 // 0.0〜1.0
	hp_rate = std::clamp(hp_rate, 0.0f, 1.0f);
	DrawBoxAA(gageX, 5, gageX + MAX_GAGE_POS, 15, backColor, TRUE);
	DrawBoxAA(gageX, 5, gageX + MAX_GAGE_POS * hp_rate, 15, hpColor, TRUE);

	// エネルギー
	float energy_rate = (float)status.energy_ / Player::MAX_ENERGY;     // 0.0〜1.0
	energy_rate = std::clamp(energy_rate, 0.0f, 1.0f);
	DrawBoxAA(gageX, 20, gageX + MAX_GAGE_POS, 30, backColor, TRUE);
	DrawBoxAA(gageX, 20, gageX + MAX_GAGE_POS * energy_rate, 30, energyColor, TRUE);
}

void GameScene::DrawGoalMessage(void) const
{
	// ゴールメッセージ（最初の GOAL_MSG_FRAME フレーム）
	if (goalMessageFrame_ >= GOAL_MSG_FRAME || goalFontHandle_ == -1) return;

	// フェードイン・アウト
	int alpha = 255;
	if (goalMessageFrame_ < GOAL_MSG_FADE)
		alpha = goalMessageFrame_ * 255 / GOAL_MSG_FADE;
	else if (goalMessageFrame_ > GOAL_MSG_FRAME - GOAL_MSG_FADE)
		alpha = (GOAL_MSG_FRAME - goalMessageFrame_) * 255 / GOAL_MSG_FADE;

	const char* msg = "";//全ての敵を倒せ";
	int w = GetDrawStringWidthToHandle(msg, (int)strlen(msg), goalFontHandle_);
	int x = (Application::SCREEN_SIZE_X - w) / 2;
	int y = Application::SCREEN_SIZE_Y / 3;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringToHandle(x, y - 2, msg, GetColor(0, 0, 0), goalFontHandle_);   // 影
	DrawStringToHandle(x, y, msg, GetColor(255, 220, 50), goalFontHandle_);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void GameScene::DrawHelp(void) const
{
	DrawFormatString(840, 20, 0x000000, "移動　　 ： WASD");
	DrawFormatString(840, 40, 0x000000, "カメラ　 ： 矢印キー");
	DrawFormatString(840, 60, 0x000000, "ダッシュ ： 右Shift");
	DrawFormatString(840, 80, 0x000000, "ジャンプ ： ＼(バクスラ)");
}