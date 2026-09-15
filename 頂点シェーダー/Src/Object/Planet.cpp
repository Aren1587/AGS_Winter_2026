#include <DxLib.h>
#include "../Utility/AsoUtility.h"
#include "../Manager/SceneManager.h"
#include "../Renderer/VertexMaterial.h"
#include "../Renderer/VertexRenderer.h"
#include "Common/Transform.h"
#include "WarpStar.h"
#include "Planet.h"

Planet::Planet(const Stage::NAME& name, const TYPE& type, const Transform& transform)
{

	name_ = name;
	type_ = type;
	transform_ = transform;

	gravityPow_ = 0.0f;
	gravityRadius_ = 0.0f;
	deadLength_ = 0.0f;

}

Planet::~Planet(void)
{
}

void Planet::Init(void)
{
	gravityPow_ = DEFAULT_GRAVITY_POW;
	gravityRadius_ = DEFAULT_GRAVITY_RADIUS;
	deadLength_ = DEFAULT_DEAD_LENGTH;

	vertexMaterial_ = std::make_unique<VertexMaterial>(
		"StandardShaderVS.cso", 0,
		"StandardShaderPS.cso", 1);

	auto dir = GetLightDirection();

	vertexMaterial_->AddConstBufPS({ dir.x, dir.y, dir.z, 0.0f });

	vertexRenderer_ = std::make_unique<VertexRenderer>(transform_.modelId, *vertexMaterial_);
}

void Planet::Update(void)
{
}

void Planet::Draw(void)
{
	auto dir = GetLightDirection();
	vertexMaterial_->SetConstBufPS(0, { dir.x, dir.y, dir.z, 0.0f });

    //MV1DrawModel(transform_.modelId);
	vertexRenderer_->Draw();
}

void Planet::SetPosition(const VECTOR& pos)
{
    transform_.pos = pos;
    transform_.Update();
}

void Planet::SetRotation(const Quaternion& rot)
{
	transform_.quaRot = rot;
	transform_.Update();
}

float Planet::GetGravityPow(void) const
{
	return gravityPow_;
}

void Planet::SetGravityPow(float pow)
{
	gravityPow_ = pow;
}

float Planet::GetGravityRadius(void) const
{
	return gravityRadius_;
}

void Planet::SetGravityRadius(float radius)
{
	gravityRadius_ = radius;
}

const Planet::TYPE& Planet::GetType(void) const
{
	return type_;
}

bool Planet::InRangeGravity(const VECTOR& pos) const
{
	return false;
}

bool Planet::InRangeDead(const VECTOR& pos) const
{
	return false;
}

void Planet::SetDeadLength(float len)
{
	deadLength_ = len;
}

const Stage::NAME& Planet::GetName(void) const
{
	return name_;
}
