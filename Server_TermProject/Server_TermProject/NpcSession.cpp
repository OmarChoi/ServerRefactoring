#include "stdafx.h"
#include "Timer.h"
#include "Manager.h"
#include "NpcSession.h"
#include "MapSession.h"
#include "GameManager.h"
#include "PlayerSession.h"
#include "NetworkManager.h"
#include "PlayerSocketHandler.h"

extern Timer g_Timer;
struct AStarNode
{
	Position	currPos;
	int			totalCost;			// 해당 좌표까지 누적 값 + 목표까지 남은 값
	int			cumulativeCost;		// 해당 좌표까지 누적 값
	AStarNode(Position pos, int t, int c) : currPos(pos), totalCost(t), cumulativeCost(c) {};
	bool operator > (const AStarNode& other) const { return totalCost > other.totalCost; }
};

NpcSession::NpcSession()
	: Creature(),
	m_type{ Monster::Type::Unknown },
	m_behavior{ Monster::Behavior::Normal },
	m_spawnPos{ -1, -1 },
	m_registUpdate{ false }
{
	m_currentState.store(Monster::State::Idle);

	stateFunc[static_cast<size_t>(Monster::State::Idle)] = [this]() { Roaming(); };
	stateFunc[static_cast<size_t>(Monster::State::Roaming)] = [this]() { Roaming(); };
	stateFunc[static_cast<size_t>(Monster::State::Attack)] = [this]() { Attack(); };
	stateFunc[static_cast<size_t>(Monster::State::Chase)] = [this]() { ChaseTarget(); };
	stateFunc[static_cast<size_t>(Monster::State::Die)] = [this]() { Die(); };
}

NpcSession::~NpcSession()
{
	lock_guard<mutex> lock(m_pathLock);
	while (!m_path.empty())
		m_path.pop();
}

void NpcSession::AddViewList(int objID)
{
	if (m_hp < FLT_EPSILON) return;
	if (m_bActive == false)
		ActiveNpc();
	std::lock_guard<std::mutex> lock(m_viewListLock);
	m_viewList.emplace(objID);
}

void NpcSession::RemoveViewList(int objID)
{
	{
		std::lock_guard<std::mutex> lock(m_viewListLock);
		if (m_viewList.find(objID) != m_viewList.end())
		{
			m_viewList.erase(objID);
			auto target = m_targetSession.load();
			if (target == nullptr) return;
			if (objID == target->GetId())
				ReleaseTarget();
		}
	}
	if (m_viewList.size() == 0)
	{
		DeActiveNpc();
	}
}

void NpcSession::SetPos(int y, int x)
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	Position prevPos = m_pos;
	Creature::SetPos(y, x);
	gameManager->GetMapSession()->ChangeSection(ObjectType::Npc, m_objectID, prevPos, m_pos);
}

void NpcSession::NpcSession::UpdateViewList()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	m_viewListLock.unlock();

	bool targetValid = false;
	unordered_set<int> newViewList;
	unordered_set<int> nearUserList;
	gameManager->GetMapSession()->GetUserInNearSection(this->m_pos, nearUserList);
	
	for (int pId : nearUserList)
	{
		auto player = gameManager->GetPlayerSession(pId);
		if (player == nullptr) continue;
		if (player->IsInGame() == false) continue;
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(pId);
		if (CanSee(player)) // 현재 내 시야 내에 있으면
		{
			if (prevViewList.find(pId) == prevViewList.end())
			{
				// 이전 시야에 없었으면
				// 해당 플레이어에게 Npc 추가
				player->AddViewNPCList(m_objectID);
				pNetwork->send_add_npc_packet(this);
			}
			else
			{
				// 이전 시야에 있었으면
				pNetwork->send_npc_move_object_packet(this);
				prevViewList.erase(pId);
			}
			newViewList.insert(pId);
		}
		else
		{
			if (prevViewList.find(pId) != prevViewList.end())
			{
				// 현재 시야에는 없지만 이전 시야에는 있었으면
				player->RemoveViewNPCList(m_objectID);
				pNetwork->send_remove_npc_object_packet(this);
				prevViewList.erase(pId);
			}
		}
	}

	for (int pId : prevViewList)
	{
		// 이전에 Npc 시야에 있었는데 현재 인근 Section에 존재하지 않음
		auto player = gameManager->GetPlayerSession(pId);
		if (player == nullptr) continue;
		if (player->IsInGame() == false)
		{
			PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(pId);
			player->RemoveViewNPCList(m_objectID);
			pNetwork->send_remove_npc_object_packet(this);
		}
	}

	if (newViewList.size() == 0)
	{
		DeActiveNpc();
		return;
	}
	m_viewListLock.lock();
	m_viewList = newViewList;
	m_viewListLock.unlock();
}

