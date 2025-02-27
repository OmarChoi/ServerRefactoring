#pragma once
#include "NpcSession.h"

class AgroNpc : public NpcSession
{
public:
	void AddViewList(int objID) override;
	void RemoveViewList(int objID) override;
	void RespawnObject() override;

private:
	void SetNearTarget();
	void CheckTarget() override;
};

