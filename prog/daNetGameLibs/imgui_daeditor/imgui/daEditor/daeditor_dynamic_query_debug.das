options no_aot
options persistent_heap

require ecs
require strings
require DagorImgui
require DagorEditor
require EcsUtils
require DagorTime
require daslib/static_let
require imgui/imgui_boost
require daslib/safe_addr
require %danetlibs.imgui_daeditor.daEditor.daEditor_common

struct EntityData
  eid : EntityId
  eidStr : string
  templateStr : string
  distance : float

// common code must be sync with same code in ecs_scene_debug
def private daeditor_draw_entity_table(all_entities : array<EntityData>; var edit_item : EntityId&; var selected_item : EntityId&)
  if !BeginTable("split", 2, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    return false

  TableSetupColumn("eid", ImGuiTableColumnFlags WidthFixed)
  TableSetupColumn("template", ImGuiTableColumnFlags WidthStretch)

  var selected : bool
  var selectedAddr = safe_addr(selected)

  using() <| $(var clipper : ImGuiListClipper)
    Begin(clipper, length(all_entities), imgui::GetTextLineHeightWithSpacing())
    while Step(clipper)
      for idx in range(clipper.DisplayStart, clipper.DisplayEnd)
        let eid = all_entities[idx].eid
        let eidStr = all_entities[idx].eidStr
        let templateStr = all_entities[idx].templateStr
        imgui::TableNextRow()
        imgui::TableNextColumn()
        imgui::Text(eidStr)
        imgui::TableNextColumn()

        imgui::PushID(eidStr)
        selected = selected_item == eid || has(eid, "daeditor__selected")
        if imgui::Selectable(templateStr, selectedAddr, ImGuiSelectableFlags AllowDoubleClick)
          selected_item = eid
          if imgui::IsMouseDoubleClicked(ImGuiMouseButton Left)
            if GetIO().KeyCtrl
              edit_item = eid
              entity_object_editor_selectEntity(eid, !has(eid, "daeditor__selected"))
            elif GetIO().KeyShift
              var doSelect = false
              for it in all_entities
                entity_object_editor_selectEntity(it.eid, doSelect)
                if it.eid == eid || it.eid == edit_item
                  entity_object_editor_selectEntity(it.eid, true)
                  doSelect = !doSelect
            else
              edit_item = eid
              editor_select_eid(eid)
              entity_object_editor_zoomAndCenter()
        imgui::PopID()

  imgui::EndTable()
  return true

def private component_name_input(var comps : array<string>&; index : int; var changed : bool&)
  let buf <- ImGuiInputTextBuffer(comps[index], length(comps[index]) + 256)
  if InputText("##edit-{index}", buf, ImGuiInputTextFlags AutoSelectAll)
    comps[index] = to_string(buf)
    changed = true

  imgui::SameLine()
  if imgui::Button("dup")
    comps |> push(comps[index])
    changed = true

  imgui::SameLine()
  if imgui::Button("x")
    comps |> erase(index)
    changed = true

def private components_list(name : string; var comps : array<string>&; var changed : bool&)
  imgui::PushID(name)
  imgui::TableNextRow()
  imgui::TableNextColumn()
  let open = TreeNodeEx(name, ImGuiTreeNodeFlags None)
  imgui::TableNextColumn()

  if imgui::Button("+")
    comps |> push("")
    changed = true

  imgui::SameLine()
  imgui::Text("size: {length(comps)}")
  imgui::SameLine()
  if imgui::Button("Clear")
    comps |> clear()
    changed = true

  if open
    var index = -1
    while ++index < length(comps)
      imgui::PushID("{name}-{index}")
      imgui::TableNextRow()
      imgui::TableNextColumn()
      imgui::Text("[{index}]")
      imgui::TableNextColumn()
      component_name_input(comps, index, changed)
      imgui::PopID()
    imgui::TreePop()
  imgui::PopID()

[imgui_window(name="Dynamic Query", group="Editor")]
def dynamic_query_daeditor()
  if IsWindowCollapsed()
    return

  static_let <|
    var data : array<EntityData>
    var compsRQ : array<string>
    var compsNO : array<string>
    var compsChanged : bool = true
    var selectedItem = INVALID_ENTITY_ID
    var editedItem = INVALID_ENTITY_ID
    var lastUpdateDate = DagorTime::ref_time_ticks()

  let dt = DagorTime::get_time_usec(int64(lastUpdateDate))
  let sec = double(dt) / double(1000_000)
  if sec > 0.2lf && compsChanged
    delete data
    data |> reserve(1024)

    ecs_dynamic_query(compsRQ, compsNO) <| $(eid : EntityId)
      data |> emplace <| [[EntityData eidStr="{eid}", templateStr=getEntityTemplateName(eid), eid = eid]]

  if BeginTable("split", 2, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    components_list("Comps RQ", compsRQ, compsChanged)
    components_list("Comps NO", compsNO, compsChanged)
    imgui::EndTable()

  imgui::BeginChild("sub", float2(0., -20.))
  daeditor_draw_entity_table(data, editedItem, selectedItem)
  imgui::EndChild()
  imgui::Text("Total: {length(data)}")
