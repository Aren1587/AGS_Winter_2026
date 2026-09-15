// VS/PS共通
#include "../Common/VertexToPixelHeader.hlsli"

// IN
#include "../Common/Vertex/VertexInputType.hlsli"
#define VERTEX_INPUT DX_MV1_VERTEX_TYPE_NMAP_1FRAME

// OUT
#define VS_OUTPUT VertexToPixelLit
#include "../Common/Vertex/VertexShader3DHeader.hlsli"

cbuffer cbParam : register(b7)
{
    float3 g_uv_scale;
    float time;
}

VS_OUTPUT main(VS_INPUT VSInput)
{
    VS_OUTPUT ret;
    
    // 頂点座標変換 +++++++++++++++++++++++++++++++++++++( 開始 )
    float4 lLocalPosition;
    float4 lWorldPosition;
    float4 lViewPosition;
    
    lLocalPosition.xyz = VSInput.pos;
    lLocalPosition.w = 1.0f;
    
    // ローカル座標をワールド座標に変換(剛体)
    lWorldPosition.w = 1.0f;
    lWorldPosition.xyz = mul(lLocalPosition, g_base.localWorldMatrix);
    
    // 正弦波
    float maxH = 10.0f;
    float height = pow(sin((time * 5.0f) + ((lWorldPosition.x * 50.0f) + (lWorldPosition.z * 50.0f))), 3.0f);
    height = ((height + 1.0f) * 0.5f) * maxH;
    lWorldPosition.x += height;
    lWorldPosition.z += height;
    
    
    ret.worldpos.xyz = lWorldPosition.xyz;
    
    // ワールド座標をビュー座標に変換
    lViewPosition.w = 1.0f;
    lViewPosition.xyz = mul(lWorldPosition, g_base.viewMatrix);
    ret.vwPos.xyz = lViewPosition.xyz;
    
    // ビュー座標を射影座標に変換
    ret.svPos = mul(lViewPosition, g_base.projectionMatrix);
    
    // 頂点座標変換 +++++++++++++++++++++++++++++++++++++( 終了 )
    
    // その他、ピクセルシェーダへ引継&初期化 ++++++++++++( 開始 )
    // UV座標
    //ret.uv.x = VSInput.uv0.x;
    //ret.uv.y = VSInput.uv0.y;
    
    ret.uv.x = VSInput.uv0.x * g_uv_scale.x;
    ret.uv.y = VSInput.uv0.y * g_uv_scale.y;
    
    // 法線をローカル空間からワールド空間へ変換
    ret.normal = normalize(mul(VSInput.norm, (float3x3) g_base.localWorldMatrix));
    
    // ディフューズカラー
    ret.diffuse = VSInput.diffuse;
    
    // ライト方向(ローカル)
    ret.lightDir = float3(0.0f, 0.0f, 0.0f);
    
    // ライトから見た座標
    ret.lightAtPos = float3(0.0f, 0.0f, 0.0f);
    
    // その他、ピクセルシェーダへ引継&初期化 ++++++++++++( 終了 )
    
    // 出力パラメータを返す
    return ret;
}