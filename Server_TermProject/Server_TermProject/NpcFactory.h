#pragma once
class NpcSession;
class NpcFactory
{
public:
    static void InitNpcFactory();
    static void Register(Monster::Behavior behavior, function<NpcSession* ()> creator);
    static NpcSession* CreateNpc(Monster::Type type); 
};

