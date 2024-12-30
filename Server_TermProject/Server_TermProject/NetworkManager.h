#pragma once
class PlayerSocketHandler;
class NetworkManager
{
private:
	WSADATA					m_WSAData;
	OVER_EXP				m_AcceptOver;
	SOCKET					m_listenSocket;
	SOCKET					m_clientSocket;

	atomic_int				m_nClient;		// 추후 매니저 전체 매니저 클래스로 이동

	PlayerSocketHandler**	m_ppPlayerSocketHandler;	// 플레이어의 통신을 관리하는 Handler 객체

public:
	HANDLE				m_hIocp;
	NetworkManager();
	~NetworkManager();

	void Init();
	void Accept();
	void Recv();
};

