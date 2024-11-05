options no_aot

require ecs
require strings
require imgui
require DagorImgui
require DngNet
require DagorDebug3D
require DagorMath
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common

require daslib/safe_addr
require daslib/static_let
require daslib/defer


[ecs_inspector(type_name="LocSnapshotsList", no_aot)]
def draw_LocSnapshotList(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<LocSnapshotsList?> refs[0].rawData)
  assume list = *val

  static_let <|
    var showSnapshots = false

  imgui::Text("{length(list)} snapshots")
  imgui::SameLine()
  imgui::Checkbox("Show##LocSnapshotsList", safe_addr(showSnapshots))

  if showSnapshots
    if BeginTable("Snapshots", 8, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
      TableSetupColumn("atTime", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("dt", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("bli", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("dd", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("x", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("y", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("z", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("", ImGuiTableColumnFlags WidthFixed)
      TableHeadersRow()

    var prevPos = float3()
    var prevT = .0
    for snapshotData in list
      imgui::TableNextRow()
      imgui::TableNextColumn()
      imgui::Text(format("%.2f", snapshotData.atTime))
      imgui::TableNextColumn()
      if prevT != .0
        imgui::Text(format("%.0f", (snapshotData.atTime - prevT) * 1000.0))
      imgui::TableNextColumn()
      imgui::Text(snapshotData.blink ? "+" : "")
      imgui::TableNextColumn()
      if prevPos != float3()
        imgui::Text(format("%.3f", distance(snapshotData.pos, prevPos)))
      imgui::TableNextColumn()
      imgui::Text(format("%.2f", snapshotData.pos.x))
      imgui::TableNextColumn()
      imgui::Text(format("%.2f", snapshotData.pos.y))
      imgui::TableNextColumn()
      imgui::Text(format("%.2f", snapshotData.pos.z))
      imgui::TableNextColumn()
      if imgui::Button("show##{name}")
        draw_debug_sphere_buffered(snapshotData.pos, 1.0, E3DCOLOR(0xFFFFFF00), 12, 2500)
        var upPoint = snapshotData.pos
        upPoint.y += 150.0
        draw_debug_line_buffered(snapshotData.pos, upPoint, E3DCOLOR(0xFFFFFF00), 2500)
      prevPos = snapshotData.pos
      prevT = snapshotData.atTime
    EndTable()

  default_menu(eid, name, "LocSnapshotList", changed, sub_menu)