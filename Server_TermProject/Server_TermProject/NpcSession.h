#pragma once
#include "Creature.h"
class NpcSession : public Creature
{
protected:
    int                                 m_targetID = -1;
    MonsterType                         m_type;
    MonsterBehavior                     m_behavior;
    int                                 m_attackRange;
    int                                 m_damage;
    Position                            m_spawnPos;
    chrono::system_clock::time_point    m_MakePathTime;
    stack<Position>                     m_path;

public:
    NpcSession();
    void AddViewList(int objID) override;
    void RemoveViewList(int objID) override;

protected:
    void UpdateViewList() override;
    bool CanSee(const Creature* other) override;
public:
    MonsterType GetType() { return m_type; }
    MonsterBehavior GetBehaviorType() { return m_behavior; }
    void SetType(MonsterType type) { m_type = type; }
    void SetTarget(int objId);

public:
    void ActiveNpc();
    void InitPosition(Position pos);
    void ReleaseTarget();
    void Respawn();

private:
    // Lua ����
    sol::state                          lua;
    mutex                               luaMutex;

    void InitLua();

public:
    void MoveInfo(NpcSession&& other);
    void SetInfoByLua();

    void Update();
private:
    void Attack();
    void CreatePath();
    void ChaseTarget();
    void MoveRandom();
    void DeActiveNpc();
    void Die() override;
    void RespawnObject() override;
};


class NpcFactory
{
public:
	static NpcSession* CreateNpc(MonsterType type);
};
