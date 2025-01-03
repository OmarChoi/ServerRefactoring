#include "stdafx.h"
#include "GameObject.h"
#include "DataBase.h"
#include "Timer.h"

extern int g_MapData[W_WIDTH][W_HEIGHT];
extern array<Player*, MAX_USER> Players;
extern array<NPC*, MAX_NPC> NPCs;
extern DataBase g_DataBase;
extern Timer g_Timer;

void DisconnectClient(int objID)
{
	Players[objID]->m_stateLock.lock();
	Players[objID]->m_playerState = CT_FREE;
	Players[objID]->m_stateLock.unlock();

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (Players[i] == nullptr) continue;
		if (Players[i]->m_playerState != CT_INGAME) continue;
		Players[i]->m_viewListLock.lock();
		if (Players[i]->m_viewList.count(i) != 0)
			Players[i]->m_viewList.erase(i);
		Players[i]->m_viewListLock.unlock();
	}

	for (int i = 0; i < MAX_NPC; ++i)
	{
		NPCs[i]->m_viewListLock.lock();
		if (NPCs[i]->m_viewList.count(i) != 0)
			NPCs[i]->m_viewList.erase(i);
		NPCs[i]->m_viewListLock.unlock();
	}
	Players[objID]->m_viewListLock.lock();
	{
		Players[objID]->m_viewList.clear();
	}
	Players[objID]->m_viewListLock.unlock();
	Players[objID]->m_viewNpcListLock.lock();
	{
		Players[objID]->m_viewNpcList.clear();
	}
	Players[objID]->m_viewNpcListLock.unlock();

	delete Players[objID];
}

GameObject::GameObject()
{
	m_hp = 200;
	m_maxHp = 200;
}

GameObject::~GameObject()
{
}

bool GameObject::CanGo(int x, int y)
{
	if (x < 0 || x >= W_WIDTH) return false;
	if (y < 0 || y >= W_HEIGHT) return false;
	return g_MapData[x][y] != 0;
}

void GameObject::AddViewList(int objID)
{
	m_viewListLock.lock();
	m_viewList.emplace(objID);
	m_viewListLock.unlock();
}

void GameObject::RemoveViewList(int objID)
{
	m_viewListLock.lock();
	if(m_viewList.find(objID) != m_viewList.end())
		m_viewList.erase(objID);
	m_viewListLock.unlock();
}

bool GameObject::canSee(const GameObject* otherPlayer)
{
	if (abs(otherPlayer->m_xPos - m_xPos) > VIEW_RANGE) return false;
	return abs(otherPlayer->m_yPos - m_yPos) < VIEW_RANGE;
}

bool GameObject::CanHit(int objID, OBJ_TYPE ot)
{
	switch (ot)
	{
	case OT_PLAYER:
		return abs(m_xPos - Players[objID]->m_xPos) + abs(m_yPos - Players[objID]->m_yPos) <= 1;
		break;
	case OT_NPC:
		return abs(m_xPos - NPCs[objID]->m_xPos) + abs(m_yPos - NPCs[objID]->m_yPos) <= 1;
		break;
	default:
		return false;
	}

}

///////////////////////////////////////////////////////////////////////////////
Player::Player(int objID)
{
	m_objectID = objID;

	m_playerState = CT_FREE;
	m_socket = 0;
	m_xPos = m_yPos = 0;
	m_prevRemain = 0;
	m_lastActionTime = 0;
	strncpy_s(m_name, "NoName", 6);

	m_type = OT_PLAYER;
}

void Player::callRecv()
{
	DWORD recvFlag = 0;
	memset(&m_recvOver.m_over, 0, sizeof(m_recvOver.m_over));
	m_recvOver.m_wsabuf.len = BUF_SIZE - m_prevRemain;
	m_recvOver.m_wsabuf.buf = m_recvOver.m_sendBuf + m_prevRemain;
	WSARecv(m_socket, &m_recvOver.m_wsabuf, 1, 0, &recvFlag, &m_recvOver.m_over, 0);
}

void Player::callSend(void* packet)
{
	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sdata->m_wsabuf, 1, 0, 0, &sdata->m_over, 0);
}

