#pragma once
#include "Creature.h"

class PlayerSession : public Creature
{
private:
	int								m_exp;

	unordered_set<int>				m_npcViewList;
	mutex							m_npcViewListLock;
	C_STATE							m_state = C_STATE::CT_FREE;
	mutex							m_stateLock;
public:
	PlayerSession() {};
	~PlayerSession() {};

	void SetExp(int exp) { m_exp = exp; }
	int GetExp() const { return m_exp; }

	void SetRandomPos();

	void SetState(C_STATE state) 
	{
		lock_guard<mutex> lock(m_stateLock);
		m_state = state;
	}
	C_STATE GetState()
	{
		lock_guard<mutex> lock(m_stateLock);
		C_STATE st = m_state;
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

