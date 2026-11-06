#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Common/Collider.h"
#include "Common/Transform.h"
#include "../Utility/AsoUtility.h"
#include "Common/Dxfileloader.h"
#include "../Manager/ResourceManager.h"
#include "Player.h"
#include "Enemy.h"

namespace
{
    constexpr float MOVE_SPEED = 5.0f;
    constexpr float GRAVITY = 0.3f;

    constexpr VECTOR INIT_POS = { 100, -10, 600 };

    constexpr float LIMIT_TIME = 3.0f;

    constexpr float DETECT_RANGE = 600.0f;   // çıìGîºåa
    constexpr int   DETECT_SEGMENTS = 64;    // â~ÇÃï™äÑêî
    constexpr float DETECT_DRAW_OFFSET_Y = 1.0f; // ínñ Ç∆ÇÃÇﬂÇËçûÇ›ñhé~

    constexpr float PIECE_LIFETIME = 15000.0f;   // 5ïb
    constexpr float PIECE_FADE = 150.0f;    // ç≈å„ÇÃ1ïbÇ≈îñÇ≠Ç»ÇÈ

    constexpr float MELEE_RANGE = 100.0f;  // Ç±ÇÍà»ì‡Ç»ÇÁÉpÉìÉ`
    constexpr float LUNGE_RANGE = 250.0f;  // Ç±ÇÍà»ì‡Ç»ÇÁìÀêiÅiDETECT_RANGEÇÊÇËè¨Ç≥Ç≠Åj
    constexpr float LUNGE_SPEED = 10.0f;
    constexpr float HIT_RANGE = 120.0f;  // çUåÇÇ™ìñÇΩÇÈãóó£

    constexpr int WINDUP_FRAME = 30;      // ó\îıìÆçÏ 0.5ïb
    constexpr int ATTACK_FRAME = 40;
    constexpr int COOLDOWN_FRAME = 90;
    constexpr int COOLDOWN_RAND = 30;      // ìGÇ≤Ç∆Ç…çUåÇÉ^ÉCÉ~ÉìÉOÇÇ∏ÇÁÇ∑

    constexpr int MAX_HP = 3;
    constexpr int DAMAGE_STUN_FRAME = 20;   // ÇÃÇØÇºÇË ñÒ0.33ïb
    constexpr int INVINCIBLE_FRAME = 15;   // îÌíeå„ÇÃñ≥ìG 0.25ïb
    constexpr int DEAD_FRAME = 60;   // ì|ÇÍÇƒÇ©ÇÁè¡Ç¶ÇÈÇ‹Ç≈
}

Enemy::Enemy(Player& player)
    :
    state_(STATE::IDLE),
    stateTimer_(0),
    isMelee_(true),
    hitDone_(false),
    attackDir_(VGet(0, 0, 0)),
    animationController_(nullptr),
    player_(player),
    isCut_(false),
    rotSpeed(180.0f),
    coreFrame_(-1),
    corePos_(VGet(0, 0, 0)),
    hp_(MAX_HP),
    invincibleTimer_(0),
    isDead_(false)
{
}

Enemy::~Enemy()
{
    Release();
}

void Enemy::Init(const VECTOR& vec)
{

    transform_.modelId = ResourceManager::GetInstance().LoadModelDuplicate(ResourceManager::SRC::NEW_PLAYER);
    transform_.pos = vec;
    MV1SetScale(transform_.modelId, AsoUtility::VECTOR_ONE);
    //cutter_ = new MeshCutter();
    MV1SetPosition(transform_.modelId, transform_.pos);
    MV1SetupReferenceMesh(transform_.modelId, -1, true);

    coreFrame_ = MV1SearchFrame(transform_.modelId, "mixamorig:Spine1");

    transform_.MakeCollider(Collider::TYPE::ENEMY);

    std::string path = Application::PATH_MODEL;
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);
    animationController_->Add(ANIM_TYPE::IDOL, path + "SkinningTest.mv1", 50.0f);
    animationController_->Add(ANIM_TYPE::RUN, path + "Running.mv1", 50.0f);
    animationController_->Add(ANIM_TYPE::ATTACK, path + "Punching.mv1", 50.0f);

    animationController_->Play(ANIM_TYPE::IDOL);

}