void Player::UpdateViewList()
{
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	auto prevNpcViewList = m_viewNpcList;
	m_viewListLock.unlock();

	unordered_set<int> newViewList;
	unordered_set<int> newNpcViewList;

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (Players[i] == nullptr) continue;
		if (i == m_objectID) continue;
		if (Players[i]->m_playerState != CT_INGAME) continue;
		if (canSee(Players[i]))	// 현재 내 시야에서 보이면
		{
			if (prevViewList.count(Players[i]->m_objectID) == 0) // 이전에는 없었으면 추가
			{
				Players[i]->AddViewList(m_objectID);
				Players[i]->send_add_object_packet(m_objectID);
				send_add_object_packet(i);
			}
			else
				Players[i]->send_move_object_packet(m_objectID);
			newViewList.emplace(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevViewList.count(Players[i]->m_objectID) != 0)	// 이전에는 있었으면 삭제
			{
				Players[i]->RemoveViewList(m_objectID);
				Players[i]->send_remove_object_packet(m_objectID);
				send_remove_object_packet(i);
			}
		}
	}

	for (int i = 0; i < MAX_NPC; ++i)
	{
		if (NPCs[i] == nullptr) break;
		if (NPCs[i]->m_isDead == true) continue;
		if (canSee(NPCs[i]))	// 현재 내 시야에서 보이면
		{
			if (NPCs[i]->m_monserType == MT_ROAMING)
				NPCs[i]->WakeUp(m_objectID);
			if (prevNpcViewList.count(i) == 0) // 이전에는 없었으면 추가
			{
				NPCs[i]->AddViewList(m_objectID);
				send_add_npc_packet(i);
			}
			send_npc_move_object_packet(i);
			newNpcViewList.emplace(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevNpcViewList.count(i) != 0)	// 이전에는 있었으면 삭제
			{
				NPCs[i]->RemoveViewList(m_objectID);
				NPCs[i]->ReleaseTarget(m_objectID);
				send_remove_object_packet(i + MAX_USER);
			}
		}
	}

	m_viewListLock.lock();
	m_viewList = newViewList;
	m_viewNpcList = newNpcViewList;
	m_viewListLock.unlock();
}

void Player::AddViewNPCList(int objID)
{
	m_viewNpcListLock.lock();
	m_viewNpcList.insert(objID);
	m_viewNpcListLock.unlock();
}

void Player::RemoveViewNPCList(int objID)
{
	m_viewNpcListLock.lock();
	if (m_viewNpcList.find(objID) != m_viewNpcList.end())
		m_viewNpcList.erase(objID);
	m_viewNpcListLock.unlock();
}

