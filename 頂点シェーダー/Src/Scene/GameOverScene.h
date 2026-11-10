#pragma once

#include "SceneBase.h"

class GameOverScene : public SceneBase
{
public:
    GameOverScene(void);
    ~GameOverScene(void);

    void Init(void) override;
    void Update(void) override;
    void Draw(void) override;

private:
    float timer_     = 0.0f;
    int   fontLarge_ = -1;
    int   fontSmall_ = -1;
};
