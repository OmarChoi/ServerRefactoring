#pragma once
class PlayerSocketHandler
{
private:
	OVER_EXP						m_recvOver;
	SOCKET							m_socket;

	int								m_remainPacket;
public:
	PlayerSocketHandler();
	~PlayerSocketHandler() {};

	void SetSocket(SOCKET socket) { m_socket = socket; }


public:
	void CallRecv();
	void ProcessPacket();
};

