#pragma once
class PlayerSession;
class GameManager
{
public:
	PlayerSession** m_ppPlayerSession;
	// NPCSession
	
public:
	GameManager() {};
	~GameManager();

	void Init();
	void AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
		float hp, float maxHp, int exp, int level);
};

