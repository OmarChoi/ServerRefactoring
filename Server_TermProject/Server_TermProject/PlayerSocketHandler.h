#pragma once
class PlayerSession;
class NpcSession;
class PlayerSocketHandler
{
private:
	OVER_EXP						m_recvOver;
	SOCKET							m_socket;
	int								m_remainPacket;
	int								m_playerID;

	unsigned int					m_moveTime;
public:
	PlayerSocketHandler();
	PlayerSocketHandler(SOCKET socket, int playerID) : m_socket(socket), m_remainPacket(0), m_playerID(playerID), m_moveTime(0) { };
	~PlayerSocketHandler() {};

public:
	void CallRecv();
	void ApplyPacketData(char* packet);
	void ProcessPacket(DWORD recvSize, OVER_EXP* over);

public:
	void VerifyUserAccount(const char* userName);

	void SendPacket(void* packet);
	void send_add_object_packet(const PlayerSession* pPlayer);
	void send_add_npc_packet(const NpcSession* pNpc);
	void send_remove_object_packet(const PlayerSession* pPlayer);
	void send_remove_npc_object_packet(const NpcSession* pNpc);
	void send_move_object_packet(const PlayerSession* pPlayer);
	void send_npc_move_object_packet(const NpcSession* pNpc);
	void send_chat_packet(int objId, const char* mess, char chatType);

	void send_login_info_packet(const PlayerSession* pPlayer);
	void send_login_ok_packet();
	void send_login_fail_packet();
	void send_stat_change_packet();
};
