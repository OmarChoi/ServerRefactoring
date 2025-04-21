#pragma once
#include "NpcSession.h"

class AgroNpc : public NpcSession
{
private:
	void Roaming() override;
	void DetectTarget();
};
