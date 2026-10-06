#pragma once

#include <memory>
#include "../MeshCutter/MeshCutter.h"
#include "Common/Transform.h"

class AnimationController;

class Moon
{
public:
    Moon();
    ~Moon();

    void Init();

    // 平面(origin, normal)はワールド座標。Sword 側で計算したものを渡す
    void Cut(VECTOR origin, VECTOR normal);

    void Update();
    void Draw();
    void Release();

    VECTOR GetCenter() const { return cutter_.GetCenter(); }
    float GetRadius() const { return cutter_.GetRadius(); }

    const void IsCut(bool isCut) const {}

private:
    std::unique_ptr<AnimationController> animationController_;

    MeshCutter cutter_;

    Transform transform_;
};