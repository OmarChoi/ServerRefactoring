#include "stdafx.h"
#include "NormalNpc.h"

void NormalNpc::ApplyDamage(int damage, int objId)
{
	if (m_targetID == -1 && objId != -1)
		m_targetID = objId;

	Creature::ApplyDamage(damage, objId);
}

void NormalNpc::RemoveViewList(int objID)
{
	if (m_targetID == objID)
	{
		// ViewList ���� �ִ� �ٸ� Target ����
		ReleaseTarget();
	}
	NpcSession::RemoveViewList(objID);
}
