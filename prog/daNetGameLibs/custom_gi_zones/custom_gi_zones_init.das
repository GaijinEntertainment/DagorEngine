options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_custom_gi_zones(base_path : string) : bool
  var ok = true
  if ecs_has_tag("render") || is_in_aot()
    ok = load_sub_folder(base_path, "render") && ok

  return ok


[export]
def test()
  let ok = load_custom_gi_zones("%danetlibs/custom_gi_zones")
  assert(ok)