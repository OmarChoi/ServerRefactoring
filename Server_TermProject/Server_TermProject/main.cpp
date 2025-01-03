#include "stdafx.h"
#include "Timer.h"
#include "Manager.h"
#include "DataBase.h"
#include "GameObject.h"
#include "NetworkManager.h"

Timer g_Timer;
DataBase g_DataBase;

array<Player*, MAX_USER> Players;
array<NPC*, MAX_NPC> NPCs;
mutex g_playerLock;

int g_MapData[W_WIDTH][W_HEIGHT];

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

		cout << "Current Op : " << ex_over->m_compType << endl;
		cout << "Current Key : " << key << endl;
		switch (ex_over->m_compType) 
		{
			case OP_ACCEPT:
			{
				manager.GetNetworkManager()->Accept();
				break;
			}
			case OP_RECV:
			{
				cout << "OP_RECV Excute \n";
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
				if (NPCs[static_cast<int>(key)]->m_isActive == false) return;
				NPCs[static_cast<int>(key)]->Move();
				if(ex_over != NULL)
					delete ex_over;
				break;
			}
			case OP_NPC_ATTACK:
			{
				if (NPCs[static_cast<int>(key)]->m_isActive == false) return;
				NPCs[static_cast<int>(key)]->Attack();
				if (ex_over != NULL)
					delete ex_over;
				break;
			}
			case OP_NPC_RESPAWN:
			{
				NPCs[static_cast<int>(key)]->m_isDead = false;
				NPCs[static_cast<int>(key)]->UpdateViewList();
				if (ex_over != NULL)
					delete ex_over;
				break;
			}
			case OP_SAVE_DATA:
			{
				for (int i = 0; i < MAX_USER; ++i)
				{
					if (Players[i] == nullptr) continue;
					if (Players[i]->m_playerState != CT_INGAME) continue;
					g_DataBase.UpdatePlayerData(Players[i]->m_nameForDB, i);
				}
				g_Timer.AddTimer(99999, chrono::system_clock::now() + 60s, TT_SAVE);
				if (ex_over != NULL)
					delete ex_over;
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

int API_MoveStart(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);

	TimerEvent ev{ my_id, chrono::system_clock::now() + 1s, TT_MOVE };
	g_Timer.m_timerLock.lock();
	g_Timer.m_timerQueue.push(ev);
	g_Timer.m_timerLock.unlock();

	return 0;
}

int API_NPCMove(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -2);
	lua_pop(L, 2);

	TimerEvent ev{ my_id, chrono::system_clock::now() + 1s, TT_MOVE };
	g_Timer.m_timerLock.lock();
	g_Timer.m_timerQueue.push(ev);
	g_Timer.m_timerLock.unlock();

	return 0;
}

void InitializeNPC()
{
	std::cout << "NPC intialize begin.\n";

	for (int i = 0; i < MAX_NPC; ++i) {

		NPCs[i] = new NPC;

		// NPCs[i]->m_state = ST_INGAME;
		auto L = NPCs[i]->m_luaState = luaL_newstate();	// 모든 NPC에 가상머신 생성
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_MoveStart", API_MoveStart);
		lua_register(L, "API_NPCMove", API_NPCMove);
		lua_register(L, "API_get_player_x", API_get_player_x);
		lua_register(L, "API_get_player_y", API_get_player_y);
		lua_register(L, "API_get_npc_x", API_get_npc_x);
		lua_register(L, "API_get_npc_y", API_get_npc_y);

		NPCs[i]->m_objectID = i;
		NPCs[i]->m_targetID = -1;
		NPCs[i]->m_type = OT_NPC;
		if (i >= MAX_NPC - MAX_NPC * 0.05)
		{
			int minVal = W_WIDTH * 3 / 4;
			int maxVal = W_WIDTH;
			while (1)
			{
				NPCs[i]->m_xPos = rand() % (maxVal - minVal + 1) + minVal;
				NPCs[i]->m_yPos = rand() % (maxVal - minVal + 1) + minVal;
				if (NPCs[i]->m_xPos > 10 && NPCs[i]->m_yPos > 10)
				{
					if(NPCs[i]->CanGo(NPCs[i]->m_xPos, NPCs[i]->m_yPos))
					{
						NPCs[i]->m_spawnXPos = NPCs[i]->m_xPos;
						NPCs[i]->m_spawnYPos = NPCs[i]->m_yPos;
						break;
					}
				}
			}
			NPCs[i]->m_monserType = MT_ROAMING;
			sprintf_s(NPCs[i]->m_name, "R_%d", i);
		}
		else
		{
			while (1)
			{
				NPCs[i]->m_xPos = rand() % W_WIDTH;
				NPCs[i]->m_yPos = rand() % W_HEIGHT;
				if (NPCs[i]->m_xPos > 10 && NPCs[i]->m_yPos > 10)
				{
					if (NPCs[i]->CanGo(NPCs[i]->m_xPos, NPCs[i]->m_yPos))
					{
						NPCs[i]->m_spawnXPos = NPCs[i]->m_xPos;
						NPCs[i]->m_spawnYPos = NPCs[i]->m_yPos;
						break;
					}
				}
			}
			sprintf_s(NPCs[i]->m_name, "S_%d", i);
			NPCs[i]->m_monserType = MT_STAYING;
		}

	}
	std::cout << "NPC initialize end.\n";
}

void ReadMap()
{
	std::cout << "Map intialize begin.\n";
	ifstream mapFile("map.txt");

	int x = 0, y = 0;
	char temp = ' ';
	while (!mapFile.eof())
	{
		mapFile >> temp;
		g_MapData[x++][y] = (int)(temp - 48);
		if (x == W_WIDTH) 
		{
			y += 1;
			x = 0;
		}
	}
	std::cout << "Map intialize end.\n";
}

int main()
{
	// ReadMap();
	// InitializeNPC();
	// g_DataBase.InitalizeDB();
	Manager& manager = Manager::GetInstance();
	
	cout << "Init Network. \n";
	thread timer_thread{ TimerThread };
	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(WorkerThread);

	for (auto& th : worker_threads)
		th.join();

	timer_thread.join();
	WSACleanup();
}