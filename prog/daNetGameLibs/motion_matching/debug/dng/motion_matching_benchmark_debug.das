options no_aot
require ecs
require math.base
require DagorConsole
require BehNodes
require DagorDataBlock
require DagorRandom
require Dacoll
require DagorDebug3D
require DagorTime
require daProfiler
require DngCamera
require DngActor
require MotionMatching

def randXZfloat3(scale : float)
  return (float3(gfrnd(), 0.f, gfrnd()) * 2.0 - float3(1, 0, 1)) * scale

def spawn_dev_soldier(pos : float3; templateName : string; init_block : block<(var compInit : ComponentsInitializer) : void>)
  return createEntity(templateName) <| $(var init : ComponentsInitializer)
    var tm = IDENT_TM
    tm[3] = pos
    set(init, "transform", tm)
    set(init, "team", 1)
    invoke(init_block, init)

def spawn_dev_soldier(pos : float3; templateName : string)
  return spawn_dev_soldier(pos, templateName) <| $ [unused_argument(init)] (var init)
    pass

def set_weapon(var init : ComponentsInitializer; primary_weapon : string)
  using() <| $(var obj : Object)
    set(obj, "primary", primary_weapon)
    set(init, "human_weap__initialWeaponTemplates", obj)

[es(on_appear)]
def test_target_appear(event : Event; var test_target, test_target_origin : float3&; transform : float3x4)
  test_target = transform[3]
  test_target_origin = transform[3]

def clamp_point_in_circle(center, point : float3; radius : float)
  let d = center - point
  let dist = length(d)
  let t = dist / radius
  if t > 1.f
    return point + d * (1.f - 1.f / t)
  else
    return point

[es(no_order)]
def tester_target_update(info : ecs::UpdateStageInfoAct;
                         var animation_tester__moveTarget : float3&;
                         test_target, test_target_posOffset : float3)
  animation_tester__moveTarget = test_target + test_target_posOffset


[es(no_order, REQUIRE=motion_matching__animTreeTester)]
def anim_tree_tester_update(info : ecs::UpdateStageInfoAct; var motion_matching__enabled : bool&)
  motion_matching__enabled = false

[es(no_order, REQUIRE=animationTestPair)]
def test_pair_update(info : ecs::UpdateStageInfoAct;
                     var transform : float3x4&;
                     tester_list : EidList)
  var avgPos = float3(0)
  if empty(tester_list)
    return

  for eid in tester_list
    query(eid) <| $ [es] (transform : float3x4)
      avgPos += transform[3]

  avgPos /= float(length(tester_list))
  transform[3] = avgPos

[es(on_appear)]
def test_pair_camera_look_at_target(event : Event;
                                    motion_matching__followWithFreeCamera : bool;
                                    motion_matching__freeCameraOffset : float3)
  if !motion_matching__followWithFreeCamera
    return
  find_query() <| $ [es] (camera__active : bool; var transform : float3x4&)
    if !camera__active
      return false
    let lookDir = -normalize(motion_matching__freeCameraOffset)
    transform[0] = normalize(cross(float3(0, 1, 0), lookDir))
    transform[1] = cross(lookDir, transform[0])
    transform[2] = lookDir
    return true

[es(no_order)]
def test_pair_update_camera(info : ecs::UpdateStageInfoAct;
                            transform : float3x4;
                            motion_matching__followWithFreeCamera : bool;
                            motion_matching__freeCameraOffset : float3)
  if !motion_matching__followWithFreeCamera
    return
  let cameraTarget = transform[3]
  find_query() <| $ [es] (camera__active : bool; var transform : float3x4&)
    if !camera__active
      return false
    transform[3] = cameraTarget + motion_matching__freeCameraOffset
    return true

def default_test_target_update(info : ecs::UpdateStageInfoAct;
                               transform : float3x4;
                               var test_target, test_target_velocity : float3&;
                               test_target_center : float3)
  let position = transform[3]
  let r = length(test_target_velocity)//radius
  let a = atan2(test_target_velocity.x, test_target_velocity.z)//angle
  let maxDa = PI * 8.f
  let maxDr = 15.f
  let maxVel = 6.f
  let dr = (gfrnd() - 0.5f) * 2.f * maxDr * info.dt
  let da = (gfrnd() - 0.5f) * 2.f * maxDa * info.dt
  var x, y : float
  sincos(a + da, x, y)

  let maxRadius = 20.f
  let innerRadius = 15.f
  let t = saturate((length(test_target_center - position) - innerRadius) / maxRadius)

  test_target_velocity = float3(x, 0.f, y) * min(r + dr, maxVel)
  test_target_velocity = lerp(test_target_velocity, normalize(test_target_center - position) * maxVel, float3(t))
  test_target += test_target_velocity * info.dt

  let WALK_AREA_RADIUS = 30f
  test_target = clamp_point_in_circle(test_target_center, test_target, WALK_AREA_RADIUS)

  let color = E3DCOLOR(0xFF00FF00)
  draw_debug_sphere_buffered(test_target, 0.1, color, 12, 10)

[es(no_order)]
def test_target_update(info : ecs::UpdateStageInfoAct;
                       eid : EntityId;
                       var transform : float3x4&;
                       var test_target, test_target_velocity : float3&;
                       var test_target_time : float&;
                       test_target_timeToReset : float;
                       test_target_origin : float3;
                       test_target_mode : string)
  test_target_time += info.dt
  if test_target_timeToReset > 0.0f && test_target_time > test_target_timeToReset
    transform[3] = test_target_origin
    teleport_phys_actor(eid, transform)
    test_target = test_target_origin
    test_target_time = 0.0f
    return

  if test_target_mode == "run forward"
    test_target += test_target_velocity * info.dt
  elif test_target_mode == "default"
    default_test_target_update(info, transform, test_target, test_target_velocity, test_target_origin)

