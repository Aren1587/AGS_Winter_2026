#include "Sword.h"
#include "../Manager/ResourceManager.h"
#include "../Utility/AsoUtility.h"

Sword::Sword()
    :
    hilt_(VGet( 0, 0, 0)),
    tip_(VGet(0, 0, 0)),
    prevHilt_(VGet(0, 0, 0)),
    prevTip_(VGet(0, 0, 0))
{
}

Sword::~Sword()
{
}

void Sword::Init()
{
    transform_.SetModel(ResourceManager::GetInstance().LoadModelDuplicate(
        ResourceManager::SRC::SWORD));
    transform_.scl = VScale(AsoUtility::VECTOR_ONE, 0.1f);
}

void Sword::Update()
{

}

void Sword::Draw()
{
    MV1DrawModel(transform_.modelId);
}

void Sword::UpdatePose(VECTOR hilt, VECTOR tip)
{
    prevHilt_ = hilt_;
    prevTip_ = tip_;
    hilt_ = hilt;
    tip_ = tip;
}

bool Sword::ComputeSwingPlane(VECTOR& outOrigin, VECTOR& outNormal) const
{
    VECTOR bladeDir = VSub(tip_, hilt_);
    VECTOR sweepDir = VSub(tip_, prevTip_);

    VECTOR normal = VCross(bladeDir, sweepDir);
    if (VSize(normal) < 1e-6f) return false;

    outOrigin = hilt_;
    outNormal = VNorm(normal);
    return true;
}

const void Sword::SetFollowFrame(int& followModelId, const TCHAR* frameName)
{
    // 追従フレーム
    int frame = MV1SearchFrame(followModelId, frameName);

    // フレームのワールド行列
    MATRIX frameMat =
        MV1GetFrameLocalWorldMatrix(followModelId, frame);

    // 剣を握りに合わせるためのオフセット
    MATRIX offset = MMult(
        MGetRotY(DX_PI_F * 180.0f),
        MGetTranslate(VGet(0.0f, 0.0f, 0.0f))
    );

    // 剣のワールド行列
    MATRIX swordMat = MMult(offset, frameMat);

    MV1SetMatrix(transform_.modelId, swordMat);

    // 剣モデル内でのローカル座標
    VECTOR localHilt = VGet(0.0f, 0.0f, 0.0f);

    // 剣の長さ
    float swordLength = 100.0f;

    // 剣の先端が +Z 方向なら
    VECTOR localTip = VGet(-swordLength, 0.0f, 0.0f);

    // ローカル座標 → ワールド座標
    VECTOR hilt = VTransform(localHilt, swordMat);
    VECTOR tip = VTransform(localTip, swordMat);

    UpdatePose(hilt, tip);
}
