require ecs
require EcsUtils
require math.base
require strings
require imgui
require DagorImgui
require DagorEditor
require DagorConsole
require imgui/imgui_boost

require daslib/static_let
require daslib/safe_addr
require daslib/defer
require %danetlibs.imgui_daeditor.imgui.imgui_common
require danetlibs.imgui_daeditor.daEditor.daEditor_common

let
  TEMPLATE_MAX_LEN = 1024

def DasHelpMarker(desc : string)
  TextDisabled("(?)")
  if IsItemHovered()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(desc)
    PopTextWrapPos()
    EndTooltip()


[imgui_window(name="Create entity", group="Editor", hotkey="Ctrl+Shift+[")]
def create_entity_daeditor()
  if IsWindowCollapsed()
    return

  DasHelpMarker("Insert template name here.\nDouble LMB click to paste template name.\nCtrl + double LMB click to append template")
  imgui::SameLine()

  let templateNameStr = imgui_get_blk_val("daeditor_create_entity", "templateName", "")
  var templateName = new ImGuiInputTextBuffer(templateNameStr, TEMPLATE_MAX_LEN)

  InputText("##edit", *templateName)

  imgui::SameLine()
  if imgui::Button("Clear")
    *templateName |> clear()
  imgui::SameLine()
  if imgui::Button("Create entity") && !empty(to_string(*templateName))
    var tm = IDENT_TM
    find_query() <| $ [es] (camera__active : bool; transform : float3x4)
      if camera__active
        tm[3] = transform[3] + transform |> rotate <| float3(0f, 0f, 2f)
      return false
    editor_open(false)
    let eid = entity_object_editor_createEntityDirect(to_string(*templateName), tm)
    editor_open()
    editor_select_eid(eid)

  DasHelpMarker("Filter (\"incl,-excl\")")
  imgui::SameLine()

  var filter = new ImGuiTextFilter
  defer <|
    unsafe
      delete filter
  *filter |> set() <| imgui_get_blk_val("daeditor_create_entity", "filter", "")
  *filter |> Draw("##create_entity_filter")

  imgui_set_blk_val("daeditor_create_entity", "filter", to_string(*filter))

  static_let <|
    var selectedItem = ""

  imgui::BeginChild("sub", float2(0, 0))

  if imgui::BeginTable("list", 1, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    getTemplateDB() |> find_templateDB() <| $(name; tmpl)
      if !(*filter |> PassFilter(name))
        return false
      imgui::TableNextRow()
      TableNextColumn()
      let text = "{tmpl.isSingleton ? "[S] " : ""}{name}"
      var selected = selectedItem == name
      if imgui::Selectable(text, safe_addr(selected), ImGuiSelectableFlags AllowDoubleClick)
        if imgui::IsMouseDoubleClicked(ImGuiMouseButton Left)
          if !GetIO().KeyCtrl || *templateName |> to_string |> empty()
            *templateName |> clear()
            *templateName |> make_input_buffer(clone_string(name), TEMPLATE_MAX_LEN)
          else
            let prevTempl = to_string(*templateName)
            let newTempl = empty(prevTempl) ? clone_string(name) : "{prevTempl}+{name}"
            *templateName |> clear()
            *templateName |> make_input_buffer(newTempl, TEMPLATE_MAX_LEN)
      if selected && selectedItem != name
        selectedItem = clone_string(name)
      return false

    imgui::EndTable()

  imgui::EndChild()

  imgui_set_blk_val("daeditor_create_entity", "templateName", to_string(*templateName))
