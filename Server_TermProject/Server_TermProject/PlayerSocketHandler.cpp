#include "stdafx.h"
#include "Manager.h"
#include "NpcSession.h"
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
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		PlayerSession* player = gameManager->GetPlayerSession(m_playerID);
		Position nextPos = player->GetPos() + movements[p->direction];
		m_moveTime = p->move_time;
		if (gameManager->CanGo(nextPos)) 
		{
			player->SetPos(nextPos);
			player->UpdateViewList();
			send_move_object_packet(player);
		}
		break;
	}
	case CS_CHAT:
	{
		break;
	}
	case CS_ATTACK:
	{
		break;
	}
	case CS_TELEPORT:
	{
		break;
	}
	case CS_LOGOUT:
	{
		break;
	}
	default:
		cout << "process something.\n";
	}
}

void PlayerSocketHandler::VerifyUserAccount(const char* userName)
{
	Manager& manager = Manager::GetInstance();
	PlayerSession* player = manager.GetGameManager()->GetPlayerSession(m_playerID);

	// �Լ��� ���� �и����� �ʿ伺�� ����
	WCHAR nameForDB[NAME_SIZE];
	char name[NAME_SIZE];
	
	if (strncmp(userName, "StressTest", 10) == 0)
	{
		player->SetName(userName);
		player->SetObjId(m_playerID);
		player->SetState(C_STATE::CT_INGAME);
		player->SetRandomPos();
		player->UpdateViewList();
		
		send_login_ok_packet();
		send_login_info_packet(player);
		return;
	}
	strncpy_s(name, userName, NAME_SIZE);
	MultiByteToWideChar(CP_UTF8, 0, userName, -1, nameForDB, NAME_SIZE);

	player->SetName(userName);
	if (manager.GetDataBaseManager()->GetUserData(nameForDB, player) == false)
	{
		ZeroMemory(player, sizeof(PlayerSession));
		send_login_fail_packet();
		return;
	}
	player->SetState(C_STATE::CT_INGAME);
	player->SetObjId(m_playerID);
	player->UpdateViewList();
	send_login_ok_packet();
	send_login_info_packet(player);
}

void PlayerSocketHandler::SendPacket(void* packet)
{
	// OVER_EXP�� Object Pooling�� �̿��Ͽ� new���� �ʰ� ����� �� �ְ� �����ص� ��������.
	OVER_EXP* sendData = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sendData->m_wsabuf, 1, 0, 0, &sendData->m_over, 0);
}

void PlayerSocketHandler::send_login_info_packet(const PlayerSession* pPlayer)
{
	SC_LOGIN_INFO_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_INFO_PACKET);
	sendData.type = SC_LOGIN_INFO;
	sendData.id = pPlayer->GetId();
	sendData.hp = pPlayer->GetHp();
	sendData.maxHp = pPlayer->GetMaxHp();
	sendData.exp = pPlayer->GetExp();
	sendData.level = pPlayer->GetLevel();
	sendData.x = pPlayer->GetPos().xPos;
	sendData.y = pPlayer->GetPos().yPos;
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
	SendPacket(&sendData);
}