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
    transform_.quaRotLocal = 
        Quaternion::Euler({ AsoUtility::Deg2RadF(180.0f), 0.0f, 0.0f });
    transform_.Update();
}

void Sword::Update()
{
    transform_.Update();
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
    int frame = MV1SearchFrame(followModelId, frameName);
    if (frame < 0) return; // 見つからない場合のガード

    MATRIX frameMat = MV1GetFrameLocalWorldMatrix(followModelId, frame);

    // スケール → ローカル回転 → 位置オフセット
    MATRIX scaleMat = MGetScale(transform_.scl);
    MATRIX rotMat = MGetRotY(DX_PI_F);
    // ずらし
    MATRIX transMat = MGetTranslate(VGet(0.0f, 0.0f, -3.0f));

    MATRIX offset = MMult(MMult(scaleMat, rotMat), transMat);
    MATRIX swordMat = MMult(offset, frameMat);

    MV1SetMatrix(transform_.modelId, swordMat);

    // 切っ先の計算
    VECTOR localHilt = VGet(0.0f, 0.0f, 0.0f);
    VECTOR localTip = VGet(1000.0f, 0.0f, 0.0f);
    VECTOR hilt = VTransform(localHilt, swordMat);
    VECTOR tip = VTransform(localTip, swordMat);

    UpdatePose(hilt, tip);
}
