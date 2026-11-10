#pragma once
#include <vector>
#include "SceneBase.h"

class GameClear : public SceneBase
{
public:
	GameClear(void);
	~GameClear(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	struct Particle
	{
		float x, y;
		float vx, vy;
		float life, maxLife;
		float size;
		int r, g, b;
		bool isBurst;
	};

	float timer_;
	float nextBurstTimer_;
	std::vector<Particle> particles_;

	void SpawnStar(void);
	void SpawnBurst(float cx, float cy);
};