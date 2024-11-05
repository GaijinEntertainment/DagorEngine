options no_aot
options persistent_heap

require ecs
require imgui
require PhysVars
require DagorImgui
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_macro
require danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspector_common

require daslib/safe_addr
require daslib/static_let
require daslib/defer

def draw_popup(name : string; value : string)
  if imgui::Button("*##physVarCopy__{name}")
    imgui::OpenPopup("physVarPopup_{name}")

  if imgui::BeginPopup("physVarPopup_{name}")
    if imgui::Button("Copy##physVar_name")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(name)
    imgui::SameLine()
    imgui::Text(name)

    if imgui::Button("Copy##physVar_value")
      imgui::CloseCurrentPopup()
      imgui::SetClipboardText(value)
    imgui::SameLine()
    imgui::Text(value)

    imgui::EndPopup()

[ecs_inspector(type_name="PhysVars")]
def draw_phys_vars_edit(eid : array<EntityId> implicit; flags : uint; name : string; refs : array<EntityComponentRef#>#; level : int; var changed : bool?; sub_menu : block<(var changed : bool?) : bool>) : void
  var val = unsafe(reinterpret<PhysVars?> refs[0].rawData)
  assume phys_vars = *val

  static_let <|
    var showVars = false

  let varsCount = phys_vars |> getVarsCount()
  imgui::Text("{varsCount} phys vars")
  imgui::SameLine()
  imgui::Checkbox("Show##physVars", safe_addr(showVars))

  if showVars
    static_let_finalize <|
      var filter = new ImGuiTextFilter

    DasHelpMarker("Filter (\"incl,-excl\")")
    imgui::SameLine()
    *filter |> Draw("##physVarsFilter_{eid}")

    if BeginTable("PhysVars", 5, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
      TableSetupColumn("Id", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("[P]", ImGuiTableColumnFlags WidthFixed)
      TableSetupColumn("Name", ImGuiTableColumnFlags WidthStretch)
      TableSetupColumn("Value", ImGuiTableColumnFlags WidthStretch)
      TableSetupColumn("*", ImGuiTableColumnFlags WidthFixed)
      TableHeadersRow()

    for i in 0..varsCount
      let varName = phys_vars |> getVarName(i)
      let filteredName = "{i} {varName}" // to allow filtering by both the id and var name
      if !(*filter |> PassFilter(filteredName))
        continue

      imgui::TableNextRow()
      imgui::TableNextColumn()
      imgui::Text("{i}")
      imgui::TableNextColumn()
      imgui::Text(phys_vars |> isVarPullable(i) ? "[P]" : "")
      imgui::TableNextColumn()
      imgui::Text(varName)
      imgui::TableNextColumn()
      let varValueText = "{phys_vars |> getVar(i)}"
      imgui::Text(varValueText)

      imgui::TableNextColumn()
      draw_popup(varName, varValueText)
    EndTable()

  default_menu(eid, name, "PhysVars", changed, sub_menu)
