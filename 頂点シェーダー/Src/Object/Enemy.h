#pragma once

#include <memory>
#include "../MeshCutter/MeshCutter.h"
#include "Common/Transform.h"

class Player;
class AnimationController;

class Enemy
{
public:

    Enemy(Player& player);
    ~Enemy();

    void Init();

    // 平面(origin, normal)はワールド座標。Sword 側で計算したものを渡す
    void Cut(VECTOR origin, VECTOR normal);

    void Update();
    void Draw();
    void Release();

    VECTOR GetCenter() const { return cutter_.GetCenter(); }
    float GetRadius() const { return cutter_.GetRadius(); }
    const Transform& GetTransform(void) const { return transform_; }

private:

    enum STATE
    {
        null,
        one,
        two,
        ui
    };

    STATE state_;

    void ChangeState(STATE state = null)const;

    static constexpr float GRAVITY = 0.3f;

    std::unique_ptr<AnimationController> animationController_;

    Player& player_;

    std::vector<std::weak_ptr<Collider>> colliders_;

    MeshCutter cutter_;

    Transform transform_;

    bool isCut_;

    void EnemyAI();
    void EnemyMove();

    void MeshInit();
};