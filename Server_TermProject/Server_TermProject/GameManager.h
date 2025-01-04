#pragma once
class PlayerSession;
class MapSession;
class GameManager
{
private:
	MapSession* m_mapSession;
	PlayerSession** m_ppPlayerSession;
public:
	// NPCSession
	
public:
	GameManager() { Init(); }
	~GameManager();

	void Init();
	void AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
		float hp, float maxHp, int exp, int level);

	bool CanGo(Position pos);
	bool CanGo(int yPos, int xPos);

};

