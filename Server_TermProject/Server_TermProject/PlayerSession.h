#pragma once
#include "Creature.h"

class PlayerSession : public Creature
{
private:
	int								m_exp;
	unordered_set<int>				m_npcViewList;
	mutex							m_npcViewListLock;
	PlayerState						m_state = PlayerState::CT_FREE;
	mutex							m_stateLock;
public:
	PlayerSession() {};
	~PlayerSession() {};

	void SetExp(int exp) { m_exp = exp; }
	int GetExp() const { return m_exp; }

	void SetRandomPos();

	void SetState(PlayerState state);
	PlayerState GetState();

	void Die() override;
private:
	void Init();

public:
	void AddViewNPCList(int objID);
	void RemoveViewNPCList(int objID);
	virtual void UpdateViewList() override;

private:
	int GetExpRequirement(int level);
};

