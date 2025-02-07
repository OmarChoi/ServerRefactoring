#include "stdafx.h"
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
	for (int i = 0; i < MAX_NPC; ++i)
	{
		m_ppNpcSession[i] = new RoamingMonster();
		m_ppNpcSession[i]->SetObjId(i);
	}
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

bool GameManager::CanGo(Position pos)
{
	return m_mapSession->CanGo(pos);
}

bool GameManager::CanGo(int yPos, int xPos)
{
	Position pos{ yPos, xPos };
	return m_mapSession->CanGo(pos);
}

int GameManager::GetTileCost(Position pos)
{
	return m_mapSession->GetCost(pos);
}
