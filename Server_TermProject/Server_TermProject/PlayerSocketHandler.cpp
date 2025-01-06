#include "stdafx.h"
#include "Manager.h"
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
	switch (packet[2]) {
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		VerifyUserAccount(p->name);
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

	// 함수로 따로 분리해줄 필요성을 느낌
	WCHAR nameForDB[NAME_SIZE];
	char name[NAME_SIZE];
	strncpy_s(name, userName, NAME_SIZE);
	MultiByteToWideChar(CP_UTF8, 0, userName, -1, nameForDB, NAME_SIZE);

	player->SetName(userName);
	if (manager.GetDataBaseManager()->GetUserData(nameForDB, player) == false)
	{
		ZeroMemory(player, sizeof(PlayerSession));
		send_login_fail_packet();
		return;
	}
	send_login_ok_packet();
	send_login_info_packet(player);
	manager.GetDataBaseManager()->UpdatePlayerData(nameForDB, m_playerID);
}

void PlayerSocketHandler::SendPacket(void* packet)
{
	// OVER_EXP을 Object Pooling을 이용하여 new하지 않고 사용할 수 있게 수정해도 괜찮을듯.
	OVER_EXP* sendData = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sendData->m_wsabuf, 1, 0, 0, &sendData->m_over, 0);
}

void PlayerSocketHandler::send_login_info_packet(const PlayerSession* pPlayer)
{
	SC_LOGIN_INFO_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_INFO_PACKET);
	sendData.type = SC_LOGIN_INFO;
	sendData.id = m_playerID;
	sendData.hp = pPlayer->GetHp();
	sendData.maxHp = pPlayer->GetMaxHp();
	sendData.exp = pPlayer->GetExp();
	sendData.level = pPlayer->GetLevel();
	sendData.x = pPlayer->GetPos().xPos;
	sendData.y = pPlayer->GetPos().yPos;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_add_object_packet(int objId)
{
	SC_ADD_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendData.type = SC_ADD_OBJECT;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_add_npc_packet(int objId)
{
	SC_ADD_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_ADD_OBJECT_PACKET);
	sendData.type = SC_ADD_OBJECT;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_remove_object_packet(int objId)
{
	SC_REMOVE_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	sendData.type = SC_REMOVE_OBJECT;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_move_object_packet(int objId)
{
	SC_MOVE_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendData.type = SC_MOVE_OBJECT;
	SendPacket(&sendData);
}

void PlayerSocketHandler::send_npc_move_object_packet(int objId)
{
	SC_MOVE_OBJECT_PACKET sendData;
	sendData.size = sizeof(SC_MOVE_OBJECT_PACKET);
	sendData.type = SC_MOVE_OBJECT;
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