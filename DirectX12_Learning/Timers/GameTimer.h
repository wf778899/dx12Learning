#pragma once

class GameTimer
{
public:
	GameTimer();
	float TotalTime() const; // в сек
	float DeltaTime() const; // в сек

	void Reset();	// вызвать до message loop
	void Start();	// при выходе из паузы
	void Stop();	// при входе в паузу
	void Tick();	// каждый фрейм (в Run())
	
	virtual ~GameTimer();
private:
	double m_SecondsPerCount;
	double m_deltaTime;

	__int64 m_baseTime;
	__int64 m_pausedTime;
	__int64 m_stopTime;
	__int64 m_prevTime;
	__int64 m_currentTime;

	bool m_stopped;
};

