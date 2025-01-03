#include "stdafx.h"
#include "Manager.h"
#include "GameManager.h"
#include "PlayerSession.h"
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
		cout << "process something. \n";
	}
}

void PlayerSocketHandler::VerifyUserAccount(const string userName)
{
	
	Manager& manager = Manager::GetInstance();
	// Manager.DataBase���� ������ Player ������ ���� PlayerSession �߰�
	// DataBase::GetUserData(WCAHR* userName);
	// ������ ���·� userInfo�� DB�� ����� ���� �޾ƿ� �� �ְ� ������ ����
	send_login_ok_packet();

	SC_LOGIN_INFO_PACKET userInfoPacket;
	userInfoPacket.size = sizeof(SC_LOGIN_INFO_PACKET);
	userInfoPacket.type = SC_LOGIN_INFO;
	
	// ���� DataBase���� ������ �� ���
	userInfoPacket.x = rand() % W_WIDTH;
	userInfoPacket.y = rand() % W_HEIGHT;
	userInfoPacket.hp = 100;
	userInfoPacket.maxHp = 100;
	userInfoPacket.exp = 0;
	userInfoPacket.level = 1;
	manager.GetGameManager()->AddPlayerSession(
		m_playerID, userName, userInfoPacket.y, userInfoPacket.x, 
		userInfoPacket.hp, userInfoPacket.maxHp, 
		userInfoPacket.exp, userInfoPacket.level
	);
	SendPacket(&userInfoPacket);
}

void PlayerSocketHandler::SendPacket(void* packet)
{
	// OVER_EXP�� Object Pooling�� �̿��Ͽ� new���� �ʰ� ����� �� �ְ� �����ص� ��������.
	OVER_EXP* sendData = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sendData->m_wsabuf, 1, 0, 0, &sendData->m_over, 0);
}

void PlayerSocketHandler::send_login_info_packet()
{
	SC_LOGIN_INFO_PACKET sendData;
	sendData.size = sizeof(SC_LOGIN_INFO_PACKET);
	sendData.type = SC_LOGIN_INFO;
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