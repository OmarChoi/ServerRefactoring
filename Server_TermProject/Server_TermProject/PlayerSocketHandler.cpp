#include "stdafx.h"
#include "Manager.h"
#include "NpcSession.h"
#include "MapSession.h"
#include "GameManager.h"
#include "PlayerSession.h"
#include "DataBaseManager.h"
#include "PlayerSocketHandler.h"

PlayerSocketHandler::PlayerSocketHandler() : m_socket(0), m_remainPacket(0)
{
	ZeroMemory(&m_recvOver.m_over, sizeof(m_recvOver.m_over));
}

void PlayerSocketHandler::CallRecv()
{
	DWORD recvFlag = 0;
	memset(&m_recvOver.m_over, 0, sizeof(m_recvOver.m_over));
	m_recvOver.m_wsabuf.len = BUF_SIZE - m_remainPacket;
	m_recvOver.m_wsabuf.buf = m_recvOver.m_sendBuf + m_remainPacket;
	WSARecv(m_socket, &m_recvOver.m_wsabuf, 1, 0, &recvFlag, &m_recvOver.m_over, 0);
}

void PlayerSocketHandler::ProcessPacket(DWORD recvDataSize, OVER_EXP* over)
{
	int remainData = recvDataSize + m_remainPacket;
	char* p = over->m_sendBuf;
	while (remainData > 1)
	{
		unsigned short packetsize = *(reinterpret_cast<unsigned short*>(p));
		if (remainData >= packetsize)
		{
			ApplyPacketData(p);
			p = p + packetsize;
			remainData = remainData - packetsize;
		}
		else
			break;
	}

	m_remainPacket = remainData;
	if (remainData)
		memcpy(over->m_sendBuf, p, remainData);

	CallRecv();
}

void PlayerSocketHandler::ApplyPacketData(char* packet)
{
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	switch (packet[2]) {
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		VerifyUserAccount(p->name);
		break;
	}
	case CS_MOVE:
	{
		if (m_playerSession == nullptr) return;
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		Position nextPos = m_playerSession->GetPos() + movements[p->direction];
		m_moveTime = p->move_time;
		if (gameManager->CanGo(nextPos) && m_playerSession->IsActive())
		{
			m_playerSession->SetPos(nextPos);
			m_playerSession->UpdateViewList();
		}
		send_move_object_packet(m_playerSession);
		break;
	}
	case CS_CHAT:
	{
		break;
	}
	case CS_ATTACK:
	{
		if (m_playerSession == nullptr) return;
		if (m_playerSession->IsActive() == false) return;
		m_playerSession->Attack();
		break;
	}
	case CS_LOGOUT:
	{
		Disconnect();
		break;
	}
	default:
		cout << "process something.\n";
	}
}

void PlayerSocketHandler::ActivatePlayer()
{
	m_playerSession->SetObjId(m_playerID);
	m_playerSession->SetActive(true);
	m_playerSession->SetState(PlayerState::CT_INGAME);
	send_login_ok_packet();
	send_login_info_packet();
	m_playerSession->UpdateViewList();
}

