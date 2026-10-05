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
#include "../Object/Moon.h"
#include "../Renderer/PixelMaterial.h"
#include "../Renderer/PixelRenderer.h"
#include "GameScene.h"

GameScene::GameScene(void)
	:
	player_(nullptr),
	skyDome_(nullptr),
	stage_(nullptr)
{
}

GameScene::~GameScene(void)
{
	DeleteGraph(postEffectScreen_);
}

void GameScene::Init(void)
{

	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Init();

	moon_ = std::make_unique<Moon>();
	moon_->Init();

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

	// ポストエフェクト用スクリーン
	postEffectScreen_ = MakeScreen(
		Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y, true);

	// ポストエフェクト用(モノトーン)
	monoMaterial_ = std::make_unique<PixelMaterial>("Monotone.cso", 1);
	monoMaterial_->AddConstBuf({ 1.0f, 1.0f, 1.0f, 1.0f });
	monoMaterial_->AddTextureBuf(SceneManager::GetInstance().GetMainScreen());
	monoRenderer_ = std::make_unique<PixelRenderer>(*monoMaterial_);
	monoRenderer_->MakeSquereVertex(
		Vector2(0, 0),
		Vector2(Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y)
	);

	// ポストエフェクト用(走査線)
	scanMaterial_ = std::make_unique<PixelMaterial>("ScanLine.cso", 2);
	scanMaterial_->AddConstBuf({ 1.0f, 1.0f, 1.0f, 1.0f });
	scanMaterial_->AddConstBuf({ 0.0f, 0.0f, 0.0f, 0.0f });
	scanMaterial_->AddTextureBuf(SceneManager::GetInstance().GetMainScreen());
	scanRenderer_ = std::make_unique<PixelRenderer>(*scanMaterial_);
	scanRenderer_->MakeSquereVertex(
		Vector2(0, 0),
		Vector2(Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y)
	);

	// ポストエフェクト用(ビネット)
	vineMaterial_ = std::make_unique<PixelMaterial>("Vignette.cso", 1);
	vineMaterial_->AddConstBuf({ 3.5f, 0.0f, 0.0f, 0.0f });
	vineMaterial_->AddTextureBuf(SceneManager::GetInstance().GetMainScreen());
	vineRenderer_ = std::make_unique<PixelRenderer>(*vineMaterial_);
	vineRenderer_->MakeSquereVertex(
		Vector2(0, 0),
		Vector2(Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y)
	);

	// ポストエフェクト用(レンズの歪み)
	lensMaterial_ = std::make_unique<PixelMaterial>("LensDistortion.cso", 1);
	lensMaterial_->AddConstBuf({ 3.5f, 0.0f, 0.0f, 0.0f });
	lensMaterial_->AddTextureBuf(SceneManager::GetInstance().GetMainScreen());
	lensRenderer_ = std::make_unique<PixelRenderer>(*lensMaterial_);
	lensRenderer_->MakeSquereVertex(
		Vector2(0, 0),
		Vector2(Application::SCREEN_SIZE_X, Application::SCREEN_SIZE_Y)
	);

	//vertexMaterial_ = std::make_unique<VertexMaterial>("VertexShader.cso", 1);
	//vertexMaterial_->AddConstBuf({ 1.0f, 1.0f, 1.0f, 1.0f });
	//vertexMaterial_->AddTextureBuf(SceneManager::GetInstance().GetMainScreen());
	//for(auto& star : stage_->GetWarpStar())
	//{
	//	//vertexRenderer_ = std::make_unique<VertexRenderer>(*vertexMaterial_, );
	//}

	// 初期モード
	mode_ = MODE::MAIN;

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
	moon_->Update();

	if (player_->IsSlashing() && !alreadyCutThisSwing_)
	{
		VECTOR tip = player_->GetSword().GetTip();
		VECTOR center = moon_->GetCenter();

		VECTOR toTip = VSub(tip, center);

		float distance = VSize(toTip);
		float radius = moon_->GetRadius();

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
			if (player_->GetSword().ComputeSwingPlane(origin, normal))
			{
				moon_->Cut(origin, normal);
				alreadyCutThisSwing_ = true;  // 1回の振りで何度も切らないようにする
			}
		}
	}
	alreadyCutThisSwing_ = false;  // 振り終わったらリセット
	if (!player_->IsSlashing())
	{
		alreadyCutThisSwing_ = false;  // 振り終わったらリセット
	}
}

void GameScene::Draw(void)
{
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);

	int mainScreen = SceneManager::GetInstance().GetMainScreen();
	// 背景
	skyDome_->Draw();
	stage_->Draw();
	
	moon_->Draw();
	player_->Draw();

	// ヘルプ
	DrawFormatString(840, 20, 0x000000, "移動　　：WASD");
	DrawFormatString(840, 40, 0x000000, "カメラ　：矢印キー");
	DrawFormatString(840, 60, 0x000000, "ダッシュ：右Shift");
	DrawFormatString(840, 80, 0x000000, "ジャンプ：＼(バクスラ)");

	//for (auto& star : stage_->GetWarpStar())
	//{
	//	
	//}

	//// ポストエフェクト(モノクロ)
	////-----------------------------------------
	//if (mode_ < MODE::MONO) { return; }
	//
	//SetDrawScreen(postEffectScreen_);

	//// 画面を初期化
	//ClearDrawScreen();

	//monoRenderer_->Draw();

	//// メインに戻す
	//SetDrawScreen(mainScreen);
	//DrawGraph(0, 0, postEffectScreen_, false);
	////-----------------------------------------

	//// ポストエフェクト(走査線)
	////-----------------------------------------
	//if (mode_ < MODE::SCAN) { return; }
	//
	//SetDrawScreen(postEffectScreen_);

	//// 画面を初期化
	//ClearDrawScreen();

	//scanMaterial_->SetConstBuf(
	//	1,
	//	{ SceneManager::GetInstance().GetTotalTime(), 0.0f, 0.0f, 0.0f }
	//);
	//scanRenderer_->Draw();

	//// メインに戻す
	//SetDrawScreen(mainScreen);
	//DrawGraph(0, 0, postEffectScreen_, false);
	////-----------------------------------------


	//// ポストエフェクト(レンズの歪み)
	////-----------------------------------------
	//if (mode_ < MODE::LENS) { return; }

	//SetDrawScreen(postEffectScreen_);

	//// 画面を初期化
	//ClearDrawScreen();

	//lensRenderer_->Draw();

	//// メインに戻す
	//SetDrawScreen(mainScreen);
	//DrawGraph(0, 0, postEffectScreen_, false);
	////-----------------------------------------
	//

	//// ポストエフェクト(ビネット)
	////-----------------------------------------
	//if (mode_ < MODE::VINE) { return; }

	//SetDrawScreen(postEffectScreen_);

	//// 画面を初期化
	//ClearDrawScreen();

	//vineRenderer_->Draw();

	//// メインに戻す
	//SetDrawScreen(mainScreen);
	//DrawGraph(0, 0, postEffectScreen_, false);
	////-----------------------------------------
}
