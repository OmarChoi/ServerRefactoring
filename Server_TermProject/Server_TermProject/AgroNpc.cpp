#include "stdafx.h"
#include "AgroNpc.h"

void AgroNpc::AddViewList(int objID)
{
	if (m_targetID == -1)
		SetTarget(objID);
	NpcSession::AddViewList(objID);
}

void AgroNpc::RemoveViewList(int objID)
{
	if (m_targetID == objID)
	{
		// ViewList ���� �ִ� �ٸ� Target ����

	}
	NpcSession::RemoveViewList(objID);
}