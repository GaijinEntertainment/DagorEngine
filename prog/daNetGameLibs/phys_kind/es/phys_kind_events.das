module phys_kind_events shared
require ecs
require DngPhysKind

[cpp_event(unicast)]
struct CmdPhysKindStep
  tick : int
  dt : float
  isForReal : bool
  useControls : bool // false on remote copies; they lack controls

[cpp_event(unicast)]
struct CmdPhysKindApplyState
  keepBodyMotion : bool // only owner corrections keep body motion

def phys_kind_ext(var state : PhysKindState&; ext_type : auto(T)) : T -const?
  return unsafe(reinterpret<T -const?>(phys_kind_state_ext(state, typeinfo(sizeof type<T>))))

def phys_kind_ext(var controls : PhysKindControls&; ext_type : auto(T)) : T -const?
  return unsafe(reinterpret<T -const?>(phys_kind_controls_ext(controls, typeinfo(sizeof type<T>))))
