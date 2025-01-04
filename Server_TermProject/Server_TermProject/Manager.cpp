#include "stdafx.h"
#include "Manager.h"
#include "GameManager.h"
#include "NetworkManager.h"

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
	if (gameManager == nullptr)
		gameManager = new GameManager();
	if (networkManager == nullptr)
		networkManager = new NetworkManager();
}
