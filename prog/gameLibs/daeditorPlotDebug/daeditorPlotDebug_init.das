options no_aot

require ecs.fs

[export]
def load_daeditorPlotDebug(base_path : string) : bool
  return load_sub_folder(base_path, "es")
