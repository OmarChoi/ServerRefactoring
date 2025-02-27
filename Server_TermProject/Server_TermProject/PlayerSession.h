#pragma once
#include "Creature.h"

class PlayerSession : public Creature
{
private:
	atomic_int						m_exp;
	atomic_bool						m_statChanged;
	unordered_set<int>				m_npcViewList;
	mutex							m_npcViewListLock;
	PlayerState						m_state = PlayerState::CT_FREE;
	mutex							m_stateLock;
public:
	PlayerSession() {};
	~PlayerSession();

	void SetExp(int exp) { m_exp = exp; }
	int GetExp() const { return m_exp; }

	void SetRandomPos();

	void SetState(PlayerState state);
	PlayerState GetState();
	bool HasStatChanged() const { return m_statChanged; }

	void Attack();
	void ApplyDamage(int damage, int objId = -1) override;
	void AddExp(int exp);
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

