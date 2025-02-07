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
#include <concurrent_unordered_set.h>
#include <concurrent_priority_queue.h>

#include "protocol_2023.h"
#include "include/lua.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

using namespace std;

constexpr int VIEW_RANGE = 8;

#define stressTest

enum COMP_TYPE 
{ 
	OP_ACCEPT, 
	OP_RECV, 
	OP_SEND, 
	OP_NPC_MOVE,
	OP_NPC_ATTACK,
	OP_NPC_RESPAWN,
	OP_SAVE_DATA
};

enum TIMER_TYPE 
{
	TT_MOVE,
	TT_ATTACK,
	TT_RESPAWN,
	TT_SAVE
};

enum MONSTER_TYPE
{
	MT_STAYING,
	MT_ROAMING
};

enum C_STATE 
{ 
	CT_FREE, 
	CT_ALLOC, 
	CT_INGAME 
};

enum STATE
{
	IDLE,
	MOVE,
	ATTACK,
	DIE,
};

enum OBJ_TYPE
{
	OT_PLAYER = 0,
	OT_NPC = MAX_USER
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

	bool operator == (const Position& other)
	{
		return yPos == other.yPos && xPos == other.xPos;
	}

	bool operator != (const Position& other)
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

const array<Position, MOVE_DIRECTION> movements = {
	Position {-1, 0},		// UP 
	Position {0, 1},		// RIGHT,
	Position {1, 0},		// Down
	Position {0, -1}		// LEFT
};

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
		m_compType = OP_RECV;
		ZeroMemory(&m_over, sizeof(m_over));
	}
	OVER_EXP(char* packet)
	{
		m_wsabuf.len = *(reinterpret_cast<unsigned short*>(packet));;
		m_wsabuf.buf = m_sendBuf;
		ZeroMemory(&m_over, sizeof(m_over));
		m_compType = OP_SEND;
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
