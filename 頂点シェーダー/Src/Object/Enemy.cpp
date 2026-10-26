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
    constexpr float MOVE_SPEED = 1.0f;
    constexpr float GRAVITY = 0.3f;

    constexpr int RANDAM_MAX = 100;

    constexpr VECTOR INIT_POS = { 100, -10, 600 };

    constexpr float LIMIT_TIME = 3.0f;
}

Enemy::Enemy(Player& player)
    :
    state_(STATE::null),
    animationController_(nullptr),
    player_(player),
    isCut_(false)
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

    transform_.MakeCollider(Collider::TYPE::ENEMY);

    std::string path = Application::PATH_MODEL;
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);
    animationController_->Add(ANIM_TYPE::IDOL, path + "SkinningTest.mv1", 50.0f);
    animationController_->Add(ANIM_TYPE::RUN, path + "Running.mv1", 50.0f);

    animationController_->Play(ANIM_TYPE::IDOL);
}

void Enemy::ChangeState(STATE state) const
{
}

void Enemy::EnemyAI()
{
    auto rand = GetRand(RANDAM_MAX);

    if (rand < 50)
    {
        test = false;
		animationController_->Play(ANIM_TYPE::IDOL);
        return;
    }
    test = true;
    animationController_->Play(ANIM_TYPE::RUN);
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

	testTime += 1.0f / 60.0f;
    if(LIMIT_TIME < testTime)
    {
        testTime = 0.0f;
        EnemyAI();
	}

    if (test)
    {
        EnemyMove();
    }
    
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
    auto pos = transform_.pos;
    pos.y += 80;
    DrawSphere3D(pos, 10, 12, 0xffffff, 0xffffff, true);
}

void Enemy::Release()
{
}