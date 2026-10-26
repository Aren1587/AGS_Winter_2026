#pragma once

#include "SceneBase.h"

class SceneManager;

class GameClear : public SceneBase
{
public:
	// コンストラクタ
	GameClear(void);

	// デストラクタ
	~GameClear(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:
	// 画像
	int imgTitle_;

};