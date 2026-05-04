options no_aot
require ecs
require DngPhysObj
require PhysObj

[es(no_order, REQUIRE=daeditor__selected)]
def animchar_update_phys_obj_wake_up_es(info : UpdateStageInfoAct;
                                        var phys_obj_net_phys : PhysObjActor)
  phys_obj_net_phys.phys |> wakeUp()
