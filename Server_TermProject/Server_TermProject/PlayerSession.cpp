#include "stdafx.h"
#include "Manager.h"
#include "NpcSession.h"
#include "GameManager.h";
#include "PlayerSession.h";
#include "NetworkManager.h";
#include "PlayerSocketHandler.h";

void PlayerSession::SetRandomPos()
{
	Manager& manager = Manager::GetInstance();
	int yPos, xPos;
	do
	{
		yPos = rand() % W_HEIGHT;
		xPos = rand() % W_WIDTH;
	} while (manager.GetGameManager()->CanGo(yPos, xPos) == false);
	SetPos(yPos, xPos);
}

void PlayerSession::Init()
{
}

void PlayerSession::AddViewNPCList(int objID)
{
	m_npcViewListLock.lock();
	m_npcViewList.insert(objID);
	m_npcViewListLock.unlock();
}

void PlayerSession::RemoveViewNPCList(int objID)
{
	m_npcViewListLock.lock();
	if (m_npcViewList.find(objID) != m_npcViewList.end())
		m_npcViewList.erase(objID);
	m_npcViewListLock.unlock();
}

void PlayerSession::UpdateViewList()
{
	C_STATE currState = m_state;
	if (currState != C_STATE::CT_INGAME) return;
	GameManager* gameManager = Manager::GetInstance().GetGameManager();
	PlayerSocketHandler* myNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(m_objectID);
	m_viewListLock.lock();
	auto prevViewList = m_viewList;
	auto prevNpcViewList = m_npcViewList;
	m_viewListLock.unlock();

	unordered_set<int> newViewList;
	unordered_set<int> newNpcViewList;

	for (int i = 0; i < MAX_USER; ++i)
	{
		PlayerSession* player = gameManager->GetPlayerSession(i);
		PlayerSocketHandler* pNetwork = Manager::GetInstance().GetNetworkManager()->GetPlayerNetwork(i);
		if (player == nullptr) continue;
		if (i == m_objectID) continue;

		C_STATE st = player->GetState();
		if (st != C_STATE::CT_INGAME) continue;
		
		if (CanSee(player))	// ���� �� �þ߿��� ���̸�
		{
			if (prevViewList.count(player->GetId()) == 0) // �������� �������� �߰�
			{
				player->AddViewList(m_objectID);
				pNetwork->send_add_object_packet(this);
				myNetwork->send_add_object_packet(player);
			}
			// �־����� �������ٴ� ���� �˷���
			pNetwork->send_move_object_packet(this);
			newViewList.emplace(i);
		}
		else	// �� �þ߿��� ������
		{
			if (prevViewList.count(i) != 0)	// �������� �־����� ����
			{
				player->RemoveViewList(m_objectID);
				pNetwork->send_remove_object_packet(this);
				myNetwork->send_remove_object_packet(player);
			}
		}
	}

	for (int i = 0; i < MAX_NPC; ++i)
	{
		NpcSession* npc = gameManager->GetNpcSession(i);
		if (npc == nullptr) continue;
		if (CanSee(npc))	// ���� �� �þ߿��� ���̸�
		{
			if (prevNpcViewList.count(i) == 0) // �������� �������� �߰�
			{
				npc->AddViewList(m_objectID);
				npc->SetTarget(m_objectID);
				myNetwork->send_add_npc_packet(npc);
			}
			newNpcViewList.emplace(i);
		}
		else	// �� �þ߿��� ������
		{
			if (prevNpcViewList.count(i) != 0)	// �������� �־����� ����
			{
				npc->RemoveViewList(m_objectID);
				myNetwork->send_remove_npc_object_packet(npc);
			}
		}
	}

	m_viewListLock.lock();
	m_viewList = newViewList;
	m_npcViewList = newNpcViewList;
	m_viewListLock.unlock();
}
