#include "stdafx.h"
#include "NetworkManager.h"
#include "GameObject.h"
#include "PlayerSocketHandler.h"

NetworkManager::NetworkManager()
{

}

NetworkManager::~NetworkManager()
{
	closesocket(m_listenSocket);
}

void NetworkManager::Init()
{
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);

	// 클라이언트 수신을 대기하는 Listner 소켓 생성
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT_NUM);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
	listen(m_listenSocket, SOMAXCONN);

	SOCKADDR_IN clientAddress;
	int addrSize = sizeof(clientAddress);

	m_ppPlayerSocketHandler = new PlayerSocketHandler* [MAX_USER];
	
	// 한개의 CompletionPort 생성
	m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_hIocp, 9999, 0);

	// 클라이언트를 수신하기 위한 소켓 설정
	m_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	// 클라이언트가 접속할 수 있게 Accept 설정
	m_AcceptOver.m_compType = OP_ACCEPT;
	AcceptEx(m_listenSocket, m_clientSocket, m_AcceptOver.m_sendBuf, 0, addrSize + 16, addrSize + 16, 0, &m_AcceptOver.m_over);
}

void NetworkManager::Accept()
{
	if (m_nClient < MAX_USER)
	{
		m_nClient += 1;
		Player* player = new Player;
		player->m_playerState = CT_ALLOC;
		m_ppPlayerSocketHandler[m_nClient]->SetSocket(m_clientSocket);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_clientSocket), m_hIocp, m_nClient, 0);
		m_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		m_ppPlayerSocketHandler[m_nClient]->CallRecv();
		// Players[tempPlayer->m_objectID] = player;
		// Players[tempPlayer->m_objectID]->callRecv();
	}
	else
	{
		cout << "Max user exceeded.\n";
	}
	ZeroMemory(&m_AcceptOver.m_over, sizeof(m_AcceptOver.m_over));
	int addrSize = sizeof(SOCKADDR_IN);
	AcceptEx(m_listenSocket, m_clientSocket, m_AcceptOver.m_sendBuf, 0, addrSize + 16, addrSize + 16, 0, &m_AcceptOver.m_over);
}

void NetworkManager::Recv()
{

}

