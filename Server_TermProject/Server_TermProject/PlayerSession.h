#pragma once
#include "Creature.h"

class PlayerSession : public Creature
{
private:
	int								m_exp;
	int								m_level;

	unordered_set<int>				m_npcViewList;
	mutex							m_npcViewListLock;
	C_STATE							m_state = C_STATE::CT_FREE;
	mutex							m_stateLock;
public:
	PlayerSession() {};
	~PlayerSession() {};

	void SetExp(int exp) { m_exp = exp; }
	int GetExp() const { return m_exp; }
	void SetLevel(int level) { m_level = level; }
	int GetLevel() const { return m_level; }

	void SetRandomPos();

	void SetState(C_STATE state) 
	{
		m_stateLock.lock();
		m_state = state;
		m_stateLock.unlock();
	}
	C_STATE GetState()
	{
		C_STATE st;
		m_stateLock.lock();
		st = m_state;
		m_stateLock.unlock();
		return st;
	}
private:
	void Init();

public:
	void AddViewNPCList(int objID);
	void RemoveViewNPCList(int objID);
	virtual void UpdateViewList() override;

protected:
};

