// VS/PS共通
#include "../Common/VertexToPixelHeader.hlsli"
 
// IN
#define PS_INPUT VertexToPixelLit
 
// PS
#include "../Common/Pixel/PixelShader3DHeader.hlsli"

cbuffer cbParam : register(b4)
{
    float4 g_color;
}
 
float4 main(PS_INPUT PSInput) : SV_TARGET0
{
 
    float4 color;
 
	// テクスチャーの色を取得
    color = diffuseMapTexture.Sample(diffuseMapSampler, PSInput.uv);
    
    // 乗算する色
    //float4 mulColor = float4(1.0f, 0.911f, 0.113f, 0.8f);

    if ((g_color.r + g_color.g + g_color.b + g_color.a) > 0.01f)
    {
        // 乗算合成
        color *= g_color.rgba;
    }
    
    if (color.a < 0.01f)
    {
        discard;
    }
 
	// 関数の戻り値がラスタライザに渡される
    return color;
 
}