bool NpcSession::CheckTarget()
{
	auto player = m_targetSession.load();
	if (player == nullptr) return false;
	if (player->IsActive() == false || player->IsInGame() == false)
	{
		ReleaseTarget();
		return false;
	}
	return true;
}

void NpcSession::SetTarget(int objId)
{
	auto player = Manager::GetInstance().GetGameManager()->GetPlayerSession(objId);
	m_targetSession.store(player);
}

void NpcSession::ActiveNpc()
{
	if (m_bActive == true) return;
	if (m_hp < FLT_EPSILON)
	{
		m_hp = m_maxHp;
	}
	SetActive(true);	
	if (!m_registUpdate.exchange(true, std::memory_order_acq_rel))
	{
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
	}
}

void NpcSession::InitPosition(Position pos)
{
	m_spawnPos = pos;
	m_lastMoveTime = chrono::high_resolution_clock::now();
	m_pos = { pos.yPos, pos.xPos };
}

void NpcSession::ReleaseTarget()
{
	SetTarget(-1);
	lock_guard<mutex> lock(m_pathLock);
	while (!m_path.empty())
		m_path.pop();
}

void NpcSession::SetInfo(int i)
{
	m_type = Monster::InfoTable[i].type;
	m_behavior = Monster::InfoTable[i].behavior;
	m_hp = Monster::InfoTable[i].hp;
	m_maxHp = Monster::InfoTable[i].hp;
	m_damage = Monster::InfoTable[i].damage;
	m_attackRange = Monster::InfoTable[i].attackRange;
	m_speed = Monster::InfoTable[i].speed;
	m_level = Monster::InfoTable[i].level;
}

void NpcSession::Update()
{
	m_registUpdate.store(false, std::memory_order_release);
	if (m_bActive == false) return;
	ChangeState();
	Monster::State currState = m_currentState.load(memory_order_relaxed);

	size_t index = static_cast<size_t>(currState);
	if (index < stateFunc.size())
		stateFunc[index]();

	if (!m_registUpdate.exchange(true, std::memory_order_acq_rel)) 
	{
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
	}
}

void NpcSession::ChangeState()
{
	bool hasTarget = CheckTarget();
	int distance = NPC_VIEW_RANGE;
	if (hasTarget == true)
	{
		auto player = m_targetSession.load();
		if (player == nullptr) return;
		distance = Utils::GetDist(m_pos, player->GetPos());
	}

	if (m_hp <= 0.0f)
	{
		m_currentState.store(Monster::State::Die);
		return;
	}

	Monster::State state = m_currentState.load(memory_order_relaxed);
	switch (state) 
	{
	case Monster::State::Idle:
		m_currentState.store(Monster::State::Roaming);
		break;
	case Monster::State::Roaming:
		if (hasTarget)
			m_currentState.store(Monster::State::Chase);
		break;
	case Monster::State::Chase:
		if (!hasTarget)
			m_currentState.store(Monster::State::Roaming);
		else if (distance <= m_attackRange)
			m_currentState.store(Monster::State::Attack);
		break;
	case Monster::State::Attack:
		if (!hasTarget)
			m_currentState.store(Monster::State::Roaming);
		else if (distance > m_attackRange)
			m_currentState.store(Monster::State::Chase);
		break;
	default:
		break;
	}
}

void NpcSession::Attack()
{
	auto target = m_targetSession.load();
	if (target == nullptr) return;
	target->ApplyDamage(m_damage);
	if (target->GetHp() < FLT_EPSILON)
	 	ReleaseTarget();
}