void Player::ProcessPacket(char* packet)
{
	switch (packet[2]) {
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(m_name, p->name);
		m_stateLock.lock();
		m_playerState = CT_INGAME;
		m_stateLock.unlock();

#ifndef stressTest
		MultiByteToWideChar(CP_UTF8, 0, m_name, -1, m_nameForDB, NAME_SIZE);

		if (g_DataBase.GetUserData(m_nameForDB, m_objectID))
		{
			send_login_ok_packet();
			send_login_info_packet();
			UpdateViewList();
		}
		else
		{
			send_login_fail_packet();
			closesocket(m_socket);
			//로그인 실패 패킷
		}
#else
		m_xPos = rand() % W_WIDTH;
		m_yPos = rand() % W_HEIGHT;
		send_login_info_packet();
		UpdateViewList();
#endif // stressTest
		break;
	}
	case CS_MOVE:
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		auto moveTime = std::chrono::system_clock::now();
		auto duration = moveTime - m_tpLastMoveTime;
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		if (durationMs < 1000ms)
		{
#ifdef stressTest
			m_lastMoveTime = p->move_time;
			m_viewListLock.lock();
			auto viewList = m_viewList;
			m_viewListLock.unlock();
			send_move_object_packet(m_objectID);
			for (auto it = viewList.begin(); it != viewList.end(); ++it)
			{
				Players[*it]->send_move_object_packet(m_objectID);
			}
#endif // stressTest
			return;
		}
		else
			m_tpLastMoveTime = moveTime;

		m_lastMoveTime = p->move_time;
		int xTrans[4] = { 0, 0, -1, 1 };
		int yTrans[4] = { -1, 1, 0, 0 };
		int tempXPos = m_xPos + xTrans[p->direction];
		int tempYPos = m_yPos + yTrans[p->direction];

		// tempXPos, tempyPos가 갈 수 있는 위치인지 확인하는 함수
		if (CanGo(tempXPos, tempYPos))
		{
			m_xPos = tempXPos;
			m_yPos = tempYPos;
		}
		UpdateViewList();
		send_move_object_packet(m_objectID);

		break;
	}
	case CS_CHAT:
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);

		m_viewListLock.lock();
		auto viewList = m_viewList;
		m_viewListLock.unlock();

		// 해당 플레이어를 보고 있는 모든 유저에게 채팅 전송
		send_chat_packet(m_objectID, p->mess, CHATTING_MESSAGE);
		for (auto it : viewList)
		{
			if (it == m_objectID) continue;
			Players[it]->send_chat_packet(m_objectID, p->mess, CHATTING_MESSAGE);
		}

		break;
	}
	case CS_ATTACK:
	{
		CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
		auto attackTime = std::chrono::system_clock::now();
		auto duration = attackTime - m_tpLastAttackTime;
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		if (durationMs < 1000ms) return;
		else
			m_tpLastAttackTime = attackTime;

		m_viewListLock.lock();
		auto viewList = m_viewNpcList;
		m_viewListLock.unlock();

		// 해당 플레이어를 보고 있는 모든 유저에게 채팅 전송
		for (auto it : viewList)
		{
			if (CanHit(NPCs[it]->m_objectID, OT_NPC))
			{
				NPCs[it]->GetHit(m_objectID);
			}
		}
		break;
	}
	case CS_TELEPORT:
	{
		// 갈 수 있는 위치가 나올 때까지 xPos, yPos 배치
		CS_TELEPORT_PACKET* p = reinterpret_cast<CS_TELEPORT_PACKET*>(packet);
		auto moveTime = std::chrono::system_clock::now();
		auto duration = moveTime - m_tpLastMoveTime;
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		if (durationMs < 1000ms)
		{
			return;
		}
		else
			m_tpLastMoveTime = moveTime;

		while (true)
		{
			int tempXPos = rand() % W_WIDTH;
			int tempYPos = rand() % W_HEIGHT;
			// 갈 수 있는 위치인지 체크
			if (CanGo(tempXPos, tempYPos))
			{
				m_xPos = tempXPos;
				m_yPos = tempYPos;
				break;
			}
		}

		// 변경 후 View List 업데이트
		UpdateViewList();
		send_move_object_packet(m_objectID);
		break;
	}
	case CS_LOGOUT:
	{
		// DB에 클라이언트 INFO 저장
		// 클라이언트를 보고 있는 모든 Player ViewList에서 삭제 & SC_REMOVE_OBJECT_PACKET 전송
		// Players 상태 FREE로 변경
		m_viewListLock.lock();
		auto viewList = m_viewList;
		m_viewListLock.unlock();

		send_remove_object_packet(m_objectID);
		for (auto i : viewList)
		{
			if (i == m_objectID) continue;
			Players[i]->send_remove_object_packet(m_objectID);
		}

#ifndef stressTest
		g_DataBase.UpdatePlayerData(m_nameForDB, m_objectID);
#endif
		DisconnectClient(m_objectID);

		break;
	}
	default:
		cout << "process something\n";
	}
}

void Player::send_login_info_packet()
{
	SC_LOGIN_INFO_PACKET sendPacket;
	sendPacket.size = sizeof(SC_LOGIN_INFO_PACKET);
	sendPacket.type = SC_LOGIN_INFO;
	sendPacket.id = m_objectID;
	sendPacket.hp = m_hp;
	sendPacket.exp = m_exp;
	sendPacket.level = m_level;
	sendPacket.x = m_xPos;
	sendPacket.y = m_yPos;
	callSend(&sendPacket);
}

void Player::send_add_object_packet(int objId)
{
	SC_ADD_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendPacket.type = SC_ADD_OBJECT;
	sendPacket.id = objId;
	sendPacket.x = Players[objId]->m_xPos;
	sendPacket.y = Players[objId]->m_yPos;
	strncpy_s(sendPacket.name, Players[objId]->m_name, NAME_SIZE);
	callSend(&sendPacket);
}

void Player::send_add_npc_packet(int objId)
{
	SC_ADD_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendPacket.type = SC_ADD_OBJECT;
	sendPacket.id = objId + MAX_USER;
	sendPacket.x = NPCs[objId]->m_xPos;
	sendPacket.y = NPCs[objId]->m_yPos;
	strncpy_s(sendPacket.name, NPCs[objId]->m_name, NAME_SIZE);
	callSend(&sendPacket);
}

