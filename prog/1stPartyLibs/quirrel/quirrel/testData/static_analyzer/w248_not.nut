function foo(_p) {}
let soldiers = {}
let debriefing = foo(1)
let squad = foo(2)
let squads = foo(3)

foreach (_soldierIdx, soldierStat in soldiers) {
    let soldierData = debriefing?.soldiers.items[soldierStat.soldierId]
    if (!soldierData)
        continue
    if (squads?[soldierData.squadId] != squad)
        continue

    foo(soldierData.guid)
}