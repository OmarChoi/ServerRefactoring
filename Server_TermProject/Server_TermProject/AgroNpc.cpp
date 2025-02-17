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
		// ViewList 내에 있는 다른 Target 설정

	}
	NpcSession::RemoveViewList(objID);
}