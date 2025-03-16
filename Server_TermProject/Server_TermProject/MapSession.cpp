#include "stdafx.h"
#include "Tile.h"
#include "Section.h"
#include "MapSession.h"

MapSession::MapSession()
{
	Init();
}

MapSession::~MapSession()
{
	if (m_tiles != nullptr) 
	{
		for (int i = 0; i < W_HEIGHT; ++i) 
		{
			for (int j = 0; j < W_WIDTH; ++j)
			{
				if (m_tiles[j][i] != nullptr)
				{
					delete m_tiles[j][i];
					m_tiles[j][i] = nullptr;
				}
			}
		}
	}
}

void MapSession::Init()
{
	cout << "Initiate Map initialization.\n";
	ifstream mapFile("map.txt");
	// 0 : ��ֹ�
	// 1 : �� �� �ִ� ����
	int x = 0, y = 0;
	char tileType = ' ';
	while (!mapFile.eof())
	{
		mapFile >> tileType;
		// Lua Script�� ���� Ÿ�Ͽ� �´� �����͸� ������ �� �ִ��� Ȯ���غ� �ʿ䰡 �ִ�.
		if (tileType == '1')	// �� �� �ִ� ����
			m_tiles[y][x++] = new Tile(1, 1, 1);
		else
			m_tiles[y][x++] = new Tile(0, 0, 9999);

		if (x == W_WIDTH)
		{
			y += 1;
			x = 0;
		}
	}

	for (int j = 0; j < W_HEIGHT / S_HEIGHT; ++j) 
	{
		for (int i = 0; i < W_WIDTH / S_WIDTH; ++i)
		{
			m_sections[j][i] = new Section();
		}
	}
	cout << "Map initialization complete.\n";
}

bool MapSession::CanGo(const Position pos)
{
	if (pos.yPos < 0 || pos.yPos >= W_HEIGHT) return false;
	if (pos.xPos < 0 || pos.xPos >= W_WIDTH) return false;
	return m_tiles[pos.yPos][pos.xPos]->CanGo();
}

int MapSession::GetCost(const Position pos)
{
	if (CanGo(pos) == false) return INT_MAX;
	return m_tiles[pos.yPos][pos.xPos]->GetCost();
}

bool MapSession::IsValidSection(int sectionY, int sectionX) 
{
	if (sectionY < 0 || sectionY >= W_HEIGHT / S_HEIGHT) return false;
	if (sectionX < 0 || sectionX >= W_WIDTH / S_WIDTH) return false;
	return true;
}

// Type - Player : 0, Npc : 1
void MapSession::ChangeSection(int type, int objId, Position prevPos, Position nextPos)
{
	pair<int, int> curr = GetSectionIndex(prevPos);
	pair<int, int> next = GetSectionIndex(nextPos);

	// �̵� �� ���� Section�� �����Ѵٸ� Return
	if (curr == next && prevPos.yPos != -1 && prevPos.xPos != -1)
		return;

	// �̵� �� Section�� ����Ǿ��ٸ� Section ���� ������Ʈ
	if (type == 0)
	{
		if(prevPos.yPos != -1 && prevPos.xPos != -1)
			m_sections[curr.first][curr.second]->DeletePlayer(objId);
		m_sections[next.first][next.second]->AddPlayer(objId);
	}
	else 
	{
		if (prevPos.yPos != -1 && prevPos.xPos != -1)
			m_sections[curr.first][curr.second]->DeleteNpc(objId);
		m_sections[next.first][next.second]->AddNpc(objId);
	}
}

pair<int, int> MapSession::GetSectionIndex(int yPos, int xPos) const
{
	return make_pair(yPos / S_HEIGHT, xPos / S_WIDTH);
}

pair<int, int> MapSession::GetSectionIndex(Position pos) const
{
	return GetSectionIndex(pos.yPos, pos.xPos);
}

void MapSession::GetCreatureInNearSection(ListType type, int sectionY, int sectionX, unordered_set<int>& nearList)
{
	// ���� ������ �������� 8���� ����
	int deltaX[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
	int deltaY[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
	for (int i = 0; i < 9; ++i)
	{
		int nearY = sectionY + deltaY[i];
		int nearX = sectionX + deltaX[i];
		if (IsValidSection(nearY, nearX) == false) continue;
		if (type == ListType::Player) 
			m_sections[nearY][nearX]->GetPlayerList(nearList);
		else {
			m_sections[nearY][nearX]->GetNpcList(nearList);
		}

	}
}

void MapSession::GetUserInNearSection(int sectionY, int sectionX, unordered_set<int>& nearList)
{
	GetCreatureInNearSection(ListType::Player, sectionY, sectionX, nearList);
}

void MapSession::GetUserInNearSection(Position pos, unordered_set<int>& nearList)
{
	pair<int, int> pair = GetSectionIndex(pos.yPos, pos.xPos);
	GetCreatureInNearSection(ListType::Player, pair.first, pair.second, nearList);
}

void MapSession::GetNpcInNearSection(int sectionY, int sectionX, unordered_set<int>& nearList)
{
	GetCreatureInNearSection(ListType::Npc, sectionY, sectionX, nearList);
}

void MapSession::GetNpcInNearSection(Position pos, unordered_set<int>& nearList)
{
	pair<int, int> pair = GetSectionIndex(pos.yPos, pos.xPos);
	GetCreatureInNearSection(ListType::Npc, pair.first, pair.second, nearList);
}
