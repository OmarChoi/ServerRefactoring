#include "stdafx.h"
#include "Manager.h"
#include "GameManager.h"
#include "PlayerSession.h"
#include "NetworkManager.h"
#include "PlayerSocketHandler.h"

NetworkManager::~NetworkManager()
{
	if (m_ppPlayerSocketHandler != nullptr) 
	{
		for (int i = 0; i < MAX_USER; ++i) 
		{
			if (m_ppPlayerSocketHandler[i] != nullptr) 
			{
				delete m_ppPlayerSocketHandler[i];
				m_ppPlayerSocketHandler[i] = nullptr;
			}
		}
		delete[] m_ppPlayerSocketHandler;
		m_ppPlayerSocketHandler = nullptr;
	}
	cout << "Close Socket" << endl;
	closesocket(m_listenSocket);
}

void NetworkManager::Init()
{
	cout << "Initiate network initialization.\n";
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);

	// Ŭ���̾�Ʈ ������ ����ϴ� Listner ���� ����
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT_NUM);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
	listen(m_listenSocket, SOMAXCONN);

	SOCKADDR_IN clientAddress;
	int addrSize = sizeof(clientAddress);

	m_ppPlayerSocketHandler = new PlayerSocketHandler* [MAX_USER];
	
	// �Ѱ��� CompletionPort ����
	m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_hIocp, MAX_USER + 1, 0);

	// Ŭ���̾�Ʈ�� �����ϱ� ���� ���� ����
	m_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	// Ŭ���̾�Ʈ�� ������ �� �ְ� Accept ����
	m_AcceptOver.m_compType = COMP_TYPE::Accept;
	::AcceptEx(m_listenSocket, m_clientSocket, m_AcceptOver.m_sendBuf, 0, addrSize + 16, addrSize + 16, 0, &m_AcceptOver.m_over);

	cout << "Network initialization complete.\n";
}

void NetworkManager::Accept()
{
	Manager& manager = Manager::GetInstance();
	int nPlayer = manager.GetPlayerNum();
	SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (nPlayer < MAX_USER)
	{
		m_ppPlayerSocketHandler[nPlayer] = new PlayerSocketHandler(m_clientSocket, nPlayer);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_clientSocket), m_hIocp, nPlayer, 0);
		m_ppPlayerSocketHandler[nPlayer]->CallRecv();

		manager.AddPlayerIndex();
		m_clientSocket = newSocket;
	}
	else
	{
		cout << "Max user exceeded.\n";
		closesocket(newSocket); 
		return;
	}

	ZeroMemory(&m_AcceptOver.m_over, sizeof(m_AcceptOver.m_over));
	int addrSize = sizeof(SOCKADDR_IN);
	AcceptEx(m_listenSocket, m_clientSocket, m_AcceptOver.m_sendBuf, 0, addrSize + 16, addrSize + 16, 0, &m_AcceptOver.m_over);
}

void NetworkManager::Recv(DWORD recvSize, OVER_EXP* over, int playerKey)
{
	m_ppPlayerSocketHandler[playerKey]->ProcessPacket(recvSize, over);
}

void NetworkManager::UpdatePlayerInfo()
{
	Manager& manager = Manager::GetInstance();
	int nPlayerNum = manager.GetPlayerNum();

	for (int i = 0; i < nPlayerNum; ++i) 
	{
		PlayerSession* player = manager.GetGameManager()->GetPlayerSession(i);
		if (player == nullptr) continue;
		if (player->GetState() != PlayerState::CT_INGAME) continue;
		if (player->HasStatChanged() == false) continue;
		m_ppPlayerSocketHandler[i]->send_stat_change_packet(player);
	}
}
