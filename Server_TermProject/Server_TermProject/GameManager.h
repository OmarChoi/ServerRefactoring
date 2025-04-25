#pragma once
class PlayerSession;
class NpcSession;
class MapSession;

class GameManager
{
private:
	array<shared_ptr<PlayerSession>, MAX_USER>	m_ppPlayerSession;
	array<shared_ptr<NpcSession>, MAX_NPC>		m_ppNpcSession;
	MapSession*									m_mapSession = nullptr;

public:
	// NPCSession
	
public:
	GameManager() { Init(); }
	~GameManager();

	void Init();
	void AddPlayerSession(int playerId, string playerName, int yPos, int xPos,
		float hp, float maxHp, int exp, int level);
	void RemovePlayerSession(int pId) { m_ppPlayerSession[pId].reset(); }
	shared_ptr<PlayerSession> GetPlayerSession(int pId) const;
	shared_ptr<NpcSession> GetNpcSession(int objId)  const;

public:
	MapSession* GetMapSession() { return m_mapSession; }
	bool CanGo(Position pos) const;
	bool CanGo(int yPos, int xPos) const;
	int GetTileCost(Position pos);
};

