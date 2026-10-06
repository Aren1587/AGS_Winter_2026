#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Moon.h"

Moon::Moon()
    :
    animationController_(nullptr)
{
}

Moon::~Moon()
{
    Release();
}

void Moon::Init()
{
    transform_.modelId = MV1LoadModel("Data/Model/model.mv1");
    MV1SetScale(transform_.modelId, { 1.0f, 1.0f, 1.0f });
    //cutter_ = new MeshCutter();
    MV1SetPosition(transform_.modelId, { 100, 100, 600 });

    std::string path = Application::PATH_MODEL;
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);
    animationController_->Add(0, path + "SkinningTest.mv1", 50.0f);

    animationController_->Play(0);

    cutter_.SetMesh(MeshCut::FromMV1(transform_.modelId));
    cutter_.SetGravity(0.3f);
    cutter_.SetFloorFromMesh();
}

void Moon::Cut(VECTOR origin, VECTOR normal) 
{
    cutter_.Cut(origin, normal);
}

void Moon::Update()
{
    animationController_->Update();
    cutter_.Update();
}

void Moon::Draw()
{
    cutter_.Draw();
    //MV1DrawModel(transform_.modelId);
    DrawSphere3D({ 100,100,600 }, 10, 12, 0xffffff, 0xffffff, true);
}

void Moon::Release()
{

}