void NpcSession::CreatePath()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	auto target = m_targetSession.load();
	if (target == nullptr) return;

	Position targetPos = target->GetPos();
	Position startPos = GetPos();
	int dist = Utils::GetDist(startPos, targetPos);
	if (gameManager->GetMapSession()->CanGo(targetPos) == false ||
		dist > VIEW_RANGE)
	{
		ReleaseTarget();
		return;
	}
	unordered_set<Position> closed;
	unordered_map<Position, int> best;
	unordered_map<Position, Position> parent;

	priority_queue<AStarNode, vector<AStarNode>, greater<AStarNode>> nextPath;
	parent[startPos] = startPos;
	best[startPos] = 0;
	nextPath.emplace(startPos, dist, 0);

	while (!nextPath.empty())
	{
		AStarNode currNode = nextPath.top();
		nextPath.pop();
		Position currPos = currNode.currPos;
		if (currPos == targetPos)
			break;
		if (closed.find(currPos) != closed.end())
			continue;
		closed.insert(currPos);

		for (int i = 0; i < 4; ++i)
		{
			Position nextPos = currPos + movements[i];
			if (!gameManager->CanGo(nextPos)) continue;
			if (closed.find(nextPos) != closed.end()) continue;

			int cCost = currNode.cumulativeCost + gameManager->GetTileCost(nextPos);
			auto it = best.find(nextPos);
			if (it != best.end() && it->second <= cCost)
				continue;

			int heuristic = Utils::GetDist(nextPos, targetPos);
			best[nextPos] = cCost;
			nextPath.emplace(nextPos, cCost + heuristic, cCost);
			parent[nextPos] = currPos;
		}
	}

	if (parent.find(targetPos) == parent.end()) 
	{
		std::lock_guard<std::mutex> lk(m_pathLock);
		while (!m_path.empty()) m_path.pop();
		return;
	}

	stack<Position> newPath;
	Position curr = targetPos;
	do {
		newPath.emplace(curr);
		auto it = parent.find(curr);
		if (it == parent.end()) break;
		curr = it->second;
	} while (curr != parent[curr]);

	{
		std::lock_guard<std::mutex> lk(m_pathLock);
		m_path.swap(newPath);
	}
}


void NpcSession::ChaseTarget()
{
	if (m_bActive == false) return;
	if(!CheckTarget()) return;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	chrono::system_clock::time_point currTime = chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_makePathTime);
	if (m_path.empty() == true || duration > 3'000ms)
	{
		m_makePathTime = chrono::system_clock::now();
		CreatePath();
	}

	Position nextPos;
	{
		lock_guard<mutex> lock(m_pathLock);
		if (m_path.empty()) return;

		nextPos = m_path.top();
		m_path.pop();
		if (nextPos == m_pos && !m_path.empty())
		{
			nextPos = m_path.top();
			m_path.pop();
		}
	}

	if (gameManager->CanGo(nextPos) == true)
		SetPos(nextPos);
	UpdateViewList();
}

void NpcSession::Roaming()
{
	if (m_bActive == false) return;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	std::uniform_int_distribution<int> dir(0, 3);
	int direction = dir(rng);
	Position currPos = m_pos;
	Position nextPos = currPos + movements[direction];
	if (gameManager->CanGo(nextPos))
		SetPos(nextPos);
	UpdateViewList();
}

void NpcSession::DeActiveNpc()
{
	SetActive(false);
	ReleaseTarget();
	m_viewListLock.lock();
	auto LastViewList = m_viewList;
	m_viewList.clear();
	m_viewListLock.unlock();

	for (int id : LastViewList)
	{
		auto player = Manager::GetInstance().GetGameManager()->GetPlayerSession(id);
		if (player == nullptr) continue;
		if (player->IsInGame() == false) continue;
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(id);
		player->RemoveViewNPCList(m_objectID);
		pNetwork->send_remove_npc_object_packet(this);
	}
}

void NpcSession::Die()
{
	DeActiveNpc();
	g_Timer.AddTimer(m_objectID + MAX_USER, chrono::system_clock::now() + 10s, TIMER_TYPE::RespawnObject);
}

void NpcSession::RespawnObject()
{
	SetPos(m_spawnPos);
	m_currentState = Monster::State::Idle;
	Creature::RespawnObject();
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
}
