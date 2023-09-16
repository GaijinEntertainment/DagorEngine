require AnimV20

[generic]
def anim_graph_enqueueState(var anim_graph : AnimationGraph;
                            var state : IAnimStateHolder;
                            state_idx : int;
                            force_dur : float = -1.f;
                            force_speed : float = -1.f)
  anim_graph_enqueueState(anim_graph, state, state_idx, force_dur, force_speed)
