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

    void Init(const VECTOR& vec);

    // 平面(origin, normal)はワールド座標。Sword 側で計算したものを渡す
    void Cut(VECTOR origin, VECTOR normal);

    void Update();
    void Draw();
    void Release();

    void TakeDamage(int damage);
    void Separate(const VECTOR& offset);
    bool IsDead() const { return isDead_; }
    bool IsRemovable() const;   // 消してよい状態か（管理側で使う）
    int  GetHp() const { return hp_; }

    VECTOR GetCenter() const { return cutter_.GetCenter(); }
    float GetRadius() const { return cutter_.GetRadius(); }
    float GetIsCut() const { return isCut_; }
    const Transform& GetTransform(void) const { return transform_; }
    const VECTOR GetCorePos() const { return corePos_; }

private:
	enum ANIM_TYPE
    {
        IDOL,
        RUN,
        ATTACK,
        ATTACK_IDOL,
    };

    enum class STATE
    {
        IDLE,
        CHASE,
        WINDUP,
        ATTACK,
        COOLDOWN,
        DAMAGE,    // 追加：のけぞり
        DEAD,      // 追加：通常撃破
    };

    int  hp_;
    int  invincibleTimer_;   // 連続ヒット防止
    bool isDead_;

    STATE state_;
    int    stateTimer_;   // フレーム数で管理
    bool   isMelee_;      // 今回の攻撃が近距離かどうか
    bool   hitDone_;      // 1回の攻撃で多段ヒットさせない
    VECTOR attackDir_;    // 突進方向（攻撃開始時に固定）

    std::unique_ptr<AnimationController> animationController_;

    Player& player_;

    std::vector<std::weak_ptr<Collider>> colliders_;

    MeshCutter cutter_;

    Transform transform_;

    bool isCut_;

    float rotSpeed;

    int coreFrame_;
    VECTOR corePos_;

    void ChangeState(STATE state);

    void UpdateIdle();
    void UpdateChase();
    void UpdateWindup();
    void UpdateAttack();
    void UpdateCooldown();

    VECTOR DirToPlayer(float& dist) const;
    void   FacePlayer();

    void EnemyMove();

    void MeshInit();

    // 索敵範囲の可視化(デバッグ用)
    void DrawDetectRange() const;
    bool IsDetectPlayer() const;
};