#pragma once

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
	// クリア画像(今は未使用)
	//int imgClear_;
};