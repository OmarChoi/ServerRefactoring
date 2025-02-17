#pragma once
#include "NpcSession.h"

class NormalNpc : public NpcSession
{
public:
	void GetDamage(int objId, int damage) override;
	void RemoveViewList(int objID) override;
};

