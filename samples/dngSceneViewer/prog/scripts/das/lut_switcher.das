require ecs
require ecs.safe
require math
require math.base
require DagorMath
require das.main_events
require das.lut_switcher_common
require DagorSystem

[es(track=(isOutsideBattleArea, isNearEnemyRespawnWarningDamage, input__enabled), on_appear, REQUIRE=(hero, watchedByPlr))]
def tonemapper_switcher_outside_battle_area(evt : Event; isOutsideBattleArea : bool; isNearEnemyRespawnWarningDamage : bool; input__enabled : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__outOfbattleArea : Object)
    switch_color_grading(ecs_hash("battle_area"), color_grading_items, color_grading__outOfbattleArea, (isOutsideBattleArea || isNearEnemyRespawnWarningDamage) && input__enabled)

[es(on_event=EventEntityDestroyed, REQUIRE=watchedByPlr)]
def tonemapper_switcher_outside_battle_area_disable(evt : Event)
  query() <| $ [es(REQUIRE=color_grading__outOfbattleArea)] (var color_grading_items : Array)
    remove_grading(ecs_hash("battle_area"), color_grading_items)

[es(track=(isAlive, lut_switcher__enabled), on_appear, REQUIRE=watchedByPlr)]
def tonemapper_switcher_death(evt : Event; isAlive : bool; lut_switcher__enabled : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__death : Object)
    switch_color_grading(ecs_hash("death"), color_grading_items, color_grading__death, !isAlive && lut_switcher__enabled)

[es(track=(isDowned, lut_switcher__enabled), on_appear, REQUIRE=watchedByPlr)]
def tonemapper_switcher_downed(evt : Event; isDowned : bool; lut_switcher__enabled : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__downed : Object)
    switch_color_grading(ecs_hash("downed"), color_grading_items, color_grading__downed, isDowned && lut_switcher__enabled)

[es(track=(isBlinded, lut_switcher__enabled, isAlive), on_appear, REQUIRE=watchedByPlr)]
def tonemapper_switcher_no_vision(evt : Event; isBlinded : bool; lut_switcher__enabled : bool; isAlive : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__is_no_vision : Object)
    switch_color_grading(ecs_hash("no_vision"), color_grading_items, color_grading__is_no_vision, isBlinded && lut_switcher__enabled && isAlive)

[es(track=(burning__isBurning, lut_switcher__enabled), on_appear, REQUIRE=watchedByPlr, REQUIRE_NOT=deadEntity)]
def tonemapper_switcher_burning(evt : Event; burning__isBurning : bool; lut_switcher__enabled : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__burning : Object)
    switch_color_grading(ecs_hash("burning"), color_grading_items, color_grading__burning, burning__isBurning && lut_switcher__enabled)

[es(track=(hasConcussedVision, lut_switcher__enabled), on_appear, REQUIRE=watchedByPlr, REQUIRE_NOT=deadEntity)]
def tonemapper_switcher_concussion(evt : Event; hasConcussedVision : bool; lut_switcher__enabled : bool)
  query() <| $ [es] (var color_grading_items : Array; color_grading__concussion : Object)
    switch_color_grading(ecs_hash("concussion"), color_grading_items, color_grading__concussion, hasConcussedVision && lut_switcher__enabled)

[es(on_disappear, REQUIRE=(watchedByPlr, lut_switcher__enabled))]
def disable_color_grading_on_destroy_es(evt : Event)
  query() <| $ [es] (var color_grading_items : Array; color_grading__death : Object)
    switch_color_grading(ecs_hash("death"), color_grading_items, color_grading__death, false)
  query() <| $ [es] (var color_grading_items : Array; color_grading__downed : Object)
    switch_color_grading(ecs_hash("downed"), color_grading_items, color_grading__downed, false)
  query() <| $ [es] (var color_grading_items : Array; color_grading__is_no_vision : Object)
    switch_color_grading(ecs_hash("no_vision"), color_grading_items, color_grading__is_no_vision, false)
  query() <| $ [es] (var color_grading_items : Array; color_grading__burning : Object)
    switch_color_grading(ecs_hash("burning"), color_grading_items, color_grading__burning, false)
  query() <| $ [es] (var color_grading_items : Array; color_grading__concussion : Object)
    switch_color_grading(ecs_hash("concussion"), color_grading_items, color_grading__concussion, false)

