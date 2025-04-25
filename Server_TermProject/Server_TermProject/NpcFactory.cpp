#include "stdafx.h"
#include "AgroNpc.h"
#include "NormalNpc.h"
#include "NpcSession.h"
#include "NpcFactory.h"

std::array<function<std::shared_ptr<NpcSession>()>, static_cast<size_t>(Monster::Behavior::Cnt)> NpcFactory::npcConstructors;
void NpcFactory::InitNpcFactory()
{
    Register(Monster::Behavior::Normal, [] { return std::make_shared<NormalNpc>(); });
    Register(Monster::Behavior::Agro, [] { return std::make_shared<AgroNpc>(); });
}

void NpcFactory::Register(Monster::Behavior behavior, function<std::shared_ptr<NpcSession>()> creator)
{
    npcConstructors[static_cast<size_t>(behavior)] = std::move(creator);
}

shared_ptr<NpcSession> NpcFactory::CreateNpc(Monster::Type type)
{
    auto behavior = Monster::InfoTable[static_cast<int>(type)].behavior;
    auto& ctor = npcConstructors[static_cast<size_t>(behavior)];

    auto npc = ctor();  // shared_ptr<NpcSession>
    if (!npc) {
        std::cerr << "[NpcFactory] Constructor returned nullptr\n";
        return nullptr;
    }

    // SetInfo 의 인자는 int 이므로 캐스트
    npc->SetInfo(static_cast<int>(type));
    return npc;
}