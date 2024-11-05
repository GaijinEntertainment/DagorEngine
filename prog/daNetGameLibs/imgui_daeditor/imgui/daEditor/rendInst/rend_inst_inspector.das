
require ecs
require DagorSystem
require DagorDataBlock
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require imgui
require RendInst


[ecs_inspector(type_name="RiExtraComponent")]
def draw_RiExtraComponent(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<RiExtraComponent?> refs[0].rawData)
  let desc = RendInstDesc(val.handle)
  let riName = desc.isValid ? getRIGenResName(desc) : "<invalid>"
  let textInfo = "{riName} {val.handle}"
  Text(textInfo)
  default_menu(eid, name, "RiExtraComponent", $() => textInfo, $(str) { logerr("RiExtraComponent to string not implemented! str={str}"); }, $() => "ri_extra\{}", changed, sub_menu)
