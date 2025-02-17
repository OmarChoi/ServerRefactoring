#include "stdafx.h"
#include "Creature.h"

Creature::Creature() : m_pos(-1, -1), m_hp(0.0f), m_maxHp(0.0f), m_objectID(-1),
m_lastMoveTime(std::chrono::high_resolution_clock::time_point::min()), m_lastAttackTime(std::chrono::high_resolution_clock::time_point::min())
{
}

void Creature::SetPos(int y, int x)
{
	// Ŭ���� ���� �ð� - ���� �ð��� 1�� �̸��� ��, �ʱ� ���� �������� �ʴ� ������ �־ ���� ����
	if (m_pos != Position{ -1, -1 }) 
	{
		auto currTime = std::chrono::high_resolution_clock::now();
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_lastMoveTime);

		// 1�ʿ� ��ĭ���� �̵��� �� �ְ� ����
		if (durationMs < 1'000ms) return;
	}
	m_lastMoveTime = chrono::high_resolution_clock::now();
	m_pos.yPos = y; m_pos.xPos = x;
}

void Creature::AddViewList(int objID)
{
	m_viewListLock.lock();
	m_viewList.emplace(objID);
	m_viewListLock.unlock();
}

void Creature::RemoveViewList(int objID)
{
	m_viewListLock.lock();
	if (m_viewList.find(objID) != m_viewList.end())
		m_viewList.erase(objID);
	m_viewListLock.unlock();
}

bool Creature::CanSee(const Creature* other)
{
	if (abs(other->GetPos().xPos - m_pos.xPos) > VIEW_RANGE) return false;
	return abs(other->GetPos().yPos - m_pos.yPos) < VIEW_RANGE;
}
