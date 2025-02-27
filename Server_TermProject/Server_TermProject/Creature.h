#pragma once
class Creature
{
protected:
	int					m_objectID;
	string				m_name;
	atomic_int			m_level;
	float				m_hp, m_maxHp;
	float				m_speed;
	int                 m_attackRange;
	int                 m_damage;
	Position			m_pos;

	bool				m_bActive;
	mutex				activeMutex;

	unordered_set<int>				m_viewList;
	mutex							m_viewListLock;

	chrono::high_resolution_clock::time_point m_lastMoveTime;
	chrono::high_resolution_clock::time_point m_lastAttackTime;

public:
	Creature();
	virtual ~Creature();

	void SetPos(int y, int x);
	void SetPos(Position pos) { SetPos(pos.yPos, pos.xPos); }
	Position GetPos() const { return m_pos; }

	void SetObjId(int i) { m_objectID = i; }

	void SetName(string userName) { m_name = userName; }
	string GetName() const { return m_name; }

	void SetHp(float hp) { m_hp = hp; }
	float GetHp() const { return m_hp; }
	
	void SetMaxHp(float maxHp) { m_maxHp = maxHp; }
	float GetMaxHp() const { return m_maxHp; }

	int GetId() const { return m_objectID; }
	int GetMoveTime() const { return m_lastMoveTime.time_since_epoch().count(); }

	void SetLevel(int level) { m_level = level; }
	int GetLevel() const { return m_level; }

	bool IsActive();
	void SetActive(bool active);

	virtual void AddViewList(int objID);
	virtual void RemoveViewList(int objID);

	virtual bool CanSee(const Creature* other);
public:
	virtual void RespawnObject();
	virtual void ApplyDamage(int damage, int objId = -1);
	virtual void Die();
protected:
	virtual void UpdateViewList() {};
};

 