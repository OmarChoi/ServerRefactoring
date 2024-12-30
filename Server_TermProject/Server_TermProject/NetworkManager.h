#pragma once
class PlayerSocketHandler;
class NetworkManager
{
private:
	WSADATA					m_WSAData;
	OVER_EXP				m_AcceptOver;
	SOCKET					m_listenSocket;
	SOCKET					m_clientSocket;

	atomic_int				m_nClient;		// ���� �Ŵ��� ��ü �Ŵ��� Ŭ������ �̵�

	PlayerSocketHandler**	m_ppPlayerSocketHandler;	// �÷��̾��� ����� �����ϴ� Handler ��ü

public:
	HANDLE				m_hIocp;
	NetworkManager();
	~NetworkManager();

	void Init();
	void Accept();
	void Recv();
};

