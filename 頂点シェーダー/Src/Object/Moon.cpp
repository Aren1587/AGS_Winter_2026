#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Moon.h"

Moon::Moon()
    :
    animationController_(nullptr),
    isCut_(false)
{
}

Moon::~Moon()
{
    Release();
}

void Moon::Init()
{
    transform_.modelId = MV1LoadModel("Data/Model/model.mv1");
    transform_.pos = { 100, 100, 600 };
    MV1SetScale(transform_.modelId, { 1.0f, 1.0f, 1.0f });
    //cutter_ = new MeshCutter();
    MV1SetPosition(transform_.modelId, transform_.pos);

    std::string path = Application::PATH_MODEL;
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);
    animationController_->Add(0, path + "SkinningTest.mv1", 50.0f);

    animationController_->Play(0);
}

void Moon::MeshInit()
{
}

void Moon::Cut(VECTOR origin, VECTOR normal) 
{
    if(!isCut_)
    {
        isCut_ = true;
        MV1RefreshReferenceMesh(transform_.modelId, 0, true);
        cutter_.SetMesh(MeshCut::FromMV1(transform_.modelId));
        cutter_.SetGravity(0.3f);
        cutter_.SetFloorFromMesh();
    }

    cutter_.Cut(origin, normal);
}

void Moon::Update()
{
    animationController_->Update();
    cutter_.Update();
}

void Moon::Draw()
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

void Moon::Release()
{

}