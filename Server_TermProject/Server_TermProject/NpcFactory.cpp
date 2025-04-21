#include "stdafx.h"
#include "AgroNpc.h"
#include "NormalNpc.h"
#include "NpcSession.h"
#include "NpcFactory.h"

static array<function<NpcSession* ()>, static_cast<size_t>(Monster::Behavior::Cnt)> npcConstructors;

void NpcFactory::InitNpcFactory()
{
    NpcFactory::Register(Monster::Behavior::Normal, []() { return new NormalNpc(); });
    NpcFactory::Register(Monster::Behavior::Agro, []() { return new AgroNpc(); });
}

void NpcFactory::Register(Monster::Behavior behavior, function<NpcSession*()> creator)
{
    npcConstructors[static_cast<size_t>(behavior)] = std::move(creator);
}

NpcSession* NpcFactory::CreateNpc(Monster::Type type)
{
    int typeIdx = static_cast<int>(type);
    NpcSession* npc = npcConstructors[static_cast<size_t>(Monster::InfoTable[typeIdx].behavior)]();
    if (npc == nullptr) 
    {
        cout << "Error : Npc Constructer Doesn't Work\n";
        return npc;
    }
    npc->SetInfo(typeIdx);
    return npc;
}
