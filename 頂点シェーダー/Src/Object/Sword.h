#pragma once

#include "DxLib.h"
#include "Common/Transform.h"

class Sword
{
public:
    Sword();
    ~Sword();

    void Init();
    void Update();
    void Draw();

    // 毎フレーム、剣の根元(hilt)と切っ先(tip)のワールド座標を渡す
    void UpdatePose(VECTOR hilt, VECTOR tip);

    // 「振った軌跡」から切断平面を作る
    //   刃の向き(hilt→tip)と、切っ先が動いた向き(前フレーム→今フレーム)の
    //   2方向を含む平面が、剣が空間を薙いだ面になる
    bool ComputeSwingPlane(VECTOR& outOrigin, VECTOR& outNormal) const;

    VECTOR GetTip() const { return tip_; }

    const Transform& GetTransform(void) const { return transform_; }

    const void SetFollowFrame(int& followModelId, const TCHAR* frameName);

private:
    VECTOR hilt_ = VGet(0, 0, 0), tip_ = VGet(0, 0, 0);
    VECTOR prevHilt_ = VGet(0, 0, 0), prevTip_ = VGet(0, 0, 0);

    Transform transform_;
};