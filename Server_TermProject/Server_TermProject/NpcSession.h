#pragma once
#include "Creature.h"
class NpcSession : public Creature
{
protected:
	bool								m_isActive;
	int									m_npcType;
	int									m_targetID = -1;
	STATE								m_state;
	chrono::system_clock::time_point	m_changeTime;
	Position							m_spawnPos;
protected:
	int									m_attackRange;
	int									m_damage;
	
protected:
	virtual void UpdateViewList() override;

public:
	lua_State*							m_luaState;
	virtual void Move() = 0;
	void SetTarget(int objId);
	void SetId(int objId) { m_objectID = objId; }
	void InitPosition(Position pos);
	void Attack();
	void GetDamage(int objId) {};
	void ReleaseTarget(int objId) {};
	void Respawn() {};
};

class SimpleNpc : public NpcSession
{
private:
	queue<Position> m_path;
public:
	virtual void Move() override;
};

class RoamingMonster : public NpcSession
{
	chrono::system_clock::time_point	m_MakePathTime;
	stack<Position> m_path;
public:
	virtual void Move() override;
	void CreatePath();
};