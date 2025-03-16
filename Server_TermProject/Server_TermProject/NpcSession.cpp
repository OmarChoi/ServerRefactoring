#include "stdafx.h"
#include "Timer.h"
#include "AgroNpc.h"
#include "Manager.h"
#include "NormalNpc.h"
#include "NpcSession.h"
#include "MapSession.h";
#include "GameManager.h";
#include "PlayerSession.h"
#include "NetworkManager.h"
#include "PlayerSocketHandler.h"

extern Timer g_Timer;

struct AStarNode
{
	Position	currPos;
	int			totalCost;			// 해당 좌표까지 누적 값 + 목표까지 남은 값
	int			cumulativeCost;		// 해당 좌표까지 누적 값
	AStarNode(Position pos, int t, int c) : currPos(pos), totalCost(c), cumulativeCost(c) {};
	bool operator > (const AStarNode& other) const { return totalCost > other.totalCost; }

};

NpcSession::NpcSession() : Creature()
{
	InitLua();
}

NpcSession::~NpcSession()
{
	while (!m_path.empty())
		m_path.pop();
}

void NpcSession::AddViewList(int objID)
{
	if (m_hp < FLT_EPSILON) return;
	if (IsActive() == false)
		ActiveNpc();
	std::lock_guard<std::mutex> lock(m_viewListLock);
	m_viewList.emplace(objID);
}

void NpcSession::RemoveViewList(int objID)
{
	std::lock_guard<std::mutex> lock(m_viewListLock);
	if (m_viewList.find(objID) != m_viewList.end())
		m_viewList.erase(objID);
}

void NpcSession::SetPos(int y, int x)
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	Creature::SetPos(y, x);
	Position nextPos{ y, x };
	gameManager->GetMapSession()->ChangeSection(0, m_objectID, m_pos, nextPos);
}

void NpcSession::NpcSession::UpdateViewList()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	m_viewListLock.unlock();

	bool targetValid = false;
	unordered_set<int> newViewList;
	for (int i = 0; i < MAX_USER; ++i)
	{
		PlayerSession* player = gameManager->GetPlayerSession(i);
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(i);
		if (player == nullptr) continue;
		if (player->GetState() != PlayerState::CT_INGAME) continue;
		if (CanSee(player))	// 현재 내 시야에서 보이면
		{
			if (prevViewList.count(player->GetId()) == 0) // 이전에는 없었으면 추가
			{
				player->AddViewNPCList(m_objectID);
				pNetwork->send_add_npc_packet(this);
			}
			else
			{
				if (i == m_targetID)
					targetValid = true;
				pNetwork->send_npc_move_object_packet(this);
			}
			newViewList.insert(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevViewList.count(player->GetId()) != 0)	// 이전에는 있었으면 삭제
			{
				player->RemoveViewNPCList(m_objectID);
				pNetwork->send_remove_npc_object_packet(this);
			}
		}
	}

	if (targetValid == false)
		ReleaseTarget();

	if (newViewList.size() == 0)
	{
		DeActiveNpc();
		return;
	}
	m_viewListLock.lock();
	m_viewList = newViewList;
	m_viewListLock.unlock();
}

bool NpcSession::CanSee(const Creature* other)
{
	if (abs(other->GetPos().xPos - m_pos.xPos) > NPC_VIEW_RANGE) return false;
	return abs(other->GetPos().yPos - m_pos.yPos) < NPC_VIEW_RANGE;
}

void NpcSession::CheckTarget()
{
	if (m_targetID == -1) return;
	PlayerSession* player = Manager::GetInstance().GetGameManager()->GetPlayerSession(m_targetID);
	if (player->IsActive() == false || player->GetState() != PlayerState::CT_INGAME)
		ReleaseTarget();
}

void NpcSession::SetTarget(int objId)
{
	std::lock_guard<std::mutex> lock(mutex);
	m_targetID = objId;
}

void NpcSession::ActiveNpc()
{
	if (m_hp < FLT_EPSILON) return;
	if (IsActive() == true) return;
	SetActive(true);

	switch (m_behavior)
	{
	case MonsterBehavior::Normal:
		break;
	case MonsterBehavior::Agro:

		break;
	}
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
}

void NpcSession::InitPosition(Position pos)
{
	m_spawnPos = pos;
	m_lastMoveTime = chrono::high_resolution_clock::now();
	m_pos.yPos = pos.yPos; m_pos.xPos = pos.xPos;
}

void NpcSession::ReleaseTarget()
{
	SetTarget(-1);
	while (!m_path.empty())
		m_path.pop();
}

void NpcSession::Respawn()
{
}

void NpcSession::InitLua()
{
	lua.open_libraries(sol::lib::base);
	lua.script_file("npc.lua");

	lua.set_function("Attack", &NpcSession::Attack, this);
	lua.set_function("ChaseTarget", &NpcSession::ChaseTarget, this);
	lua.set_function("MoveRandom", &NpcSession::MoveRandom, this);
	lua.set_function("DeActiveNpc", &NpcSession::DeActiveNpc, this);
}

void NpcSession::MoveInfo(NpcSession&& other)
{
	m_type = other.m_type;
	m_behavior = other.m_behavior;
	m_hp = other.m_hp;
	m_maxHp = other.m_maxHp;
	m_damage = other.m_damage;
	m_attackRange = other.m_attackRange;
	m_speed = other.m_speed;
	m_level.store(other.m_level);
}

