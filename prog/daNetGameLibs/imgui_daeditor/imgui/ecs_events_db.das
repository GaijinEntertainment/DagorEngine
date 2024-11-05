require ecs
require strings
require imgui
require DagorImgui
require daslib/static_let
require daslib/defer
require imgui/imgui_boost
require EcsUtils
require %danetlibs.imgui_daeditor.imgui.imgui_common


def describe_event_extra_flag(flag : uint16)
  var res = ""
  if (uint(flag) & uint(ecs::EVFLG_SCHEMELESS)) == uint(ecs::EVFLG_SCHEMELESS)
    res = empty(res) ? "schemeless" : res + " schemeless"

  if (uint(flag) & uint(ecs::EVFLG_DESTROY)) == uint(ecs::EVFLG_DESTROY)
    res = empty(res) ? "with destructor" : res + " with destructor"

  if (uint(flag) & uint(ecs::EVFLG_SERIALIZE)) == uint(ecs::EVFLG_SERIALIZE)
    res = empty(res) ? "serializable" : res + " serializable"

  return res


def describe_event_cast_flag(flag : uint16)
  if (uint(flag) & uint(ecs::EVCAST_BOTH)) == uint(ecs::EVCAST_BOTH)
    return "Unicast & Broadcast"

  if (uint(flag) & uint(ecs::EVCAST_UNICAST)) == uint(ecs::EVCAST_UNICAST)
    return "Unicast"

  if (uint(flag) & uint(ecs::EVCAST_BROADCAST)) == uint(ecs::EVCAST_BROADCAST)
    return "Broadcast"

  return "<n/a>"

def imgui_hint(desc : string)
  imgui_hint <| $
    return desc

def imgui_hint(desc : block<() : string>)
  if IsItemHovered()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(invoke(desc))
    PopTextWrapPos()
    EndTooltip()

def imgui_click_to_copy(val)
  if imgui::IsItemHovered()
    imgui::BeginTooltip()
    imgui::PushTextWrapPos(GetFontSize() * 35.0f)
    imgui::TextUnformatted("RMB click to copy value")
    imgui::PopTextWrapPos()
    imgui::EndTooltip()
    if imgui::IsMouseClicked(ImGuiMouseButton Right)
      SetClipboardText("{val}")


[imgui_window(name="Events db", group="ECS")]
def event_db_window()
  if IsWindowCollapsed()
    return
  let eventsDb & = unsafe(get_events_db())
  let eventsNum = eventsDb |> events_db_getEventsCount()

  SetNextItemWidth(-FLT_MIN)
  var filter = new ImGuiTextFilter
  defer <|
    unsafe
      delete filter
  *filter |> set() <| imgui_get_blk_val("ecs_events_db", "filter", "")

  *filter |> Draw("##eventsDbFilter")

  imgui_set_blk_val("ecs_events_db", "filter", to_string(*filter))

  imgui::BeginChild("sub", float2(0, 0))

  if BeginTable("split", 4, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    TableSetupColumn("Name", ImGuiTableColumnFlags WidthFixed)
    TableSetupColumn("Type", ImGuiTableColumnFlags WidthFixed)
    TableSetupColumn("Size", ImGuiTableColumnFlags WidthFixed)
    TableSetupColumn("Description", ImGuiTableColumnFlags WidthFixed)
    TableHeadersRow()
    for i in range(eventsNum)
      let id = uint(i)
      let name = eventsDb |> events_db_getEventName(id)
      let evtType = eventsDb |> events_db_getEventType(id)
      let flags = eventsDb |> events_db_getEventFlags(id)
      let castFlag = describe_event_cast_flag(flags)
      let extraFlags = describe_event_extra_flag(flags)
      let withScheme = eventsDb |> events_db_hasEventScheme(id)

      if !(*filter |> PassFilter(name) || *filter |> PassFilter("{evtType}")
        || *filter |> PassFilter(castFlag) || *filter |> PassFilter(extraFlags)
        || (withScheme && *filter |> PassFilter("event structure")))
        continue
      imgui::TableNextRow()
      imgui::TableNextColumn()
      imgui::Text("{name}")
      imgui_click_to_copy(name)

      imgui::TableNextColumn()
      imgui::Text("{evtType}")
      imgui_click_to_copy(evtType)

      imgui::TableNextColumn()
      let size = eventsDb |> events_db_getEventSize(id)
      imgui::Text("{int(size)}")
      imgui_hint() <| $()
        return "{int(size)} bytes"

      imgui::TableNextColumn()
      imgui::Text("{castFlag} {extraFlags}")

      if withScheme
        imgui::TableNextRow()
        imgui::TableNextColumn()

        let fieldsCount = eventsDb |> events_db_getFieldsCount(id)
        let treeFlags = fieldsCount == 0u ? ImGuiTreeNodeFlags Leaf | ImGuiTreeNodeFlags NoTreePushOnOpen | ImGuiTreeNodeFlags Bullet : ImGuiTreeNodeFlags DefaultOpen
        let open = TreeNodeEx("Event structure##{id}", treeFlags)

        imgui::TableNextColumn()
        let schemeHash = eventsDb |> events_db_getEventSchemeHash(id)
        imgui::Text("{schemeHash}")
        imgui_click_to_copy(schemeHash)

        if open && fieldsCount > 0u
          for j in range(fieldsCount)
            let fieldName = eventsDb |> events_db_getFieldName(id, j)
            let fieldType = eventsDb |> events_db_getFieldType(id, j)
            let fieldOffset = eventsDb |> events_db_getFieldOffset(id, j)
            let typeName = getComponentTypes() |> component_types_findTypeName(fieldType)

            imgui::TableNextRow()
            imgui::TableNextColumn()
            imgui::Text("{fieldName}: {typeName}")

            imgui::TableNextColumn()
            imgui::Text("{fieldType}")
            imgui_click_to_copy(fieldType)

            imgui::TableNextColumn()
            imgui::Text("{int(fieldOffset)}")
            imgui_hint() <| $()
              return "offset {int(fieldOffset)} bytes"

          imgui::TreePop()



    EndTable()

  imgui::EndChild()

