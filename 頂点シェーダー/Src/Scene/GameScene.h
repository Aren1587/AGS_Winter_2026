#pragma once
#include <memory>
#include "SceneBase.h"
class Stage;
class SkyDome;
class Player;
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

private:

	// ステージ
	std::unique_ptr<Stage> stage_;

	// スカイドーム
	std::unique_ptr<SkyDome> skyDome_;

	// プレイヤー
	std::unique_ptr<Player> player_;

	// ポストエフェクトモード
	MODE mode_;

	// ポストエフェクト用スクリーン
	int postEffectScreen_;
	
	// ポストエフェクト用(モノクロ)
	std::unique_ptr<PixelMaterial> monoMaterial_;
	std::unique_ptr<PixelRenderer> monoRenderer_;

	// ポストエフェクト用(走査線)
	std::unique_ptr<PixelMaterial> scanMaterial_;
	std::unique_ptr<PixelRenderer> scanRenderer_;

	// ポストエフェクト用(ビネット)
	std::unique_ptr<PixelMaterial> vineMaterial_;
	std::unique_ptr<PixelRenderer> vineRenderer_;

	// ポストエフェクト用(レンズの歪み)
	std::unique_ptr<PixelMaterial> lensMaterial_;
	std::unique_ptr<PixelRenderer> lensRenderer_;
};
