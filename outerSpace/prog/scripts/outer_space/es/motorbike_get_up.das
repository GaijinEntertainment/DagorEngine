require ecs
require DngPhysKind
require outer_space.events_outer_space
require danetlibs.package_motorbike.main.motorbike_common


[es(tag=server, REQUIRE=motorbike__fallen)]
def motorbike_get_up_request_es(evt : RequestMotorbikeGetUp; eid : EntityId)
  sendEvent(eid, CmdMotorbikeGetUp())


// test convenience, where a game would have the rider walk back to the bike
[es(tag=server, after=motorbike_fall_es)]
def motorbike_auto_get_up_es(info : UpdatePhysEvent; eid : EntityId; phys_kind_net_phys : PhysKindActor; motorbike__fallen : bool;
                             motorbike_test__getUpAfter_sec : float; var motorbike_test__fallenTime : float&)
  let still = motorbike__fallen && length_sq(phys_kind_net_phys.phys.currentState.velocity) < 1.0
  motorbike_test__fallenTime = still ? motorbike_test__fallenTime + info.dt : 0.0
  if motorbike_test__fallenTime > motorbike_test__getUpAfter_sec
    motorbike_test__fallenTime = 0.0
    sendEvent(eid, CmdMotorbikeGetUp())
