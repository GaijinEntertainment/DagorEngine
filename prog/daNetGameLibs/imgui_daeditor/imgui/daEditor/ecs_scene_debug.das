options no_aot
options persistent_heap

require DagorTime
require ecs
require EcsUtils
require DagorImgui
require DagorEditor
require DagorConsole
require DagorDataBlock
require imgui/imgui_boost
require daslib/static_let
require daslib/strings_boost
require daslib/safe_addr
require daslib/algorithm
require %danetlibs.imgui_daeditor.daEditor.daEditor_common
require %danetlibs.imgui_daeditor.imgui.imgui_common
require %danetlibs.imgui_daeditor.imgui.daEditor.ecs_inspectors_common


struct EntityData
  eid : EntityId
  eidStr : string
  templateStr : string
  extraStr : string
  distance : float

def DasHelpMarker(desc : string)
  TextDisabled("(?)")
  if IsItemHovered()
    BeginTooltip()
    PushTextWrapPos(GetFontSize() * 35.0f)
    TextUnformatted(desc)
    PopTextWrapPos()
    EndTooltip()

def get_max_mismatches_by_filter_length(filter_length : int)
  let allowedMismatchesByFilterLength <- [[int[] 3; 3; 5; 8; 10; 13]]
  var maxMismatches = 0
  for m in allowedMismatchesByFilterLength
    if filter_length > m
      maxMismatches ++
    else
      break
  return maxMismatches

def get_entity_displayed_name(template_str : string; extra_str : string)
  return strip(extra_str) == "" ? template_str : "{template_str} / {extra_str}"

