options no_aot = true
require ecs.fs
require %toolScripts/libs_init

[init]
def load
  load_libs()
  load_folder("%scripts/das")
  load_folder("%scripts/das/effects")
