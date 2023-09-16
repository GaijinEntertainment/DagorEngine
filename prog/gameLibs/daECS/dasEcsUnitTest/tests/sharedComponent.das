require ecs

// ES with find_query example
[es(on_event=EventStart)]
def shared_component_es(evt : Event; [[shared_comp]] shared_str : das_string)
  assert(string(shared_str) == "sample_shared_str")
