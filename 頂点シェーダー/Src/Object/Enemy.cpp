#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Common/Collider.h"
#include "Common/Transform.h"
#include "../Utility/AsoUtility.h"
#include "Common/Dxfileloader.h"
#include "Player.h"
#include "Enemy.h"

namespace
{
    constexpr float MOVE_SPEED = 3.0f;
    constexpr float GRAVITY = 0.3f;

    constexpr int RANDAM_MAX = 100;

    constexpr VECTOR INIT_POS = { 100, -10, 600 };

    constexpr float LIMIT_TIME = 3.0f;

    constexpr float DETECT_RANGE = 300.0f;   // çıìGîºåa
    constexpr int   DETECT_SEGMENTS = 64;    // â~ÇÃï™äÑêî
    constexpr float DETECT_DRAW_OFFSET_Y = 1.0f; // ínñ Ç∆ÇÃÇﬂÇËçûÇ›ñhé~
}

Enemy::Enemy(Player& player)
    :
    state_(STATE::null),
    animationController_(nullptr),
    player_(player),
    isCut_(false),
    test(false),
    testTime(0.0f),
    rotSpeed(180.0f),
    coreFrame_(-1)
{
}

Enemy::~Enemy()
{
    Release();
}

void Enemy::Init(const VECTOR& vec)
{

    transform_.modelId = MV1LoadModel("Data/Model/model.mv1");
    transform_.pos = vec;
    MV1SetScale(transform_.modelId, AsoUtility::VECTOR_ONE);
    //cutter_ = new MeshCutter();
    MV1SetPosition(transform_.modelId, transform_.pos);
    MV1SetupReferenceMesh(transform_.modelId, -1, true);

    coreFrame_ = MV1SearchFrame(transform_.modelId, "mixamorig:Spine");

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
    auto corePos = ConvWorldPosToScreenPos(MV1GetFramePosition(transform_.modelId, coreFrame_));

    testTime += 1.0f / 60.0f;
    if (LIMIT_TIME < testTime)
    {
        testTime = 0.0f;
        EnemyAI();
    }

    if (test && IsDetectPlayer())
    {
        EnemyMove();
    }

    transform_.Update();

    MV1RefreshReferenceMesh(transform_.modelId, -1, true);
}

void Enemy::ChangeState(STATE state) const
{
}

void Enemy::EnemyAI()
{
    auto rand = GetRand(RANDAM_MAX);

    if (rand < 50 || IsDetectPlayer())
    {
        test = true;
        animationController_->Play(ANIM_TYPE::RUN);
        return;
    }
    test = false;
    animationController_->Play(ANIM_TYPE::IDOL);
}

void Enemy::EnemyMove()
{
    VECTOR toPlayer = VSub(player_.GetTransform().pos, transform_.pos);
    toPlayer.y = 0.0f;                       // êÖïΩï˚å¸ÇæÇØå©ÇÈ
    float dist = VSize(toPlayer);
    if (dist < 0.001f) return;

    VECTOR dir = VScale(toPlayer, 1.0f / dist);

    // à⁄ìÆ
    transform_.pos = VAdd(transform_.pos, VScale(dir, MOVE_SPEED));

    // å¸Ç´ÅFñ⁄ïWäpÇãÅÇﬂÇÈ
    float targetAngle = atan2f(-dir.x, -dir.z);

    // Ç±Ç±Ç≈ transform_ ÇÃâÒì]Ç…îΩâfÇ∑ÇÈÅiâ∫ãLÅj
    transform_.quaRot = Quaternion::AngleAxis(targetAngle, AsoUtility::AXIS_Y);
}

void Enemy::MeshInit()
{
    cutter_.SetMesh(MeshCut::FromMV1(transform_.modelId));
    cutter_.SetGravity(GRAVITY);
    cutter_.SetFloorFromMesh();
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
        MV1DrawModel(transform_.modelId);
    }

    if (!isCut_)
    {
        DrawDetectRange();
    }

    auto pos = transform_.pos;
    pos.y += 80;
    DrawSphere3D(pos, 10, 12, 0xffffff, 0xffffff, true);
}

void Enemy::Release()
{
}