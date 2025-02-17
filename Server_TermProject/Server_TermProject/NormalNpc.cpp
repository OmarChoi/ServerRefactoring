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
		// ViewList 내에 있는 다른 Target 설정
		ReleaseTarget();
	}
	NpcSession::RemoveViewList(objID);
}
