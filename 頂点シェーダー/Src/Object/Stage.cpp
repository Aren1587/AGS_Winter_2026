#include <vector>
#include <map>
#include <DxLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "WarpStar.h"
#include "Metal.h"
#include "Rim.h"
#include "Water.h"
#include "Gate.h"
#include "Player.h"
#include "Planet.h"
#include "Common/Collider.h"
#include "Common/Transform.h"
#include "Stage.h"

Stage::Stage(Player& player)
	: resMng_(ResourceManager::GetInstance()), player_(player)
{
	activeName_ = NAME::MAIN_PLANET;
	step_ = 0.0f;
}

Stage::~Stage(void)
{
	
	// ワープスター
	warpStars_.clear();
	
	// 惑星
	planets_.clear();

	// 月
	metal_.clear();

	rim_.clear();

	water_.clear();

	gate_.clear();
}

void Stage::Init(void)
{
	MakeMainStage();
	MakeWarpStar();
	MakeMetal();
	MakeRim();
	MakeWater();
	MakeGate();

	step_ = -1.0f;
}

void Stage::Update(void)
{

	// ワープスター
	for (const auto& s : warpStars_)
	{
		s->Update();
	}

	// メタル
	for (const auto& s : metal_)
	{
		s->Update();
	}

	// Rim
	for (const auto& s : rim_)
	{
		s->Update();
	}

	// 水
	for (const auto& s : water_)
	{
		s->Update();
	}

	for (const auto& s : gate_)
	{
		s->Update();
	}

	// 惑星
	for (const auto& s : planets_)
	{
		s.second->Update();
	}

}

void Stage::Draw(void)
{
	// ワープスター
	for (const auto& s : warpStars_)
	{
		s->Draw();
	}

	// 月
	for (const auto& s : metal_)
	{
		s->Draw();
	}

	// Rim
	for (const auto& s : rim_)
	{
		s->Draw();
	}

	// 水
	for (const auto& s : water_)
	{
		s->Draw();
	}

	// 惑星
	for (const auto& s : planets_)
	{
		s.second->Draw();
	}

	// gate
	for (const auto& s : gate_)
	{
		s->Draw();
	}

	// ワープスター
	for (const auto& s : warpStars_)
	{
		s->Draw();
	}

	// 月
	for (const auto& s : metal_)
	{
		s->Draw();
	}

	// Rim
	for (const auto& s : rim_)
	{
		s->Draw();
	}

	// 水
	for (const auto& s : water_)
	{
		s->Draw();
	}

	// 惑星
	for (const auto& s : planets_)
	{
		s.second->Draw();
	}

}

void Stage::ChangeStage(NAME type)
{

	activeName_ = type;

	// 対象のステージを取得する
	activePlanet_ = GetPlanet(activeName_);

	// ステージの当たり判定をプレイヤーに設定
	player_.ClearCollider();
	player_.AddCollider(activePlanet_.lock()->GetTransform().collider);

	step_ = TIME_STAGE_CHANGE;

}

std::weak_ptr<Planet> Stage::GetPlanet(NAME type)
{
	if (planets_.count(type) == 0)
	{
		return nullPlanet;
	}

	return planets_[type];
}

void Stage::MakeMainStage(void)
{

	// 最初の惑星
	//------------------------------------------------------------------------------
	Transform planetTrans;
	planetTrans.SetModel(
		resMng_.LoadModelDuplicate(ResourceManager::SRC::MAIN_PLANET));
	planetTrans.scl = AsoUtility::VECTOR_ONE;
	planetTrans.quaRot = Quaternion();
	planetTrans.pos = { 0.0f, -100.0f, 0.0f };

	// 当たり判定(コライダ)作成
	planetTrans.MakeCollider(Collider::TYPE::STAGE);

	planetTrans.Update();

	NAME name = NAME::MAIN_PLANET;
	std::shared_ptr<Planet> planet =
		std::make_shared<Planet>(
			name, Planet::TYPE::GROUND, planetTrans);
	planet->Init();
	planets_.emplace(name, std::move(planet));
	//------------------------------------------------------------------------------

}

void Stage::MakeWarpStar(void)
{

	Transform trans;
	std::unique_ptr<WarpStar> star;

	// 落とし穴惑星へのワープスター
	//------------------------------------------------------------------------------
	trans.pos = { -910.0f, 200.0f, 894.0f };
	trans.scl = { 0.6f, 0.6f, 0.6f };
	trans.quaRot = Quaternion::Euler(
		AsoUtility::Deg2RadF(-25.0f),
		AsoUtility::Deg2RadF(-50.0f),
		AsoUtility::Deg2RadF(0.0f)
	);

	star = std::make_unique<WarpStar>(player_, trans);
	star->Init();
	warpStars_.push_back(std::move(star));
	//------------------------------------------------------------------------------

}

void Stage::MakeMetal(void)
{
	Transform trans;
	std::unique_ptr<Metal> metal;

	//------------------------------------------------------------------------------
	trans.pos = { 0.0f, 0.0f, 894.0f };
	trans.scl = { 1.0f, 1.0f, 1.0f };
	trans.quaRot = Quaternion::Euler(
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f)
	);

	metal = std::make_unique<Metal>(player_, trans);
	metal->Init();
	metal_.push_back(std::move(metal));
	//------------------------------------------------------------------------------
}

void Stage::MakeRim(void)
{
	Transform trans;
	std::unique_ptr<Rim> rim;

	//------------------------------------------------------------------------------
	trans.pos = { 0.0f, 0.0f, 894.0f };
	trans.scl = { 1.0f, 1.0f, 1.0f };
	trans.quaRot = Quaternion::Euler(
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f)
	);

	rim = std::make_unique<Rim>(player_, trans);
	rim->Init();
	rim_.push_back(std::move(rim));
	//------------------------------------------------------------------------------
}

void Stage::MakeWater(void)
{
	Transform trans;
	std::unique_ptr<Water> water;

	//------------------------------------------------------------------------------
	trans.pos = { 0.0f, 0.0f, -600.0f };
	trans.scl = { 1.0f, 1.0f, 1.0f };
	trans.quaRot = Quaternion::Euler(
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f)
	);

	water = std::make_unique<Water>(player_, trans);
	water->Init();
	water_.push_back(std::move(water));
	//------------------------------------------------------------------------------
}

void Stage::MakeGate(void)
{
	Transform trans;
	std::unique_ptr<Gate> gate;

	//------------------------------------------------------------------------------
	trans.pos = { 0.0f, 0.0f, 0.0f };
	trans.scl = { 1.0f, 1.0f, 1.0f };
	trans.quaRot = Quaternion::Euler(
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f),
		AsoUtility::Deg2RadF(0.0f)
	);

	gate = std::make_unique<Gate>(player_, trans);
	gate->Init();
	gate_.push_back(std::move(gate));
	//------------------------------------------------------------------------------
}
