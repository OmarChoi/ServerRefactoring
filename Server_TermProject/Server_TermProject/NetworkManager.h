#pragma once
class PlayerSocketHandler;
class NetworkManager
{
private:
	WSADATA					m_WSAData;
	OVER_EXP				m_AcceptOver;
	SOCKET					m_listenSocket;
	SOCKET					m_clientSocket;
	PlayerSocketHandler**	m_ppPlayerSocketHandler;	// �÷��̾��� ����� �����ϴ� Handler ��ü

	mutex					m_acceptLock;
public:
	HANDLE				m_hIocp;
	NetworkManager() { Init(); }
	~NetworkManager();

	void Init();
	void Accept();
	void Recv(DWORD recvSize, OVER_EXP* over, int playerKey);
	void UpdatePlayerInfo();
	// ���� �ʿ�
	PlayerSocketHandler* GetPlayerNetwork(int objId) { return m_ppPlayerSocketHandler[objId]; }
};

