options no_aot
require ecs
require ecs.safe
require math.base
require DagorMathUtils
require DngActor
require DngHuman
require HumanPhys


[es(on_appear, tag=dev)]
def mm_test_behavior_init(evt : Event;
                          transform : float3x4;
                          var test_behaviour__initialPos : float3&)
  test_behaviour__initialPos = transform[3]

[es(no_order, tag=dev)]
def mm_test_behavior_update(info : ecs::UpdateStageInfoAct;
                            eid : EntityId;
                            test_behaviour__actions : Array;
                            test_behaviour__initialPos : float3;
                            transform : float3x4;
                            var test_behaviour__timer : float&;
                            var human_net_phys : HumanActor)
  assume ct = human_net_phys.phys.producedCT
  var time = test_behaviour__timer
  for action in test_behaviour__actions
    let actionObj = action as Object
    let duration = actionObj?.duration ?? 0f
    time -= duration
    if time >= 0f
      continue
    test_behaviour__timer += info.dt
    ct |> human_control_state_set_control_bit(HumanPhysControlType HCT_CROUCH,  actionObj?.crouch ?? false)
    ct |> human_control_state_set_control_bit(HumanPhysControlType HCT_CRAWL, actionObj?.crawl ?? false)
    ct |> human_control_state_set_control_bit(HumanPhysControlType HCT_SPRINT, actionObj?.sprint ?? false)
    ct |> human_control_state_set_control_bit(HumanPhysControlType HCT_JUMP, actionObj?.jump ?? false)
    let lookAngles = actionObj?.look ?? float2(0f, 0f)
    let lookDir = angles_to_dir(deg_to_rad(lookAngles))
    ct |> human_control_state_set_wish_shoot_dir(lookDir)
    ct |> human_control_state_set_wish_look_dir(lookDir)
    let moveAngle = get_float(*actionObj, "move")
    if moveAngle != null
      let angle = deg_to_rad(*moveAngle)
      ct |> human_control_state_set_walk_dir(float2(cos(angle), sin(angle)))
    else
      ct |> human_control_state_set_walk_dir(float2(0f, 0f))
    return
  test_behaviour__timer = 0f
  var tm = transform
  tm[3] = test_behaviour__initialPos
  teleport_phys_actor(eid, tm)
