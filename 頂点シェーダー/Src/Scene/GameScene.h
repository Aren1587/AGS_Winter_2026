#pragma once

#include <memory>
#include "SceneBase.h"
class Stage;
class SkyDome;
class Player;
class Enemy;
class PixelMaterial;
class PixelRenderer;

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
	void DrawUI(void);

private:

	// ステージ
	std::unique_ptr<Stage> stage_;

	// スカイドーム
	std::unique_ptr<SkyDome> skyDome_;

	// プレイヤー
	std::unique_ptr<Player> player_;
	std::vector<std::unique_ptr<Enemy>> enemys_;

	// ポストエフェクトモード
	MODE mode_;

	bool allReadyCutThisSwing_ = false;

	float hpPos;
};
