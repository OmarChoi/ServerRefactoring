#include "stdafx.h"
#include "Manager.h"
#include "NetworkManager.h"
#include "GameManager.h"

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
}

void Manager::Init()
{
	if (networkManager == nullptr)
	{
		networkManager = new NetworkManager();
		networkManager->Init();
	}
	if (gameManager == nullptr)
	{
		gameManager = new GameManager();
		gameManager->Init();
	}
}
