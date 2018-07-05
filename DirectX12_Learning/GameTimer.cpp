#include "stdafx.h"
#include "GameTimer.h"


GameTimer::GameTimer()
	: m_SecondsPerCount(0.0)
	, m_deltaTime(-1.0)
	, m_baseTime(0)
	, m_pausedTime(0)
	, m_prevTime(0)
	, m_currentTime(0)
	, m_stopped(false)
{
	__int64 countPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSec);
	m_SecondsPerCount = 1.0 / (double)countPerSec;
}

// Общее время, прошедшее от вызова Reset(). 
float GameTimer::TotalTime() const
{
	// Если таймер остановлен, не нужно считать время с момента останова - только до. И из этого времени вычесть паузы,
	// которые могли быть до текущего останова (они аккумулируются за всё время работы приложения).
	if (m_stopped) {
		return (float)(((m_stopTime - m_pausedTime) - m_baseTime) * m_SecondsPerCount);
	}
	// Если таймер не остановлен, считаем время от начала и до текущего момента, исключая общее время пауз
	return (float)(((m_currentTime - m_pausedTime) - m_baseTime) * m_SecondsPerCount);
}


float GameTimer::DeltaTime() const
{
	return (float)m_deltaTime;
}


void GameTimer::Reset()
{
	__int64 currTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_baseTime = currTime;
	m_prevTime = currTime;
	m_stopTime = 0;
	m_stopped = false;
}


void GameTimer::Start()
{
	__int64 startTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	if (m_stopped) {
		m_pausedTime += (startTime - m_stopTime);
		m_prevTime = startTime;
		m_stopTime = 0;
		m_stopped = false;
	}
}


void GameTimer::Stop()
{
	if (!m_stopped) {
		__int64 currTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_stopTime = currTime;
		m_stopped = true;
	}
}


void GameTimer::Tick()
{
	if (m_stopped) {
		m_deltaTime = 0.0;
		return;
	}
	__int64 currTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currentTime = currTime;
	m_deltaTime = (m_currentTime - m_prevTime) * m_SecondsPerCount;
	m_prevTime = m_currentTime;

	if (m_deltaTime < 0.0) {
		m_deltaTime = 0.0;
	}
}


GameTimer::~GameTimer()
{
}