[imgui_window(name="Scene", group="Editor", hotkey="Ctrl+Shift+/")]
def ecs_scene_window()
  if IsWindowCollapsed()
    return

  static_let_finalize <|
    var filter = new ImGuiTextFilter
  static_let <|
    var allEntities : array<EntityId>
    var prevData : array<EntityData>
    var data : array<EntityData>

    var searchComponents = false
    var includeComponent : array<string>
    var excludeComponent : array<string>
    var searchTypes = false
    var includeTypes : array<string>
    var excludeTypes : array<string>
    var shingles : array<string>
    var totalFilterLength = 0

    var dataIndex = 0
    var selectedItem = INVALID_ENTITY_ID
    var editedItem = INVALID_ENTITY_ID
    var lastUpdateDate = DagorTime::ref_time_ticks()
    var fuzzySearch = true
    var updateEntityList = true

  init_entity_object_editor()

  *filter |> set() <| imgui_get_blk_val("ecs_scene", "filter", "")
  fuzzySearch = imgui_get_blk_val("ecs_scene", "fuzzySearchEnabled", fuzzySearch)
  updateEntityList = imgui_get_blk_val("ecs_scene", "updateEntityList", updateEntityList)

  DasHelpMarker("Filter (\"incl,-excl\")\nt:typeName (t:EntityId, -t:EidList)\nc:componentName (c:animchar, -c:hero)")
  SameLine()
  SetNextItemWidth(-55.)
  *filter |> Draw("##entitiesFilter")

  SameLine()
  if imgui::Button("Clear")
    *filter |> Clear()

  imgui_set_blk_val("ecs_scene", "filter", to_string(*filter))

  imgui::BeginChild("sub", float2(0., -25.))

  // common code must be sync with other daeditor_draw_entity_table
  if BeginTable("split", 2, ImGuiTableFlags BordersOuter | ImGuiTableFlags RowBg)
    TableSetupColumn("eid", ImGuiTableColumnFlags WidthFixed)
    TableSetupColumn("template", ImGuiTableColumnFlags WidthStretch)

    let now = DagorTime::ref_time_ticks()
    let dt = DagorTime::get_time_usec(int64(lastUpdateDate))
    let sec = double(dt) / double(1000_000)
    if sec > 0.2lf && length(allEntities) == 0
      lastUpdateDate = now
      prevData <- data
      allEntities |> clear()
      allEntities |> reserve(4096 * 8)
      delete data
      data |> reserve(1024)

      searchComponents = false
      delete includeComponent
      delete excludeComponent
      searchTypes = false
      delete includeTypes
      delete excludeTypes
      var filters : table<string; int>
      for filterRange in filter.Filters
        if !filterRange |> empty()
          let filterString = string(filterRange)
          let filterStringLength = length(filterString)
          if filterStringLength >= 2 && filterString |> starts_with("c:")
            searchComponents = true
            let inc = filterString |> slice(2) |> strip()
            if length(inc) > 0
              includeComponent |> push(inc)
          elif filterStringLength >= 3 && filterString |> starts_with("-c:")
            searchComponents = true
            let exc = filterString |> slice(3) |> strip()
            if length(exc) > 0
              excludeComponent |> push(exc)
          elif filterStringLength >= 2 && filterString |> starts_with("t:")
            searchTypes = true
            let typ = filterString |> slice(2) |> strip()
            if length(typ) > 0
              includeTypes |> push(typ)
          elif filterStringLength >= 3 && filterString |> starts_with("-t:")
            searchTypes = true
            let typ = filterString |> slice(3) |> strip()
            if length(typ) > 0
              excludeTypes |> push(typ)
          else
            filters[filterString] = filterStringLength

      delete shingles
      totalFilterLength = 0
      for filterString, filterStringLength in keys(filters), values(filters)
        totalFilterLength += filterStringLength
        for i in range(filterStringLength - 1)
          shingles |> push(slice(filterString, i, i + 2))

      if updateEntityList && !is_editor_activated()
        entity_object_editor_updateObjectsList()
      entity_obj_editor_for_each_entity() <| $(ent)
        allEntities |> push(ent.eid)

    var selectedEid = INVALID_ENTITY_ID
    query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
      selectedEid = eid
    let selectedTemplateLength = length("+daeditor_selected")
    var allEntitiesNum = length(allEntities)
    if allEntitiesNum > 0
      let hasFilters = length(shingles) > 0
      var entitiesPerFrame = hasFilters || searchComponents || searchTypes ? 5000 : 10000
      let maxMismatches = fuzzySearch ? get_max_mismatches_by_filter_length(totalFilterLength) : 0
      let maxMisNeighbourCount = fuzzySearch ? 100 : 0
      while entitiesPerFrame-- > 0 && allEntitiesNum > 0
        let eid = allEntities[--allEntitiesNum]
        allEntities |> resize(allEntitiesNum)
        let eidStr = string(int(uint(eid)))
        let templateStr = getEntityTemplateName(eid)
        let extraStr = call_entity_extra_name_getter(eid)
        if searchComponents
          if length(excludeComponent) > 0
            let excluded = find_component(eid) <| $ [unused_argument(_)] (comp_name; _)
              for exclude in excludeComponent
                if find(comp_name, exclude) >= 0
                  return true
              return false
            if excluded
              continue
          if length(includeComponent) > 0
            let match = find_component(eid) <| $ [unused_argument(_)] (comp_name; _)
              for inc in includeComponent
                if find(comp_name, inc) >= 0
                  data |> emplace <| [[EntityData eid=eid, eidStr=eidStr, templateStr=templateStr, extraStr=extraStr]]
                  return true
              return false
            if match || !(hasFilters || searchTypes)
              continue
        if searchTypes
          if length(excludeTypes) > 0
            let excluded = find_component(eid) <| $ [unused_argument(_)] (_; comp)
              let typeName = getComponentTypes() |> component_types_findTypeName(comp.userType)
              for exc in excludeTypes
                if find(typeName, exc) >= 0
                  return true
              return false
            if excluded
              continue
          if length(includeTypes) > 0
            let match = find_component(eid) <| $ [unused_argument(_)] (_; comp)
              let typeName = getComponentTypes() |> component_types_findTypeName(comp.userType)
              for inc in includeTypes
                if find(typeName, inc) >= 0
                  data |> emplace <| [[EntityData eid=eid, eidStr=eidStr, templateStr=templateStr, extraStr=extraStr]]
                  return true
              return false
            if match || !hasFilters
              continue
        if hasFilters
          var misNeighbourCount = 0
          var mismatchCount = 0
          var prevIdx = -1
          let searchedStr = get_entity_displayed_name(templateStr, extraStr)
          if !(*filter |> PassFilter(eidStr))
            for shingle in shingles
              let idx = find(searchedStr, shingle)
              if idx == -1
                mismatchCount++
                if mismatchCount > maxMismatches
                  break
              elif prevIdx != -1 && prevIdx + 1 != idx
                misNeighbourCount++
                if misNeighbourCount > maxMisNeighbourCount
                  break
              prevIdx = idx
            if mismatchCount > maxMismatches || misNeighbourCount > maxMisNeighbourCount
              continue
          let templStrLength = length(searchedStr) - (eid == selectedEid ? selectedTemplateLength : 0)
          let itemDistance = float(mismatchCount) + abs(float(templStrLength) - float(length(shingles) + 1)) * 0.015 + float(misNeighbourCount) * 0.5
          var value = [[EntityData eid=eid, eidStr=eidStr, templateStr=templateStr, extraStr=extraStr, distance=itemDistance]]
          let insertIdx = lower_bound(data, value) <| $(x, y) => x.distance < y.distance
          data |> emplace(value, insertIdx)
        else
          data |> emplace <| [[EntityData eid=eid, eidStr=eidStr, templateStr=templateStr, extraStr=extraStr]]

    var selected : bool
    var selectedAddr = safe_addr(selected)

    using() <| $(var clipper : ImGuiListClipper)
      clipper |> Begin(length(prevData), imgui::GetTextLineHeightWithSpacing())
      while clipper |> Step()
        for idx in range(clipper.DisplayStart, clipper.DisplayEnd)
          let eid = prevData[idx].eid
          let eidStr = prevData[idx].eidStr
          let templateStr = prevData[idx].templateStr
          let extraStr = prevData[idx].extraStr
          let displayStr = get_entity_displayed_name(templateStr, extraStr)
          imgui::TableNextRow()
          imgui::TableNextColumn()
          imgui::Text(eidStr)
          imgui::TableNextColumn()
          selected = selectedItem == eid || eid |> has("daeditor__selected")
          imgui::PushID(eidStr)
          if imgui::Selectable(displayStr, selectedAddr, ImGuiSelectableFlags AllowDoubleClick)
            selectedItem = eid
            if imgui::IsMouseDoubleClicked(ImGuiMouseButton Left)
              if GetIO().KeyCtrl
                editedItem = eid
                entity_object_editor_selectEntity(eid, !has(eid, "daeditor__selected"))
              elif GetIO().KeyShift
                var doSelect = false
                for it in prevData
                  entity_object_editor_selectEntity(it.eid, doSelect)
                  if it.eid == eid || it.eid == editedItem
                    entity_object_editor_selectEntity(it.eid, true)
                    doSelect = !doSelect
              else
                editedItem = eid
                editor_select_eid(eid)
                entity_object_editor_zoomAndCenter()
          imgui::PopID()

    imgui::EndTable()

  imgui::EndChild()

  imgui::Text("Total: {length(prevData)}")
  imgui::SameLine(imgui::GetWindowWidth() - 240.0)
  imgui::Checkbox("Update List", safe_addr(updateEntityList))
  if IsItemHovered()
    BeginTooltip()
    TextUnformatted("Auto update list when entities are created/removed.\nWhen daEditor is active this is handled by daEditor itself")
    EndTooltip()
  imgui::SameLine(imgui::GetWindowWidth() - 120.0)
  imgui::Checkbox("Fuzzy Search", safe_addr(fuzzySearch))

  imgui_set_blk_val("ecs_scene", "fuzzySearchEnabled", fuzzySearch)
  imgui_set_blk_val("ecs_scene", "updateEntityList", updateEntityList)


[console_cmd(name="imgui.editor_ecs_scene")]
def show_ecs_scene()
  let isVisible = imgui_get_state() != ImGuiState OFF && imgui_window_is_visible("Editor", "Scene")
  let setVisible = !isVisible
  console_command(setVisible ? "imgui.activate" : "imgui.off")
  imgui_window_set_visible("Editor", "Scene", setVisible)


[init]
def init_ecs_inspector_shortcut()
  console_command("consoleKeybindings.bind ctrl+shift+/ imgui.editor_ecs_scene")