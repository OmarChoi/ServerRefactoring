#include "stdafx.h"
#include "Creature.h"

Creature::Creature() : m_pos(-1, -1), m_hp(0.0f), m_maxHp(0.0f), m_objectID(-1), 
m_damage(100), m_bActive(false), m_attackRange(1), m_speed(1.0), m_level(1),
m_lastMoveTime(std::chrono::high_resolution_clock::now()), m_lastAttackTime(std::chrono::high_resolution_clock::now())
{
}

Creature::~Creature()
{
	m_viewListLock.lock();
	m_viewList.clear();
	m_viewListLock.unlock();
}

void Creature::SetPos(int y, int x)
{
	// 클래스 생성 시간 - 현재 시간이 1초 미만일 때, 초기 값이 설정되지 않는 문제가 있어서 예외 설정
	if (m_pos != Position{ -1, -1 }) 
	{
		auto currTime = std::chrono::high_resolution_clock::now();
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_lastMoveTime);

		// 1초에 한칸씩만 이동할 수 있게 설정
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

bool Creature::IsActive()
{
	std::lock_guard<std::mutex> lock(activeMutex);
	return m_bActive;
}

void Creature::SetActive(bool active)
{
	// 초기화하는 경우 체력을 정상적으로 설정해주고 활성화 함수 호출.
	if (m_hp < FLT_EPSILON && active == true) return;
	std::lock_guard<std::mutex> lock(activeMutex);
	m_bActive = active;
}

void Creature::ApplyDamage(int damage, int objId)
{
	m_hp = std::max(0.0f, m_hp - damage);
	if (m_hp < FLT_EPSILON)
		Die();
}

void Creature::Die()
{
	SetActive(false);
}

void Creature::RespawnObject()
{
	m_hp = m_maxHp;
	SetActive(true);
	UpdateViewList();
}