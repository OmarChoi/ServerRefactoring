#include "stdafx.h"
#include "AgroNpc.h"
#include "GameManager.h"

void AgroNpc::AddViewList(int objID)
{
	if (m_targetID == -1)
		SetTarget(objID);
	NpcSession::AddViewList(objID);
}

void AgroNpc::RemoveViewList(int objID)
{
	NpcSession::RemoveViewList(objID);
	if (m_targetID == objID)
		SetNearTarget();
}

void AgroNpc::SetNearTarget()
{
	m_viewListLock.lock();
	auto viewList = m_viewList;
	m_viewListLock.unlock();
	if (!viewList.empty())
		SetTarget(*m_viewList.begin());
}

void AgroNpc::CheckTarget()
{
	NpcSession::CheckTarget();
	if (m_targetID == -1) 
		SetNearTarget();
}

void AgroNpc::RespawnObject()
{
	NpcSession::RespawnObject();
	SetNearTarget();
}