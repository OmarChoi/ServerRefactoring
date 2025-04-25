#include "stdafx.h"
#include "Timer.h"
#include "Manager.h"
#include "NpcSession.h"
#include "MapSession.h";
#include "GameManager.h";
#include "PlayerSession.h";
#include "NetworkManager.h";
#include "PlayerSocketHandler.h";

extern Timer g_Timer;

PlayerSession::~PlayerSession()
{
	{
		lock_guard<mutex> lock(m_npcViewListLock);
		m_npcViewList.clear();
	}
}

void PlayerSession::SetPos(int y, int x)
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	Creature::SetPos(y, x);
	Position nextPos{ y, x };
	gameManager->GetMapSession()->ChangeSection(ObjectType::Player, m_objectID, m_pos, nextPos);
}

void PlayerSession::SetRandomPos()
{
	Manager& manager = Manager::GetInstance();
	int yPos, xPos;
	do
	{
		uniform_int_distribution<int> distX(0, W_WIDTH - 1);
		uniform_int_distribution<int> distY(0, W_HEIGHT - 1);
		yPos = distY(rng);
		xPos = distX(rng);
	} while (manager.GetGameManager()->CanGo(yPos, xPos) == false);
	SetPos(yPos, xPos);
}

void PlayerSession::Init(PlayerSocketHandler* socket)
{
	m_pNetwork = socket;
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

bool PlayerSession::IsInGame()
{
	PlayerState currState = GetState();
	if (currState != PlayerState::CT_INGAME)
		return false;
	return true;
}

void PlayerSession::Attack()
{
	chrono::time_point attackTime = chrono::high_resolution_clock::now();
	auto duration = attackTime - m_lastAttackTime;
	auto durationMs = chrono::duration_cast<chrono::milliseconds>(duration);
	if (durationMs < 1000ms) return;
	m_lastAttackTime = attackTime;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_viewListLock.lock();
	auto nearNpc = m_npcViewList;
	m_viewListLock.unlock();

	for (auto it : nearNpc)
	{
		auto npc = gameManager->GetNpcSession(it);
		if (Utils::GetDist(npc->GetPos(), m_pos) <= 1)
		{
			npc->ApplyDamage(m_damage);
			if (npc->IsActive() == false)
				AddExp(npc->GetLevel());
		}
	}
}

void PlayerSession::ApplyDamage(int damage, int objId)
{
	Creature::ApplyDamage(damage, objId);
	m_statChanged.store(true, memory_order_relaxed);
}

void PlayerSession::AddExp(int exp)
{
	m_exp.fetch_add(exp);
	int mRequirement = GetExpRequirement(m_level.load());
	if (m_exp > mRequirement)
	{
		m_level.fetch_add(1);
		m_exp.fetch_sub(mRequirement);
	}
	m_statChanged.store(true, memory_order_relaxed);
}

void PlayerSession::Die()
{
	Creature::Die();
	
	int penalty = GetExpRequirement(m_level) * 0.2f;
	m_exp = std::max(0, m_exp - penalty);
	
	// N초 후 부활 타이머에 삽입
	if (IsInGame() == false) return;
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 5s, TIMER_TYPE::RespawnObject);
}

void PlayerSession::RespawnObject()
{
	if (IsInGame() == false) return;
	Creature::RespawnObject();
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
	auto it = m_npcViewList.find(objID);
	if (it != m_npcViewList.end())
		m_npcViewList.erase(it);
	m_npcViewListLock.unlock();
}

void PlayerSession::UpdateViewList()
{
	if (IsInGame() == false) return;
	UpdatePlayerViewList();
	UpdateNpcViewList();
}

