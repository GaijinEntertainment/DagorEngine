require ecs
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_common
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require imgui
require daslib/safe_addr


[unused_argument(level)]
def draw_float4_as_color(eid : array<EntityId> implicit; name, typeName : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<float4?> refs[0].rawData)
  var color = [[float[4] (*val).x; (*val).y; (*val).z; (*val).w]]
  SetNextItemWidth(-FLT_MIN)
  if ColorEdit4("##{name}", safe_addr(color[0]), ImGuiColorEditFlags Float)
    *changed = true
    (*val).x = color[0]
    (*val).y = color[1]
    (*val).z = color[2]
    (*val).w = color[3]
  default_menu(eid, name, typeName, $() => to_string(*val), $(str) { update_value(to_float4(str), val, changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


[ecs_inspector(type_name="Point4", tag="color_grading")]
def draw_postfx_vectors(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  draw_float4_as_color(eid, name, "Point4", refs, level, changed, sub_menu)


[ecs_inspector(type_name="Point4", tag="edit_color_grading")]
def draw_edit_color_grading_vectors(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  draw_float4_as_color(eid, name, "Point4", refs, level, changed, sub_menu)