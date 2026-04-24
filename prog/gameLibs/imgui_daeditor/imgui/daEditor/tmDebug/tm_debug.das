options no_aot

require ecs
require ecs.common
require EcsUtils
require DagorMath
require DagorImgui
require DagorConsole
require DagorDebug3DSolid
require DagorEditor
require gameLibs.imgui_daeditor.daEditor.daEditor_common
require imgui/imgui_boost
require daslib/strings_boost
require daslib/static_let
require daslib/safe_addr

[imgui_window(name="Transform", group="Editor")]
def trasnform_window()
  if IsWindowCollapsed()
    return

  static_let <|
    var step = 1.0
    var rotStep = PI / 6.0
    var tmCompName = "transform"
    var lastSelectedEid = INVALID_ENTITY_ID

  var tm : float3x4
  var hasSelectedTm = false
  var hasSelected = false
  var isCursorTm = false
  var selectedEid = INVALID_ENTITY_ID
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId; cursor_tm__p1 : float3 const?)
    selectedEid = eid
    hasSelected = true
    isCursorTm = cursor_tm__p1 != null

  var inscope tmComponents : array<string>
  if hasSelected
    collect_tm_components(selectedEid, tmComponents)

    if selectedEid != lastSelectedEid
      lastSelectedEid = selectedEid
      tmCompName = "transform"

    var hasCurrent = false
    var hasTransform = false
    for name in tmComponents
      if name == tmCompName
        hasCurrent = true
      if name == "transform"
        hasTransform = true

    if !hasCurrent
      if hasTransform
        tmCompName = "transform"
      elif length(tmComponents) > 0
        tmCompName = tmComponents[0]

    var tmRef = get_tm_component_ref(selectedEid, tmCompName)
    if tmRef != null
      tm := *tmRef
      hasSelectedTm = true

  let stepChange = 0.1
  let fastStepChange = 1.0
  imgui::Text("step:")
  SameLine()
  SetNextItemWidth(100.0)
  InputScalar("##cursor_step", ImGuiDataType.Float, safe_addr(step), safe_addr(stepChange), safe_addr(fastStepChange), "%.2f", ImGuiInputTextFlags.None)
  SameLine()
  imgui::Text("rot step:")
  SameLine()
  SetNextItemWidth(100.0)
  InputScalar("##cursor_rot_step", ImGuiDataType.Float, safe_addr(rotStep), safe_addr(stepChange), safe_addr(fastStepChange), "%.2f", ImGuiInputTextFlags.None)
  imgui::Text("component:")
  SameLine()
  SetNextItemWidth(200.0)
  if hasSelected && length(tmComponents) > 0
    if BeginCombo("##tm_component", tmCompName, ImGuiComboFlags.None)
      for name in tmComponents
        var selected = name == tmCompName
        var selectedAddr = safe_addr(selected)
        if imgui::Selectable(name, selectedAddr)
          tmCompName = name
        if selected
          SetItemDefaultFocus()
      EndCombo()
  elif hasSelected
    imgui::Text("no float3x4 components")
  else
    imgui::Text("select entity")
  imgui::Separator()

  if hasSelectedTm
    ImGuiEx_BeginColumn()

    imgui::Text("Grow side 1")

    if imgui::Button("+##cursor_ex[0]")
      extend_tm(tmCompName, 0, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[0]")
      extend_tm(tmCompName, 0, -step, false)

    if imgui::Button("+##cursor_ex[1]")
      extend_tm(tmCompName, 1, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[1]")
      extend_tm(tmCompName, 1, -step, false)

    if imgui::Button("+##cursor_ex[2]")
      extend_tm(tmCompName, 2, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[2]")
      extend_tm(tmCompName, 2, -step, false)

    ImGuiEx_NextColumn()

    imgui::Text("Grow side 2")

    if imgui::Button("+##cursor_shr[0]")
      extend_tm(tmCompName, 0, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[0]")
      extend_tm(tmCompName, 0, -step, true)

    if imgui::Button("+##cursor_shr[1]")
      extend_tm(tmCompName, 1, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[1]")
      extend_tm(tmCompName, 1, -step, true)

    if imgui::Button("+##cursor_shr[2]")
      extend_tm(tmCompName, 2, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[2]")
      extend_tm(tmCompName, 2, -step, true)

    ImGuiEx_NextColumn()

    imgui::Text("Move along axis")

    if imgui::Button("+##cursor_mv[0]")
      move_tm(tmCompName, 0, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[0]")
      move_tm(tmCompName, 0, step)

    if imgui::Button("+##cursor_mv[1]")
      move_tm(tmCompName, 1, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[1]")
      move_tm(tmCompName, 1, step)

    if imgui::Button("+##cursor_mv[2]")
      move_tm(tmCompName, 2, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[2]")
      move_tm(tmCompName, 2, step)

    ImGuiEx_NextColumn()

    imgui::Text("Rotate")

    if imgui::Button("+##cursor_rt[0]")
      rotate_tm(tmCompName, 0, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[0]")
      rotate_tm(tmCompName, 0, -rotStep)

    if imgui::Button("+##cursor_rt[1]")
      rotate_tm(tmCompName, 1, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[1]")
      rotate_tm(tmCompName, 1, -rotStep)

    if imgui::Button("+##cursor_rt[2]")
      rotate_tm(tmCompName, 2, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[2]")
      rotate_tm(tmCompName, 2, -rotStep)

    ImGuiEx_NextColumn()

    imgui::Text("FLIP")

    if imgui::Button("^##cursor_fp[0]")
      flip_tm(tmCompName, 0)

    if imgui::Button("^##cursor_fp[1]")
      flip_tm(tmCompName, 1)

    if imgui::Button("^##cursor_fp[2]")
      flip_tm(tmCompName, 2)

    ImGuiEx_NextColumn()

    imgui::Text("SWAP")

    if imgui::Button("^##cursor_sw[0]")
      swap_tm(tmCompName, 0)

    if imgui::Button("^##cursor_sw[1]")
      swap_tm(tmCompName, 1)

    if imgui::Button("^##cursor_sw[2]")
      swap_tm(tmCompName, 2)

    ImGuiEx_EndColumn()


  if hasSelectedTm
    imgui::Separator()
    if imgui::Button("Make IDENT##")
      make_ident_tm(tmCompName)

    if !isCursorTm
      SameLine()
      if imgui::Button("Draw selected tm##")
        draw_debug_tm_buffered(tm, E3DCOLOR(0x7733ff33), 1000)

  if hasSelectedTm && isCursorTm
    imgui::Separator()
    query() <| $ [es(REQUIRE=daeditor__selected)] (var cursor_tm__alpha : float&; cursor_tm__p1, cursor_tm__p2 : float3)
      if cursor_tm__p1 != float3() && cursor_tm__p2 != float3()
        if imgui::Button("-##cursor_alpha")
          cursor_tm__alpha -= 0.01
        SameLine()
        if imgui::Button("--##cursor_alpha")
          cursor_tm__alpha -= 0.1
        SameLine()
        imgui::Text("Alpha: {cursor_tm__alpha}")
        SameLine()
        if imgui::Button("+##cursor_alpha")
          cursor_tm__alpha += 0.01
        SameLine()
        if imgui::Button("++##cursor_alpha")
          cursor_tm__alpha += 0.1

    query() <| $ [es(REQUIRE=cursor__pos)] ()
      if imgui::Button("Set p1 from cursor##")
        console_command("cursor.tm_set_p1")
      SameLine()
      if imgui::Button("Set p2 from cursor##")
        console_command("cursor.tm_set_p2")

  imgui::Separator()
  if imgui::Button("toogle 3d cursor##")
    console_command("cursor.toggle_3d_cursor")
  SameLine()
  if imgui::Button("create cursor TM##")
    query() <| $ [es] (camera__active : bool; transform : float3x4)
      if camera__active
        var ctm := transform
        ctm[3] += ctm[2] * 2.0
        let cEid = createEntitySync("cursor_tm") <| $(init)  //sync because we want to select it immediately
          init |> set("transform", ctm)
        editor_open(false)
        editor_open(true)
        entity_object_unselectAll()
        entity_object_editor_selectEntity(cEid, true)
  SameLine()
  if imgui::Button("delete all cursor TMs##")
    query() <| $ [es(REQUIRE=cursor_tm__color)] (eid : EntityId)
      destroyEntity(eid)


def collect_tm_components(eid : EntityId; var names : array<string>&)
  let compsNum = getEntityManager() |> getNumComponents(eid)
  for idx in urange(compsNum >= 0 ? compsNum : 0)
    let compRef = getEntityManager() |> getEntityComponentRef(eid, idx)
    let typeName = getComponentTypes() |> component_types_findTypeName(compRef.userType)
    if typeName != "TMatrix"
      continue
    let name = getDataComponents() |> data_components_getComponentNameById(compRef.componentId)
    names |> push(name)

def find_tm_component_id(eid : EntityId; comp_name : string) : uint16
  let compsNum = getEntityManager() |> getNumComponents(eid)
  for idx in urange(compsNum >= 0 ? compsNum : 0)
    let compRef = getEntityManager() |> getEntityComponentRef(eid, idx)
    let name = getDataComponents() |> data_components_getComponentNameById(compRef.componentId)
    if name != comp_name
      continue
    let typeName = getComponentTypes() |> component_types_findTypeName(compRef.userType)
    return typeName == "TMatrix" ? compRef.componentId : INVALID_COMPONENT_INDEX
  return INVALID_COMPONENT_INDEX

def get_tm_component_ref(eid : EntityId; comp_name : string) : float3x4?
  let compRef = getEntityManager() |> getComponentRef(eid, comp_name)
  if compRef.rawData == null
    return null
  let typeName = getComponentTypes() |> component_types_findTypeName(compRef.userType)
  if typeName != "TMatrix"
    return null
  return unsafe(reinterpret<float3x4?> compRef.rawData)

def get_tm_component_ref_rw(eid : EntityId; comp_name : string) : float3x4?
  let compId = find_tm_component_id(eid, comp_name)
  if compId == INVALID_COMPONENT_INDEX
    return null
  var compRef : EntityComponentRef = getEntityManagerRW() |> getComponentRefRW(eid, compId)
  if compRef.rawData == null
    return null
  return unsafe(reinterpret<float3x4 -const?> compRef.rawData)

def make_ident_tm(comp_name : string)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    (*tm)[0] = float3(1, 0, 0)
    (*tm)[1] = float3(0, 1, 0)
    (*tm)[2] = float3(0, 0, 1)
    entity_obj_editor_saveComponent(eid, comp_name)

def extend_tm(comp_name : string; axis : int; step : float; from_right : bool)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    (*tm)[axis] += normalize((*tm)[axis]) * step
    (*tm)[3] -= normalize((*tm)[axis]) * step * 0.5 * (from_right ? 1.0 : -1.0)
    entity_obj_editor_saveComponent(eid, comp_name)

def move_tm(comp_name : string; axis : int; step : float)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    (*tm)[3] -= normalize((*tm)[axis]) * step
    entity_obj_editor_saveComponent(eid, comp_name)

def rotate_tm(comp_name : string; axis : int; step : float)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    let q = quat((*tm)[axis], step)
    (*tm)[0] = q * (*tm)[0]
    (*tm)[1] = q * (*tm)[1]
    let v3Ln = length((*tm)[2])
    (*tm)[2] = normalize(cross((*tm)[0], (*tm)[1])) * v3Ln
    entity_obj_editor_saveComponent(eid, comp_name)

def flip_tm(comp_name : string; idx : int)
  let m = fixed_array<float>(-1.0, -1.0, 1.0)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    for i in range(3)
      let k = (i + idx) % 3
      (*tm)[k] *= m[i]
    entity_obj_editor_saveComponent(eid, comp_name)

def swap_tm(comp_name : string; idx : int)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    var tm = get_tm_component_ref_rw(eid, comp_name)
    if tm == null
      return
    let idx2 = (idx + 1) % 3
    let idx3 = (idx + 2) % 3
    swap((*tm)[idx], (*tm)[idx2])
    (*tm)[idx3] *= -1.0
    entity_obj_editor_saveComponent(eid, comp_name)