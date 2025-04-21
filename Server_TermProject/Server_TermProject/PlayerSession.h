#pragma once
#include "Creature.h"

class PlayerSocketHandler;
class PlayerSession : public Creature
{
private:
	atomic_int						m_exp;
	atomic_bool						m_statChanged;
	unordered_set<int>				m_npcViewList;
	mutex							m_npcViewListLock;
	PlayerState						m_state = PlayerState::CT_FREE;
	mutex							m_stateLock;
	PlayerSocketHandler*			m_pNetwork;
public:
	PlayerSession() {};
	~PlayerSession();

	void SetExp(int exp) { m_exp = exp; }
	int GetExp() const { return m_exp; }

	virtual void SetPos(int y, int x);
	virtual void SetPos(Position pos) { SetPos(pos.yPos, pos.xPos); }
	void SetRandomPos();

	void SetState(PlayerState state);
	PlayerState GetState();
	bool HasStatChanged() const { return m_statChanged; }

	void Attack();
	void ApplyDamage(int damage, int objId = -1) override;
	void AddExp(int exp);
	void Die() override;
public:
	void Init(PlayerSocketHandler* socket);

public:
	void AddViewNPCList(int objID);
	void RemoveViewNPCList(int objID);
	virtual void UpdateViewList() override;
	void UpdatePlayerViewList();
	void UpdateNpcViewList();
	void LogOut();

private:
	void UpdateDBInfo();

private:
	int GetExpRequirement(int level);
};

