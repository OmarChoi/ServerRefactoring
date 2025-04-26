#include "stdafx.h"
#include "Section.h"

void Section::GetPlayerList(unordered_set<int>& playerList)
{
	lock_guard<mutex> lock(playerListLock);
	playerList.insert(m_playerList.begin(), m_playerList.end());
}

void Section::GetNpcList(unordered_set<int>& npcList)
{
	lock_guard<mutex> lock(npcListLock);
	npcList.insert(m_npcList.begin(), m_npcList.end());
}

void Section::AddPlayer(int i) 
{
	lock_guard<mutex> lock(playerListLock);
	m_playerList.emplace(i);
}

void Section::DeletePlayer(int i)
{
	lock_guard<mutex> lock(playerListLock);
	auto it = find(m_playerList.begin(), m_playerList.end(), i);
	if (it != m_playerList.end())
		m_playerList.erase(it);
}

void Section::AddNpc(int i)
{
	lock_guard<mutex> lock(npcListLock);
	m_npcList.emplace(i);
}

void Section::DeleteNpc(int i)
{
	lock_guard<mutex> lock(npcListLock);
	auto it = find(m_npcList.begin(), m_npcList.end(), i);
	if (it != m_npcList.end())
		m_npcList.erase(it);
}