[es(tag=gameClient, on_appear, REQUIRE=watchedByPlr, REQUIRE_NOT=hero)]
def tonemapper_switcher_spectator(evt : Event)
  query() <| $ [es] (var color_grading_items : Array; color_grading__alive : Object)
    add_grading(ecs_hash("alive"), color_grading_items, color_grading__alive)

[es(tag=gameClient, on_event=EventLutSwitcherAliveState, on_appear)]
def tonemapper_switcher_alive_es(
                                 evt : Event;
                                 var color_grading_items : ecs::Array;
                                 color_grading__alive : Object)
  print("tonemapper_switcher_alive_es")
  clear(color_grading_items)
  add_grading(ecs_hash("alive"), color_grading_items, color_grading__alive)

[es(track=lut_switcher__enabled, on_appear, REQUIRE=watchedByPlr)]
def tonemapper_switcher_input(evt : Event; lut_switcher__enabled : bool)
  if !lut_switcher__enabled
    query() <| $ [es] (var color_grading_items : Array; color_grading__alive : Object)
      clear(color_grading_items)
      add_grading(ecs_hash("alive"), color_grading_items, color_grading__alive)

[es(before=post_fx_es)]
def combined_color_grading_init_es(
                                   info : ecs::UpdateStageInfoAct;
                                   tonemap_save : Object;
                                   var color_grading_items : Array;
                                   var color_grading : Object)
  update_object_existing_fields_from(color_grading, tonemap_save)

  var idx = length(color_grading_items)
  while --idx >= 0
    var itemObj = getRW_ecs_object(color_grading_items[idx])
    var lifeTime = get_float(*itemObj, "life_time") ?? 0.
    let timings = get_Point3(*itemObj, "timings") ?? float3(0, 10000, 0)

    lifeTime += info.dt;
    if lifeTime > timings.x + timings.y + timings.z
      erase(color_grading_items, idx)
      continue

    set(*itemObj, "life_time", lifeTime)

  for grading_item in color_grading_items
    let itemObj = getRW_ecs_object(grading_item)
    let lifeTime = get_float(*itemObj, "life_time") ?? 0.
    let timings = get_Point3(*itemObj, "timings") ?? float3(0, 10000, 0)

    var state = 0.0
    if lifeTime < timings.x
      state = safediv(lifeTime, timings.x)
    elif lifeTime < timings.x + timings.y
      state = 1.0
    else
      state = saturate(1.0 - safediv((lifeTime - timings.x - timings.y), timings.z))

    // Apply easing function here
    state = state * state * (3.0 - 2.0 * state)

    for field in *itemObj
      let baseValueF4 = color_grading[field.key] ?as float4
      if baseValueF4 != null
        let newValue = *(field.value as float4)
        set(color_grading, field.key, lerp(*baseValueF4, newValue, float4(state)))
        continue
      let baseValueF = color_grading[field.key] ?as float
      if baseValueF != null
        let newValue = get_float(field.value) ?? 0.0
        set(color_grading, field.key, lerp(*baseValueF, newValue, state))

[es(after=combined_color_grading_init_es)]//we can also require for some special tag/bool
def edit_color_grading_es(info : ecs::UpdateStageInfoAct; edit_color_grading : Object; edit_white_balance : Object; edit_tonemap : Object)
  query() <| $ [es] (var color_grading : Object)
    amend_object_from(color_grading, edit_color_grading)
  query() <| $ [es] (var tonemap : Object)
    amend_object_from(tonemap, edit_tonemap)
  query() <| $ [es] (var white_balance : Object)
    amend_object_from(white_balance, edit_white_balance)

[es(on_appear)]
def edit_color_grading_appear(evt : Event; var edit_color_grading : Object; var edit_white_balance : Object; var edit_tonemap : Object)
  query() <| $ [es] (color_grading : Object)
    update_object_existing_fields_from(edit_color_grading, color_grading)
  query() <| $ [es] (tonemap : Object)
    update_object_existing_fields_from(edit_tonemap, tonemap)
  query() <| $ [es] (white_balance : Object)
    update_object_existing_fields_from(edit_white_balance, white_balance)
