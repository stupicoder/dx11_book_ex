#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer()
	: mSecondsPerCount(0.0)
	, mDeltaTime(0.0)
	, mBaseTime(0)
	, mPausedTime(0)
	, mPrevTime(0)
	, mCurTime(0)
	, mbStopped(false)
{
	__int64 CountsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&CountsPerSec); // 초당 몇 번 수행되는지 = 주파수
	mSecondsPerCount = 1.0 / (double)CountsPerSec; // 한 번에 몇초 걸리는지 = 개수당 초
}

float GameTimer::GetDeltaTime() const
{
	return (float)mDeltaTime;
}

// Returns the total time elapsed since Reset() was called.
// Not counting anytime when the clock is stopped.
float GameTimer::GetTotalTime() const
{
	// 4.3.4 전체 시간

	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if (mbStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}

	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime

	else
	{
		return (float)(((mCurTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}

void GameTimer::Reset()
{
	__int64 CurTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurTime);

	mBaseTime = CurTime;
	mPrevTime = CurTime;
	mStopTime = 0;
	mbStopped = false;
}

void GameTimer::Start()
{
	__int64 StartTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&StartTime);

	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if (mbStopped)
	{
		mPausedTime += (StartTime - mStopTime);
		mPrevTime = StartTime;
		mStopTime = 0;
		mbStopped = false;
	}
}

void GameTimer::Stop()
{
	if (!mbStopped)
	{
		__int64 CurTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&CurTime);

		mStopTime = CurTime;
		mbStopped = true;
	}
}

void GameTimer::Tick()
{
	// 4.3.3 프레임 간 경과 시간

	if (mbStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 CurTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&CurTime);
	mCurTime = CurTime;

	// 이전 프레임과 현재 프레임의 Counter 차이에 Counter 별 시간을 곱하면 Deltatime 계산
	// Time difference between this frame and the previous.
	mDeltaTime = (mCurTime - mPrevTime) * mSecondsPerCount;

	// Prepare for next frame.
	mPrevTime = mCurTime;

	/*
		Force nonegative.  The DXSDK's CDUTTimer mentions that if the
		processor goes into a power save mode or we get shuffled to another
		processor, then mDeltaTIme can be negative.
	*/
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}
