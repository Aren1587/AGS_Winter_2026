#include "CutEffect.h"
#include <cmath>

namespace
{
	constexpr int   LINE_LIFE = 14;     // 閃光ラインの寿命(フレーム)
	constexpr float LINE_LENGTH = 450.0f; // 片側の長さ
	constexpr int   FLASH_LIFE = 6;      // 画面フラッシュの長さ
	constexpr int   SPARK_NUM = 40;
	constexpr float SPARK_SPEED_MIN = 8.0f;
	constexpr float SPARK_SPEED_MAX = 28.0f;
	constexpr float PI = 3.14159265f;

	float Rand01(void) { return GetRand(10000) / 10000.0f; }
}

void CutEffect::Start(VECTOR pos, VECTOR normal, VECTOR bladeDir)
{
	center_ = pos;

	// 切断平面上の直交2軸(u: 刃に沿う方向, v: 平面内で直交する方向)
	VECTOR n = VNorm(normal);
	VECTOR u = VSub(bladeDir, VScale(n, VDot(bladeDir, n)));
	if (VSize(u) < 1e-4f) u = VGet(1, 0, 0);
	u = VNorm(u);
	VECTOR v = VNorm(VCross(n, u));

	lineDir_ = u;
	sideDir_ = v;
	lineLife_ = LINE_LIFE;
	flashLife_ = FLASH_LIFE;

	// 火花は切断面の上を放射状に飛ばす(=斬った面から飛び散って見える)
	sparks_.clear();
	for (int i = 0; i < SPARK_NUM; i++)
	{
		float a = Rand01() * PI * 2.0f;
		float s = SPARK_SPEED_MIN + Rand01() * (SPARK_SPEED_MAX - SPARK_SPEED_MIN);
		VECTOR dir = VAdd(VScale(u, cosf(a)), VScale(v, sinf(a)));

		Spark sp;
		sp.pos = pos;
		sp.vel = VScale(dir, s);
		sp.maxLife = 15 + GetRand(15);
		sp.life = sp.maxLife;
		sparks_.push_back(sp);
	}
}

void CutEffect::Update(void)
{
	if (lineLife_ > 0)  lineLife_--;
	if (flashLife_ > 0) flashLife_--;

	for (auto& sp : sparks_)
	{
		if (sp.life <= 0) continue;
		sp.pos = VAdd(sp.pos, sp.vel);
		sp.vel = VScale(sp.vel, 0.92f);   // 減速
		sp.vel.y -= 0.4f;                 // 少し落ちる
		sp.life--;
	}
}

void CutEffect::Draw(void)
{
	if (lineLife_ <= 0 && sparks_.empty()) return;

	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(FALSE);   // 加算描画は深度を書かない
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

	const int cyan = GetColor(80, 200, 255);
	const int white = GetColor(255, 255, 255);

	// --- 閃光ライン:最初に長く伸び、細くなりながら消える ---
	if (lineLife_ > 0)
	{
		float t = (float)lineLife_ / LINE_LIFE;      // 1→0
		float grow = 1.0f - t * t;                   // 素早く伸びる
		float len = LINE_LENGTH * (0.4f + 0.6f * grow);
		float width = 6.0f * t;                      // 太さ(オフセット幅)

		VECTOR a = VSub(center_, VScale(lineDir_, len));
		VECTOR b = VAdd(center_, VScale(lineDir_, len));

		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * t));
		// 芯は白、周りは青。オフセットした線を重ねて太く見せる
		DrawLine3D(a, b, white);
		for (int i = -2; i <= 2; i++)
		{
			if (i == 0) continue;
			VECTOR off = VScale(sideDir_, width * i * 0.5f);
			DrawLine3D(VAdd(a, off), VAdd(b, off), cyan);
		}
	}

	// --- 火花:進行方向に伸ばした短い線 ---
	for (const auto& sp : sparks_)
	{
		if (sp.life <= 0) continue;
		float t = (float)sp.life / sp.maxLife;
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * t));
		VECTOR tail = VSub(sp.pos, VScale(sp.vel, 1.5f));
		DrawLine3D(tail, sp.pos, t > 0.5f ? white : cyan);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetWriteZBuffer3D(TRUE);
}

void CutEffect::DrawScreen(void)
{
	if (flashLife_ <= 0) return;

	float t = (float)flashLife_ / FLASH_LIFE;
	SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(120 * t));
	DrawBox(0, 0, 4000, 4000, GetColor(120, 200, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}