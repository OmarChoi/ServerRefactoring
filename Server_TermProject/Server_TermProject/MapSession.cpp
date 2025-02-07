#include "stdafx.h"
#include "Tile.h"
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
	// 0 : 장애물
	// 1 : 갈 수 있는 지역
	int x = 0, y = 0;
	char tileType = ' ';
	while (!mapFile.eof())
	{
		mapFile >> tileType;
		// Lua Script를 통해 타일에 맞는 데이터를 가져올 수 있는지 확인해볼 필요가 있다.
		if (tileType == '1')	// 갈 수 있는 지역
			m_tiles[y][x++] = new Tile(1, 1, 1);
		else
			m_tiles[y][x++] = new Tile(0, 0, 9999);

		if (x == W_WIDTH)
		{
			y += 1;
			x = 0;
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
