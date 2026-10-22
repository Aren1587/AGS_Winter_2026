#include <algorithm>
#include <string>
#include "../Application.h"
#include "../Utility/AsoUtility.h"
#include "../Manager/InputManager.h"
#include "../Manager/SceneManager.h"
#include "../Manager/ResourceManager.h"
#include "../Manager/Camera.h"
#include "Common/AnimationController.h"
#include "Common/Capsule.h"
#include "Common/Collider.h"
#include "Sword.h"
#include "Planet.h"
#include "Player.h"

namespace
{
	// スピード
	constexpr float SPEED_MOVE = 5.0f;
	constexpr float SPEED_RUN = 10.0f;

	// 回転完了までの時間
	constexpr float TIME_ROT = 1.0f;

	// ジャンプ力
	constexpr float POW_JUMP = 35.0f;

	// 最大値
	constexpr int MAX_HP = 100;
	constexpr int MAX_ENERGY = 100;

	// 初期値
	constexpr int INIT_HP = 100;
	constexpr int INIT_ENERGY = 50;

	constexpr VECTOR INIT_POS = { 0.0f, -30.0f, 0.0f };
	const VECTOR INIT_ROT = { 0.0f, AsoUtility::Deg2RadF(180.0f), 0.0f };

	constexpr VECTOR INIT_POS_TOP = { 0.0f, 110.0f, 0.0f };
	constexpr VECTOR INIT_POS_DOWN = { 0.0f, 30.0f, 0.0f };

	// ジャンプボタン長押し限界
	constexpr float JUMP_BUTTON_HOLD_MAX = 5.0f;

	// アニメーションの攻撃開始時間
	constexpr float ATTACK_TIME = 30.0f;

	// アニメーションの攻撃終了時間
	constexpr float ATTACK_END_TIME = 40.0f;

	constexpr float LINE_LENGTH = 1200.0f;     // ラインの長さ(画面サイズより長めに)
	constexpr float MOUSE_DEAD_ZONE = 3.0f;    // 角度を更新する最小マウス移動量
	constexpr int   LINE_COLOR_CORE = 0xE0FFFF; // 中心の白っぽい青
	constexpr int   LINE_COLOR_GLOW = 0x1E90FF; // 外側の青

	// 元の斬りアニメが画面上で何度(rad)に見えるか。実機で合わせる
	constexpr float ATTACK_BASE_ANGLE = 0.0f;

	constexpr float TRAIL_LIFE = 0.25f;        // 残像が消えるまでの時間(秒)
	constexpr size_t TRAIL_MAX_POINTS = 40;    // 記録する点の上限
	constexpr float TRAIL_ROOT_RATE = 0.3f;    // 手→切っ先の何割の位置を根元にするか(小さいほど幅が広い)
}

Player::Player(void)
	:
	animationController_(nullptr),
	state_(STATE::NONE),
	speed_(0.0f),
	moveDir_(AsoUtility::VECTOR_ZERO),
	movePow_(AsoUtility::VECTOR_ZERO),
	movedPos_(AsoUtility::VECTOR_ZERO),
	playerRotY_(Quaternion()),
	goalQuaRot_(Quaternion()),
	stepRotTime_(0.0f),
	jumpPow_(AsoUtility::VECTOR_ZERO),
	isJump_(false),
	isAttack_(false),
	stepJump_(0.0f),
	gravHitPosDown_(AsoUtility::VECTOR_ZERO),
	gravHitPosUp_(AsoUtility::VECTOR_ZERO),
	imgShadow_(-1),
	capsule_(nullptr)
{
	status_.hp_ = INIT_HP;
	status_.energy_ = INIT_ENERGY;

	// 状態管理
	stateChanges_.emplace(STATE::NONE, std::bind(&Player::ChangeStateNone, this));
	stateChanges_.emplace(STATE::PLAY, std::bind(&Player::ChangeStatePlay, this));
}

Player::~Player(void)
{
}

