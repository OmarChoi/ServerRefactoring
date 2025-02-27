#include "stdafx.h"
#include "Timer.h"
#include "Manager.h"
#include "NetworkManager.h"

extern NetworkManager g_NetworkManager;

void Timer::AddTimer(int objId, chrono::system_clock::time_point timerPoint, TIMER_TYPE timerEvent)
{
	TimerEvent newEvent{ objId, timerPoint, timerEvent };
	m_timerQueue.push(newEvent);
}

void Timer::ProcessTimer()
{
	Manager& manager = Manager::GetInstance();
	HANDLE iocpObject = manager.GetNetworkManager()->m_hIocp;
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
			case TIMER_TYPE::NpcUpdate:
				exover->m_compType = COMP_TYPE::NpcUpdate;
				PostQueuedCompletionStatus(iocpObject, 1, ev.m_objId, &exover->m_over);
				break;
			case TIMER_TYPE::PlayerUpdate:
				exover->m_compType = COMP_TYPE::UpdatePlayerInfo;
				PostQueuedCompletionStatus(iocpObject, 1, ev.m_objId, &exover->m_over);
				break;
			case TIMER_TYPE::SaveData:
				exover->m_compType = COMP_TYPE::SaveData;
				PostQueuedCompletionStatus(iocpObject, 1, ev.m_objId, &exover->m_over);
				break;
			case TIMER_TYPE::RespawnObject:
				exover->m_compType = COMP_TYPE::RespawnObject;
				PostQueuedCompletionStatus(iocpObject, 1, ev.m_objId, &exover->m_over);
				break;
			}
			continue;
		}
		this_thread::sleep_for(1ms);
	}
}
