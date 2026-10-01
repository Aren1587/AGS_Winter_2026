#include "Moon.h"

void Moon::Init()
{
    int handle = MV1LoadModel("Data/Model/model.mv1");
    MV1SetScale(handle, { 0.5, 0.5, 0.5 });
    //cutter_ = new MeshCutter();
    MV1SetPosition(handle, { 100, 100, 600
        });
    cutter_.SetMesh(MeshCut::FromMV1(handle));
    cutter_.SetGravity(0.3f);
    cutter_.SetFloorFromMesh();
}