void Player::send_remove_object_packet(int objId)
{
	SC_REMOVE_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	sendPacket.type = SC_REMOVE_OBJECT;
	sendPacket.id = objId;
	callSend(&sendPacket);
}

void Player::send_move_object_packet(int objId)
{
	SC_MOVE_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendPacket.type = SC_MOVE_OBJECT;
	sendPacket.id = objId;
	sendPacket.x = Players[objId]->m_xPos;
	sendPacket.y = Players[objId]->m_yPos;
	sendPacket.move_time = m_lastMoveTime;
	callSend(&sendPacket);
}

void Player::send_npc_move_object_packet(int objId)
{
	SC_MOVE_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendPacket.type = SC_MOVE_OBJECT;
	sendPacket.id = objId + MAX_USER;
	sendPacket.x = NPCs[objId]->m_xPos;
	sendPacket.y = NPCs[objId]->m_yPos;
	sendPacket.move_time = m_lastMoveTime;
	callSend(&sendPacket);
}

void Player::send_chat_packet(int objId, const char* mess, char chatType)
{
	SC_CHAT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_CHAT_PACKET);
	sendPacket.type = SC_CHAT;
	sendPacket.chatType = chatType;
	sendPacket.id = objId;
	strncpy_s(sendPacket.mess, mess, sizeof(mess));
	callSend(&sendPacket);
}

void Player::send_login_ok_packet()
{
	SC_LOGIN_OK_PACKET sendPacket;
	sendPacket.size = sizeof(SC_LOGIN_OK_PACKET);
	sendPacket.type = SC_LOGIN_OK;
	callSend(&sendPacket);
}

void Player::send_login_fail_packet()
{
	SC_LOGIN_FAIL_PACKET sendPacket;
	sendPacket.size = sizeof(SC_LOGIN_FAIL_PACKET);
	sendPacket.type = SC_LOGIN_FAIL;
	callSend(&sendPacket);
}

void Player::send_stat_change_packet()
{
	SC_STAT_CHANGE_PACKET sendPacket;
	sendPacket.size = sizeof(SC_STAT_CHANGE_PACKET);
	sendPacket.type = SC_STAT_CHANGE;
	sendPacket.hp = m_hp;
	sendPacket.max_hp = m_maxHp;
	sendPacket.exp = m_exp;
	sendPacket.level = m_level;
	callSend(&sendPacket);
}

///////////////////////////////////////////////////////////////////////////////
NPC::NPC()
{
	static int npcCount = 0;
	m_type = OT_NPC;
	m_isActive = false;
	m_targetID = -1;
	m_changeTime = chrono::system_clock::now();
}

NPC::~NPC()
{
}

void NPC::UpdateViewList()
{
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	m_viewListLock.unlock();

	unordered_set<int> newViewList;
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (Players[i] == nullptr) break;
		if (Players[i]->m_playerState != CT_INGAME) continue;
		if (canSee(Players[i]))	// 현재 내 시야에서 보이면
		{
			if (prevViewList.count(Players[i]->m_objectID) == 0) // 이전에는 없었으면 추가
			{
				Players[i]->AddViewNPCList(m_objectID);
				Players[i]->send_add_npc_packet(m_objectID);
			}
			else
			{
				Players[i]->send_npc_move_object_packet(m_objectID);
			}
			newViewList.insert(i);
		}
		else	// 내 시야에서 없으면
		{
			if (prevViewList.count(Players[i]->m_objectID) != 0)	// 이전에는 있었으면 삭제
			{
				Players[i]->RemoveViewNPCList(m_objectID);
				Players[i]->send_remove_object_packet(m_objectID + MAX_USER);
			}
		}
	}

	if (newViewList.empty())
		ReleaseTarget(-1);

	m_viewListLock.lock();
	m_viewList = newViewList;
	m_viewListLock.unlock();
}

void NPC::WakeUp(int objID)
{
	m_targetLock.lock();
	if (m_targetID == -1)
	{
		m_targetID = objID;
		m_targetLock.unlock();

		m_activeLock.lock();
		m_isActive = true;
		m_activeLock.unlock();
		
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
	}
	else
		m_targetLock.unlock();
}

void NPC::ReleaseTarget(int objID)
{
	if (objID != -1)
	{
		if (m_targetID != objID)
		{
			return;
		}
	}
	m_targetLock.lock();
	m_targetID = -1;
	m_targetLock.unlock();

	m_activeLock.lock();
	m_isActive = false;
	m_activeLock.unlock();
}

