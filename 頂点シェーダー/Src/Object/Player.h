#pragma once

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <deque>
#include <DxLib.h>

#include "ActorBase.h"

class AnimationController;
class Collider;
class Capsule;
class Sword;

class Player : public ActorBase
{
public:

	struct Status
	{
		int hp_;
		float energy_;
	};

	// 状態
	enum class STATE
	{
		NONE,
		PLAY,
		WARP_RESERVE,
		WARP_MOVE,
		DEAD,
		VICTORY,
		END
	};

	// アニメーション種別
	enum class ANIM_TYPE
	{
		IDLE,
		RUN,
		FAST_RUN,
		JUMP,
		WARP_PAUSE,
		FLY,
		FALLING,
		VICTORY,
		ATTACK,
		FAST_ATTACK,
	};

	// 最大値
	static constexpr int MAX_HP = 100;
	static constexpr int MAX_ENERGY = 100;

	// コンストラクタ
	Player(void);

	// デストラクタ
	~Player(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

	// 衝突判定に用いられるコライダ制御
	void AddCollider(std::weak_ptr<Collider> collider);
	void ClearCollider(void);

	// 衝突用カプセルの取得
	const Capsule& GetCapsule(void) const;

	const bool IsSlashing() const { return isSlashing_; }
	const std::unique_ptr<Sword>& GetSword() const { return sword_; }
	const Status& GetStatus() const { return status_; }

	bool IsLineAttack() const { return lineAttack_; }
	bool GetSlashPlane(VECTOR& outOrigin, VECTOR& outNormal) const;
private:

	struct TrailPoint
	{
		VECTOR base;   // 刃の根元寄りの点
		VECTOR tip;    // 切っ先
		float  age;    // 経過時間(秒)
	};
	std::deque<TrailPoint> slashTrail_;

	// アニメーション
	std::unique_ptr<AnimationController> animationController_;

	std::unique_ptr<Sword> sword_;

	// 状態管理
	STATE state_;
	// 状態管理(状態遷移時初期処理)
	std::map<STATE, std::function<void(void)>> stateChanges_;
	// 状態管理(更新ステップ)
	std::function<void(void)> stateUpdate_;

	// 移動スピード
	float speed_;

	// 移動方向
	VECTOR moveDir_;

	// 移動量
	VECTOR movePow_;

	// 移動後の座標
	VECTOR movedPos_;

	// 回転
	Quaternion playerRotY_;
	Quaternion goalQuaRot_;
	float stepRotTime_;

	// ジャンプ量
	VECTOR jumpPow_;

	// ジャンプ判定
	bool isJump_;
	bool isAttack_;

	// ジャンプの入力受付時間
	float stepJump_;

	// 衝突判定に用いられるコライダ
	std::vector<std::weak_ptr<Collider>> colliders_;
	std::unique_ptr<Capsule> capsule_;

	// 衝突チェック
	VECTOR gravHitPosDown_;
	VECTOR gravHitPosUp_;

	// 丸影
	int imgShadow_;

	bool isSlashing_ = false;

	Status status_;

	float attackTimer_ = 0.0f;

	bool  isBladeMode_ = false;     // 斬撃モード中か
	float slashLineAngle_ = 0.0f;   // 画面上のライン角度(rad)

	int   spineFrame_ = -1;
	float attackLineAngle_ = 0.0f;  // 攻撃開始時に確定したライン角度
	bool  lineAttack_ = false;      // ラインに沿った攻撃中か
	float lineBlend_ = 0.0f;        // 補正の強さ(0~1)

	bool   hasSlashPlane_ = false;
	VECTOR slashPlaneOrigin_ = VGet(0, 0, 0);
	VECTOR slashPlaneNormal_ = VGet(0, 0, 0);
	bool BuildSlashPlane(float angle, VECTOR& o, VECTOR& n) const;

	void ApplySlashLineToBody(void);

	void InitAnimation(void);

	// 状態遷移
	void ChangeState(STATE state);
	void ChangeStateNone(void);
	void ChangeStatePlay(void);

	// 更新ステップ
	void UpdateNone(void);
	void UpdatePlay(void);
	void UpdateSlashLine(void);
	void UpdateSlashTrail(void);

	// 描画系
	void DrawShadow(void);
	void DrawSlashLine(void);
	void DrawSlashTrail(void);

	// 操作
	void ProcessMove(void);
	void ProcessJump(void);
	void Attack(void);
	void SlashAttackMode(void);

	// 回転
	void SetGoalRotate(double rotRad);
	void Rotate(void);

	// 衝突判定
	void Collision(void);
	void CollisionGravity(void);
	void CollisionCapsule(void);

	// 移動量の計算
	void CalcGravityPow(void);

	// 着地モーション終了
	bool IsEndLanding(void);
};