void Enemy::Update()
{
    animationController_->Update();
    cutter_.Update();

    if (isCut_) return;

    if (invincibleTimer_ > 0) --invincibleTimer_;

    switch (state_)
    {
    case STATE::IDLE:     UpdateIdle();     break;
    case STATE::CHASE:    UpdateChase();    break;
    case STATE::WINDUP:   UpdateWindup();   break;
    case STATE::ATTACK:   UpdateAttack();   break;
    case STATE::COOLDOWN: UpdateCooldown(); break;
    case STATE::DAMAGE:
        if (--stateTimer_ <= 0)
            ChangeState(IsDetectPlayer() ? STATE::CHASE : STATE::IDLE);
        break;
    case STATE::DEAD:
        if (stateTimer_ > 0) --stateTimer_;
        break;
    }

    corePos_ = MV1GetFramePosition(transform_.modelId, coreFrame_);
    transform_.Update();
    MV1RefreshReferenceMesh(transform_.modelId, -1, true);
}

void Enemy::ChangeState(STATE state)
{
    state_ = state;
    switch (state_)
    {
    case STATE::IDLE:
        animationController_->Play(ANIM_TYPE::IDOL);
        break;
    case STATE::CHASE:
        animationController_->Play(ANIM_TYPE::RUN);
        break;
    case STATE::WINDUP:
        stateTimer_ = WINDUP_FRAME;
        animationController_->Play(ANIM_TYPE::IDOL);   // ó≠ÇﬂópÇÃìÆÇ´Ç™Ç†ÇÍÇŒç∑Çµë÷Ç¶
        break;
    case STATE::ATTACK:
        stateTimer_ = ATTACK_FRAME;
        hitDone_ = false;
        float dist;
        attackDir_ = DirToPlayer(dist);                // ìÀêiï˚å¸ÇÕÇ±Ç±Ç≈å≈íË
        animationController_->Play(ANIM_TYPE::ATTACK);
        break;
    case STATE::COOLDOWN:
        stateTimer_ = COOLDOWN_FRAME + GetRand(COOLDOWN_RAND);
        animationController_->Play(ANIM_TYPE::IDOL);
        break;
    case STATE::DAMAGE:
        stateTimer_ = DAMAGE_STUN_FRAME;
        animationController_->Play(ANIM_TYPE::IDOL);   // îÌíeÉAÉjÉÅÇ™Ç†ÇÍÇŒç∑Çµë÷Ç¶
        break;
    case STATE::DEAD:
        stateTimer_ = DEAD_FRAME;
        isDead_ = true;
        animationController_->Play(ANIM_TYPE::IDOL);   // ì|ÇÍÇÈÉAÉjÉÅÇ™Ç†ÇÍÇŒç∑Çµë÷Ç¶
        break;
    }
}

void Enemy::UpdateIdle()
{
    if (IsDetectPlayer()) ChangeState(STATE::CHASE);
}

void Enemy::UpdateChase()
{
    if (!IsDetectPlayer()) { ChangeState(STATE::IDLE); return; }

    float dist;
    DirToPlayer(dist);
    if (dist <= LUNGE_RANGE)
    {
        isMelee_ = (dist <= MELEE_RANGE);   // ãóó£Ç≈çUåÇÇÃéÌóﬁÇåàÇﬂÇÈ
        ChangeState(STATE::WINDUP);
        return;
    }
    EnemyMove();
}

void Enemy::UpdateWindup()
{
    FacePlayer();                            // ó≠ÇﬂíÜÇÕå¸Ç´Çí«Ç§
    if (--stateTimer_ <= 0) ChangeState(STATE::ATTACK);
}

void Enemy::UpdateAttack()
{
    const int half = ATTACK_FRAME / 2;

    // ëOîºÅFíÜãóó£çUåÇÇ»ÇÁìÀêiÅiå¸Ç´ÇÕå≈íËÇ»ÇÃÇ≈ÅAâ°Ç…ìÆÇØÇŒîÇØÇÁÇÍÇ‹Ç∑Åj
    if (!isMelee_ && stateTimer_ > half)
    {
        transform_.pos = VAdd(transform_.pos, VScale(attackDir_, LUNGE_SPEED));
    }

    // å„îºÇ…ì¸Ç¡ÇΩèuä‘Ç…1âÒÇæÇØìñÇΩÇËîªíË
    if (!hitDone_ && stateTimer_ <= half)
    {
        hitDone_ = true;
        float dist;
        DirToPlayer(dist);
        if (dist <= HIT_RANGE)
        {
            // TODO: Playerë§ÇÃÉ_ÉÅÅ[ÉWèàóùÇåƒÇ‘
        }
    }

    if (--stateTimer_ <= 0) ChangeState(STATE::COOLDOWN);
}

