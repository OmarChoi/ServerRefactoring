#pragma once
class NpcSession;
class NpcFactory
{
private:
    static std::array<function<std::shared_ptr<NpcSession>()>, 
        static_cast<size_t>(Monster::Behavior::Cnt)> npcConstructors;
public:
    static void InitNpcFactory();
    static void Register(Monster::Behavior behavior, function<std::shared_ptr<NpcSession>()> creator);
    static shared_ptr<NpcSession> CreateNpc(Monster::Type type);
};

