#include "stdafx.h"
#include "AgroNpc.h"
#include "Manager.h"
#include "GameManager.h"
#include "PlayerSession.h"

void AgroNpc::Roaming()
{
	NpcSession::Roaming();
	DetectTarget();
}

void AgroNpc::DetectTarget()
{
	// 인근 Section 내부에 있는 PlayerCharacter 중에서 NPC_VIEW_RANGE 내에 있는지 확인
	m_viewListLock.lock();
	auto viewList = m_viewList;
	m_viewListLock.unlock();
	int minDist = NPC_VIEW_RANGE;
	int targetID = -1;
	for (int i : viewList) 
	{
		PlayerSession* player = Manager::GetInstance().GetGameManager()->GetPlayerSession(i);
		if (player == nullptr) continue;
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