void Enemy::UpdateCooldown()
{
    if (--stateTimer_ <= 0)
    {
        ChangeState(IsDetectPlayer() ? STATE::CHASE : STATE::IDLE);
    }
}

VECTOR Enemy::DirToPlayer(float& dist) const
{
    VECTOR v = VSub(player_.GetTransform().pos, transform_.pos);
    v.y = 0.0f;
    dist = VSize(v);
    if (dist < 0.001f) return VGet(0, 0, 0);
    return VScale(v, 1.0f / dist);
}

void Enemy::FacePlayer()
{
    float dist;
    VECTOR dir = DirToPlayer(dist);
    if (dist < 0.001f) return;
    transform_.quaRot = Quaternion::AngleAxis(atan2f(-dir.x, -dir.z), AsoUtility::AXIS_Y);
}

void Enemy::EnemyMove()
{
    float dist;
    VECTOR dir = DirToPlayer(dist);
    if (dist < 0.001f) return;
    transform_.pos = VAdd(transform_.pos, VScale(dir, MOVE_SPEED));
    FacePlayer();
}

void Enemy::MeshInit()
{
    cutter_.SetMesh(MeshCut::FromMV1(transform_.modelId));
    cutter_.SetGravity(GRAVITY);
    cutter_.SetFloorFromMesh();
    //cutter_.SetLifetime(PIECE_LIFETIME);
    cutter_.SetLifetime(PIECE_FADE);
}

bool Enemy::IsDetectPlayer() const
{
    VECTOR toPlayer = VSub(player_.GetTransform().pos, transform_.pos);
    toPlayer.y = 0.0f; // êÖïΩãóó£ÇæÇØÇ≈îªíË
    return VSize(toPlayer) <= DETECT_RANGE;
}

void Enemy::DrawDetectRange() const
{
    const VECTOR c = transform_.pos;
    const float y = c.y + DETECT_DRAW_OFFSET_Y;

    // îÕàÕì‡Ç…ì¸Ç¡ÇΩÇÁê‘ÅAäOÇ»ÇÁóŒ
    const unsigned int color = IsDetectPlayer() ? 0xff0000 : 0x00ff00;

    for (int i = 0; i < DETECT_SEGMENTS; i++)
    {
        float a0 = DX_TWO_PI_F * i / DETECT_SEGMENTS;
        float a1 = DX_TWO_PI_F * (i + 1) / DETECT_SEGMENTS;

        VECTOR p0 = VGet(c.x + cosf(a0) * DETECT_RANGE, y, c.z + sinf(a0) * DETECT_RANGE);
        VECTOR p1 = VGet(c.x + cosf(a1) * DETECT_RANGE, y, c.z + sinf(a1) * DETECT_RANGE);

        DrawLine3D(p0, p1, color);
    }
}

void Enemy::Cut(VECTOR origin, VECTOR normal) 
{
    if(!isCut_)
    {
        isCut_ = true;
        MeshInit();
    }

    cutter_.Cut(origin, normal);
}

void Enemy::Draw()
{
    if (isCut_)
    {
        cutter_.Draw();
    }
    else
    {
        // éÄñSíÜÇÕç≈å„ÇÃ30ÉtÉåÅ[ÉÄÇ≈ì_ñ≈
        const bool blink = isDead_ && stateTimer_ < 30 && (stateTimer_ / 3) % 2 == 0;
        if (!IsRemovable() && !blink)
        {
            MV1DrawModel(transform_.modelId);
        }
    }

    if (!isCut_ && !isDead_)
    {
        DrawDetectRange();
    }

    DrawSphere3D(corePos_, 10, 12, 0xffffff, 0xffffff, true);
}

void Enemy::Release()
{
}

void Enemy::TakeDamage(int damage)
{
    if (isCut_ || isDead_ || invincibleTimer_ > 0) return;

    hp_ -= damage;
    invincibleTimer_ = INVINCIBLE_FRAME;

    if (hp_ <= 0)
    {
        ChangeState(STATE::DEAD);
        return;
    }
    ChangeState(STATE::DAMAGE);   // ó≠ÇﬂÇ‚çUåÇÇÕíÜífÇ≥ÇÍÇÈ
}

bool Enemy::IsRemovable() const
{
    // ÉJÉbÉgçœÇ›: îjï–Ç™Ç∑Ç◊ÇƒéıñΩÇ≈è¡Ç¶ÇΩÇÁçÌèúÇµÇƒÇÊÇ¢
    if (isCut_)
    {
        return cutter_.GetPieceCount() == 0;
    }

    // í èÌåÇîj
    return isDead_ && stateTimer_ <= 0;
}
