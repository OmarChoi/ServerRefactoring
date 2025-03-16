#pragma once
class Section
{
private:
	unordered_set<int>				m_playerList;
	mutex							playerListLock;

	unordered_set<int>				m_npcList;
	mutex							npcListLock;

public:
	void GetPlayerList(unordered_set<int>& list);
	void GetNpcList(unordered_set<int>& list);

	void AddPlayer(int i);
	void DeletePlayer(int i);

	void AddNpc(int i);
	void DeleteNpc(int i);
};

