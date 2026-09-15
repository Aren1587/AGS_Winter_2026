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
    float3 g_camera_pos;
    float time;
    float4 g_diff_color;
    float4 g_ambient_color;
}
 
float4 main(PS_INPUT PSInput) : SV_TARGET0
{
    float4 color;
    
	// テクスチャーの色を取得
    color = diffuseMapTexture.Sample(diffuseMapSampler, PSInput.uv + time);
    if (color.a < 0.01f)
    {
        discard;
    }
    
    //// 法線
    //float3 norm = PSInput.normal;
    
    //// 拡散光の強さ
    //float difDot = dot(norm, -g_light);
    
    //// 頂点からのカメラ方向
    //float3 toEye = normalize(g_camera_pos - PSInput.worldpos);
    
    //float rimDot = dot(norm, toEye);
    
    //rimDot = abs(rimDot);
    //rimDot = 1.0f - rimDot;
    //rimDot = pow(rimDot, 2.0f);
    
    
    //PSInput.uv = clamp(PSInput.uv, 0.0f, 1.0f);
    
    //// 
    //float4 rimColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    //// 拡散光
    //float3 diffuse = color.rgb * (difDot * g_diff_color.rgb);
    
    //// 色の合成
    //float3 rgb = diffuse + (rimDot * rimColor.rgb) + g_ambient_color.rgb;
    
    //return float4(rgb, color.a);
    return color;
}