[es(no_order)]
def test_pair_text_update(info : UpdateStageInfoRenderDebug;
                          transform : float3x4;
                          motion_matching__debugLabel : string)

  if !empty(motion_matching__debugLabel)
    let pos = transform[3] - float3(0, 1, 0)
    add_debug_text_mark(pos, motion_matching__debugLabel, -1, 0f, E3DCOLOR(0xFFFFFFFF))



[es(no_order, REQUIRE=animationTestPair)]
def test_pair_beh_tree_controll(info : ecs::UpdateStageInfoAct;
                                transform : float3x4;
                                test_target : float3;
                                tester_list : EidList)

  let right = transform[0]
  let distanceBetweenTester = 1.5f
  let n = length(tester_list)
  for eid, i in tester_list, range(n)
    query(eid) <| $ [es] (var animation_tester__moveTarget : float3&)
      let offset = (float(i) - float(n - 1) * 0.5f) * distanceBetweenTester
      animation_tester__moveTarget = test_target + right * offset



[es(no_order)]
def tester_beh_tree_control(info : ecs::UpdateStageInfoAct;
                            animation_tester__moveTarget : float3;
                            var beh_tree : BehaviourTree&)
  beh_tree.blackBoard |> get_or_create("wishPos", animation_tester__moveTarget)
  beh_tree.blackBoard |> set("wishPos", animation_tester__moveTarget)
  let color = E3DCOLOR(0xFFFFFF00)
  draw_debug_sphere_buffered(animation_tester__moveTarget, 0.1, color, 12, 10)

def get_spawn_pos_from_current_camera()
  var targetPos = float3(0, 0, 0)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      let cameraPos = transform[3]
      let dir = transform[2]
      var dist = 500.0f
      var norm : float3
      traceray_normalized(cameraPos, dir, dist, norm)
      targetPos = cameraPos + dist * dir
    return camera__active
  return targetPos

[console_cmd(name="motion_matching.spawn_test_pair")]
def spawn_test_pair()
  clear_bots()
  let spawnTransform = pos_tm(get_spawn_pos_from_current_camera())
  createEntity("animation_test_pair") <| $(var init : ComponentsInitializer)
    set(init, "transform", spawnTransform)
    let right = spawnTransform[0]
    assume tm = spawnTransform
    using() <| $(var tester_list : EidList)
      tester_list |> push(spawn_dev_soldier(tm[3] - right, "walk_soldier+dev_soldier_tag+motion_matching_anim_tree_tester"))
      tester_list |> push(spawn_dev_soldier(tm[3], "walk_soldier+dev_soldier_tag+motion_matching_tester"))
      tester_list |> push(spawn_dev_soldier(tm[3] + right, "walk_soldier+dev_soldier_tag+motion_matching_inertial_blend_tester"))
      set(init, "tester_list", tester_list)
  enable_free_camera()

[console_cmd(name="motion_matching.test_run_forward")]
def spawn_test_soldier()
  clear_bots()
  let spawnPos = get_spawn_pos_from_current_camera()
  spawn_dev_soldier(spawnPos, "walk_soldier+dev_soldier_tag+animation_self_tester") <| $(var init : ComponentsInitializer)
    set(init, "test_target_posOffset", float3(0.0, 0.0, 5.0))
    set(init, "test_target_velocity", float3(0.0, 0.0, 3.0))
    set(init, "test_target_mode", "run forward")
    set(init, "test_target_timeToReset", 5.0)
    set_weapon(init, "gewehr_43_gun")


[console_cmd(name="motion_matching.spawn_bots")]
def spawn_bots(botCount : int)
  clear_bots()
  set_rnd_seed(61346)
  let curTime = float(get_time_msec()) / 1000.f
  createEntity("animation_test_scope") <| $(var init : ComponentsInitializer)
    set(init, "animation_test__startTime", curTime)

  var cameraTm : float3x4
  var found = false
  query() <| $ [es(REQUIRE=animationTestSpawnArea)] (transform : float3x4)
    let spawnArea = sqrt(float(botCount) + 1.f) * PI * 0.5
    for _i in range(0, botCount)
      spawn_dev_soldier(transform[3] + randXZfloat3(spawnArea), "walk_soldier+dev_soldier_tag+animation_self_tester")
    cameraTm[0] = float3(1, 0, 0)
    cameraTm[1] = float3(0, 1, 0)
    cameraTm[2] = float3(0, 0, 1)
    cameraTm[3] = float3(0, 5, -15) + transform[3]
    found = true
  if found
    set_cam_itm(cameraTm)
    enable_free_camera()


[es(on_appear, REQUIRE=animationTestScope)]
def start_animation_test_sampling(event : Event)
  set_mode(da_profiler_ALL | da_profiler_CONTINUOUS)

[es(on_disappear, REQUIRE=animationTestScope)]
def stop_animation_test_sampling(event : Event)
  request_dump()
  set_mode(da_profiler_ALL)
  clear_bots()

[es(no_order, REQUIRE=animationTestScope)]
def animation_test_timer(info : ecs::UpdateStageInfoAct; eid : EntityId; animation_test__startTime, animation_test__duration : float)
  if animation_test__startTime + animation_test__duration < info.curTime
    destroyEntity(eid)

[console_cmd(name="motion_matching.clear_tester")]
def clear_bots()
  query() <| $ [es(REQUIRE=animation_tester__moveTarget)] (eid : EntityId)
    destroyEntity(eid)
  query() <| $ [es(REQUIRE=animationTestPair)] (eid : EntityId)
    destroyEntity(eid)