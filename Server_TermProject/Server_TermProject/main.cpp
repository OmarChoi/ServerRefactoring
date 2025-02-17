#include "stdafx.h"
#include <omp.h>
#include "Timer.h"
#include "Manager.h"
#include "NpcSession.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "DataBaseManager.h"

Timer g_Timer;

void WorkerThread()
{
	Manager& manager = Manager::GetInstance();
	while (true) 
	{
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(manager.GetNetworkManager()->m_hIocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) 
		{
			if (ex_over->m_compType == COMP_TYPE::Accept)
				cout << "Accept Error";
			else 
			{
				cout << "GQCS Error on client[" << key << "]\n";
				// disconnect(static_cast<int>(key));
				if (ex_over->m_compType == COMP_TYPE::Send) delete ex_over;
				continue;
			}
		}

		switch (ex_over->m_compType)
		{
		case COMP_TYPE::Accept:
		{
			manager.GetNetworkManager()->Accept();
			break;
		}
		case COMP_TYPE::Recv:
		{
			manager.GetNetworkManager()->Recv(num_bytes, ex_over, key);
			break;
		}
		case COMP_TYPE::Send:
		{
			delete ex_over;
			break;
		}
		case COMP_TYPE::NpcUpdate:
		{
			NpcSession* npc = manager.GetGameManager()->GetNpcSession(static_cast<int>(key));
			npc->Update();
			if (ex_over != NULL)
				delete ex_over;
			break;
		}
		case COMP_TYPE::SaveData:
		{
			// GameManager에 존재하는 PlayerSession을 순회하여 플레이어 데이터 저장할 수 있는 함수 추가
			// manager.GetDataBaseManager()->UpdatePlayerData();
			g_Timer.AddTimer(99999, chrono::system_clock::now() + 60s, TIMER_TYPE::SaveData);
			break;
		}
		default:
			cout << "Error\n";
			break;
		}
	}
}

VOID TimerThread()
{
#ifndef stressTest
	g_Timer.AddTimer(99999, chrono::system_clock::now() + 5s, TT_SAVE);
#endif
	g_Timer.ProcessTimer();
}

int main()
{
	Manager& manager = Manager::GetInstance();
	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads - 1; ++i)
		worker_threads.emplace_back(WorkerThread);
	thread timer_thread{ TimerThread };
	for (auto& th : worker_threads)
		th.join();
	timer_thread.join();
	WSACleanup();
}