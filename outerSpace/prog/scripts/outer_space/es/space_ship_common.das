module space_ship_common shared
require DngPhysObj
require PhysObj

struct SpaceShipControlsState
  lin : float3
  rot : float3
  rotateView_rad : float2
  rotateView_base_quat : float4
  min_target_ht : float
  free_look_around : bool
  use_stabilization : bool
  use_fwd_stabilization : bool

def private get_phys_obj_controls_impl(var phys_obj_net_phys : PhysObjActor&; getctfn : auto) : SpaceShipControlsState?
  let ctRawPtr = getctfn |> invoke(phys_obj_net_phys.phys, typeinfo(sizeof type<SpaceShipControlsState>))
  var ct : SpaceShipControlsState?
  unsafe
    ct = reinterpret<SpaceShipControlsState?>(ctRawPtr)
  return ct

def get_phys_obj_produced_controls(var phys_obj_net_phys : PhysObjActor&) : SpaceShipControlsState?
  return get_phys_obj_controls_impl(phys_obj_net_phys, $(var p:PhysObj&; sz : int) : auto => getProducedCustomControls(p, sz) )

def get_phys_obj_applied_controls(var phys_obj_net_phys : PhysObjActor&) : SpaceShipControlsState?
  return get_phys_obj_controls_impl(phys_obj_net_phys, $(var p:PhysObj&; sz : int) : auto => getAppliedCustomControls(p, sz) )
