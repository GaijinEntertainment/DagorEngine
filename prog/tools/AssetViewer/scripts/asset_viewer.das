options no_aot = true//since this is merely loader
require ecs
require ecs.fs
require ECSGlobalTags
require DagorDataBlock
require DagorSystem

[init]
def load

  if ecs_has_tag("dasDebug")
    load_optional_folder("%dngdas/remote/es")

  load_folder("%asset_viewer/animation")