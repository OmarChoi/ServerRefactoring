myid = 99999;
move_count = 0;

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

MonsterInfo = {
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

function GetDistance(x1, y1, x2, y2)
    local dx = x1 - x2
    local dy = y1 - y2
    return math.sqrt(dx * dx + dy * dy)
end

function GetMonsterInfo(monsterType)
    return MonsterInfo[monsterType] 
        or { type = MonsterType.Unknown, behavior = MonsterBehavior.Normal, 
        level = -1, hp = -1.0, damage = -1, attackRange = -1, speed = -1 }
end

function OnUpdate(hasTarget, distance, attackRange)
    if hasTarget then
        if distance <= attackRange then     
            Attack()
        else
            ChaseTarget()
        end
    else
        MoveRandom()
    end
end