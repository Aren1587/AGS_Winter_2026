#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/SceneManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "Player.h"
#include "Gate.h"

Gate::Gate(
	Player& player, const Transform& transform) : player_(player)
{
	transform_ = transform;

	state_ = STATE::NONE;

	// 状態管理
	stateChanges_.emplace(STATE::IDLE, std::bind(&Gate::ChangeStateIdle, this));
	stateChanges_.emplace(STATE::RESERVE, std::bind(&Gate::ChangeStateReserve, this));
	stateChanges_.emplace(STATE::MOVE, std::bind(&Gate::ChangeStateMove, this));
}

Gate::~Gate(void)
{
}

void Gate::Init(void)
{
	time_ = 0.0f;

	// モデルの基本情報

	transform_.quaRot = Quaternion();

	// 水
	transform_.SetModel(
		resMng_.LoadModelDuplicate(
			ResourceManager::SRC::GATE)
	);
	transform_.scl = VScale({ 1.0f, 1.0f, 1.0f }, 1.0f);

	transform_.Update();
	ChangeState(STATE::IDLE);
	vertexMaterial_ = std::make_unique<VertexMaterial>(
		"GateVS.cso", 1,
		"GatePS.cso", 4);

	vertexMaterial_->AddConstBufVS({ 4.0f, 4.0f, 4.0f, time_ });

	auto dir = GetLightDirection();
	vertexMaterial_->AddConstBufPS({ dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->AddConstBufPS({ cameraPos.x, cameraPos.y, cameraPos.z, time_ });

	vertexMaterial_->AddConstBufPS({ 1.0f, 1.0f, 1.0f, 1.0f });
	vertexMaterial_->AddConstBufPS({ 0.0f, 0.0f, 0.0f, 1.0f });

	int image = LoadGraph((Application::PATH_MODEL + "Gate/Noise2.png").c_str(), true);

	vertexMaterial_->SetTextureAddress(VertexMaterial::TEXADDRESS::NONE);
	vertexMaterial_->SetTextureBuf(1, image);

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);

	ChangeState(STATE::IDLE);
}

void Gate::Update(void)
{
	time_ += (1.0f * SceneManager::GetInstance().GetDeltaTime() * 0.5f);

	vertexMaterial_->SetConstBufVS(0, { 4.0f, 4.0f, 4.0f, time_ });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->SetConstBufPS(1, { cameraPos.x, cameraPos.y, cameraPos.z, time_ });

	// 更新ステップ
	stateUpdate_();
}

void Gate::Draw(void)
{
	// 描画
	//MV1DrawModel(transform_.modelId);

	/*auto dir = GetLightDirection();
	vertexMaterial_->SetConstBufPS(0, { dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->SetConstBufPS(1, { cameraPos.x, cameraPos.y, cameraPos.z, time_ });*/
	vertexRenderer_->Draw();
}

void Gate::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();

}

void Gate::ChangeStateNone(void)
{
}

void Gate::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&Gate::UpdateIdle, this);
}

void Gate::ChangeStateReserve(void)
{
	stateUpdate_ = std::bind(&Gate::UpdateReserve, this);
}

void Gate::ChangeStateMove(void)
{
	stateUpdate_ = std::bind(&Gate::UpdateMove, this);
}

// 更新ステップ
void Gate::UpdateNone(void)
{

}
void Gate::UpdateIdle(void)
{

}
void Gate::UpdateReserve(void)
{

}
void Gate::UpdateMove(void)
{

}