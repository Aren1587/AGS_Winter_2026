namespace
{
    static constexpr float MOVE_SPEED = 1.0f;
}

#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Common/Collider.h"
#include "Common/Transform.h"
#include "../Utility/AsoUtility.h"
#include "Player.h"
#include "Enemy.h"

Enemy::Enemy(Player& player)
    :
    animationController_(nullptr),
    player_(player),
    isCut_(false)
{
}

Enemy::~Enemy()
{
    Release();
}

void Enemy::Init()
{
    transform_.modelId = MV1LoadModel("Data/Model/model.mv1");
    transform_.pos = { 100, 100, 600 };
    MV1SetScale(transform_.modelId, { 1.0f, 1.0f, 1.0f });
    //cutter_ = new MeshCutter();
    MV1SetPosition(transform_.modelId, transform_.pos);
    MV1SetupReferenceMesh(transform_.modelId, -1, true);

    transform_.MakeCollider(Collider::TYPE::ENEMY);

    std::string path = Application::PATH_MODEL;
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);
    animationController_->Add(0, path + "SkinningTest.mv1", 50.0f);
    animationController_->Add(1, path + "Running.mv1", 50.0f);

    animationController_->Play(0);
}

void Enemy::ChangeState(STATE state) const
{
}

void Enemy::EnemyAI()
{
    auto rand = GetRand(100);
    
    auto isRand = true;/*rand < 20;*/

    if (!isRand)
    {
        return;
    }
    EnemyMove();
}

void Enemy::EnemyMove()
{
    VECTOR toPlayer = VSub(player_.GetTransform().pos, transform_.pos);
    toPlayer.y = 0.0f;                       // …•½•ûŒü‚¾‚¯Œ©‚é
    float dist = VSize(toPlayer);
    if (dist < 0.001f) return;

    VECTOR dir = VScale(toPlayer, 1.0f / dist);

    // ˆÚ“®
    transform_.pos = VAdd(transform_.pos, VScale(dir, MOVE_SPEED));

    // Œü‚«F–Ú•WŠp‚ð‹‚ß‚é
    float targetAngle = atan2f(dir.x, dir.z);

    // ‚±‚±‚Å transform_ ‚Ì‰ñ“]‚É”½‰f‚·‚éi‰º‹Lj
    transform_.quaRot = Quaternion::AngleAxis(targetAngle, AsoUtility::AXIS_Y);
}

void Enemy::MeshInit()
{
    cutter_.SetMesh(MeshCut::FromMV1(transform_.modelId));
    cutter_.SetGravity(GRAVITY);
    cutter_.SetFloorFromMesh();
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

void Enemy::Update()
{
    animationController_->Update();
    cutter_.Update();
    EnemyAI();
    transform_.Update();

    MV1RefreshReferenceMesh(transform_.modelId, -1, true);
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
    DrawSphere3D({ 100,100,600 }, 10, 12, 0xffffff, 0xffffff, true);
}

void Enemy::Release()
{
}