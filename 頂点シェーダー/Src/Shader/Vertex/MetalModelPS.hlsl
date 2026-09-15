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
    float g_specular_pow;
    float4 g_diff_color;
    float4 g_specular_color;
    float4 g_ambient_color;
}
 
float4 main(PS_INPUT PSInput) : SV_TARGET0
{
    float4 color;
 
	// テクスチャーの色を取得
    color = diffuseMapTexture.Sample(diffuseMapSampler, PSInput.uv);
    if (color.a < 0.01f)
    {
        discard;
    }
    
    // 法線
    float3 norm = PSInput.normal;
    
    // 拡散光の強さ
    float difDot = dot(norm, -g_light);
    
    // 頂点からのカメラ方向
    float3 toEye = normalize(g_camera_pos - PSInput.worldpos);
    
    // 反射ベクトル
    float3 reflectDir = normalize(g_light + (norm * 2.0f));
    
    // 反射光の強さ（反対側が負の値になるので絶対値で消す）
    float refDot = abs(dot(reflectDir, toEye));
    
    // 反射光を絞る
    float refDotEx = pow(refDot, g_specular_pow);
    
    // 拡散光
    float3 diffuse = color.rgb * (difDot * g_diff_color.rgb);
    
    // 色の合成
    float3 rgb = diffuse + (refDotEx * g_specular_color.rgb) + g_ambient_color.rgb;
    
    return float4(rgb, color.a);
 
}