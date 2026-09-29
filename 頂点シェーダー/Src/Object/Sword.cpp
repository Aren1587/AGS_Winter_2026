#include "Sword.h"

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
