#include <DxLib.h>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/ResourceManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "Player.h"
#include "Metal.h"

Metal::Metal(
	Player& player, const Transform& transform) : player_(player)
{
	transform_ = transform;

	state_ = STATE::NONE;

	// 状態管理
	stateChanges_.emplace(STATE::IDLE, std::bind(&Metal::ChangeStateIdle, this));
	stateChanges_.emplace(STATE::RESERVE, std::bind(&Metal::ChangeStateReserve, this));
	stateChanges_.emplace(STATE::MOVE, std::bind(&Metal::ChangeStateMove, this));
}

Metal::~Metal(void)
{
}

void Metal::Init(void)
{
	// モデルの基本情報
	
	transform_.quaRot = Quaternion();

	//// 月
	//transform_.SetModel(
	//	resMng_.LoadModelDuplicate(
	//		ResourceManager::SRC::MOON)
	//);
	//transform_.scl = VScale(AsoUtility::VECTOR_ONE, 1.0f);

	//// 盾
	//transform_.SetModel(
	//	resMng_.LoadModelDuplicate(
	//		ResourceManager::SRC::SHIELD)
	//);
	//transform_.scl = VScale(AsoUtility::VECTOR_ONE, 1.0f);
	//transform_.quaRotLocal =
	//	Quaternion::Euler(
	//		AsoUtility::Deg2RadF(0.0f),
	//		AsoUtility::Deg2RadF(30.0f),
	//		0.0f
	//	);

	// 斧
	transform_.SetModel(
		resMng_.LoadModelDuplicate(
			ResourceManager::SRC::AXE)
	);
	transform_.scl = { 10.0f,10.0,10.0 };
	transform_.pos = { 0.0f, 10.0f, 894.0f };
	transform_.quaRotLocal =
		Quaternion::Euler(
			AsoUtility::Deg2RadF(-90.0f),
			AsoUtility::Deg2RadF(0.0f),
			0.0f
		);




	transform_.Update();

	vertexMaterial_ = std::make_unique<VertexMaterial>("MetalModelVS.cso", 0,
														"MetalModelPS.cso", 5);

	auto dir = GetLightDirection();
	vertexMaterial_->AddConstBufPS({ dir.x, dir.y, dir.z, 0.0f });

	auto cameraPos = GetCameraPosition();
	vertexMaterial_->AddConstBufPS({ cameraPos.x, cameraPos.y, cameraPos.z, 10.0f });
	
	vertexMaterial_->AddConstBufPS({ 1.0f, 1.0f, 1.0f, 1.0f });
	vertexMaterial_->AddConstBufPS({ 1.0f, 1.0f, 1.0f, 1.0f });
	vertexMaterial_->AddConstBufPS({ 0.0f, 0.0f, 0.0f, 1.0f });

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);

	ChangeState(STATE::IDLE);

	int tex = MV1GetMaterialDifMapTexture(transform_.modelId, 0);
}

void Metal::Update(void)
{
	// 更新ステップ
	stateUpdate_();
}

void Metal::Draw(void)
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

void Metal::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();

}

void Metal::ChangeStateNone(void)
{
}

void Metal::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&Metal::UpdateIdle, this);
}

void Metal::ChangeStateReserve(void)
{
	stateUpdate_ = std::bind(&Metal::UpdateReserve, this);
}

void Metal::ChangeStateMove(void)
{
	stateUpdate_ = std::bind(&Metal::UpdateMove, this);
}

void Metal::UpdateNone(void)
{
}

void Metal::UpdateIdle(void)
{
}

void Metal::UpdateReserve(void)
{
}

void Metal::UpdateMove(void)
{
}