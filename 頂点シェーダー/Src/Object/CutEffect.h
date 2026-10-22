#pragma once
#include <DxLib.h>
#include <vector>

// 斬った瞬間の演出(閃光ライン + 火花 + 画面フラッシュ)
class CutEffect
{
public:
	// pos      : 演出の中心(切断平面上の点)
	// normal   : 切断平面の法線
	// bladeDir : 刃の向き(tip - hilt)
	void Start(VECTOR pos, VECTOR normal, VECTOR bladeDir);

	void Update(void);
	void Draw(void);        // 3D部分(敵の描画後に呼ぶ)
	void DrawScreen(void);  // 画面フラッシュ(UIの前後どちらでも可)

private:
	struct Spark
	{
		VECTOR pos;
		VECTOR vel;
		int life;
		int maxLife;
	};

	std::vector<Spark> sparks_;

	// 閃光ライン
	VECTOR center_ = VGet(0, 0, 0);
	VECTOR lineDir_ = VGet(1, 0, 0);
	VECTOR sideDir_ = VGet(0, 1, 0);
	int lineLife_ = 0;
	int flashLife_ = 0;
};