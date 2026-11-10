#include "GameClear.h"
#include <DxLib.h>
#include <algorithm>
#include <cmath>
#include "../Application.h"
#include "../Manager/SceneManager.h"
#include "../Manager/InputManager.h"

static constexpr int W = Application::SCREEN_SIZE_X;
static constexpr int H = Application::SCREEN_SIZE_Y;

GameClear::GameClear(void) : timer_(0.0f), nextBurstTimer_(0.3f) {}
GameClear::~GameClear(void) {}

void GameClear::Init(void)
{
	timer_          = 0.0f;
	nextBurstTimer_ = 0.3f;
	particles_.clear();
	for (int i = 0; i < 30; i++) SpawnStar();
}

void GameClear::SpawnStar(void)
{
	Particle p;
	p.x       = static_cast<float>(rand() % W);
	p.y       = static_cast<float>(H + 5 + rand() % 60);
	p.vx      = (rand() % 40 - 20) * 0.015f;
	p.vy      = -(rand() % 60 + 30) * 0.04f;
	p.life    = p.maxLife = static_cast<float>(rand() % 90 + 80);
	p.size    = static_cast<float>(rand() % 3 + 1);
	int style = rand() % 3;
	if      (style == 0) { p.r = 255; p.g = 220; p.b = 60;  }
	else if (style == 1) { p.r = 255; p.g = 255; p.b = 255; }
	else                 { p.r = 150; p.g = 200; p.b = 255; }
	p.isBurst = false;
	particles_.push_back(p);
}

void GameClear::SpawnBurst(float cx, float cy)
{
	const int COUNT = 28;
	for (int i = 0; i < COUNT; i++)
	{
		Particle p;
		p.x    = cx; p.y = cy;
		float angle = (DX_PI_F * 2.0f / COUNT) * i + (rand() % 100) * 0.03f;
		float speed = (rand() % 50 + 50) * 0.1f;
		p.vx   = cosf(angle) * speed;
		p.vy   = sinf(angle) * speed;
		p.life = p.maxLife = static_cast<float>(rand() % 25 + 40);
		p.size = static_cast<float>(rand() % 3 + 2);
		int style = rand() % 4;
		if      (style == 0) { p.r = 255; p.g = 220; p.b = 50;  }
		else if (style == 1) { p.r = 255; p.g = 100; p.b = 100; }
		else if (style == 2) { p.r = 100; p.g = 255; p.b = 150; }
		else                 { p.r = 150; p.g = 200; p.b = 255; }
		p.isBurst = true;
		particles_.push_back(p);
	}
}

void GameClear::Update(void)
{
	timer_ += 1.0f / 60.0f;

	nextBurstTimer_ -= 1.0f / 60.0f;
	if (nextBurstTimer_ <= 0.0f)
	{
		float bx = static_cast<float>(rand() % W);
		float by = static_cast<float>(H / 4 + rand() % (H / 2));
		SpawnBurst(bx, by);
		nextBurstTimer_ = 0.4f + (rand() % 40) * 0.01f;
	}

	for (auto& p : particles_)
	{
		p.x    += p.vx;
		p.y    += p.vy;
		if (p.isBurst) p.vy += 0.07f;
		p.life -= 1.0f;
	}

	particles_.erase(
		std::remove_if(particles_.begin(), particles_.end(),
			[](const Particle& p) { return p.life <= 0.0f || p.y > H + 30; }),
		particles_.end());

	int starCount = 0;
	for (const auto& p : particles_) if (!p.isBurst) starCount++;
	while (starCount < 25) { SpawnStar(); starCount++; }

	if (timer_ > 1.5f)
	{
		auto& input = InputManager::GetInstance();
		if (input.IsTrgDown(KEY_INPUT_SPACE) ||
			input.IsPadBtnNew(InputManager::JOYPAD_NO::PAD1, InputManager::JOYPAD_BTN::DOWN))
		{
			SceneManager::GetInstance().ChangeScene(SceneManager::SCENE_ID::TITLE);
		}
	}
}

void GameClear::Draw(void)
{
	// 暗いグラデーション背景
	for (int y = 0; y < H; y += 2)
	{
		float t = static_cast<float>(y) / H;
		DrawBox(0, y, W, y + 2,
			GetColor(
				static_cast<int>(5  + t * 10),
				static_cast<int>(5  + t * 20),
				static_cast<int>(30 + t * 50)),
			TRUE);
	}

	// 加算合成でパーティクル描画
	for (const auto& p : particles_)
	{
		int alpha = static_cast<int>(255.0f * p.life / p.maxLife);
		SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
		int s = static_cast<int>(p.size);
		DrawBox(
			static_cast<int>(p.x) - s, static_cast<int>(p.y) - s,
			static_cast<int>(p.x) + s, static_cast<int>(p.y) + s,
			GetColor(p.r, p.g, p.b), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	// GAME CLEAR テキスト: 上からスライドイン + 色サイクル + スケールパルス
	float slideT = timer_ < 1.0f ? timer_ : 1.0f;
	slideT = 1.0f - (1.0f - slideT) * (1.0f - slideT); // ease out
	int titleY = static_cast<int>(-100.0f + (H / 2 - 60) * slideT);

	float hue   = timer_ * 1.5f;
	int tr = 255;
	int tg = static_cast<int>(200 + 55 * sinf(hue));
	int tb = static_cast<int>(60  + 60 * sinf(hue + 2.0f));

	double pulse = 1.0 + 0.06 * sin(timer_ * 2.5);

	SetFontSize(64);
	int textW = GetDrawStringWidth("GAME CLEAR", 10);
	int textX = W / 2 - static_cast<int>(textW * pulse * 0.5);

	DrawExtendString(textX + 4, titleY + 4, pulse, pulse, "GAME CLEAR", GetColor(0, 0, 0));
	DrawExtendString(textX,     titleY,     pulse, pulse, "GAME CLEAR", GetColor(tr, tg, tb));

	// ヒントテキスト: フェードイン + 点滅
	if (timer_ > 1.5f)
	{
		float fadeIn = (timer_ - 1.5f) / 0.5f;
		if (fadeIn > 1.0f) fadeIn = 1.0f;
		int alpha = static_cast<int>(255.0f * fadeIn * (0.65f + 0.35f * sinf(timer_ * 3.5f)));
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		SetFontSize(22);
		const char* hint = "Space / A で戻る";
		int hintW = GetDrawStringWidth(hint, -1);
		DrawString(W / 2 - hintW / 2, H * 3 / 4, hint, GetColor(255, 255, 255));
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	SetFontSize(16);
}