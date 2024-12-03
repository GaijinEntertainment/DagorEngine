options no_aot = true//since this is merely loader
require app
require ecs.fs
require %scripts/dng_libs_init
require ECSGlobalTags

[init]
def load
  var ok = true
  ok = load_dng_libs() && ok
  ok = load_folder("%scripts/das") && ok
  ok = load_folder("%scripts/das/effects") && ok
  ok = load_folder("%scripts/das/character_controller") && ok
  if ecs_has_tag("sound") || app::get_game_name() == "aot"
    ok = load_folder("%scripts/das/sound") && ok
