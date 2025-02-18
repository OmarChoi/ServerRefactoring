#pragma once
#include "NpcSession.h"

class NormalNpc : public NpcSession
{
public:
	void ApplyDamage(int damage, int objId = -1) override;
	void RemoveViewList(int objID) override;
};

