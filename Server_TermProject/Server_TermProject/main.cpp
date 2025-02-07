#include "stdafx.h"
#include <omp.h>
#include "Timer.h"
#include "Manager.h"
#include "GameObject.h"
#include "NpcSession.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "DataBaseManager.h"

Timer g_Timer;

array<Player*, MAX_USER> Players;
array<NPC*, MAX_NPC> NPCs;
mutex g_playerLock;

constexpr int SafeZoneSize = 10;

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
			if (ex_over->m_compType == OP_ACCEPT) 
				cout << "Accept Error";
			else 
			{
				cout << "GQCS Error on client[" << key << "]\n";
				// disconnect(static_cast<int>(key));
				if (ex_over->m_compType == OP_SEND) delete ex_over;
				continue;
			}
		}

		switch (ex_over->m_compType)
		{
		case OP_ACCEPT:
		{
			manager.GetNetworkManager()->Accept();
			break;
		}
		case OP_RECV:
		{
			manager.GetNetworkManager()->Recv(num_bytes, ex_over, key);
			break;
		}
		case OP_SEND:
		{
			delete ex_over;
			break;
		}
		case OP_NPC_MOVE:
		{
			NpcSession* npc = manager.GetGameManager()->GetNpcSession(static_cast<int>(key));
			npc->Move();
			if (ex_over != NULL)
				delete ex_over;
			break;
		}
		case OP_NPC_ATTACK:
		{
			NpcSession* npc = manager.GetGameManager()->GetNpcSession(static_cast<int>(key));
			npc->Attack();
			if (ex_over != NULL)
				delete ex_over;
			break;
		}
		case OP_NPC_RESPAWN:
		{
			NpcSession* npc = manager.GetGameManager()->GetNpcSession(static_cast<int>(key));
			npc->Respawn();
			if (ex_over != NULL)
				delete ex_over;
			break;
		}
		case OP_SAVE_DATA:
		{
			// GameManager에 존재하는 PlayerSession을 순회하여 플레이어 데이터 저장할 수 있는 함수 추가
			// manager.GetDataBaseManager()->UpdatePlayerData();
			g_Timer.AddTimer(99999, chrono::system_clock::now() + 60s, TT_SAVE);
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

int API_get_player_x(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = Players[user_id]->m_xPos;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_player_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = Players[user_id]->m_yPos;
	lua_pushnumber(L, y);
	return 1;
}

int API_get_npc_x(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = NPCs[user_id]->m_xPos;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_npc_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = NPCs[user_id]->m_yPos;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	// Players[user_id]->send_chat_packet(my_id, mess, CHATTING_MESSAGE);
	return 0;
}

//int API_MoveStart(lua_State* L)
//{
//	int my_id = (int)lua_tointeger(L, -1);
//	lua_pop(L, 2);
//
//	TimerEvent ev{ my_id, chrono::system_clock::now() + 1s, TT_MOVE };
//	g_Timer.m_timerLock.lock();
//	g_Timer.m_timerQueue.push(ev);
//	g_Timer.m_timerLock.unlock();
//
//	return 0;
//}
//
//int API_NPCMove(lua_State* L)
//{
//	int my_id = (int)lua_tointeger(L, -2);
//	lua_pop(L, 2);
//
//	TimerEvent ev{ my_id, chrono::system_clock::now() + 1s, TT_MOVE };
//	g_Timer.m_timerLock.lock();
//	g_Timer.m_timerQueue.push(ev);
//	g_Timer.m_timerLock.unlock();
//
//	return 0;
//}

void InitializeNPC()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	std::cout << "Initiate Npc Object initialization.\n";

	int numThreads = std::thread::hardware_concurrency();
	omp_set_num_threads(numThreads);  // 원하는 스레드 수로 설정

#pragma omp parallel
	{
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> distX(0, W_WIDTH - 1);
		std::uniform_int_distribution<int> distY(0, W_HEIGHT - 1);

#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < 200'00; ++i)
		{
			NpcSession* npc = gameManager->GetNpcSession(i);

			auto L = npc->m_luaState = luaL_newstate();
			luaL_openlibs(L);
			luaL_loadfile(L, "npc.lua");
			lua_pcall(L, 0, 0, 0);

			lua_getglobal(L, "set_uid");
			lua_pushnumber(L, i);
			lua_pcall(L, 1, 0, 0);

			// Lua API 등록
			lua_register(L, "API_SendMessage", API_SendMessage);
			// lua_register(L, "API_MoveStart", API_MoveStart);
			// lua_register(L, "API_NPCMove", API_NPCMove);
			lua_register(L, "API_get_player_x", API_get_player_x);
			lua_register(L, "API_get_player_y", API_get_player_y);
			lua_register(L, "API_get_npc_x", API_get_npc_x);
			lua_register(L, "API_get_npc_y", API_get_npc_y);

			npc->SetId(i);
			npc->SetTarget(-1);
			npc->SetName("Npc" + std::to_string(i));

			int yPos, xPos;
			do 
			{
				yPos = distY(rng);
				xPos = distX(rng);
			} while (yPos < SafeZoneSize && xPos < SafeZoneSize || !gameManager->CanGo(yPos, xPos));

			npc->InitPosition({ yPos, xPos });
		}
	}
	std::cout << "Npc Object initialization complete.\n";
}

int main()
{
	Manager& manager = Manager::GetInstance();
	InitializeNPC();
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