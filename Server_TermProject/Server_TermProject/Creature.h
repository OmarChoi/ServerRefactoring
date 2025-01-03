#pragma once
class Creature
{
protected:
	int					m_objectID;
	float				m_hp, m_maxHp;
	Position			m_pos;

	chrono::system_clock::time_point m_tpLastMoveTime;
	chrono::system_clock::time_point m_tpLastAttackTime;

public:
	Creature();

	void SetPos(int y, int x) { m_pos.yPos = y; m_pos.xPos = x; }
	Position GetPos() { return m_pos; }

	void SetHp(float hp) { m_hp = hp; }
	float GetHp() { return m_hp; }
	
	void SetMaxHp(float maxHp) { m_maxHp = maxHp; }
	float GetMaxHp() { return m_maxHp; }

private:

};

 