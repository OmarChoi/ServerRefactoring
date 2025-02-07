#include "stdafx.h"
#include "Manager.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "DataBaseManager.h"

Manager::Manager()
{
	Init();
}

Manager::~Manager()
{
	if (networkManager != nullptr)
		delete networkManager;
	if (gameManager != nullptr)
		delete gameManager;
	if (databaseManager != nullptr)
		delete databaseManager;
}

void Manager::Init()
{
	static std::random_device rd;
	m_Gen.seed(rd());

	if (gameManager == nullptr)
		gameManager = new GameManager();
	if (networkManager == nullptr)
		networkManager = new NetworkManager();
	if (databaseManager == nullptr)
		databaseManager = new DataBaseManager();
}
