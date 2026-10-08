#pragma once
#include <string>
#include <map>
class SceneManager;

class AnimationController
{

public:

	// アニメーションデータ
	struct Animation
	{
		int model = -1;
		int attachNo = -1;
		int animIndex = 0;
		float speed = 0.0f;
		float totalTime = 0.0f;
		float step = 0.0f;
	};

	// コンストラクタ
	AnimationController(int modelId);
	// デストラクタ
	~AnimationController(void);

	// アニメーション追加
	void Add(int type, const std::string& path, float speed);

	// アニメーション再生
	// blendTime : 前のアニメーションからのブレンド時間(秒)。0以下でブレンドなし
	void Play(int type, bool isLoop = true,
		float startStep = 0.0f, float endStep = -1.0f, bool isStop = false, bool isForce = false,
		float blendTime = 0.2f);

	void Update(void);

	// アニメーション終了後に繰り返すループステップ
	void SetEndLoop(float startStep, float endStep, float speed);

	// 再生中のアニメーション
	int GetPlayType(void) const;

	// 再生終了
	bool IsEnd(void) const;

	// ブレンド中か
	bool IsBlending(void) const;

private:

	// 1つのアニメーションの再生状態
	// (現在のアニメーションと、ブレンド元のアニメーションで共通して使う)
	struct PlayState
	{
		Animation anim;

		// アニメーションをループするかしないか
		bool isLoop = false;

		// アニメーションを止めたままにする
		bool isStop = false;

		// アニメーション終了後に繰り返すループステップ
		float stepEndLoopStart = -1.0f;
		float stepEndLoopEnd = -1.0f;
		float endLoopSpeed = 0.0f;

		// 逆再生(1.0f:通常、-1.0f:逆再生)
		float switchLoopReverse = 1.0f;
	};

	// モデルのハンドルID
	int modelId_;

	// 種類別のアニメーションデータ
	std::map<int, Animation> animations_;

	// 再生中のアニメーション
	int playType_;
	PlayState cur_;

	// ブレンド元のアニメーション
	PlayState prev_;
	bool hasPrev_;

	// ブレンド時間と経過時間
	float blendTime_;
	float blendTimer_;

	// 1つのアニメーションの時間を進める
	void Advance(PlayState& state, float deltaTime);

	// ブレンド元のアニメーションを外す
	void DetachPrev(void);

};