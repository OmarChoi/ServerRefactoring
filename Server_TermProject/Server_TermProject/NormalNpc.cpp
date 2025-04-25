#include "stdafx.h"
#include "NormalNpc.h"

void NormalNpc::ApplyDamage(int damage, int objId)
{
	auto target = m_targetSession.load();
	SetTarget(objId);
	Creature::ApplyDamage(damage, objId);
}

void NormalNpc::RemoveViewList(int objID)
{
	auto target = m_targetSession.load();
	if (target != nullptr)
		ReleaseTarget();
	NpcSession::RemoveViewList(objID);
}
