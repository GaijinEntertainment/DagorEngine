options no_aot = true
require ecs
require ecs.fs
require danetlibs.frame_graph_renderer.frame_graph_renderer_init
require danetlibs.composite_entity.composite_entity_init

[init]
def load
  load_folder("%scripts/outer_space/es")
  load_frame_graph_renderer("%danetlibs/frame_graph_renderer")
  load_composite_entity("%danetlibs/composite_entity")
