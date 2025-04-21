#pragma once
class PlayerSession;
class NpcSession;
class MapSession;

class GameManager
{
private:
	array<PlayerSession*, MAX_USER> m_ppPlayerSession;
	array<NpcSession*, MAX_NPC>		m_ppNpcSession;
	MapSession*						m_mapSession = nullptr;

public:
	// NPCSession
	
public:
	GameManager() { Init(); }
	~GameManager();

	void Init();
	void AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
		float hp, float maxHp, int exp, int level);
	void RemovePlayerSession(int pId) { m_ppPlayerSession[pId] = nullptr; }
	PlayerSession* GetPlayerSession(int pId);
	NpcSession* GetNpcSession(int objId);

public:
	MapSession* GetMapSession() { return m_mapSession; }
	bool CanGo(Position pos) const;
	bool CanGo(int yPos, int xPos) const;
	int GetTileCost(Position pos);
};

