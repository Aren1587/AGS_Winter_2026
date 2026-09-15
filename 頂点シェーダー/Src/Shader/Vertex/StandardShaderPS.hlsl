// VS/PS共通
#include "../Common/VertexToPixelHeader.hlsli"
 
// IN
#define PS_INPUT VertexToPixelLit
 
// PS
#include "../Common/Pixel/PixelShader3DHeader.hlsli"

cbuffer cbParam : register(b4)
{
    float3 g_light;
    float dummy;
}
 
float4 main(PS_INPUT PSInput) : SV_TARGET0
{
    //return float4(1, 0, 0, 1);
    
    // ③法線がワールド空間になっているか確認
    //return float4(
    //    PSInput.normal.x, PSInput.normal.x, PSInput.normal.x, 1.0f);
    
    ////float3 lightDir = g_common.light[0].direction;
    //return float4(g_light.x, g_light.y, g_light.z, 1.0f);
 
    float4 color;
 
	// テクスチャーの色を取得
    color = diffuseMapTexture.Sample(diffuseMapSampler, PSInput.uv);
    
    if (color.a < 0.01f)
    {
        discard;
    }
    
    // ランバート反射
    float lightDot = dot(PSInput.normal, -g_light) * 1;
    float3 rgb = color.rgb * lightDot;
    rgb += float3(0.2, 0.2, 0.2);
    
    return float4(rgb, color.a);
}