void Player::Init(void)
{
	// モデルの基本設定
	transform_.SetModel(resMng_.LoadModelDuplicate(
		ResourceManager::SRC::PLAYER));
	//transform_.SetModel(resMng_.LoadModelDuplicate(
	//	ResourceManager::SRC::NEW_PLAYER));
	transform_.scl = AsoUtility::VECTOR_ONE;
	transform_.pos = INIT_POS;
	transform_.quaRot = Quaternion();
	transform_.quaRotLocal =
		Quaternion::Euler(INIT_ROT);
	transform_.Update();

	// アニメーションの設定
	InitAnimation();

	spineFrame_ = MV1SearchFrame(transform_.modelId, "mixamorig:Spine1");

	// カプセルコライダ
	capsule_ = std::make_unique<Capsule>(transform_);
	capsule_->SetLocalPosTop(INIT_POS_TOP);
	capsule_->SetLocalPosDown(INIT_POS_DOWN);
	capsule_->SetRadius(20.0f);

	// 丸影画像
	imgShadow_ = resMng_.Load(ResourceManager::SRC::PLAYER_SHADOW).handleId_;

	// 初期状態
	ChangeState(STATE::PLAY);

	sword_ = std::make_unique<Sword>();

	sword_->Init();
}

void Player::Update(void)
{
	// 更新ステップ
	stateUpdate_();

	sword_->Update();

	// モデル制御更新
	transform_.Update();

	// アニメーション再生
	animationController_->Update();

	ApplySlashLineToBody();

	sword_->SetFollowFrame(transform_.modelId, "mixamorig:RightHandMiddle1");
	UpdateSlashTrail();

	//status_.hp_ -= 1;
}

void Player::Draw(void)
{
	// モデルの描画
	MV1DrawModel(transform_.modelId);
	sword_->Draw();
	DrawSlashTrail();
	DrawSphere3D(GetSword()->GetTip(), 10, 12, 0xffff00, 0xffff00, true);

	// 丸影描画
	DrawShadow();

	DrawSlashLine();
}

void Player::AddCollider(std::weak_ptr<Collider> collider)
{
	colliders_.push_back(collider);
}

void Player::ClearCollider(void)
{
	colliders_.clear();
}

const Capsule& Player::GetCapsule(void) const
{
	return *capsule_;
}