void PlayerSession::UpdatePlayerViewList()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	m_viewListLock.unlock();
	unordered_set<int> newViewList;
	unordered_set<int> nearUserList;
	gameManager->GetMapSession()->GetUserInNearSection(this->m_pos, nearUserList);
	for (int pId : nearUserList)
	{
		if (pId == m_objectID) continue;
		auto player = gameManager->GetPlayerSession(pId);
		if (player == nullptr) continue;
		if (player->IsInGame() == false) continue;
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(pId);
		if (CanSee(player)) // 현재 내 시야 내에 있으면
		{
			if (prevViewList.find(pId) == prevViewList.end())
			{
				// 이전에는 내 시야에 없었으면 클라이언트에 추가 요청
				player->AddViewList(m_objectID);
				pNetwork->send_add_object_packet(this);
				m_pNetwork->send_add_object_packet(player.get());
			}
			else
				prevViewList.erase(pId);
			pNetwork->send_move_object_packet(this);
			newViewList.insert(pId);
		}
		else
		{
			if (prevViewList.find(pId) != prevViewList.end())
			{
				// 이전에는 있었는데 현재는 없으면
				player->RemoveViewList(m_objectID);
				pNetwork->send_remove_object_packet(this);
				m_pNetwork->send_remove_object_packet(player.get());
				prevViewList.erase(pId);
			}
		}
	}

	// 현재 같은 Section에 없는데 이전에는 시야에 있었을 때 ex) 로그아웃, Teleport 등
	for (int pId : prevViewList)
	{
		auto player = gameManager->GetPlayerSession(pId);
		if (player == nullptr) continue;
		if (player->IsInGame() == false)
		{
			PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(pId);
			player->RemoveViewList(m_objectID);
			pNetwork->send_remove_object_packet(this);
		}
		m_pNetwork->send_remove_object_packet(player.get());
	}

	m_viewListLock.lock();
	m_viewList = move(newViewList);
	m_viewListLock.unlock();
}

void PlayerSession::UpdateNpcViewList()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_npcViewListLock.lock();
	auto prevNpcViewList = m_npcViewList;
	m_npcViewListLock.unlock();

	unordered_set<int> newNpcViewList;
	unordered_set<int> nearNpcList;
	gameManager->GetMapSession()->GetNpcInNearSection(this->m_pos, nearNpcList);

	for (int nId : nearNpcList)
	{
		auto npc = gameManager->GetNpcSession(nId);
		if (npc == nullptr) continue;
		if (npc->GetHp() < FLT_EPSILON) continue;
		if (CanSee(npc))
		{
			if (prevNpcViewList.find(nId) == prevNpcViewList.end())
			{
				npc->AddViewList(m_objectID);
				m_pNetwork->send_add_npc_packet(npc.get());
				prevNpcViewList.erase(nId);
			}
			newNpcViewList.emplace(nId);
		}
		else
		{
			if (prevNpcViewList.find(nId) != prevNpcViewList.end())
			{
				npc->RemoveViewList(m_objectID);
				m_pNetwork->send_remove_npc_object_packet(npc.get());
				prevNpcViewList.erase(nId);
			}
		}
	}

	for (int nId : prevNpcViewList)
	{
		auto npc = gameManager->GetNpcSession(nId);
		if (CanSee(npc)) continue;
		npc->RemoveViewList(m_objectID);
		m_pNetwork->send_remove_npc_object_packet(npc.get());
	}

	m_npcViewListLock.lock();
	m_npcViewList = move(newNpcViewList);
	m_npcViewListLock.unlock();
}

void PlayerSession::LogOut()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();

	// DataBase에 플레이어 정보 갱신
	UpdateDBInfo();
	
	// PlayerSession 반환
	SetState(PlayerState::CT_FREE);
	SetActive(false);
	gameManager->GetMapSession()->DeleteCreature(ObjectType::Player, m_objectID, GetPos());
	{
		lock_guard<mutex> lock(m_viewListLock);
		for (int pId : m_viewList)
		{
			if (pId == m_objectID) continue;
			auto player = gameManager->GetPlayerSession(pId);
			if (player == nullptr) continue;
			if (player->IsInGame() == false) continue;
			PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(pId);
			player->RemoveViewList(m_objectID);
			pNetwork->send_remove_object_packet(this);
		}
		m_viewList.clear();
	}

	if (m_pNetwork) 
		m_pNetwork = nullptr;
}

void PlayerSession::UpdateDBInfo()
{
}

int PlayerSession::GetExpRequirement(int level)
{
	return 100 * pow(2, level - 1);
}
