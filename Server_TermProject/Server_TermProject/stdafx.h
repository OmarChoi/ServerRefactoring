#pragma once
#define NOMINMAX
#include <iostream>
#include <array>
#include <map>
#include <unordered_map>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_set>
#include <chrono>
#include <queue>
#include <string>
#include <sqlext.h>  
#include <locale.h>
#include <fstream>
#include <stack>
#include <random>
#include <algorithm>
#include <shared_mutex>
#include <concurrent_unordered_set.h>
#include <concurrent_priority_queue.h>

#include "protocol_2023.h"
#include "include/lua.hpp"

// Sol2 is licensed under the MIT License
#include "include/sol/sol.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

using namespace std;

constexpr int VIEW_RANGE = 8;
constexpr int NPC_VIEW_RANGE = 10;
constexpr int SafeZoneSize = 10;
inline thread_local std::mt19937 rng(std::random_device{}());

inline mutex PrintLock;
#define stressTest

enum class COMP_TYPE
{ 
	Accept, 
	Recv, 
	Send,
	NpcUpdate,
	RespawnObject,
	UpdatePlayerInfo,
	SaveData
};

enum class TIMER_TYPE
{
	NpcUpdate,
	PlayerUpdate,
	RespawnObject,
	SaveData
};

enum class PlayerState 
{ 
	CT_FREE, 
	CT_ALLOC, 
	CT_INGAME 
};

enum class ObjectType
{
	Player = 0,
	Npc = 1
};

enum CHAT_TYPE : char
{
	BATTLE_MESSAGE,
	CHATTING_MESSAGE
};

struct Position
{
	int yPos;
	int xPos;

	Position() : yPos(0), xPos(0) {};
	Position(int y, int x) : yPos(y), xPos(x) {};

	bool operator == (const Position& other) const
	{
		return yPos == other.yPos && xPos == other.xPos;
	}

	bool operator != (const Position& other) const
	{
		return !(*this == other);
	}

	Position operator + (const Position& other) 
	{
		Position sumPos;
		sumPos.yPos = yPos + other.yPos;
		sumPos.xPos = xPos + other.xPos;
		return sumPos;
	}

	Position& operator += (const Position& other)
	{
		yPos = yPos + other.yPos;
		xPos = xPos + other.xPos;		
		return *this;
	}

	bool operator < (const Position& other) const
	{
		if (yPos == other.yPos)
			return xPos < other.xPos;
		return yPos < other.yPos;
	}
};

namespace std {
	template<>
	struct hash<Position>
	{
		size_t operator()(const Position& p) const
		{
			return (hash<int>()(p.yPos) << 31) ^ hash<int>()(p.xPos);
		}
	};
}

const array<Position, MOVE_DIRECTION> movements = 
{
	Position {-1, 0},		// UP 
	Position {0, 1},		// RIGHT,
	Position {1, 0},		// Down
	Position {0, -1}		// LEFT
};

namespace Monster
{
	enum class Type
	{
		Unknown = 0,
		Slime = 1,
		Goblin = 2,
		Orc = 3,
		Cnt
	};

	enum class Behavior
	{
		Normal = 0,
		Agro = 1,
		Cnt
	};

	enum class State
	{
		Idle = 0,
		Roaming = 1,
		Chase = 2,
		Attack = 3,
		Die = 4,
		Cnt = 5
	};

	struct Info
	{
		Type type;
		Behavior behavior;
		int level;
		float hp;
		int damage;
		float attackRange;
		float speed;
	};

	inline array<Info, static_cast<size_t>(Type::Cnt)> InfoTable =
	{
		Info{ Type::Unknown, Behavior::Normal, -1, -1.0f, -1, -1.0f, -1.0f },
		Info{ Type::Slime,   Behavior::Normal, 1,  100.0f, 1, 1.0f,  0.8f },
		Info{ Type::Goblin,  Behavior::Agro,   10, 200.0f, 2, 1.0f,  1.2f },
		Info{ Type::Orc,     Behavior::Agro,   20, 300.0f, 5, 2.0f,  1.0f }
	};
}

class OVER_EXP {
public:
	WSAOVERLAPPED m_over;
	WSABUF m_wsabuf;
	char m_sendBuf[BUF_SIZE];
	COMP_TYPE m_compType;
	OVER_EXP()
	{
		m_wsabuf.len = BUF_SIZE;
		m_wsabuf.buf = m_sendBuf;
		m_compType = COMP_TYPE::Recv;
		ZeroMemory(&m_over, sizeof(m_over));
	}
	OVER_EXP(char* packet)
	{
		m_wsabuf.len = *(reinterpret_cast<unsigned short*>(packet));;
		m_wsabuf.buf = m_sendBuf;
		ZeroMemory(&m_over, sizeof(m_over));
		m_compType = COMP_TYPE::Send;
		memcpy(m_sendBuf, packet, *(reinterpret_cast<unsigned short*>(packet)));
	}
};

struct TimerEvent
{
	int m_objId;
	chrono::system_clock::time_point m_execTime;
	TIMER_TYPE m_Type;
	constexpr bool operator < (const TimerEvent& lValue) const
	{
		return (m_execTime > lValue.m_execTime);
	}
};

namespace Utils
{
	inline int GetDist(Position startPos, Position destPos)
	{
		// 단순히 가로 거리 + 세로거리
		return abs(destPos.yPos - startPos.yPos) + abs(destPos.xPos - startPos.xPos);
	}

	inline string GetMonsterName(Monster::Type type)
	{
		if (type == Monster::Type::Slime) return "Slime";
		else if (type == Monster::Type::Goblin) return "Goblin";
		else if (type == Monster::Type::Orc) return "Orc";
		else return "UnKnown";
	}
}