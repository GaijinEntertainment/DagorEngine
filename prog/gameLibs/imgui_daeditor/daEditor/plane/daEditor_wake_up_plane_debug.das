options no_aot
require ecs
require DngPlane

[es(no_order, REQUIRE=daeditor__selected)]
def animchar_update_plane_wake_up_es(info : UpdateStageInfoAct;
                                     var plane_net_phys : PlaneActor&)
  plane_net_phys.phys |> wakeUp()
