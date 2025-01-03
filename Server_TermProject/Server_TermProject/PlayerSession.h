#pragma once
#include "Creature.h"

class PlayerSocketHandler;
class PlayerSession : public Creature
{
private:
	string m_userName;
	int m_exp;
	int m_level;
public:
	PlayerSession() {};
	~PlayerSession() {};

	void SetName(string userName) { m_userName = userName; }
	string GetName() { return m_userName; }
	void SetExp(int exp) { m_exp = exp; }
	int GetExp() { return m_exp; }
	void SetLevel(int level) { m_level = level; }
	int GetLevel() { return m_level; }

private:
	void Init();
};