void NpcSession::SetInfoByLua()
{
	sol::function getMonsterInfo = lua["GetMonsterInfo"];
	sol::table monsterInfo = getMonsterInfo(static_cast<int>(m_type));
	if (monsterInfo == sol::nil)
	{
		cerr << "NpcLuaManager_SetInfo : Monster Type Error";
		return;
	}
	m_type = static_cast<MonsterType>(monsterInfo["type"].get<int>());
	m_behavior = static_cast<MonsterBehavior>(monsterInfo["behavior"].get<int>());
	m_hp = monsterInfo["hp"].get_or(-1.0);
	m_maxHp = m_hp;
	m_damage = monsterInfo["damage"].get_or(-1);
	m_attackRange = monsterInfo["attackRange"].get_or(-1);
	m_speed = monsterInfo["speed"].get_or(-1.0f);
	m_level = monsterInfo["level"].get_or(-1);
	SetActive(false);
}

void NpcSession::Update()
{
	if (IsActive() == false) return;
	CheckTarget();
	bool hasTarget = (m_targetID == -1) ? false : true;
	int distance = NPC_VIEW_RANGE;
	if (hasTarget == true)
	{
		PlayerSession* player = Manager::GetInstance().GetGameManager()->GetPlayerSession(m_targetID);
		distance = Utils::GetDist(m_pos, player->GetPos());
	}

	sol::protected_function onUpdate = lua["OnUpdate"];
	onUpdate(hasTarget, distance, m_attackRange);
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
}

void NpcSession::Attack()
{
	PlayerSession* targetPlayer = Manager::GetInstance().GetGameManager()->GetPlayerSession(m_targetID);
	targetPlayer->ApplyDamage(m_damage);
	if (targetPlayer->GetHp() < FLT_EPSILON)
	 	ReleaseTarget();
}

void NpcSession::CreatePath()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	if (m_targetID == -1) return;

	PlayerSession* target = gameManager->GetPlayerSession(m_targetID);
	if (target == nullptr) {
		cout << "Error : Target Doesn't Exist\n";
		return;
	}
	Position targetPos = target->GetPos();

	vector<vector<bool>> closed(W_HEIGHT, vector<bool>(W_WIDTH, false));
	vector<vector<int>> best(W_HEIGHT, vector<int>(W_WIDTH, INT32_MAX));
	vector<vector<Position>> parent(W_HEIGHT, vector<Position>(W_WIDTH, { -1, -1 }));

	priority_queue<AStarNode, vector<AStarNode>, greater<AStarNode>> nextPath;

	nextPath.emplace(m_pos, Utils::GetDist(m_pos, targetPos), 0);
	parent[m_pos.yPos][m_pos.xPos] = m_pos;

	while (!nextPath.empty())
	{
		AStarNode currNode = nextPath.top();
		Position currPos = currNode.currPos;
		nextPath.pop();


		if (currPos == targetPos)
			break;

		if (closed[currPos.yPos][currPos.xPos])
			continue;
		closed[currPos.yPos][currPos.xPos] = true;

		for (int i = 0; i < 4; ++i)
		{
			Position nextPos = currPos + movements[i];
			if (gameManager->CanGo(nextPos) == false) continue;
			if (closed[nextPos.yPos][nextPos.xPos]) continue;

			int cCost = currNode.cumulativeCost + gameManager->GetTileCost(nextPos);
			int heuristic = Utils::GetDist(nextPos, targetPos);

			if (best[nextPos.yPos][nextPos.xPos] <= cCost + heuristic)
				continue;


			best[nextPos.yPos][nextPos.xPos] = cCost + heuristic;
			nextPath.emplace(nextPos, best[nextPos.yPos][nextPos.xPos], cCost);
			parent[nextPos.yPos][nextPos.xPos] = currPos;
		}
	}

	while (!m_path.empty())
		m_path.pop();

	m_path.emplace(targetPos);
	while (true)
	{
		Position pos = m_path.top();
		if (gameManager->CanGo(pos.yPos, pos.xPos) == false)
			break;
		if (pos == parent[pos.yPos][pos.xPos])
			break;

		m_path.emplace(parent[pos.yPos][pos.xPos]);
	}
}

void NpcSession::ChaseTarget()
{
	if (IsActive() == false) return;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	chrono::system_clock::time_point currTime = chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_MakePathTime);
	if (m_path.empty() == true || duration > 3'000ms)
	{
		m_MakePathTime = chrono::system_clock::now();
		CreatePath();
	}

	Position nextPos = m_path.top();
	m_path.pop();
	if (nextPos == m_pos && m_path.empty() == false)
	{
		nextPos = m_path.top();
		m_path.pop();
	}

	if (gameManager->CanGo(nextPos) == true)
		SetPos(nextPos);
	UpdateViewList();
}

void NpcSession::MoveRandom()
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
		PlayerSession* player = Manager::GetInstance().GetGameManager()->GetPlayerSession(id);
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(id);
		player->RemoveViewNPCList(m_objectID);
		pNetwork->send_remove_npc_object_packet(this);
	}
}

void NpcSession::Die()
{
	DeActiveNpc();
	g_Timer.AddTimer(m_objectID + MAX_USER, chrono::system_clock::now() + 5s, TIMER_TYPE::RespawnObject);
}

void NpcSession::RespawnObject()
{
	SetPos(m_spawnPos);
	Creature::RespawnObject();
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TIMER_TYPE::NpcUpdate);
}

//=============================================================================
NpcSession* NpcFactory::CreateNpc(MonsterType type)
{
	NpcSession tempNpc;
	tempNpc.SetType(type);
	tempNpc.SetInfoByLua();
	NpcSession* newNpc = nullptr;
	if (tempNpc.GetBehaviorType() == MonsterBehavior::Agro)
		newNpc = new AgroNpc();
	else
		newNpc = new NormalNpc();

	newNpc->MoveInfo(std::move(tempNpc));
	return newNpc;
}
