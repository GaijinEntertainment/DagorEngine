options no_aot = true//since this is merely loader
require ecs
require ecs.fs
require ECSGlobalTags
require DagorDataBlock
require DagorSystem
require danetlibs.motion_matching.motion_matching_init

[init]
def load

  if ecs_has_tag("dasDebug")
    load_optional_folder("%dngdas/das/remote/es")

  load_motion_matching("%danetlibs/motion_matching")
  load_folder("%asset_viewer/animation")