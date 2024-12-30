#pragma once
class PlayerSocketHandler;
class PlayerSession
{
private:
	OVER_EXP						m_recvOver;
	SOCKET							m_socket;

	int								m_remainPacket;
	PlayerSocketHandler*			m_pSocket;
public:
	PlayerSession() = delete;
	PlayerSession(SOCKET socket) : m_socket(socket) {};
	~PlayerSession() {};

private:
	void Init();
};

