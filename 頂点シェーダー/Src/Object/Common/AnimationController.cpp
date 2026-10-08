#include <DxLib.h>
#include <algorithm>
#include "../../Manager/SceneManager.h"
#include "AnimationController.h"

AnimationController::AnimationController(int modelId)
{
	modelId_ = modelId;

	playType_ = -1;

	hasPrev_ = false;
	blendTime_ = 0.0f;
	blendTimer_ = 0.0f;
}

AnimationController::~AnimationController(void)
{
	for (const auto& anim : animations_)
	{
		MV1DeleteModel(anim.second.model);
	}
}

void AnimationController::Add(int type, const std::string& path, float speed)
{

	Animation anim;

	anim.model = MV1LoadModel(path.c_str());
	anim.animIndex = type;
	anim.speed = speed;

	if (animations_.count(type) == 0)
	{
		// 追加
		animations_.emplace(type, anim);
	}
	else
	{
		// 入れ替え
		animations_[type].model = anim.model;
		animations_[type].animIndex = anim.animIndex;
		animations_[type].attachNo = anim.attachNo;
		animations_[type].totalTime = anim.totalTime;
		animations_[type].speed = anim.speed;
	}

}

void AnimationController::Play(int type, bool isLoop,
	float startStep, float endStep, bool isStop, bool isForce, float blendTime)
{

	if (playType_ == type && !isForce)
	{
		return;
	}

	auto it = animations_.find(type);
	if (it == animations_.end())
	{
		// 未登録のアニメーション
		return;
	}

	// ブレンド中に別のアニメーションへ切り替えた場合は、
	// 一番古いブレンド元を破棄する
	if (hasPrev_)
	{
		DetachPrev();
	}

	if (playType_ != -1)
	{
		if (blendTime > 0.0f)
		{
			// 現在のアニメーションをブレンド元として残す
			prev_ = cur_;
			hasPrev_ = true;
		}
		else
		{
			// ブレンドしない場合は、すぐにモデルから外す
			MV1DetachAnim(modelId_, cur_.anim.attachNo);
		}
	}

	// アニメーション種別を変更
	playType_ = type;
	cur_ = PlayState();
	cur_.anim = it->second;

	// 初期化
	cur_.anim.step = startStep;

	// モデルにアニメーションを付ける
	int animIdx = 0;
	if (MV1GetAnimNum(cur_.anim.model) > 1)
	{
		// アニメーションが複数保存されていたら、番号1を指定
		animIdx = 1;
	}
	cur_.anim.attachNo = MV1AttachAnim(modelId_, animIdx, cur_.anim.model);

	// アニメーション総時間の取得
	if (endStep > 0.0f)
	{
		cur_.anim.totalTime = endStep;
	}
	else
	{
		cur_.anim.totalTime = MV1GetAttachAnimTotalTime(modelId_, cur_.anim.attachNo);
	}

	cur_.isLoop = isLoop;
	cur_.isStop = isStop;

	// ブレンドの初期化
	if (hasPrev_)
	{
		blendTime_ = blendTime;
		blendTimer_ = 0.0f;
		// 新しい方は0%から、前の方は100%から始める
		MV1SetAttachAnimBlendRate(modelId_, cur_.anim.attachNo, 0.0f);
		MV1SetAttachAnimBlendRate(modelId_, prev_.anim.attachNo, 1.0f);
	}
	else
	{
		MV1SetAttachAnimBlendRate(modelId_, cur_.anim.attachNo, 1.0f);
	}

}

void AnimationController::Update(void)
{

	if (playType_ == -1)
	{
		// 何も再生していない
		return;
	}

	// 経過時間の取得
	float deltaTime = SceneManager::GetInstance().GetDeltaTime();

	// 現在のアニメーションを進める
	Advance(cur_, deltaTime);

	if (hasPrev_)
	{
		// ブレンド元も進め続ける(止めるとポーズが固まって不自然になる)
		Advance(prev_, deltaTime);

		// ブレンド率の計算
		blendTimer_ += deltaTime;
		float rate = (std::min)(blendTimer_ / blendTime_, 1.0f);

		MV1SetAttachAnimBlendRate(modelId_, cur_.anim.attachNo, rate);
		MV1SetAttachAnimBlendRate(modelId_, prev_.anim.attachNo, 1.0f - rate);

		MV1SetAttachAnimTime(modelId_, prev_.anim.attachNo, prev_.anim.step);

		if (rate >= 1.0f)
		{
			// ブレンド完了
			DetachPrev();
		}
	}

	// アニメーション設定
	MV1SetAttachAnimTime(modelId_, cur_.anim.attachNo, cur_.anim.step);

}

void AnimationController::Advance(PlayState& state, float deltaTime)
{

	if (state.isStop)
	{
		return;
	}

	Animation& anim = state.anim;

	// 再生
	anim.step += (deltaTime * anim.speed * state.switchLoopReverse);

	// アニメーション終了判定
	bool isEnd = false;
	if (state.switchLoopReverse > 0.0f)
	{
		// 通常再生の場合
		if (anim.step > anim.totalTime)
		{
			isEnd = true;
		}
	}
	else
	{
		// 逆再生の場合
		if (anim.step < anim.totalTime)
		{
			isEnd = true;
		}
	}

	if (!isEnd)
	{
		return;
	}

	// アニメーションが終了したら
	if (state.isLoop)
	{
		// ループ再生
		if (state.stepEndLoopStart > 0.0f)
		{
			// アニメーション終了後の指定フレーム再生
			state.switchLoopReverse *= -1.0f;
			if (state.switchLoopReverse > 0.0f)
			{
				anim.step = state.stepEndLoopStart;
				anim.totalTime = state.stepEndLoopEnd;
			}
			else
			{
				anim.step = state.stepEndLoopEnd;
				anim.totalTime = state.stepEndLoopStart;
			}
			anim.speed = state.endLoopSpeed;
		}
		else
		{
			// 通常のループ再生
			anim.step = 0.0f;
		}
	}
	else
	{
		// ループしない
		anim.step = anim.totalTime;
	}

}

void AnimationController::DetachPrev(void)
{
	if (hasPrev_)
	{
		MV1DetachAnim(modelId_, prev_.anim.attachNo);
		prev_ = PlayState();
		hasPrev_ = false;
	}
}

void AnimationController::SetEndLoop(float startStep, float endStep, float speed)
{
	cur_.stepEndLoopStart = startStep;
	cur_.stepEndLoopEnd = endStep;
	cur_.endLoopSpeed = speed;
}

int AnimationController::GetPlayType(void) const
{
	return playType_;
}

bool AnimationController::IsEnd(void) const
{

	if (cur_.isLoop)
	{
		// ループ設定されているなら、
		// 無条件で終了しないを返す
		return false;
	}

	if (cur_.anim.step >= cur_.anim.totalTime)
	{
		// 再生時間を過ぎたらtrue
		return true;
	}

	return false;

}

bool AnimationController::IsBlending(void) const
{
	return hasPrev_;
}