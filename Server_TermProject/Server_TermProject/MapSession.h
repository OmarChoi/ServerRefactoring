#pragma once
constexpr int S_WIDTH = 100;
constexpr int S_HEIGHT = 100;

class Tile;
class Section;

enum class ListType
{
	Player = 0,
	Npc = 1
};

class MapSession
{
private:
	Tile*							m_tiles[W_HEIGHT][W_WIDTH];
	Section*						m_sections[W_HEIGHT / S_HEIGHT][W_WIDTH / S_WIDTH];

public:
	MapSession();
	~MapSession();
	void Init();
public:
	bool CanGo(const Position pos);
	int GetCost(const Position pos);

public:
	// Section
	void ChangeSection(int type, int objId, Position prevPos, Position nextPos);
	pair<int, int> GetSectionIndex(int yPos, int xPos) const;
	pair<int, int> GetSectionIndex(Position pos) const;
	void GetUserInNearSection(int sectionY, int sectionX, unordered_set<int>& nearList);
	void GetUserInNearSection(Position pos, unordered_set<int>& nearList);
	void GetNpcInNearSection(int sectionY, int sectionX, unordered_set<int>& nearList);
	void GetNpcInNearSection(Position pos, unordered_set<int>& nearList);

private:
	bool IsValidSection(int sectionY, int sectionX);
	void GetCreatureInNearSection(ListType type, int sectionY, int sectionX, unordered_set<int>& nearList);
};