void Player::InitAnimation(void)
{
	std::string path = Application::PATH_MODEL + "Player/";
	animationController_ = std::make_unique<AnimationController>(transform_.modelId);
	animationController_->Add((int)ANIM_TYPE::IDLE, path + "Idle.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::RUN, path + "Run.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::FAST_RUN, path + "FastRun.mv1", 20.0f);
	animationController_->Add((int)ANIM_TYPE::JUMP, path + "Jump.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::WARP_PAUSE, path + "WarpPose.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FLY, path + "Flying.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FALLING, path + "Falling.mv1", 80.0f);
	animationController_->Add((int)ANIM_TYPE::VICTORY, path + "Victory.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::ATTACK, path + "Stable Sword Inward Slash.mv1", 60.0f);
	animationController_->Add((int)ANIM_TYPE::FAST_ATTACK, path + "StableSwordInwardSlash.mv1", 60.0f);

	animationController_->Play((int)ANIM_TYPE::IDLE);
}

void Player::ChangeState(STATE state)
{
	// 状態変更
	state_ = state;

	// 各状態遷移の初期処理
	stateChanges_[state_]();
}

void Player::ChangeStateNone(void)
{
	stateUpdate_ = std::bind(&Player::UpdateNone, this);
}

void Player::ChangeStatePlay(void)
{
	stateUpdate_ = std::bind(&Player::UpdatePlay, this);
}

void Player::UpdateNone(void)
{
}

void Player::UpdatePlay(void)
{
	// 移動処理
	ProcessMove();

	// ジャンプ処理
	ProcessJump();

	// 攻撃
	Attack();

	// 斬撃モード(肩越しカメラ)
	SlashAttackMode();

	// 移動方向に応じた回転
	Rotate();

	// 重力による移動量
	CalcGravityPow();

	// 衝突判定
	Collision();

	// 回転させる
	transform_.quaRot = playerRotY_;
}

void Player::DrawShadow(void)
{
	float PLAYER_SHADOW_HEIGHT = 300.0f;
	float PLAYER_SHADOW_SIZE = 30.0f;

	int i;
	MV1_COLL_RESULT_POLY_DIM HitResDim;
	MV1_COLL_RESULT_POLY* HitRes;
	VERTEX3D Vertex[3] = { VERTEX3D(), VERTEX3D(), VERTEX3D() };
	VECTOR SlideVec;
	int ModelHandle;

	// ライティングを無効にする
	SetUseLighting(FALSE);

	// Ｚバッファを有効にする
	SetUseZBuffer3D(TRUE);

	// テクスチャアドレスモードを CLAMP にする( テクスチャの端より先は端のドットが延々続く )
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);

	// 影を落とすモデルの数だけ繰り返し
	for (const auto& c : colliders_)
	{
		// チェックするモデルは、jが0の時はステージモデル、1以上の場合はコリジョンモデル
		ModelHandle = c.lock()->modelId_;

		// プレイヤーの直下に存在する地面のポリゴンを取得
		HitResDim = MV1CollCheck_Capsule(
			ModelHandle, -1,
			transform_.pos, VAdd(transform_.pos, { 0.0f, -PLAYER_SHADOW_HEIGHT, 0.0f }), PLAYER_SHADOW_SIZE);

		// 頂点データで変化が無い部分をセット
		Vertex[0].dif = GetColorU8(255, 255, 255, 255);
		Vertex[0].spc = GetColorU8(0, 0, 0, 0);
		Vertex[0].su = 0.0f;
		Vertex[0].sv = 0.0f;
		Vertex[1] = Vertex[0];
		Vertex[2] = Vertex[0];

		// 球の直下に存在するポリゴンの数だけ繰り返し
		HitRes = HitResDim.Dim;
		for (i = 0; i < HitResDim.HitNum; i++, HitRes++)
		{
			// ポリゴンの座標は地面ポリゴンの座標
			Vertex[0].pos = HitRes->Position[0];
			Vertex[1].pos = HitRes->Position[1];
			Vertex[2].pos = HitRes->Position[2];

			// ちょっと持ち上げて重ならないようにする
			SlideVec = VScale(HitRes->Normal, 0.5f);
			Vertex[0].pos = VAdd(Vertex[0].pos, SlideVec);
			Vertex[1].pos = VAdd(Vertex[1].pos, SlideVec);
			Vertex[2].pos = VAdd(Vertex[2].pos, SlideVec);

			// ポリゴンの不透明度を設定する
			Vertex[0].dif.a = 0;
			Vertex[1].dif.a = 0;
			Vertex[2].dif.a = 0;
			if (HitRes->Position[0].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[0].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[0].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			if (HitRes->Position[1].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[1].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[1].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			if (HitRes->Position[2].y > transform_.pos.y - PLAYER_SHADOW_HEIGHT)
				Vertex[2].dif.a = static_cast<int>(roundf(128.0f * (1.0f - fabs(HitRes->Position[2].y - transform_.pos.y) / PLAYER_SHADOW_HEIGHT)));

			// ＵＶ値は地面ポリゴンとプレイヤーの相対座標から割り出す
			Vertex[0].u = (HitRes->Position[0].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[0].v = (HitRes->Position[0].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].u = (HitRes->Position[1].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[1].v = (HitRes->Position[1].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].u = (HitRes->Position[2].x - transform_.pos.x) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;
			Vertex[2].v = (HitRes->Position[2].z - transform_.pos.z) / (PLAYER_SHADOW_SIZE * 2.0f) + 0.5f;

			// 影ポリゴンを描画
			DrawPolygon3D(Vertex, 1, imgShadow_, TRUE);
		}

		// 検出した地面ポリゴン情報の後始末
		MV1CollResultPolyDimTerminate(HitResDim);
	}

	// ライティングを有効にする
	SetUseLighting(TRUE);

	// Ｚバッファを無効にする
	SetUseZBuffer3D(FALSE);
}

void Player::ProcessMove(void)
{
	auto& ins = InputManager::GetInstance();

	// 移動量をゼロ
	movePow_ = AsoUtility::VECTOR_ZERO;

	// X軸回転を除いた、重力方向に垂直なカメラ角度(XZ平面)を取得
	Quaternion cameraRot = mainCamera.GetQuaRotOutX();

	// 回転したい角度
	double rotRad = 0;

	VECTOR dir = AsoUtility::VECTOR_ZERO;

	// カメラ方向に前進したい
	if (ins.IsNew(KEY_INPUT_W))
	{
		rotRad = AsoUtility::Deg2RadD(0.0);
		dir = cameraRot.GetForward();
	}

	// カメラ方向から後退したい
	if (ins.IsNew(KEY_INPUT_S))
	{
		rotRad = AsoUtility::Deg2RadD(180.0);
		dir = cameraRot.GetBack();
	}

	// カメラ方向から右側へ移動したい
	if (ins.IsNew(KEY_INPUT_D))
	{
		rotRad = AsoUtility::Deg2RadD(90.0);
		dir = cameraRot.GetRight();
	}

	// カメラ方向から左側へ移動したい
	if (ins.IsNew(KEY_INPUT_A))
	{
		rotRad = AsoUtility::Deg2RadD(270.0);
		dir = cameraRot.GetLeft();
	}

	if (!AsoUtility::EqualsVZero(dir) && (isJump_ || IsEndLanding())) {

		// 移動処理
		speed_ = SPEED_MOVE;
		if (ins.IsNew(KEY_INPUT_RSHIFT))
		{
			speed_ = SPEED_RUN;
		}
		moveDir_ = dir;
		movePow_ = VScale(dir, speed_);

		// 回転処理
		SetGoalRotate(rotRad);

		if (!isAttack_ && !isJump_ && IsEndLanding())
		{
			// アニメーション
			if (ins.IsNew(KEY_INPUT_RSHIFT))
			{
				animationController_->Play((int)ANIM_TYPE::FAST_RUN);
			}
			else
			{
				animationController_->Play((int)ANIM_TYPE::RUN);
			}
		}
		
	}
	else
	{
		if (!isAttack_ && !isJump_ && IsEndLanding())
		{
			animationController_->Play((int)ANIM_TYPE::IDLE);
		}
	}
}

void Player::ProcessJump(void)
{
	bool isHit = CheckHitKey(KEY_INPUT_BACKSLASH);

	// ジャンプ
	if (isHit && (isJump_ || IsEndLanding()))
	{

		if (!isJump_)
		{
			// 無理やりアニメーション
			animationController_->Play((int)ANIM_TYPE::JUMP, true, 13.0f, 25.0f);
			animationController_->SetEndLoop(23.0f, 25.0f, 5.0f);
		}

		isJump_ = true;

		// ジャンプの入力受付時間をヘラス
		stepJump_ += scnMng_.GetDeltaTime();
		if (stepJump_ < JUMP_BUTTON_HOLD_MAX)
		{
			jumpPow_ = VScale(AsoUtility::DIR_U, POW_JUMP);
		}

	}

	// ボタンを離したらジャンプ力に加算しない
	if (!isHit)
	{
		stepJump_ = JUMP_BUTTON_HOLD_MAX;
	}

}

void Player::Attack(void)
{
	if (InputManager::GetInstance().IsTrgDown(KEY_INPUT_E))
	{
		animationController_->Play((int)ANIM_TYPE::ATTACK, false);
		isAttack_ = true;

		// 斬撃モード中ならラインの角度を固定して使う
		lineAttack_ = isBladeMode_;
		attackLineAngle_ = slashLineAngle_;
	}

	if (animationController_->IsEnd())
	{
		isAttack_ = false;
		isSlashing_ = false;
		lineAttack_ = false;
	}

	if (!isAttack_) return;
	// ↓攻撃している

	attackTimer_ = animationController_->GetTime();

	bool attackEnd = attackTimer_ >= ATTACK_END_TIME;

	if (!isSlashing_)
	{
		if (attackTimer_ >= ATTACK_TIME)
		{
			isSlashing_ = true;
		}
	}
	
}

void Player::ApplySlashLineToBody(void)
{
	if (spineFrame_ < 0) return;

	// 攻撃中だけ補正を効かせ、出入りは滑らかにする
	float target = lineAttack_ ? 1.0f : 0.0f;
	lineBlend_ += (target - lineBlend_) * 0.2f;

	MV1ResetFrameUserLocalMatrix(transform_.modelId, spineFrame_);
	if (lineBlend_ < 0.01f) return;

	// ラインは向きの区別がないので -90°~90° に畳む
	float a = slashLineAngle_;
	if (a > DX_PI_F * 0.5f)       a -= DX_PI_F;
	else if (a < -DX_PI_F * 0.5f) a += DX_PI_F;

	float roll = (a - ATTACK_BASE_ANGLE) * lineBlend_;

	// 画面に垂直な軸(カメラ前方向)まわりの回転をワールドで作り、
	// 親フレームのローカル空間へ変換する
	MATRIX rotW = MGetRotAxis(GetCameraFrontVector(), roll);

	int parent = MV1GetFrameParent(transform_.modelId, spineFrame_);
	MATRIX pw = MV1GetFrameLocalWorldMatrix(transform_.modelId, parent);
	pw.m[3][0] = pw.m[3][1] = pw.m[3][2] = 0.0f;   // 平行移動は除く

	MATRIX user = MMult(MMult(pw, rotW), MInverse(pw));
	MV1SetFrameUserLocalMatrix(transform_.modelId, spineFrame_, user);
}

void Player::SlashAttackMode(void)
{
	// Qキーを押している間、肩越しカメラ
	bool isSlashMode = InputManager::GetInstance().IsNew(KEY_INPUT_Q);
	mainCamera.SetShoulder(isSlashMode);

	// モード開始時にマウスを中央へ戻して基準を作る
	if (isSlashMode && !isBladeMode_)
	{
		SetMouseDispFlag(FALSE);
		SetMousePoint(Application::SCREEN_SIZE_X / 2, Application::SCREEN_SIZE_Y / 2);
	}
	if (!isSlashMode && isBladeMode_)
	{
		SetMouseDispFlag(TRUE);
	}
	isBladeMode_ = isSlashMode;

	if (isBladeMode_)
	{
		UpdateSlashLine();
	}
}

void Player::UpdateSlashLine(void)
{
	// マウスが中央からどれだけ動いたか=入力方向
	int cx = Application::SCREEN_SIZE_X / 2;
	int cy = Application::SCREEN_SIZE_Y / 2;
	int mx, my;
	GetMousePoint(&mx, &my);
	float dx = static_cast<float>(mx - cx);
	float dy = static_cast<float>(my - cy);
	SetMousePoint(cx, cy);

	// 動いた方向にラインを向ける(小さな揺れは無視)
	if (dx * dx + dy * dy > MOUSE_DEAD_ZONE * MOUSE_DEAD_ZONE)
	{
		slashLineAngle_ = atan2f(dy, dx);
	}
}

void Player::UpdateSlashTrail(void)
{
	float dt = scnMng_.GetDeltaTime();

	// 経過時間を進めて、寿命切れを古い順に消す
	for (auto& p : slashTrail_) p.age += dt;
	while (!slashTrail_.empty() && slashTrail_.front().age > TRAIL_LIFE)
	{
		slashTrail_.pop_front();
	}

	// 斬っている間だけ新しい点を記録する
	if (isSlashing_)
	{
		VECTOR hilt = GetSword()->GetHilt();
		VECTOR tip = GetSword()->GetTip();
		VECTOR base = VAdd(hilt, VScale(VSub(tip, hilt), TRAIL_ROOT_RATE));

		slashTrail_.push_back({ base, tip, 0.0f });
		if (slashTrail_.size() > TRAIL_MAX_POINTS) slashTrail_.pop_front();
	}
}

void Player::DrawSlashTrail(void)
{
	if (slashTrail_.size() < 2) return;

	std::vector<VERTEX3D> vtx;
	vtx.reserve(slashTrail_.size() * 2);

	for (const auto& p : slashTrail_)
	{
		// 1.0(新しい)→0.0(古い)。二乗で消え際を急にする
		float t = 1.0f - p.age / TRAIL_LIFE;
		t = std::clamp(t, 0.0f, 1.0f);
		t *= t;

		// 切っ先は白っぽく、根元側は青く。加算ブレンドなので色の強さで消す
		VERTEX3D v = {};
		v.norm = VGet(0.0f, 0.0f, -1.0f);
		v.spc = GetColorU8(0, 0, 0, 0);

		v.pos = p.tip;
		v.dif = GetColorU8((int)(224 * t), (int)(255 * t), (int)(255 * t), 30);
		vtx.push_back(v);

		v.pos = p.base;
		v.dif = GetColorU8((int)(30 * t), (int)(144 * t), (int)(255 * t), 30);
		vtx.push_back(v);
	}

	SetUseLighting(FALSE);
	SetUseBackCulling(DX_CULLING_NONE);   // 裏からも見えるように
	SetWriteZBuffer3D(FALSE);             // 後ろのものを隠さない
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

	DrawPrimitive3D(vtx.data(), (int)vtx.size(),
		DX_PRIMTYPE_TRIANGLESTRIP, DX_NONE_GRAPH, FALSE);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetWriteZBuffer3D(TRUE);
	SetUseBackCulling(DX_CULLING_LEFT);
	SetUseLighting(TRUE);
}

void Player::DrawSlashLine(void)
{
	if (!isBladeMode_) return;

	float cx = Application::SCREEN_SIZE_X / 2.0f;
	float cy = Application::SCREEN_SIZE_Y / 2.0f;
	float vx = cosf(slashLineAngle_) * LINE_LENGTH * 0.5f;
	float vy = sinf(slashLineAngle_) * LINE_LENGTH * 0.5f;

	// 加算合成で発光っぽく:太い青 → 細い白の順に重ねる
	SetDrawBlendMode(DX_BLENDMODE_ADD, 50);
	DrawLineAA(cx - vx, cy - vy, cx + vx, cy + vy, LINE_COLOR_GLOW, 10.0f);
	DrawLineAA(cx - vx, cy - vy, cx + vx, cy + vy, LINE_COLOR_GLOW, 5.0f);
	DrawLineAA(cx - vx, cy - vy, cx + vx, cy + vy, LINE_COLOR_CORE, 2.0f);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Player::SetGoalRotate(double rotRad)
{
	VECTOR cameraRot = mainCamera.GetAngles();
	Quaternion axis = Quaternion::AngleAxis((double)cameraRot.y + rotRad, AsoUtility::AXIS_Y);

	// 現在設定されている回転との角度差を取る
	double angleDiff = Quaternion::Angle(axis, goalQuaRot_);

	// しきい値
	if (angleDiff > 0.1)
	{
		stepRotTime_ = TIME_ROT;
	}

	goalQuaRot_ = axis;
}

void Player::Rotate(void)
{
	stepRotTime_ -= scnMng_.GetDeltaTime();

	// 回転の球面補間
	playerRotY_ = Quaternion::Slerp(
		playerRotY_, goalQuaRot_, (TIME_ROT - stepRotTime_) / TIME_ROT);
}

void Player::Collision(void)
{
	// 現在座標を起点に移動後座標を決める
	movedPos_ = VAdd(transform_.pos, movePow_);

	// 衝突(カプセル)
	CollisionCapsule();

	// 衝突(重力)
	CollisionGravity();

	// 移動
	transform_.pos = movedPos_;
}

void Player::CollisionGravity(void)
{
	// ジャンプ量を加算
	movedPos_ = VAdd(movedPos_, jumpPow_);

	// 重力方向
	VECTOR dirGravity = AsoUtility::DIR_D;

	// 重力方向の反対
	VECTOR dirUpGravity = AsoUtility::DIR_U;

	// 重力の強さ
	float gravityPow = Planet::DEFAULT_GRAVITY_POW;

	float checkPow = 10.0f;
	gravHitPosUp_ = VAdd(movedPos_, VScale(dirUpGravity, gravityPow));
	gravHitPosUp_ = VAdd(gravHitPosUp_, VScale(dirUpGravity, checkPow * 2.0f));
	gravHitPosDown_ = VAdd(movedPos_, VScale(dirGravity, checkPow));
	for (const auto& c : colliders_)
	{

		// 地面との衝突
		auto hit = MV1CollCheck_Line(
			c.lock()->modelId_, -1, gravHitPosUp_, gravHitPosDown_);

		// 最初は上の行のように実装して、木の上に登ってしまうことを確認する
		//if (hit.HitFlag > 0)
		if (hit.HitFlag > 0 && VDot(dirGravity, jumpPow_) > 0.9f)
		{

			// 衝突地点から、少し上に移動
			movedPos_ = VAdd(hit.HitPosition, VScale(dirUpGravity, 2.0f));

			// ジャンプリセット
			jumpPow_ = AsoUtility::VECTOR_ZERO;
			stepJump_ = 0.0f;

			if (isJump_)
			{
				// 着地モーション
				animationController_->Play(
					(int)ANIM_TYPE::JUMP, false, 29.0f, 45.0f, false, true);
			}

			isJump_ = false;
		}
	}
}

void Player::CollisionCapsule(void)
{
	// カプセルを移動させる
	Transform trans = Transform(transform_);
	trans.pos = movedPos_;
	trans.Update();
	Capsule cap = Capsule(*capsule_, trans);

	// カプセルとの衝突判定
	for (const auto& c : colliders_)
	{
		auto hits = MV1CollCheck_Capsule(
			c.lock()->modelId_, -1,
			cap.GetPosTop(), cap.GetPosDown(), cap.GetRadius());

		for (int i = 0; i < hits.HitNum; i++)
		{
			auto hit = hits.Dim[i];

			for (int tryCnt = 0; tryCnt < 10; tryCnt++)
			{

				int pHit = HitCheck_Capsule_Triangle(
					cap.GetPosTop(), cap.GetPosDown(), cap.GetRadius(),
					hit.Position[0], hit.Position[1], hit.Position[2]);

				if (pHit)
				{
					movedPos_ = VAdd(movedPos_, VScale(hit.Normal, 1.0f));
					// カプセルを移動させる
					trans.pos = movedPos_;
					trans.Update();
					continue;
				}
				break;
			}
		}
		// 検出した地面ポリゴン情報の後始末
		MV1CollResultPolyDimTerminate(hits);
	}
}

void Player::CalcGravityPow(void)
{
	// 重力方向
	VECTOR dirGravity = AsoUtility::DIR_D;

	// 重力の強さ
	float gravityPow = Planet::DEFAULT_GRAVITY_POW;

	// 重力
	VECTOR gravity = VScale(dirGravity, gravityPow);
	jumpPow_ = VAdd(jumpPow_, gravity);

	// 最初は実装しない。地面と突き抜けることを確認する。
	// 内積
	float dot = VDot(dirGravity, jumpPow_);
	if (dot >= 0.0f)
	{
		// 重力方向と反対方向(マイナス)でなければ、ジャンプ力を無くす
		jumpPow_ = gravity;
	}
}

bool Player::IsEndLanding(void)
{
	bool ret = true;

	// アニメーションがジャンプではない
	if (animationController_->GetPlayType() != (int)ANIM_TYPE::JUMP)
	{
		return ret;
	}

	// アニメーションが終了しているか
	if (animationController_->IsEnd())
	{
		return ret;
	}

	return false;
}