void PlayerSocketHandler::VerifyUserAccount(const char* userName)
{
	Manager& manager = Manager::GetInstance();
	m_playerSession = manager.GetGameManager()->GetPlayerSession(m_playerID);
	WCHAR nameForDB[NAME_SIZE];
	char name[NAME_SIZE];
	
	if (strncmp(userName, "StressTest", 10) == 0)
	{
		m_playerSession->Init(this);
		m_playerSession->SetName(userName);
		m_playerSession->SetLevel(1);
		m_playerSession->SetExp(0);
		m_playerSession->SetHp(10'000);
		m_playerSession->SetMaxHp(10'000);
		m_playerSession->SetRandomPos();
		ActivatePlayer();
		manager.GetGameManager()->GetMapSession()->ChangeSection(ObjectType::Player, m_playerID, { -1, -1 }, m_playerSession->GetPos());
		return;
	}
	strncpy_s(name, userName, NAME_SIZE);
	MultiByteToWideChar(CP_UTF8, 0, userName, -1, nameForDB, NAME_SIZE);

	m_playerSession->SetName(userName);
	if (manager.GetDataBaseManager()->GetUserData(nameForDB, m_playerSession) == false)
	{
		ZeroMemory(m_playerSession, sizeof(PlayerSession));
		send_login_fail_packet();
		// m_playerSession Release
		return;
	}
	m_playerSession->Init(this);
	manager.GetGameManager()->GetMapSession()->ChangeSection(ObjectType::Player, m_playerID, { -1, -1 }, m_playerSession->GetPos());
	ActivatePlayer();
}

void PlayerSocketHandler::Disconnect()
{
	// 해당 클라이언트가 접속해있는 캐릭터 접속 종료
	// 소켓 연결 끊기
	// ObjectPool에 PlayerSocketHandler 반환
	m_playerSession->LogOut();
	m_playerSession = nullptr;
}

void PlayerSocketHandler::SendPacket(void* packet)
{
	// Pooling
	OVER_EXP* sendData = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sendData->m_wsabuf, 1, 0, 0, &sendData->m_over, 0);
}

void PlayerSocketHandler::send_login_info_packet()
{
	SC_LOGIN_INFO_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_INFO_PACKET);
	sendData.type = SC_LOGIN_INFO;
	sendData.id = m_playerSession->GetId();
	sendData.hp = static_cast<int>(m_playerSession->GetHp());
	sendData.maxHp = static_cast<int>(m_playerSession->GetMaxHp());
	sendData.exp = m_playerSession->GetExp();
	sendData.level = m_playerSession->GetLevel();
	sendData.x = m_playerSession->GetPos().xPos;
	sendData.y = m_playerSession->GetPos().yPos;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_add_object_packet(const PlayerSession* pPlayer)
{
	SC_ADD_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendData.type = SC_ADD_OBJECT;
	sendData.id = pPlayer->GetId();
	sendData.x = pPlayer->GetPos().xPos;
	sendData.y = pPlayer->GetPos().yPos;
	strncpy_s(sendData.name, sizeof(sendData.name), pPlayer->GetName().c_str(), _TRUNCATE);
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_add_npc_packet(const NpcSession* pNpc)
{
	SC_ADD_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendData.type = SC_ADD_OBJECT;
	sendData.id = pNpc->GetId() + MAX_USER;
	sendData.x = pNpc->GetPos().xPos;
	sendData.y = pNpc->GetPos().yPos;
	strncpy_s(sendData.name, sizeof(sendData.name), pNpc->GetName().c_str(), _TRUNCATE);
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_remove_object_packet(const PlayerSession* pPlayer)
{
	SC_REMOVE_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	sendPacket.type = SC_REMOVE_OBJECT;
	sendPacket.id = pPlayer->GetId();
	SendPacket(&sendPacket);
}

void PlayerSocketHandler::send_remove_npc_object_packet(const NpcSession* pNpc)
{
	SC_REMOVE_OBJECT_PACKET sendPacket;
	sendPacket.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	sendPacket.type = SC_REMOVE_OBJECT;
	sendPacket.id = pNpc->GetId() + MAX_USER;
	SendPacket(&sendPacket);
}

void PlayerSocketHandler::send_move_object_packet(const PlayerSession* pPlayer)
{
	SC_MOVE_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendData.type = SC_MOVE_OBJECT;
	sendData.id = pPlayer->GetId();
	sendData.y = pPlayer->GetPos().yPos;
	sendData.x = pPlayer->GetPos().xPos;
	sendData.move_time = m_moveTime;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_npc_move_object_packet(const NpcSession* pNpc)
{
	SC_MOVE_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendData.type = SC_MOVE_OBJECT;
	sendData.id = pNpc->GetId() + MAX_USER;;
	sendData.y = pNpc->GetPos().yPos;
	sendData.x = pNpc->GetPos().xPos;
	sendData.move_time = m_moveTime;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_chat_packet(int objId, const char* mess, char chatType)
{
	SC_CHAT_PACKET sendData;
	sendData.size = sizeof(SC_CHAT_PACKET);
	sendData.type = SC_CHAT;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_login_ok_packet()
{
	SC_LOGIN_OK_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_OK_PACKET);
	sendData.type = SC_LOGIN_OK;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_login_fail_packet()
{
	SC_LOGIN_FAIL_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_FAIL_PACKET);
	sendData.type = SC_LOGIN_FAIL;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_stat_change_packet()
{
	SC_STAT_CHANGE_PACKET sendData;
	sendData.size = sizeof(SC_STAT_CHANGE_PACKET);
	sendData.type = SC_STAT_CHANGE;
	sendData.hp = static_cast<int>(m_playerSession->GetHp());
	sendData.exp = m_playerSession->GetExp();
	sendData.level = m_playerSession->GetLevel();
	SendPacket(&sendData);
}