#include "stdafx.h"
#include "NormalNpc.h"

void NormalNpc::GetDamage(int objId, int damage)
{
	if (m_targetID == -1) 
		m_targetID = objId;

	NpcSession::GetDamage(objId, damage);
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
