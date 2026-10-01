#pragma once

#include <memory>
#include "../MeshCutter/MeshCutter.h"
class AnimationController;

class Moon
{
public:
    void Init();

    // 平面(origin, normal)はワールド座標。Sword 側で計算したものを渡す
    void Cut(VECTOR origin, VECTOR normal)
    {
        cutter_.Cut(origin, normal);
    }

    void Update();
    void Draw() { cutter_.Draw(); }

    VECTOR GetCenter() const { return cutter_.GetCenter(); }
    float GetRadius() const { return cutter_.GetRadius(); }

private:
    std::unique_ptr<AnimationController> animationController_;

    MeshCutter cutter_;
};