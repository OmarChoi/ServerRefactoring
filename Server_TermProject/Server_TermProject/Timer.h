#pragma once

class Timer
{
public:
    concurrency::concurrent_priority_queue<TimerEvent> m_timerQueue;
	mutex m_timerLock;
public:
	Timer() {};
	~Timer() {};
public:
	void AddTimer(int objId, chrono::system_clock::time_point timerPoint, TIMER_TYPE timerEvent);
	void ProcessTimer();
};