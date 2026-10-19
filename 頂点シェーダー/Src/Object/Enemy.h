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
	enum ANIM_TYPE
    {
        IDOL,
        RUN,
    };

    enum STATE
    {
        null,
        one,
        ui
    };

    STATE state_;

    void ChangeState(STATE state = null)const;

    std::unique_ptr<AnimationController> animationController_;

    Player& player_;

    std::vector<std::weak_ptr<Collider>> colliders_;

    MeshCutter cutter_;

    Transform transform_;

    bool isCut_;

    bool test = false;
    float testTime = 0.0f;
	const float limitTime = 3.0f;

    float rotSpeed = 180.0f;

    void EnemyAI();
    void EnemyMove();

    void MeshInit();
};