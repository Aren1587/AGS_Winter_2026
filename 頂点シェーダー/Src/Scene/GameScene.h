#pragma once

#include <memory>
#include <vector>
#include <DxLib.h>
#include "SceneBase.h"
class Stage;
class SkyDome;
class Player;
class Enemy;
class PixelMaterial;
class PixelRenderer;
class CutEffect;
class InputManager;

class GameScene : public SceneBase
{
public:

	// ポストエフェクトモード
	enum class MODE
	{
		MAIN,
		MONO,
		SCAN,
		LENS,
		VINE,
		MAX
	};

	// コンストラクタ
	GameScene(void);

	// デストラクタ
	~GameScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;
	void DrawUI(void)const;

private:

	// 1回の斬撃判定で共通して使う情報
	struct SlashInfo
	{
		bool   useLine;      // ラインモードで判定するか
		VECTOR lineOrigin;   // ラインの切断面の原点
		VECTOR lineNormal;   // ラインの切断面の法線
		VECTOR tip;          // 剣先
		VECTOR hilt;         // 柄
		VECTOR camFront;     // カメラ前方
		VECTOR playerPos;    // プレイヤー位置
	};

	// 初期化
	void InitPlayer(void);
	void InitEnemies(void);
	void InitStage(void);
	void InitCamera(void);
	void InitShadow(void);

	// 更新
	void UpdateDebugInput(InputManager& ins);
	void CheckGameEnd(void);
	void UpdateModeChange(InputManager& ins);
	bool UpdateHitStop(void);
	void UpdateWorld(void);
	void UpdateTimers(void);
	void CheckPlayerDamage(void);
	void SeparateEnemies(void);

	// 斬撃
	void UpdateSlash(void);
	SlashInfo MakeSlashInfo(void) const;
	bool CheckSlashHit(Enemy& enemy, const SlashInfo& info, const VECTOR& center, VECTOR& origin, VECTOR& normal) const;
	void ApplySlashHit(Enemy& enemy, const SlashInfo& info, const VECTOR& center, const VECTOR& origin, const VECTOR& normal);

	void RemoveEnemies(void);

	// ---------- 描画 ----------
	void DrawWorld(void);
	void DrawWeakPoints(void) const;
	void DrawHitFlash(void) const;

	// UI
	void DrawGauges(void) const;
	void DrawGoalMessage(void) const;
	void DrawHelp(void) const;

	// ステージ
	std::unique_ptr<Stage> stage_;

	// スカイドーム
	std::unique_ptr<SkyDome> skyDome_;

	// プレイヤー
	std::unique_ptr<Player> player_;
	std::vector<std::unique_ptr<Enemy>> enemys_;

	std::unique_ptr<CutEffect> cutEffect_;
	int hitStopFrame_ = 0;

	// ポストエフェクトモード
	MODE mode_;

	int shadowMapHandle_;
	int fillLight_ = -1;

	bool alreadyCutThisSwing_ = false;

	int playerHitEffectFrame_ = 0;
	int prevPlayerHp_ = 100;

	int goalFontHandle_ = -1;
	int goalMessageFrame_ = 0;

	float worldTimeAccum_ = 0.0f;   // プレイヤー以外の時間の貯金
};