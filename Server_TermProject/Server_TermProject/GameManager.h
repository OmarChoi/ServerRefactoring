#pragma once
class PlayerSession;
class NpcSession;
class MapSession;

class GameManager
{
private:
	MapSession* m_mapSession;
	PlayerSession** m_ppPlayerSession;
	NpcSession** m_ppNpcSession;
	mutex		mapMutex;
public:
	// NPCSession
	
public:
	GameManager() { Init(); }
	~GameManager();

	void Init();
	void AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
		float hp, float maxHp, int exp, int level);
	PlayerSession* GetPlayerSession(int pId) { return m_ppPlayerSession[pId]; }
	NpcSession* GetNpcSession(int objId) { return m_ppNpcSession[objId]; }

	bool CanGo(Position pos) const;
	bool CanGo(int yPos, int xPos) const;
	int GetTileCost(Position pos);
};

