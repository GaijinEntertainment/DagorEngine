require ecs
require app
require PhysObj
require RendInst
require math.base
require DagorMath
require DngNet
require DngPhysObj
require Dacoll
require PhysDecl

[cpp_event(unicast)]
struct CmdRendinstDamage
  handle : riex_handle_t
  damage : float
  pos : float3
  impulse : float3
  time : float

[cpp_event(unicast)]
struct EventShellExplosionShockWave
  position : float3
  dmg_radius : float
  dmg_hp : float

[es(tag=gameClient, REQUIRE=eid)]
def rendinst_visual_phys_move_by_projectile(evt : CmdRendinstDamage)
  let riEid = find_ri_extra_eid(evt.handle)
  query(riEid) <| $ [es(REQUIRE=rendinst_phys_move__enabled)] (transform : float3x4; var phys_obj_net_phys : PhysObjActor&)
    let itm = inverse(transform)
    let localPos = itm * evt.pos - phys_obj_net_phys.phys.centerOfMass
    assume mass = phys_obj_net_phys.phys.mass
    let impulse = evt.impulse * safeinv(mass)
    assume moi = phys_obj_net_phys.phys.momentOfInertia
    let invMoi = float3(safeinv(moi.x), safeinv(moi.y), safeinv(moi.z)) * safeinv(mass)
    let impulseMoment = cross(localPos, rotate(itm, impulse) * invMoi)
    phys_obj_net_phys.phys.currentState.velocity += impulse
    phys_obj_net_phys.phys.currentState.omega += impulseMoment
    phys_obj_net_phys.phys |> wakeUp()
  query(riEid) <| $ [es] (var phys_body : PhysBody)
    var mass = 0f
    var moi = float3(0f, 0f, 0f)
    phys_body_getMassMatrix(phys_body, mass, moi.x, moi.y, moi.z)
    let impulse = evt.impulse * safeinv(mass)
    let vel = phys_body_getVelocity(phys_body) + impulse
    phys_body_setVelocity(phys_body, vel, true)


[es(tag=gameClient, REQUIRE=msg_sink)]
def rendinst_visual_phys_move_by_explosion(evt : EventShellExplosionShockWave)
  query() <| $ [es(REQUIRE=rendinst_phys_move__enabled)] (transform : float3x4; var phys_obj_net_phys : PhysObjActor&)
    let distSq = distance_sq(evt.position, transform[3])
    if distSq < evt.dmg_radius * evt.dmg_radius
      phys_obj_net_phys.phys |> wakeUp()
      let dir = normalize(evt.position - transform[3])
      let impulse = -dir * evt.dmg_hp * 20.f * (1.f - distSq / square(evt.dmg_radius))
      phys_obj_net_phys.phys.currentState.velocity += impulse
      phys_obj_net_phys.phys |> wakeUp()
  query() <| $ [es] (transform : float3x4; var phys_body : PhysBody)
    let distSq = distance_sq(evt.position, transform[3])
    if distSq < evt.dmg_radius * evt.dmg_radius
      var mass = 0f
      var moi = float3(0f, 0f, 0f)
      phys_body_getMassMatrix(phys_body, mass, moi.x, moi.y, moi.z)
      let dir = normalize(evt.position - transform[3])
      let impulse = -dir * evt.dmg_hp * 20.f * (1.f - distSq / square(evt.dmg_radius))
      let vel = phys_body_getVelocity(phys_body) + impulse
      phys_body_setVelocity(phys_body, vel, true)


def find_index(eid_list : EidList; eid : EntityId) : int
  for i in range(length(eid_list))
    if eid_list[i] == eid
      return i
  return -1

