#pragma once
class Tile;
class MapSession
{
private:
	Tile* m_tiles[W_HEIGHT][W_WIDTH];
public:
	MapSession();
	~MapSession();
	void Init();
public:
	bool CanGo(const Position pos);
	int GetCost(const Position pos);
};

