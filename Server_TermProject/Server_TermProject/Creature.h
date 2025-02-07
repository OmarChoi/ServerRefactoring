#pragma once
class Creature
{
protected:
	int					m_objectID;
	string				m_name;
	float				m_hp, m_maxHp;
	Position			m_pos;

	unordered_set<int>				m_viewList;
	mutex							m_viewListLock;

	chrono::high_resolution_clock::time_point m_lastMoveTime;
	chrono::high_resolution_clock::time_point m_lastAttackTime;

public:
	Creature();

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

	void AddViewList(int objID);
	void RemoveViewList(int objID);

	bool CanSee(const Creature* other);
protected:
	virtual void UpdateViewList() {};
};

 