#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/SceneManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "Player.h"
#include "Water.h"

Water::Water(
	Player& player, const Transform& transform) : player_(player)
{
	transform_ = transform;

	state_ = STATE::NONE;

	// 状態管理
	stateChanges_.emplace(STATE::IDLE, std::bind(&Water::ChangeStateIdle, this));
	stateChanges_.emplace(STATE::RESERVE, std::bind(&Water::ChangeStateReserve, this));
	stateChanges_.emplace(STATE::MOVE, std::bind(&Water::ChangeStateMove, this));
}

Water::~Water(void)
{
}

void Water::Init(void)
{
	time_ = 0.0f;

	// モデルの基本情報

	transform_.quaRot = Quaternion();

	// 水
	transform_.SetModel(
		resMng_.LoadModelDuplicate(
			ResourceManager::SRC::WATER)
	);
	transform_.scl = VScale({ 4.0f, 1.0f, 4.0f }, 1.0f);

	transform_.Update();
	ChangeState(STATE::IDLE);
	vertexMaterial_ = std::make_unique<VertexMaterial>(
		"WaterVS.cso", 1,
		"WaterPS.cso", 4);

	vertexMaterial_->AddConstBufVS({ 4.0f, 4.0f, 4.0f, time_ });

	auto dir = GetLightDirection();
	vertexMaterial_->AddConstBufPS({ dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->AddConstBufPS({ cameraPos.x, cameraPos.y, cameraPos.z, time_ });

	vertexMaterial_->AddConstBufPS({ 1.0f, 1.0f, 1.0f, 1.0f });
	vertexMaterial_->AddConstBufPS({ 0.0f, 0.0f, 0.0f, 1.0f });

	vertexMaterial_->SetTextureAddress(VertexMaterial::TEXADDRESS::WRAP);

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);

	ChangeState(STATE::IDLE);
}

void Water::Update(void)
{
	time_ += (1.0f * SceneManager::GetInstance().GetDeltaTime() * 0.5f);

	vertexMaterial_->SetConstBufVS(0, { 4.0f, 4.0f, 4.0f, time_ });
	
	auto cameraPos = GetCameraPosition();
	vertexMaterial_->SetConstBufPS(1, { cameraPos.x, cameraPos.y, cameraPos.z, time_ });

	// 更新ステップ
	stateUpdate_();
}

void Water::Draw(void)
{
	// 描画
	//MV1DrawModel(transform_.modelId);

	/*auto dir = GetLightDirection();
	vertexMaterial_->SetConstBufPS(0, { dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->SetConstBufPS(1, { cameraPos.x, cameraPos.y, cameraPos.z, time_ });*/
	vertexRenderer_->Draw();
}

void Water::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();

}

void Water::ChangeStateNone(void)
{
}

void Water::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&Water::UpdateIdle, this);
}

void Water::ChangeStateReserve(void)
{
	stateUpdate_ = std::bind(&Water::UpdateReserve, this);
}

void Water::ChangeStateMove(void)
{
	stateUpdate_ = std::bind(&Water::UpdateMove, this);
}

// 更新ステップ
void Water::UpdateNone(void)
{

}
void Water::UpdateIdle(void)
{

}
void Water::UpdateReserve(void)
{

}
void Water::UpdateMove(void)
{

}