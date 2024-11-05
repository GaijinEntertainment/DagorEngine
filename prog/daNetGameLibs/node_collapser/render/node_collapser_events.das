module node_collapser_events shared
require ecs

[cpp_event(broadcast)]
struct HideNodesEvent
  at : float3
