#pragma once
#include "Creature.h"
class NpcSession : public Creature
{
protected:
    Monster::Type                       m_type;
    Monster::Behavior                   m_behavior;

protected:
    atomic_int                          m_targetID;
    atomic<Monster::State>              m_currentState;
    Position                            m_spawnPos;

    chrono::system_clock::time_point    m_makePathTime;
    stack<Position>                     m_path;
    mutex                               m_pathLock;

private:
    array<function<void()>, static_cast<size_t>(Monster::State::Cnt)> stateFunc;

public:
    NpcSession();
    virtual ~NpcSession();

public:
    void AddViewList(int objID) override;
    void RemoveViewList(int objID) override;
    void SetPos(int y, int x) override;
    void SetPos(Position pos) override { SetPos(pos.yPos, pos.xPos); }

protected:
    void UpdateViewList() override;
    bool CanSee(const Creature* other) override;
    virtual bool CheckTarget();

public:
    Monster::Type GetType() { return m_type; }
    Monster::Behavior GetBehaviorType() { return m_behavior; }
    void SetType(Monster::Type type) { m_type = type; }
    void SetTarget(int objId);

public:
    void ActiveNpc();
    void InitPosition(Position pos);
    void ReleaseTarget();
    
public:
    void SetInfo(int i);
    void Update();

private:
    void ChangeState();
    void Attack();
    void CreatePath();
    void ChaseTarget();
    void DeActiveNpc();
    void Die() override;

protected:
    virtual void Roaming();
    virtual void RespawnObject() override;
};
