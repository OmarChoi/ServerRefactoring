#include "stdafx.h"
#include "Creature.h"

Creature::Creature() : m_pos(0, 0), m_hp(0.0f), m_maxHp(0.0f), m_objectID(-1),
m_tpLastMoveTime(chrono::system_clock::now()), m_tpLastAttackTime(chrono::system_clock::now())
{
}
