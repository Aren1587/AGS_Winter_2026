#include <DxLib.h>
#include "../Application.h"
#include "../Manager/ResourceManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "Player.h"
#include "WarpStar.h"

WarpStar::WarpStar(
	Player& player, const Transform& transform) : player_(player)
{

	transform_ = transform;

	state_ = STATE::NONE;

	// 状態管理
	stateChanges_.emplace(STATE::IDLE, std::bind(&WarpStar::ChangeStateIdle, this));
	stateChanges_.emplace(STATE::RESERVE, std::bind(&WarpStar::ChangeStateReserve, this));
	stateChanges_.emplace(STATE::MOVE, std::bind(&WarpStar::ChangeStateMove, this));

}

WarpStar::~WarpStar(void)
{
}

void WarpStar::Init(void)
{

	// モデルの基本情報
	transform_.SetModel(
		resMng_.LoadModelDuplicate(
			ResourceManager::SRC::WARP_STAR)
	);
	transform_.Update();

	ChangeState(STATE::IDLE);

	vertexMaterial_ = std::make_unique<VertexMaterial>("VertexShader.cso", 1,
														"PixelShader.cso", 1);

	vertexMaterial_->AddConstBufPS(FLOAT4(1.0f, 0.911f, 0.113f, 0.8f));

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);
}

void WarpStar::Update(void)
{

	// 更新ステップ
	stateUpdate_();

}

void WarpStar::Draw(void)
{
	// オリジナルシェーダ設定(ON)
	MV1SetUseOrigShader(true);
	
	// 定数バッファの設定
	UpdateShaderConstantBuffer(constBufVS_);
	// 定数バッファの設定
	UpdateShaderConstantBuffer(constBuf_);
	
	// 頂点シェーダー用の定数バッファを定数バッファレジスタにセット
	SetShaderConstantBuffer(
		constBufVS_, DX_SHADERTYPE_VERTEX, 7);
	// 頂点シェーダー用の定数バッファを定数バッファレジスタにセット
	SetShaderConstantBuffer(
		constBuf_, DX_SHADERTYPE_PIXEL, 4);
	
	// 頂点シェーダー設定
	SetUsePixelShader(shaderVS_);
	// 頂点シェーダー設定
	SetUsePixelShader(shader_);
	
	
	// 描画
	MV1DrawModel(transform_.modelId);

	// オリジナルシェーダ設定(OFF)
	MV1SetUseOrigShader(false);

	vertexRenderer_->Draw();
}

void WarpStar::ChangeState(STATE state)
{

	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();

}

void WarpStar::ChangeStateNone(void)
{
}

void WarpStar::ChangeStateIdle(void)
{
	stateUpdate_ = std::bind(&WarpStar::UpdateIdle, this);
}

void WarpStar::ChangeStateReserve(void)
{
	stateUpdate_ = std::bind(&WarpStar::UpdateReserve, this);
}

void WarpStar::ChangeStateMove(void)
{
	stateUpdate_ = std::bind(&WarpStar::UpdateMove, this);
}

void WarpStar::UpdateNone(void)
{
}

void WarpStar::UpdateIdle(void)
{
}

void WarpStar::UpdateReserve(void)
{
}

void WarpStar::UpdateMove(void)
{
}
