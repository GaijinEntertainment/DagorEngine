options no_aot
require ecs
require vehicle
require DngPhysObj
require PhysObj
require Ship
require DngShip
require DngActor

[es(no_order, REQUIRE=daeditor__selected)]
def animchar_update_phys_obj_wake_up_es(info : UpdateStageInfoAct;
                                        var phys_obj_net_phys : PhysObjActor)
  phys_obj_net_phys.phys |> wakeUp()

[es(no_order, REQUIRE=daeditor__selected)]
def animchar_update_vehicle_wake_up_es(info : UpdateStageInfoAct;
                                       var vehicle_net_phys : VehiclePhysActor)
  vehicle_net_phys.phys |> wakeUp()

[es(no_order, REQUIRE=daeditor__selected)]
def animchar_update_ship_wake_up_es(info : UpdateStageInfoAct;
                                    var ship_net_phys : ShipActor)
  ship_net_phys.phys |> ship_phys_wake_up()
