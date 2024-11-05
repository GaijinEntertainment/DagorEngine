options no_aot

require ecs
require app
require DagorMath
require DagorImgui
require DagorConsole
require DagorDebug3DSolid
require DagorEditor
require danetlibs.imgui_daeditor.daEditor.daEditor_common
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

  var tm : float3x4
  var hasSelectedTm = false
  var isCursorTm = false
  query() <| $ [es(REQUIRE=daeditor__selected)] (transform : float3x4; cursor_tm__p1 : float3 const?)
    tm := transform
    hasSelectedTm = true
    isCursorTm = cursor_tm__p1 != null

  let stepChange = 0.1
  let fastStepChange = 1.0
  imgui::Text("step:")
  SameLine()
  SetNextItemWidth(100.0)
  InputScalar("##cursor_step", ImGuiDataType Float, safe_addr(step), safe_addr(stepChange), safe_addr(fastStepChange), "%.2f", ImGuiInputTextFlags None)
  SameLine()
  imgui::Text("rot step:")
  SameLine()
  SetNextItemWidth(100.0)
  InputScalar("##cursor_rot_step", ImGuiDataType Float, safe_addr(rotStep), safe_addr(stepChange), safe_addr(fastStepChange), "%.2f", ImGuiInputTextFlags None)
  imgui::Separator()

  if hasSelectedTm
    ImGuiEx_BeginColumn()

    imgui::Text("Grow side 1")

    if imgui::Button("+##cursor_ex[0]")
      extend_tm(0, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[0]")
      extend_tm(0, -step, false)

    if imgui::Button("+##cursor_ex[1]")
      extend_tm(1, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[1]")
      extend_tm(1, -step, false)

    if imgui::Button("+##cursor_ex[2]")
      extend_tm(2, step, false)
    SameLine()
    if imgui::Button("-##cursor_ex[2]")
      extend_tm(2, -step, false)

    ImGuiEx_NextColumn()

    imgui::Text("Grow side 2")

    if imgui::Button("+##cursor_shr[0]")
      extend_tm(0, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[0]")
      extend_tm(0, -step, true)

    if imgui::Button("+##cursor_shr[1]")
      extend_tm(1, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[1]")
      extend_tm(1, -step, true)

    if imgui::Button("+##cursor_shr[2]")
      extend_tm(2, step, true)
    SameLine()
    if imgui::Button("-##cursor_shr[2]")
      extend_tm(2, -step, true)

    ImGuiEx_NextColumn()

    imgui::Text("Move along axis")

    if imgui::Button("+##cursor_mv[0]")
      move_tm(0, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[0]")
      move_tm(0, step)

    if imgui::Button("+##cursor_mv[1]")
      move_tm(1, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[1]")
      move_tm(1, step)

    if imgui::Button("+##cursor_mv[2]")
      move_tm(2, -step)
    SameLine()
    if imgui::Button("-##cursor_mv[2]")
      move_tm(2, step)

    ImGuiEx_NextColumn()

    imgui::Text("Rotate")

    if imgui::Button("+##cursor_rt[0]")
      rotate_tm(0, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[0]")
      rotate_tm(0, -rotStep)

    if imgui::Button("+##cursor_rt[1]")
      rotate_tm(1, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[1]")
      rotate_tm(1, -rotStep)

    if imgui::Button("+##cursor_rt[2]")
      rotate_tm(2, rotStep)
    SameLine()
    if imgui::Button("-##cursor_rt[2]")
      rotate_tm(2, -rotStep)

    ImGuiEx_EndColumn()


  if hasSelectedTm
    imgui::Separator()
    if imgui::Button("Make IDENT##")
      query() <| $ [es(REQUIRE=daeditor__selected)] (var transform : float3x4)
        transform[0] = float3(1, 0, 0)
        transform[1] = float3(0, 1, 0)
        transform[2] = float3(0, 0, 1)

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


def extend_tm(axis : int; step : float; from_right : bool)
  query() <| $ [es(REQUIRE=daeditor__selected)] (var transform : float3x4)
    transform[axis] += normalize(transform[axis]) * step
    transform[3] -= normalize(transform[axis]) * step * 0.5 * (from_right ? 1.0 : -1.0)

def move_tm(axis : int; step : float)
  query() <| $ [es(REQUIRE=daeditor__selected)] (var transform : float3x4)
    transform[3] -= normalize(transform[axis]) * step

def rotate_tm(axis : int; step : float)
  query() <| $ [es(REQUIRE=daeditor__selected)] (var transform : float3x4)
    let q = quat(transform[axis], step)
    transform[0] = q * transform[0]
    transform[1] = q * transform[1]
    let v3Ln = length(transform[2])
    transform[2] = normalize(cross(transform[0], transform[1])) * v3Ln
