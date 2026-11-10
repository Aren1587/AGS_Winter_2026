#pragma once
#include "SceneBase.h"

class TitleScene : public SceneBase
{

public:

	TitleScene(void);
	~TitleScene(void);

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

private:

	int imgTitle_;
	int fontSub_;
	int timer_;
	int select_;

};