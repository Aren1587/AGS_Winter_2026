#pragma once

#include <memory>
#include "SceneBase.h"
class Stage;
class SkyDome;
class Player;
class Enemy;
class PixelMaterial;
class PixelRenderer;
class CutEffect;

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

	bool alreadyCutThisSwing_ = false;

	float worldTimeScale_ = 1.0f;   // 現在のスケール(滑らかに変化させる)
	float worldTimeAccum_ = 0.0f;   // プレイヤー以外の時間の貯金
};
