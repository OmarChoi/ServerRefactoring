#include "stdafx.h"
#include "Timer.h"
#include "Manager.h"
#include "NpcSession.h"
#include "GameManager.h";
#include "PlayerSession.h";
#include "NetworkManager.h";
#include "PlayerSocketHandler.h";

extern Timer g_Timer;

void PlayerSession::SetRandomPos()
{
	Manager& manager = Manager::GetInstance();
	int yPos, xPos;
	do
	{
		std::uniform_int_distribution<int> distX(0, W_WIDTH - 1);
		std::uniform_int_distribution<int> distY(0, W_HEIGHT - 1);
		yPos = distY(rng);
		xPos = distX(rng);
	} while (manager.GetGameManager()->CanGo(yPos, xPos) == false);
	SetPos(yPos, xPos);
}

void PlayerSession::Init()
{
}

void PlayerSession::SetState(PlayerState state)
{
	lock_guard<mutex> lock(m_stateLock);
	m_state = state;
}

PlayerState PlayerSession::GetState()
{
	lock_guard<mutex> lock(m_stateLock);
	PlayerState st = m_state;
	return st;
}

void PlayerSession::Die()
{
	Creature::Die();
	
	int penalty = GetExpRequirement(m_level) * 0.2f;
	m_exp = std::max(0, m_exp - penalty);

	// N초 후 부활 타이머에 삽입
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 5s, TIMER_TYPE::RespawnObject);
}

void PlayerSession::AddViewNPCList(int objID)
{
	m_npcViewListLock.lock();
	m_npcViewList.insert(objID);
	m_npcViewListLock.unlock();
}

void PlayerSession::RemoveViewNPCList(int objID)
{
	m_npcViewListLock.lock();
	if (m_npcViewList.find(objID) != m_npcViewList.end())
		m_npcViewList.erase(objID);
	m_npcViewListLock.unlock();
}

void PlayerSession::UpdateViewList()
{
	PlayerState currState = m_state;
	if (currState != PlayerState::CT_INGAME) return;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	PlayerSocketHandler* myNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(m_objectID);
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	auto prevNpcViewList = m_npcViewList;
	m_viewListLock.unlock();

	unordered_set<int> newViewList;
	unordered_set<int> newNpcViewList;

	for (int i = 0; i < MAX_USER; ++i)
	{
		PlayerSession* player = gameManager->GetPlayerSession(i);
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(i);
		if (player == nullptr) continue;
		if (i == m_objectID) continue;

		PlayerState st = player->GetState();
		if (st != PlayerState::CT_INGAME) continue;
		
		if (CanSee(player))	// 현재 내 시야에서 보이면
		{
			if (prevViewList.count(player->GetId()) == 0) // 이전에는 없었으면 추가
			{
				player->AddViewList(m_objectID);
				pNetwork->send_add_object_packet(this);
				myNetwork->send_add_object_packet(player);
			}
			// 있었으면 움직였다는 것을 알려줌
			pNetwork->send_move_object_packet(this);
			newViewList.emplace(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevViewList.count(i) != 0)	// 이전에는 있었으면 삭제
			{
				player->RemoveViewList(m_objectID);
				pNetwork->send_remove_object_packet(this);
				myNetwork->send_remove_object_packet(player);
			}
		}
	}

	for (int i = 0; i < MAX_NPC; ++i)
	{
		NpcSession* npc = gameManager->GetNpcSession(i);
		if (npc == nullptr) continue;
		if (CanSee(npc))	// 현재 내 시야에서 보이면
		{
			if (prevNpcViewList.count(i) == 0) // 이전에는 없었으면 추가
			{
				npc->AddViewList(m_objectID);
				myNetwork->send_add_npc_packet(npc);
			}
			newNpcViewList.emplace(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevNpcViewList.count(i) != 0)	// 이전에는 있었으면 삭제
			{
				npc->RemoveViewList(m_objectID);
				myNetwork->send_remove_npc_object_packet(npc);
			}
		}
	}

	m_viewListLock.lock();
	m_viewList = newViewList;
	m_npcViewList = newNpcViewList;
	m_viewListLock.unlock();
}

int PlayerSession::GetExpRequirement(int level)
{
	return 100 * pow(2, level - 1);
}
