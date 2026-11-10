#pragma once
#include "SceneBase.h"

class PauseScene : public SceneBase
{
public:
	enum class MENU_ITEM
	{
		RESUME = 0,
		TITLE,
		EXIT,
		MAX
	};

	PauseScene();
	~PauseScene();

	void Init(void) override;
	void Update(void) override;
	void Draw(void) override;

	bool IsResume(void) const { return isResume_; }
	bool IsGotoTitle(void) const { return isGotoTitle_; }
	bool IsExit(void) const { return isExit_; }

	void SetIsGameScene(bool isGameScene) { isGameScene_ = isGameScene; }
	void SetShowTitleOption(bool show) { showTitleOption_ = show; }

private:
	MENU_ITEM selectItem_;
	bool isResume_;
	bool isGotoTitle_;
	bool isExit_;
	bool isGameScene_;
	bool showTitleOption_;
};
