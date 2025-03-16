#include "stdafx.h"
#include <omp.h>
#include "MapSession.h"
#include "NpcSession.h"
#include "GameManager.h"
#include "PlayerSession.h"

GameManager::~GameManager()
{
	if (m_ppPlayerSession != nullptr)
	{
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (m_ppPlayerSession[i] != nullptr)
			{
				delete m_ppPlayerSession[i];
				m_ppPlayerSession[i] = nullptr;
			}
		}
		delete[] m_ppPlayerSession;
		m_ppPlayerSession = nullptr;
	}
}

void GameManager::Init()
{
	if (m_mapSession == nullptr)
		m_mapSession = new MapSession();

	m_ppPlayerSession = new PlayerSession * [MAX_USER];
	for (int i = 0; i < MAX_USER; ++i) 
	{
		m_ppPlayerSession[i] = new PlayerSession();
		m_ppPlayerSession[i]->SetObjId(i);
	}

	m_ppNpcSession = new NpcSession * [MAX_NPC];
	std::cout << "Initiate Npc Object initialization.\n";

	int numThreads = std::thread::hardware_concurrency();
	omp_set_num_threads(numThreads);

#pragma omp parallel
	{
		std::uniform_int_distribution<int> distX(0, W_WIDTH - 1);
		std::uniform_int_distribution<int> distY(0, W_HEIGHT - 1);

#pragma omp for schedule(dynamic) nowait
		for (int i = 0; i < MAX_NPC; ++i)
		{
			std::uniform_int_distribution<int> monsterType(1, 3);
			MonsterType type = static_cast<MonsterType>(monsterType(rng));
			m_ppNpcSession[i] = NpcFactory::CreateNpc(type);
			m_ppNpcSession[i]->SetObjId(i);
			
			string name = Utils::GetMonsterName(type) + to_string(i);
			m_ppNpcSession[i]->SetName(name);
			
			int yPos, xPos;
			do
			{
				yPos = distY(rng);
				xPos = distX(rng);
			} while (yPos < SafeZoneSize && xPos < SafeZoneSize || !CanGo(yPos, xPos));

			m_ppNpcSession[i]->InitPosition({ yPos, xPos });
			m_mapSession->ChangeSection(1, i, { -1, -1 }, { yPos, xPos });
		}
	}
	std::cout << "Npc Object initialization complete.\n";
}

void GameManager::AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
	float hp, float maxHp, int exp, int level)
{
	if (m_ppPlayerSession[playerId] == nullptr) 
	{
		cout << "Error : GameManager Init Error\n";
		cout << "m_ppPlayerSession was not created properly.\n";
		m_ppPlayerSession[playerId] = new PlayerSession();
	}
	m_ppPlayerSession[playerId]->SetObjId(playerId);
	m_ppPlayerSession[playerId]->SetName(playerName);
	m_ppPlayerSession[playerId]->SetPos(xPos, yPos);
	m_ppPlayerSession[playerId]->SetHp(hp);
	m_ppPlayerSession[playerId]->SetMaxHp(maxHp);
	m_ppPlayerSession[playerId]->SetExp(exp);
	m_ppPlayerSession[playerId]->SetLevel(level);
}

bool GameManager::CanGo(Position pos) const
{
	return m_mapSession->CanGo(pos);
}

bool GameManager::CanGo(int yPos, int xPos) const
{
	Position pos{ yPos, xPos };
	return m_mapSession->CanGo(pos);
}

int GameManager::GetTileCost(Position pos)
{
	return m_mapSession->GetCost(pos);
}