int MaxLength(pair<int, int> current, pair<int, int> destination) {
	int x1 = current.first;
	int y1 = current.second;
	int x2 = destination.first;
	int y2 = destination.second;
	// 가로 거리 + 세로거리
	return abs(x2 - x1) + abs(y2 - y1);
}

void NPC::TrackingPath(pair<int, int> start)
{
	int rows = W_WIDTH;
	int cols = W_HEIGHT;

	// 상하좌우 정의
	vector<pair<int, int>> movements = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

	priority_queue<pair<double, pair<int, int>>, vector<pair<double, pair<int, int>>>, greater<pair<double, pair<int, int>>>> open_set;
	vector<vector<bool>> closed_set(rows, vector<bool>(cols, false));

	vector<vector<double>> g_score(rows, vector<double>(cols, INT_MAX));	// 누적 비용	// INT_MAX로 cols, rows 모두 초기화
	vector<vector<double>> f_score(rows, vector<double>(cols, INT_MAX));	// 예상되는 총 비용 // 누적 비용 + 남은 거리
	vector<vector<pair<int, int>>> came_from(rows, vector<pair<int, int>>(cols, { -1, -1 }));

	g_score[start.first][start.second] = 0;									// 시작점 0
	f_score[start.first][start.second] = MaxLength(start, m_targetPos);		// 시작점 MaxLength

	open_set.push({ f_score[start.first][start.second], start });	// 시작점 남은 최대 비용, 시작 지점

	while (!open_set.empty()) {
		pair<double, pair<int, int>> current = open_set.top();
		open_set.pop();
		double current_f_score = current.first;
		pair<int, int> current_point = current.second;

		if (current_point == m_targetPos) {	// 목적지에 도달했으면
			while (current_point != start) {
				m_path.push(current_point);
				current_point = came_from[current_point.first][current_point.second];
			}
			return;
		}

		closed_set[current_point.first][current_point.second] = true;	// 지나간 길이라고 true표시

		for (const auto& movement : movements) {	// 4방향 모두 경로 테스트
			int neighbor_x = current_point.first + movement.first;
			int neighbor_y = current_point.second + movement.second;

			// 갈 수 있는 길인지 확인하고 갈 수 있으면 해당 칸 점수 +
			if (neighbor_x >= 0 && neighbor_x < rows && neighbor_y >= 0 && neighbor_y < cols && g_MapData[neighbor_x][neighbor_y] != 0) {
				pair<int, int> neighbor = { neighbor_x, neighbor_y };
				double tentative_g_score = g_score[current_point.first][current_point.second] + 1;

				if (tentative_g_score < g_score[neighbor.first][neighbor.second]) {
					g_score[neighbor.first][neighbor.second] = tentative_g_score;
					f_score[neighbor.first][neighbor.second] = tentative_g_score + MaxLength(neighbor, m_targetPos);
					open_set.push({ f_score[neighbor.first][neighbor.second], neighbor });
					came_from[neighbor.first][neighbor.second] = current_point;
				}
			}
		}
	}
}

void NPC::SimplyMove()
{
	int tempX = m_xPos;
	int tempY = m_yPos;
	m_targetLock.lock();
	int tID = m_targetID;
	m_targetLock.unlock();
	if (tID == -1) return;

	if ((Players[tID]->m_xPos - m_xPos) > 0) tempX += 1;
	if ((Players[tID]->m_xPos - m_xPos) < 0) tempX -= 1;
	if ((Players[tID]->m_yPos - m_yPos) > 0) tempY += 1;
	if ((Players[tID]->m_yPos - m_yPos) < 0) tempY -= 1;

	if (tempX != m_xPos)
	{
		if (CanGo(tempX, m_yPos))
		{
			m_xPos = tempX;
			return;
		}
	}
	if (tempY != m_yPos)
		if (CanGo(m_xPos, tempY))
			m_yPos = tempY;
}

