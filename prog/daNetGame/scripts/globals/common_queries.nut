from "%sqstd/ecs.nut" import *
let {TEAM_UNASSIGNED} = require("team")

let find_human_player_by_connidQuery = SqQuery("find_human_player_by_connidQuery", {comps_ro=[["connid", TYPE_INT]], comps_rq=["player"], comps_no=["playerIsBot"]})
function find_human_player_by_connid(filter_connid){
  return find_human_player_by_connidQuery.perform(@(eid, _comp) eid, "eq(connid,{0})".subst(filter_connid)) ?? INVALID_ENTITY_ID
}

let find_player_that_possessQuery = SqQuery("find_player_that_possessQuery", {comps_ro=[["possessed", TYPE_EID],["disconnected", TYPE_BOOL]], comps_rq = ["player"]})

function find_any_player_that_possess(possessed_eid){
  if (possessed_eid == INVALID_ENTITY_ID)
    return INVALID_ENTITY_ID
  return find_player_that_possessQuery.perform(@(eid, _comp) eid, "eq(possessed,{0}:eid)".subst(possessed_eid)) ?? INVALID_ENTITY_ID
}

function find_connected_player_that_possess(possessed_eid){
  if (possessed_eid == INVALID_ENTITY_ID)
    return INVALID_ENTITY_ID
  return find_player_that_possessQuery.perform(@(eid, _comp) eid, "and(eq(possessed,{0}:eid),eq(disconnected,false))".subst(possessed_eid)) ?? INVALID_ENTITY_ID
}

let find_local_player_compsQuery = SqQuery("find_local_player_compsQuery", {comps_ro=[["is_local", TYPE_BOOL]], comps_rq = ["player"]}, "is_local")
function find_local_player(){
  return find_local_player_compsQuery.perform(@(eid, _comp) eid) ?? INVALID_ENTITY_ID
}

let get_controlledHeroQuery = SqQuery("get_controlled_heroQuery",  {comps_ro=[["possessed", TYPE_EID],["is_local", TYPE_BOOL]], comps_rq=["player"]}, "is_local")
function get_controlled_hero(){
  return get_controlledHeroQuery.perform(@(_eid, comp) comp["possessed"]) ?? INVALID_ENTITY_ID
}

let get_watchedHeroQuery = SqQuery("get_watched_heroQuery",  {comps_ro=[["watchedByPlr", TYPE_EID]]})
function get_watched_hero(){
  let local_player = find_local_player()
  return get_watchedHeroQuery.perform(@(eid, _comp) eid, $"eq(watchedByPlr,{local_player}:eid)") ?? INVALID_ENTITY_ID
}


let get_teamQuery = SqQuery("get_teamQuery",  {comps_ro = [["team__id", TYPE_INT]]})
function get_team_eid(team_id){
  return get_teamQuery.perform(@(eid, _comp) eid, "eq(team__id,{0})".subst(team_id)) ?? INVALID_ENTITY_ID
}

let find_local_player_team_Query = SqQuery("find_local_player_team_Query", {comps_ro=[["is_local", TYPE_BOOL],["team", TYPE_INT]], comps_rq = ["player"]}, "is_local")
function get_local_player_team(){
  return find_local_player_team_Query.perform(@(_eid, comp) comp["team"])
}

let get_alive_teams_Query = SqQuery("get_alive_teams_Query", {comps_ro=[["countAsAlive", TYPE_BOOL],["isAlive", TYPE_BOOL],["team", TYPE_INT]]})
function get_alive_teams(){
  let teams = []
  get_alive_teams_Query.perform(function(_eid, comp){
    if (comp.countAsAlive && comp.isAlive && !teams.contains(comp.team))
      teams.append(comp.team)
  })
  return teams
}

let get_heroTeamQuery = SqQuery("get_heroTeamQuery",  {comps_ro = [["team", TYPE_INT]]})
let get_alive_teammates_count_Query = SqQuery("get_alive_teammates_count_Query", {comps_ro=[["countAsAlive", TYPE_BOOL],["isAlive", TYPE_BOOL],["team", TYPE_INT]]})
function get_alive_teammates_count(hero_eid){
  local result = 0
  let teamId = get_heroTeamQuery.perform(hero_eid, @(_eid, comp) comp.team) ?? TEAM_UNASSIGNED
  get_alive_teammates_count_Query.perform(function(_eid, comp){
    if (teamId == comp.team && comp.countAsAlive && comp.isAlive)
      result += 1
  })
  return result
}

return {
  find_human_player_by_connid
  find_any_player_that_possess
  find_connected_player_that_possess
  find_local_player
  get_controlled_hero
  get_team_eid
  get_watched_hero
  get_local_player_team
  get_alive_teams
  get_alive_teammates_count,
}