def process_riextra(ri_name, template_name : string;
                    eid : EntityId;
                    var phys_moved_rendinst__riList : EidList;
                    riHandles : array<riex_handle_t>;
                    box : bbox3f)
  let resIdx = get_rigen_extra_res_idx(ri_name)
  if resIdx < 0
    return

  // add new
  getRiGenExtraInstances(resIdx, box) <| $(handles : array<riex_handle_t>#)
    for handle in handles
      let idx = riHandles |> find_index(handle)
      if idx >= 0 // we already have phys_moved_rendinst in our list
        continue
      // find if this ri is already awake or if it's a moved rendinst (asleep)
      let riEid = find_ri_extra_eid(handle)
      var isAlreadyActive = false
      query(riEid) <| $ [es] (var rendinst_phys_move__heroList : EidList)
        push(rendinst_phys_move__heroList, eid)
        push(phys_moved_rendinst__riList, riEid)
        isAlreadyActive = true

      if isAlreadyActive
        continue

      let desc = RendInstDesc(handle)
      enable_disable_ri_instance(desc, false)

      // see if it's asleep so we can wake it
      query(riEid) <| $ [es(REQUIRE=rendinst_phys_move__asleep)] ()
        destroyEntity(riEid)

      // otherwise create a new one
      var tm : float3x4
      get_rigen_extra_matrix(handle, tm)
      let newRiEid = createEntity(template_name) <| $(var init)
        using() <| $(var heroList : EidList)
          push(heroList, eid)
          set(init, "rendinst_phys_move__heroList", heroList)
        init |> set("transform", tm)
        init |> set("initialTransform", tm)
        init |> set("ri_extra__handle", handle)
      push(phys_moved_rendinst__riList, newRiEid)


def prepare_to_become_moved(ri_eid : EntityId;
                            wait_time : float)
  query(ri_eid) <| $ [es] (var rendinst_phys_move__becomeMovedAt : float?)
    let becomeMovedAt = get_sync_time() + wait_time
    if rendinst_phys_move__becomeMovedAt != null
      *rendinst_phys_move__becomeMovedAt = becomeMovedAt
    else
      addSubTemplate(ri_eid, "rendinst_phys_move_waiting_to_become_moved") <| $ [es] (var init : ecs::ComponentsInitializer)
        init |> set("rendinst_phys_move__becomeMovedAt", becomeMovedAt)


def gather_and_update_ri_handles(hero_eid : EntityId;
                                 phys_moved_rendinst_hero__checkRad : float;
                                 phys_moved_rendinst_hero__lastCheckPos : float3;
                                 var phys_moved_rendinst__riList : EidList) : array<riex_handle_t>
  var riHandles : array<riex_handle_t>

  // check old ones
  let listLen = length(phys_moved_rendinst__riList)
  for i in range(listLen)
    let idx = listLen - i - 1 // iterate from the tail to make erasing from the list safe
    let riEid = phys_moved_rendinst__riList[idx]
    query(riEid) <| $ [es] (transform : float3x4; ri_extra__handle : riex_handle_t;
                            rendinst_phys_move__waitingToBecomeMovedTime : float = 5.0;
                            var rendinst_phys_move__heroList : EidList)
      if distance_sq(transform[3], phys_moved_rendinst_hero__lastCheckPos) >= square(phys_moved_rendinst_hero__checkRad)
        erase(phys_moved_rendinst__riList, idx)
        let heroIdx = find_index(rendinst_phys_move__heroList, hero_eid)
        if heroIdx < 0
          return
        erase(rendinst_phys_move__heroList, heroIdx)
        if length(rendinst_phys_move__heroList) > 0
          return

        prepare_to_become_moved(riEid, rendinst_phys_move__waitingToBecomeMovedTime)

      else
        riHandles |> push(ri_extra__handle)
  return <- riHandles


[es(no_order)]
def waiting_to_become_moved_rendist_update(act : UpdateStageInfoAct;
                                           eid aka ri_eid : EntityId;
                                           transform aka ri_transform : float3x4;
                                           rendinst_phys_move__becomeMovedAt : float;
                                           rendinst_phys_move__heroList : EidList;
                                           ri_extra__handle : riex_handle_t;
                                           rendinst_phys__afterMovetemplate = "moved_rendinst")
  if act.curTime < rendinst_phys_move__becomeMovedAt
    return

  if length(rendinst_phys_move__heroList) > 0
    removeSubTemplate(ri_eid, "rendinst_phys_move_waiting_to_become_moved")
  else
    // Nobody is in the range - make it a 'moved' rendinst
    let desc = RendInstDesc(ri_extra__handle)
    enable_disable_ri_instance(desc, true)
    createEntity(rendinst_phys__afterMovetemplate) <| $(var init)
      init |> set("transform", ri_transform)
      init |> set("initialTransform", ri_transform)
      init |> set("ri_extra__handle", ri_extra__handle)
    destroyEntity(ri_eid)


[es(tag=server, no_order, REQUIRE_NOT=deadEntity)]
def rendinst_phys_template_update(act : UpdateStageInfoAct;
                                  eid : EntityId;
                                  transform aka hero_transform : float3x4;
                                  phys_moved_rendinst_hero__checkRad : float;
                                  isAlive : bool = true;
                                  var phys_moved_rendinst_hero__lastCheckPos : float3&;
                                  var phys_moved_rendinst__riList : EidList)
  if !isAlive
    return

  let heroPos = hero_transform[3]
  if distance_sq(heroPos, phys_moved_rendinst_hero__lastCheckPos) < square(phys_moved_rendinst_hero__checkRad * 0.5)
    return

  phys_moved_rendinst_hero__lastCheckPos = heroPos
  let riHandles : array<riex_handle_t> <- gather_and_update_ri_handles(eid, phys_moved_rendinst_hero__checkRad,
                                                                       phys_moved_rendinst_hero__lastCheckPos,
                                                                       phys_moved_rendinst__riList)
  find_query() <| $ [es] (phys_moved_rendinst_list : Object; client_phys_moved_rendinst_list : Object)
    // prepare box for the query
    let bb3 = BBox3(heroPos, phys_moved_rendinst_hero__checkRad)
    var box : bbox3f
    bbox3f_init(box, bb3.boxMin)
    bbox3f_add_pt(box, bb3.boxMax)

    for item in phys_moved_rendinst_list
      let templName = get_ecs_string(item.value)
      if templName != null
        process_riextra(item.key, string(*templName), eid, phys_moved_rendinst__riList, riHandles, box)
    for item in client_phys_moved_rendinst_list
      let resIdx = get_rigen_extra_res_idx(item.key)
      if resIdx < 0
        continue

      // disable client ri
      getRiGenExtraInstances(resIdx, box) <| $(handles : array<riex_handle_t>#)
        for handle in handles
          let desc = RendInstDesc(handle)
          enable_disable_ri_instance(desc, false)
    return true


def clear_phys_moved_rendinst_hero_ri_list(hero_eid : EntityId;
                                           var ri_list : EidList&)
  for riEid in ri_list
    query(riEid) <| $ [es] (rendinst_phys_move__waitingToBecomeMovedTime : float = 5.0;
                            var rendinst_phys_move__heroList : EidList)
      let heroIdx = rendinst_phys_move__heroList |> find_index(hero_eid)
      if heroIdx >= 0
        erase(rendinst_phys_move__heroList, heroIdx)
        if length(rendinst_phys_move__heroList) == 0
          prepare_to_become_moved(riEid, rendinst_phys_move__waitingToBecomeMovedTime)
  ri_list |> clear()


[es(tag=server, on_event=EventEntityDied, on_disappear)]
def phys_moved_rendinst_hero_died_or_disappeared(evt : Event;
                                                 eid aka hero_eid : EntityId;
                                                 var phys_moved_rendinst_hero__lastCheckPos : float3&;
                                                 var phys_moved_rendinst__riList : EidList)
  phys_moved_rendinst_hero__lastCheckPos = float3(VERY_BIG_NUMBER)
  clear_phys_moved_rendinst_hero_ri_list(hero_eid, phys_moved_rendinst__riList)


[es(tag=gameClient, no_order, REQUIRE_NOT=deadEntity)]
def rendinst_client_phys_template_update(act : UpdateStageInfoAct;
                                         eid : EntityId;
                                         transform aka hero_transform : float3x4;
                                         client_phys_moved_rendinst_hero__checkRad : float;
                                         isAlive : bool = true;
                                         var client_phys_moved_rendinst_hero__lastCheckPos : float3&;
                                         var client_phys_moved_rendinst__riList : EidList)
  if !isAlive
    return

  let heroPos = hero_transform[3]
  if distance_sq(heroPos, client_phys_moved_rendinst_hero__lastCheckPos) < square(client_phys_moved_rendinst_hero__checkRad * 0.5)
    return

  client_phys_moved_rendinst_hero__lastCheckPos = heroPos
  let riHandles : array<riex_handle_t> <- gather_and_update_ri_handles(eid, client_phys_moved_rendinst_hero__checkRad,
                                                                       client_phys_moved_rendinst_hero__lastCheckPos,
                                                                       client_phys_moved_rendinst__riList)
  find_query() <| $ [es] (client_phys_moved_rendinst_list : Object)
    // prepare box for the query
    let bb3 = BBox3(heroPos, client_phys_moved_rendinst_hero__checkRad)
    var box : bbox3f
    bbox3f_init(box, bb3.boxMin)
    bbox3f_add_pt(box, bb3.boxMax)

    for item in client_phys_moved_rendinst_list
      let templName = get_ecs_string(item.value)
      if templName != null
        process_riextra(item.key, string(*templName), eid, client_phys_moved_rendinst__riList, riHandles, box)
    return true


[es(tag=gameClient, on_event=EventEntityDied, on_disappear)]
def client_phys_moved_rendinst_hero_died_or_disappeared(evt : Event;
                                                        eid aka hero_eid : EntityId;
                                                        var client_phys_moved_rendinst_hero__lastCheckPos : float3&;
                                                        var client_phys_moved_rendinst__riList : EidList)
  client_phys_moved_rendinst_hero__lastCheckPos = float3(VERY_BIG_NUMBER)
  clear_phys_moved_rendinst_hero_ri_list(hero_eid, client_phys_moved_rendinst__riList)
