options no_aot

require ecs
require ecs.fs
require ECSGlobalTags


[export, unused_argument(base_path)]
def load_imgui_daeditor(base_path : string) : bool
  var ok = true

  if typeinfo builtin_module_exists(DagorImgui)
    ok = load_sub_folder(base_path, "imgui") && ok

  if typeinfo builtin_module_exists(DagorImgui) && typeinfo builtin_module_exists(DagorEditor)
    ok = load_sub_folder(base_path, "imgui/daEditor") && ok
    if typeinfo builtin_module_exists(DagorResources)
      ok = load_sub_folder(base_path, "imgui/animgraph") && ok
    if typeinfo builtin_module_exists(RendInst)
      ok = load_sub_folder(base_path, "imgui/daEditor/rendInst") && ok
    if typeinfo builtin_module_exists(DngNet)
      ok = load_sub_folder(base_path, "imgui/daEditor/locSnapshots") && ok
    if typeinfo builtin_module_exists(DagorDebug3DSolid)
      ok = load_sub_folder(base_path, "imgui/daEditor/tmDebug") && ok

  if typeinfo builtin_module_exists(DagorEditor)
    ok = load_sub_folder(base_path, "daEditor") && ok
    if typeinfo builtin_module_exists(DngPhysObj)
      ok = load_sub_folder(base_path, "daEditor/physobj") && ok
    if typeinfo builtin_module_exists(vehicle)
      ok = load_sub_folder(base_path, "daEditor/vehicle") && ok
    if typeinfo builtin_module_exists(DngShip)
      ok = load_sub_folder(base_path, "daEditor/ship") && ok

  ok = ok // workaround for "variable ... can be made const" in case if both DagorImgui & DagorImgui are absent
  return ok



