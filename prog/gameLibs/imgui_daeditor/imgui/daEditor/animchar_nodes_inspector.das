options no_aot

require ecs
require imgui
require AnimV20
require DagorMath
require DagorSystem
require gameLibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common
require gameLibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro

require daslib/safe_addr



[ecs_inspector(type_name="AnimcharNodesMat44", is_multi_line=true)]
def draw_AnimcharNodesMat44(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<ComponentRenderInfo#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<AnimcharNodesMat44?> refs[0].rawData)

  imgui::Text("AnimcharNodesMat44")

  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  imgui::Text("{name}.bsph")
  imgui::TableSetColumnIndex(1)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat4("##{name}.bsph", unsafe(addr((*val).bsph.x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, "{name}.bsph", "Point4", $() => to_string((*val).bsph), $(str) { update_value(to_float4(str), unsafe(addr((*val).bsph)), changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)

  imgui::TableNextRow()
  imgui::TableSetColumnIndex(0)
  imgui::AlignTextToFramePadding()
  imgui::Text("{name}.wofs")
  imgui::TableSetColumnIndex(1)
  SetNextItemWidth(-FLT_MIN)
  *changed = InputFloat4("##{name}.wofs", unsafe(addr((*val).wofs.x)), "%.4f", ImGuiInputTextFlags.None) || *changed
  default_menu(eid, "{name}.wofs", "Point4", $() => to_string((*val).wofs), $(str) { update_value(to_float4(str), unsafe(addr((*val).wofs)), changed); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)


  AnimcharNodesMat44_getWtms(*val) <| $(wtms)
    var tm = float3x4(wtms[0])
    imgui::TableNextRow()
    imgui::TableSetColumnIndex(0)
    imgui::AlignTextToFramePadding()
    imgui::Text("{name}.wtm[0] (prs)")
    imgui::TableSetColumnIndex(1)
    draw_edit_position_rotation_scale_tm(tm, name, changed)
    default_menu(eid, "{name}.wtm[0] (prs)", "TMatrix", $() => to_string(tm), $ [unused_argument(str)] (str : string) { logerr("Unsupported"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)

    imgui::TableNextRow()
    imgui::TableSetColumnIndex(0)
    imgui::AlignTextToFramePadding()
    imgui::Text("{name}.wtm[0] (tm)")
    imgui::TableSetColumnIndex(1)
    SetNextItemWidth(-FLT_MIN)
    *changed = InputFloat3("##{name}_tm0", unsafe(addr(tm[0].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
    SetNextItemWidth(-FLT_MIN)
    *changed = InputFloat3("##{name}_tm1", unsafe(addr(tm[1].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
    SetNextItemWidth(-FLT_MIN)
    *changed = InputFloat3("##{name}_tm2", unsafe(addr(tm[2].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
    SetNextItemWidth(-FLT_MIN)
    *changed = InputFloat3("##{name}_tm3", unsafe(addr(tm[3].x)), "%.4f", ImGuiInputTextFlags.None) || *changed
    default_menu(eid, "{name}.wtm[0] (tm)", "TMatrix", $() => to_string(tm), $ [unused_argument(str)] (str : string) { logerr("Unsupported"); }, $() => to_string_via_blk(name, eid[0]), changed, sub_menu)
