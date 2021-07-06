#pragma once


// 4.3 타이밍과 애니메이션
class GameTimer
{
public:
	GameTimer();

	float GetDeltaTime() const; // 초 단위
	float GetTotalTime() const; // 초 단위

	void Reset(); // 메세지 루프 이전에 호출
	void Start(); // 타이머 시작-재개 시 호출해야함
	void Stop(); // 일시정지 시 호출해야 함
	void Tick(); // 매 프레임 호출해야함

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurTime;

	bool mbStopped;
};