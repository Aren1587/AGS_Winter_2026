#include "../Application.h"
#include "VertexMaterial.h"

VertexMaterial::VertexMaterial(
	std::string shaderFileNameVS, int constBufFloat4SizeVS,
	std::string shaderFileNamePS, int constBufFloat4SizePS)
{

	// 頂点シェーダのロード
	shaderVS_ = LoadVertexShader(
		(Application::PATH_SHADER + shaderFileNameVS).c_str());

	// 頂点定数バッファの確保サイズ(FLOAT4をいくつ作るか)
	constBufFloat4SizeVS_ = constBufFloat4SizeVS;

	// 頂点シェーダー用の定数バッファを作成
	constBufVS_ = CreateShaderConstantBuffer(sizeof(FLOAT4) * constBufFloat4SizeVS);



	// ピクセルシェーダのロード
	shaderPS_ = LoadPixelShader(
		(Application::PATH_SHADER + shaderFileNamePS).c_str());

	// ピクセル定数バッファの確保サイズ(FLOAT4をいくつ作るか)
	constBufFloat4SizePS_ = constBufFloat4SizePS;

	// ピクセルシェーダー用の定数バッファを作成
	constBufPS_ = CreateShaderConstantBuffer(sizeof(FLOAT4) * constBufFloat4SizePS);



	// テクスチャアドレス
	texAddress_ = TEXADDRESS::CLAMP;

}

VertexMaterial::~VertexMaterial(void)
{
}

// 定数バッファを追加
void VertexMaterial::AddConstBufVS(const FLOAT4& contBuf)
{
	if (constBufFloat4SizeVS_ > constBufsVS_.size())
	{
		constBufsVS_.emplace_back(contBuf);
	}
}

void VertexMaterial::AddConstBufPS(const FLOAT4& contBuf)
{
	if (constBufFloat4SizePS_ > constBufsPS_.size())
	{
		constBufsPS_.emplace_back(contBuf);
	}
}

// 定数バッファを更新
void VertexMaterial::SetConstBufVS(int idx, const FLOAT4& contBuf)
{
	if (idx >= constBufsVS_.size())
	{
		return;
	}

	constBufsVS_[idx] = contBuf;
}

void VertexMaterial::SetConstBufPS(int idx, const FLOAT4& contBuf)
{
	if (idx >= constBufsPS_.size())
	{
		return;
	}

	constBufsPS_[idx] = contBuf;
}

// テクスチャを更新
void VertexMaterial::SetTextureBuf(int slot, int texDiffuse)
{
	if (textures_.count(slot) == 0)
	{
		textures_.emplace(slot, texDiffuse);
	}
	else
	{
		textures_[slot] = texDiffuse;
	}
}

// シェーダ
int VertexMaterial::GetShaderVS(void) const
{
	return shaderVS_;
}

int VertexMaterial::GetShaderPS(void) const
{
	return shaderPS_;
}

// 定数バッファハンドル
int VertexMaterial::GetConstBufVS(void) const
{
	return constBufVS_;
}

int VertexMaterial::GetConstBufPS(void) const
{
	return constBufPS_;
}

// 定数バッファ
const std::vector<FLOAT4>& VertexMaterial::GetConstBufsVS(void) const
{
	return constBufsVS_;
}

const std::vector<FLOAT4>& VertexMaterial::GetConstBufsPS(void) const
{
	return constBufsPS_;
}

// 画像
const std::map<int, int>& VertexMaterial::GetTextures(void) const
{
	return textures_;
}

// テクスチャアドレスを取得
VertexMaterial::TEXADDRESS VertexMaterial::GetTextureAddress(void) const
{
	return texAddress_;
}

// テクスチャアドレスを設定
void VertexMaterial::SetTextureAddress(TEXADDRESS texA)
{
	texAddress_ = texA;
}