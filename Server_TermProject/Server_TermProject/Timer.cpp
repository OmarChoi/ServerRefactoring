#include "stdafx.h"
#include "Timer.h"
#include "NetworkManager.h"

extern NetworkManager g_NetworkManager;

void Timer::AddTimer(int objId, chrono::system_clock::time_point timerPoint, TIMER_TYPE timerEvent)
{
	TimerEvent newEvent{ objId, timerPoint, timerEvent };
	m_timerQueue.push(newEvent);
}

void Timer::ProcessTimer()
{
	while (true) {
		TimerEvent ev;
		auto current_time = chrono::system_clock::now();
		if (true == m_timerQueue.try_pop(ev)) {
			if (ev.m_execTime > current_time) 
			{
				m_timerQueue.push(ev);
				this_thread::sleep_for(1ms);
				continue;
			}
			OVER_EXP* exover = new OVER_EXP;
			switch (ev.m_Type)
			{
			case TT_MOVE:
				exover->m_compType = OP_NPC_MOVE;
				PostQueuedCompletionStatus(g_NetworkManager.m_hIocp, 1, ev.m_objId, &exover->m_over);
				break;
			case TT_ATTACK:
				exover->m_compType = OP_NPC_ATTACK;
				PostQueuedCompletionStatus(g_NetworkManager.m_hIocp, 1, ev.m_objId, &exover->m_over);
				break;
			case TT_RESPAWN:
				exover->m_compType = OP_NPC_RESPAWN;
				PostQueuedCompletionStatus(g_NetworkManager.m_hIocp, 1, ev.m_objId, &exover->m_over);
				break;
			case TT_SAVE:
				exover->m_compType = OP_SAVE_DATA;
				PostQueuedCompletionStatus(g_NetworkManager.m_hIocp, 1, ev.m_objId, &exover->m_over);
				break;
			}
			continue;
		}
		this_thread::sleep_for(1ms);
	}
}
