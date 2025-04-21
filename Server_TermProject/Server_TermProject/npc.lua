MonsterType = {
    Unknown = 0,
    Slime = 1,
    Goblin = 2,
    Orc = 3
}

MonsterBehavior = {
    Normal = 0,
    Agro = 1,
}

MonsterState = {
	IDLE = 0,
	Roaming = 1,
	Chase = 2,
	Attack = 3,
	Die = 4,
};

MonsterInfo = {
    [MonsterType.Unknown] = {
        type = MonsterType.Unknown, behavior = MonsterBehavior.Normal, 
        level = -1, hp = -1.0, damage = -1, attackRange = -1, speed = -1
    },
    [MonsterType.Slime] = {
        type = MonsterType.Slime, behavior = MonsterBehavior.Normal, level = 1,
        hp = 100.0, damage = 10, attackRange = 1, speed = 0.8
    },
    [MonsterType.Goblin] = {
        type = MonsterType.Goblin, behavior = MonsterBehavior.Agro, level = 10,
        hp = 200.0, damage = 20, attackRange = 1, speed = 1.2
    },
    [MonsterType.Orc] = {
        type = MonsterType.Orc, behavior = MonsterBehavior.Agro, level = 20,
        hp = 300.0, damage = 30, attackRange = 2, speed = 1.0
    }
}

function GetMonsterInfo(monsterType)
    return MonsterInfo[monsterType] or MonsterInfo[MonsterType.Unknown]
end

function ChangeState(state, hp, hasTarget, attackRange, targetDist)
    if hp <= 0.0 then
        return MonsterState.Die
    end

    if state == MonsterState.IDLE then
        return MonsterState.Roaming

    elseif state == MonsterState.Roaming and hasTarget then
        return MonsterState.Chase

    elseif state == MonsterState.Chase then
        if not hasTarget then
            return MonsterState.Roaming
        elseif targetDist <= attackRange then
            return MonsterState.Attack
        end

    elseif state == MonsterState.Attack then
        if not hasTarget then
            return MonsterState.Roaming
        elseif targetDist > attackRange then
            return MonsterState.Chase
        end
    end

    return state
end