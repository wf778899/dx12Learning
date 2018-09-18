#pragma once

class GameTimer
{
public:
	GameTimer();
	float TotalTime() const; // � ���
	float DeltaTime() const; // � ���

	void Reset();	// ������� �� message loop
	void Start();	// ��� ������ �� �����
	void Stop();	// ��� ����� � �����
	void Tick();	// ������ ����� (� Run())
	
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