void NPC::Move()
{
	//if (m_targetID == -1)
	//{
	//	// 랜덤 Move
	//	int tempX = m_xPos;
	//	int tempY = m_yPos;
	//	switch (rand() % 4)
	//	{
	//	case 0:
	//		tempY -= 1;
	//		break;
	//	case 1:
	//		tempX += 1;
	//		break;
	//	case 2:
	//		tempY += 1;
	//		break;
	//	case 3:
	//		tempX -= 1;
	//		break;
	//	}
	//	if (CanGo(tempX, tempY))
	//	{
	//		m_xPos = tempX;
	//		m_yPos = tempY;
	//	}
	//	UpdateViewList();
	//	auto viewList = m_viewList;
	//	for (auto it = viewList.begin(); it != viewList.end(); ++it)
	//		Players[*it]->send_npc_move_object_packet(m_objectID);
	//	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
	//}
	//else
	//{
		// Target향해서 움직이기
	if (m_targetID == -1)
		return;
	//if (m_monserType == MT_STAYING)
	//{
	//	TIMER_TYPE tempVal = TT_MOVE;
	//	SimplyMove();
	//	pair<int, int> targetPos = pair<int, int>(Players[m_targetID]->m_xPos, Players[m_targetID]->m_yPos);
	//	pair<int, int> currentPos = pair<int, int>(m_xPos, m_yPos);
	//	if (GetDistance(targetPos, currentPos) == 0)
	//		tempVal = TT_ATTACK;
	//	UpdateViewList();
	//	g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, tempVal);
	//}
	//else
	//{
		pair<int, int> targetPos = pair<int, int>(Players[m_targetID]->m_xPos, Players[m_targetID]->m_yPos);
		pair<int, int> currentPos = pair<int, int>(m_xPos, m_yPos);
		pair<int, int> nextPos;
		chrono::system_clock::time_point currentTime = chrono::system_clock::now();
		if (currentTime - m_changeTime > 5s || currentPos == m_targetPos
			|| MaxLength(targetPos, currentPos) < MaxLength(m_targetPos,currentPos))
		{
			m_changeTime = chrono::system_clock::now();
			m_targetPos = targetPos;
			while (!m_path.empty()) 
				m_path.pop();
			TrackingPath(currentPos);
		}

		if (m_path.empty())
		{
			nextPos = currentPos;
		}
		else
		{
			nextPos = m_path.top();
			m_path.pop();
		}


		if (MaxLength(targetPos, nextPos) == 1)
		{
			g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_ATTACK);
		}
		if (currentPos == nextPos)
		{
			g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
		}
		else
		{
			m_xPos = nextPos.first;
			m_yPos = nextPos.second;

			UpdateViewList();
			auto viewList = m_viewList;

			for (auto it = viewList.begin(); it != viewList.end(); ++it)
				Players[*it]->send_npc_move_object_packet(m_objectID);

			g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
		}
		//}
	//}
}

bool NPC::GetAgro(int x, int y)
{
	if (abs(x - m_xPos) > VIEW_RANGE) return false;
	return abs(y - m_yPos) < VIEW_RANGE;
}

void NPC::Attack()
{
	auto* p = Players[m_targetID];
	if (CanHit(m_targetID, OT_PLAYER) == true)
	{
		p->m_hp -= 10;
		p->send_stat_change_packet();
		p->send_chat_packet(p->m_objectID, "-10", BATTLE_MESSAGE);
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_ATTACK);
	}
	else
	{
		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 1s, TT_MOVE);
	}
}

void NPC::GetHit(int objID)
{
	Player* p = Players[objID];
	if (m_targetID == -1 && m_monserType == MT_STAYING)
	{
		WakeUp(objID);
	}
	m_hp -= 50;
	p->send_chat_packet(m_objectID + MAX_USER, "-50", BATTLE_MESSAGE);
	if (m_hp < FLT_EPSILON)
	{
		m_xPos = m_spawnXPos;
		m_yPos = m_spawnYPos;
		m_targetID = -1;
		m_isActive = false;
		m_isDead = true;
		m_hp = m_maxHp;
		m_viewListLock.lock();
		unordered_set<int> tempViewList = m_viewList;
		m_viewListLock.unlock();

		for (int i : tempViewList)
		{
			Players[i]->RemoveViewList(m_objectID);
			Players[i]->send_remove_object_packet(m_objectID + MAX_USER);
		}

		m_viewListLock.lock();
		m_viewList.clear();
		m_viewListLock.unlock();

		g_Timer.AddTimer(m_objectID, chrono::system_clock::now() + 3s, TT_RESPAWN);

		// 플레이어 경험치 추가
		// 플레이어 경험치가 일정 이상이면 레벨업
		// stat_change_packet
		p->m_exp += 100;
		if (p->m_exp >= 100 * pow(2, p->m_level - 1))
		{
			p->m_exp = 0;
			p->m_level += 1;
		}
		p->send_stat_change_packet();
	}
}