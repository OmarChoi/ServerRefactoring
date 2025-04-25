#include "stdafx.h"
#include "AgroNpc.h"
#include "Manager.h"
#include "GameManager.h"
#include "PlayerSession.h"

void AgroNpc::Roaming()
{
	DetectTarget();
	NpcSession::Roaming();
}

void AgroNpc::DetectTarget()
{
	m_viewListLock.lock();
	auto viewList = m_viewList;
	m_viewListLock.unlock();
	int minDist = NPC_VIEW_RANGE;
	int targetID = -1;
	for (int i : viewList) 
	{
		auto player = Manager::GetInstance().GetGameManager()->GetPlayerSession(i);
		if (player == nullptr) continue;
		if (player->IsInGame() == false) continue;
		int dist = Utils::GetDist(m_pos, player->GetPos());
		if (dist < minDist)
		{
			minDist = dist;
			targetID = i;
		}
	}

	if (targetID != -1)
		SetTarget(targetID);
}