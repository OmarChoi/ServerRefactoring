#pragma once
class PlayerSocketHandler;
class NetworkManager
{
private:
	WSADATA					m_WSAData;
	OVER_EXP				m_AcceptOver;
	SOCKET					m_listenSocket;
	SOCKET					m_clientSocket;
	PlayerSocketHandler**	m_ppPlayerSocketHandler;	// 플레이어의 통신을 관리하는 Handler 객체

	mutex					m_acceptLock;
public:
	HANDLE				m_hIocp;
	NetworkManager() { Init(); }
	~NetworkManager();

	void Init();
	void Accept();
	void Recv(DWORD recvSize, OVER_EXP* over, int playerKey);

	// 수정 필요
	PlayerSocketHandler* GetPlayerNetwork(int objId) { return m_ppPlayerSocketHandler[objId]; }
};

