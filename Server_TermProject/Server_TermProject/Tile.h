#pragma once
class Tile
{
private:
	int m_tileType;
	bool m_canGo;
	int m_cost;

public:
	Tile() : m_tileType(0), m_canGo(true), m_cost(1) {};
	Tile(int type, bool canGo, int cost) : m_tileType(type), m_canGo(canGo), m_cost(cost) {};

public:
	bool CanGo() { return m_canGo; }
	int GetCost() { return m_cost; }
};

