#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "Player.h"
#include "Rim.h"

Rim::Rim(
	Player& player, const Transform& transform) : player_(player)
{
	transform_ = transform;

	state_ = STATE::NONE;

	// 状態管理
	stateChanges_.emplace(STATE::IDLE, std::bind(&Rim::ChangeStateIdle, this));
	stateChanges_.emplace(STATE::RESERVE, std::bind(&Rim::ChangeStateReserve, this));
	stateChanges_.emplace(STATE::MOVE, std::bind(&Rim::ChangeStateMove, this));
}

Rim::~Rim(void)
{
}

void Rim::Init(void)
{
	// モデルの基本情報

	transform_.quaRot = Quaternion();

	// 月
	transform_.SetModel(
		resMng_.LoadModelDuplicate(
			ResourceManager::SRC::MOON)
	);
	transform_.scl = VScale(AsoUtility::VECTOR_ONE, 1.0f);




	transform_.Update();

	vertexMaterial_ = std::make_unique<VertexMaterial>(
		"RimShaderVS.cso", 0,
		"RimShaderPS.cso", 4);

	auto dir = GetLightDirection();
	vertexMaterial_->AddConstBufPS({ dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->AddConstBufPS({ cameraPos.x, cameraPos.y, cameraPos.z, 10.0f });

	vertexMaterial_->AddConstBufPS({ 1.0f, 1.0f, 1.0f, 1.0f });
	vertexMaterial_->AddConstBufPS({ 0.0f, 0.0f, 0.0f, 1.0f });

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);

	ChangeState(STATE::IDLE);
}

void Rim::Update(void)
{
	// 更新ステップ
	stateUpdate_();
}

void Rim::Draw(void)
{
	// 描画
	//MV1DrawModel(transform_.modelId);

	auto dir = GetLightDirection();
	vertexMaterial_->SetConstBufPS(0, { dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->SetConstBufPS(1, { cameraPos.x, cameraPos.y, cameraPos.z, 10.0f });

	//MV1DrawModel(transform_.modelId);
	vertexRenderer_->Draw();
}

void Rim::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();

}

void Rim::ChangeStateNone(void)
{
}

void Rim::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&Rim::UpdateIdle, this);
}

void Rim::ChangeStateReserve(void)
{
	stateUpdate_ = std::bind(&Rim::UpdateReserve, this);
}

void Rim::ChangeStateMove(void)
{
	stateUpdate_ = std::bind(&Rim::UpdateMove, this);
}

// 更新ステップ
void Rim::UpdateNone(void)
{

}
void Rim::UpdateIdle(void)
{

}
void Rim::UpdateReserve(void)
{

}
void Rim::UpdateMove(void)
{

}