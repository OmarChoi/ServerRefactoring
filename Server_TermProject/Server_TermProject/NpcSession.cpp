#include "stdafx.h"
#include "Timer.h"
#include "Manager.h";
#include "NpcSession.h"
#include "GameManager.h";
#include "PlayerSession.h";
#include "NetworkManager.h";
#include "PlayerSocketHandler.h";

extern Timer g_Timer;

int GetDist(Position startPos, Position destPos)
{
	// 단순히 가로 거리 + 세로거리
	return abs(destPos.yPos - startPos.yPos) + abs(destPos.xPos - startPos.xPos);
}

struct AStarNode
{
	Position	currPos;
	int			totalCost;			// 해당 좌표까지 누적 값 + 목표까지 남은 값
	int			cumulativeCost;		// 해당 좌표까지 누적 값
	AStarNode(Position pos, int t, int c) : currPos(pos), totalCost(c), cumulativeCost(c) {};
	bool operator > (const AStarNode& other) const { return totalCost > other.totalCost; }

};

void NpcSession::UpdateViewList()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	m_viewListLock.unlock();

	unordered_set<int> newViewList;
	for (int i = 0; i < MAX_USER; ++i)
	{
		PlayerSession* player = gameManager->GetPlayerSession(i);
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(i);
		if (player == nullptr) continue;
		if (player->GetState() != C_STATE::CT_INGAME) continue;
		if (CanSee(player))	// 현재 내 시야에서 보이면
		{
			if (prevViewList.count(player->GetId()) == 0) // 이전에는 없었으면 추가
			{
				player->AddViewNPCList(m_objectID);
				SetTarget(player->GetId());
				pNetwork->send_add_npc_packet(this);
			}
			else
			{ 
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

	if (newViewList.empty())
		ReleaseTarget(-1);

	m_viewListLock.lock();
	m_viewList = newViewList;
	m_viewListLock.unlock();
}

void NpcSession::SetTarget(int objId)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (m_targetID == -1) 
	{
		m_targetID = objId;
		if(objId != -1)
			g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
	}
}

void NpcSession::InitPosition(Position pos)
{
	m_spawnPos = pos;
	SetPos(pos);
}

void NpcSession::Attack()
{
	if (m_targetID == -1)
	{
		m_state = STATE::IDLE;
		return;
	}
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	// gameManager->GetPlayerSession(m_targetID)->GetDamage(m_damage);
	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_ATTACK);
}

void SimpleNpc::Move()
{
	if (m_targetID == -1)
	{
		m_state = STATE::IDLE;
		return;
	}
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	PlayerSession* targetPlayer = gameManager->GetPlayerSession(m_targetID);
	Position currPos = m_pos;
	Position targetPos = targetPlayer->GetPos();

	if (GetDist(currPos, targetPos) <= m_attackRange)
	{
		while (!m_path.empty())
			m_path.pop();

		m_state = STATE::ATTACK;
		return;
	}

	if (m_path.empty())
		return;

	if (m_path.back() != targetPos)
		m_path.emplace(targetPos);

	Position nextGoal = m_path.front();
	m_path.pop();

	int tempY = currPos.yPos;
	int tempX = currPos.xPos;

	if (currPos.yPos < nextGoal.yPos) { tempY += 1; }
	else if (currPos.yPos > nextGoal.yPos) { tempY -= 1; }
	else if (currPos.xPos < nextGoal.xPos) { tempX += 1; }
	else if (currPos.xPos > nextGoal.xPos) { tempX -= 1; }

	if (gameManager->CanGo(tempY, tempX))
		SetPos(tempY, tempX);

	// g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
}

void RoamingMonster::Move()
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	if (m_targetID != -1)
	{
		// 타겟이 있을 떄
		// - m_path에 존재하는 경로를 한 칸씩 이동
		chrono::system_clock::time_point currTime = chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_MakePathTime);
		if (m_path.empty() == true || duration > 5'000ms)
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
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
	}
	// else
	// {
	// 	// 타겟이 없을 때
	// 	// - 랜덤한 방향으로 이동
	// 	while (true)
	// 	{
	// 		std::uniform_int_distribution<int> dir(0, 3);
	// 		Position nextPos = m_pos + movements[dir(Manager::GetInstance().m_Gen)];
	// 		if (gameManager->CanGo(nextPos) == true)
	// 		{
	// 			SetPos(nextPos);
	// 			break;
	// 		}
	// 	}
	// }
	// 마지막 이동시간 기록 및 다음 이동을 위한 명령을 타이머에 추가
}

void RoamingMonster::CreatePath()
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

	nextPath.emplace(m_pos, GetDist(m_pos, targetPos), 0);
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
			int heuristic = GetDist(nextPos, targetPos);

			if (best[nextPos.yPos][nextPos.xPos] <= cCost + heuristic)
				continue;


			best[nextPos.yPos][nextPos.xPos] = cCost + heuristic;
			nextPath.emplace(nextPos, best[nextPos.yPos][nextPos.xPos], cCost);
			parent[nextPos.yPos][nextPos.xPos] = currPos;
		}
	}

	while (!m_path.empty())
		m_path.pop();

	if (gameManager->CanGo(targetPos.yPos, targetPos.xPos) == false) {
		cout << target->GetId() << " : (" << targetPos.yPos << ", " << targetPos.xPos << ")\n";
	}
	m_path.emplace(targetPos);
	while (true)
	{
		Position pos = m_path.top();
		if (pos == parent[pos.yPos][pos.xPos])
			break;

		m_path.emplace(parent[pos.yPos][pos.xPos]);
	}
}
