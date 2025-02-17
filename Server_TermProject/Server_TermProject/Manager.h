#pragma once
class GameManager;
class NetworkManager;
class DataBaseManager;

class Manager
{
public:
	static Manager& GetInstance()
	{
		static Manager instance;
		return instance;
	}

	Manager(const Manager&) = delete;
	Manager& operator=(const Manager&) = delete;

private:
	atomic_int				m_nCurrentPlayer;

private:
	Manager();
	~Manager();

	void Init();

private:
	NetworkManager*			networkManager;
	GameManager*			gameManager;
	DataBaseManager*		databaseManager;

public:
	NetworkManager* const GetNetworkManager() { return networkManager; }
	GameManager* const  GetGameManager() { return gameManager; }
	DataBaseManager* const  GetDataBaseManager() { return databaseManager; }

	const int GetPlayerNum() { return m_nCurrentPlayer; }
	void AddPlayerIndex() { m_nCurrentPlayer += 1; }
};
