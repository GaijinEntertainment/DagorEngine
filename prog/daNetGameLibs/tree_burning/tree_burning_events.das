require ecs
require RendInst

[cpp_event(broadcast)]
struct EventDestroyBurnedTreesInRadius
  pos : float3
  radius : float

[cpp_event(broadcast)]
struct EventDestroyBurnedTree
  handle : riex_handle_t

[cpp_event(unicast)]
struct EventCreateTreeBurningChain
  riExHandle : riex_handle_t
  treeTm : float3x4
  canopyType : int
  canopyBoxMin : float3
  canopyBoxMax : float3
  burningSourceWpos : float3

[cpp_event(unicast)]
struct EventAddTreeBurningChainSource
  burningSourceWpos : float3
