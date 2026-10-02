#include <memory>
#include "../Application.h"
#include "Common/AnimationController.h"
#include "Moon.h"

void Moon::Init()
{
    int handle = MV1LoadModel("Data/Model/model.mv1");
    MV1SetScale(handle, { 0.5, 0.5, 0.5 });
    //cutter_ = new MeshCutter();
    MV1SetPosition(handle, { 100, 100, 600
        });

    std::string path = Application::PATH_MODEL + "Player/";
    animationController_ = std::make_unique<AnimationController>(transform_.modelId);

    cutter_.SetMesh(MeshCut::FromMV1(handle));
    cutter_.SetGravity(0.3f);
    cutter_.SetFloorFromMesh();
}

void Moon::Update()
{
    animationController_->Update();
    cutter_.Update();
}