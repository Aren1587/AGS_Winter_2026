#pragma once
#include <DxLib.h>
#include "../Common/Vector2.h"
#include "../Renderer/VertexMaterial.h"


class VertexRenderer
{
public:
	// 頂点シェーダ用オリジナル定数バッファの使用開始スロット
	static constexpr int CONSTANT_BUF_SLOT_BEGIN_VS = 7;
	// ピクセルシェーダ用オリジナル定数バッファの使用開始スロット
	static constexpr int CONSTANT_BUF_SLOT_BEGIN_PS = 4;

	// コンストラクタ
	VertexRenderer(int& modelId, VertexMaterial& vertexMaterial);

	// デストラクタ
	~VertexRenderer(void);

	// 描画
	void Draw(void);

private:

	// モデルのハンドルID
	int& modelId_;

	// モデルマテリアル
	VertexMaterial& vertexMaterial_;

	// シェーダ設定(頂点)
	void SetReserveVS(void);

	// シェーダ設定(ピクセル)
	void SetReservePS(void);
};