options no_aot

require ecs
require math
require math.base
require DagorMath
require DagorImgui
require imgui
require daslib/safe_addr
require daslib/static_let
require AnimV20
require GeomNodeTree
require DngCamera
require ImGuizmo


let ROTATE_SENS = 0.01 // rad per px, matches freecam look sensitivity
let PAN_SENS = 0.002 // fraction of pivot distance per px
let ZOOM_SENS = 0.005 // exponent per px
let WHEEL_SENS = 0.17 // exponent per wheel notch
let MIN_DIST = 0.05
let MAX_DIST = 1000.0
let DEFAULT_DIST = 3.0


enum MoveMode
  NONE
  ROTATE
  PAN
  ZOOM


def nav_dist(store : float = 0.0) : float
  static_let <|
    var dist = DEFAULT_DIST
  if store != 0.0
    dist = clamp(store, MIN_DIST, MAX_DIST)
  return dist


[es(tag=dev, no_order)]
def daeditor_camera_nav_es(info : UpdateStageInfoAct)
  if imgui_get_state() != ImGuiState.ACTIVE
    return

  let io & = imgui::GetIO()

  // NOT io.WantCaptureMouse: the fullscreen dockspace host window hovers over all empty
  // viewport space, so imgui always "wants" the mouse there; hovered-window identity is
  // the real empty-space test
  var mode = MoveMode.NONE
  var wheel = 0.0

  if io.KeyAlt && imguizmo_is_viewport_hovered() && !imguizmo_is_over() && !imguizmo_is_using()
    if imgui::IsMouseDown(ImGuiMouseButton.Left)
      mode = MoveMode.ROTATE
    elif imgui::IsMouseDown(ImGuiMouseButton.Middle)
      mode = MoveMode.PAN
    elif imgui::IsMouseDown(ImGuiMouseButton.Right)
      mode = MoveMode.ZOOM
    wheel = io.MouseWheel

  if mode == MoveMode.NONE && wheel == 0.0
    return

  var tm = get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM

  var dist = nav_dist()
  var pivot = tm[3] + tm[2] * dist
  let delta = io.MouseDelta
  if mode == MoveMode.ROTATE
    var yaw, zAng, pitch : float
    matrix_to_euler(tm, yaw, zAng, pitch)
    yaw += delta.x * ROTATE_SENS
    pitch = clamp(pitch + delta.y * ROTATE_SENS, -deg_to_rad(89.0), deg_to_rad(89.0))
    var q : quat
    euler_to_quat(yaw, 0.0, pitch, q)
    make_tm(q, float3(0.0), tm)
  elif mode == MoveMode.PAN
    pivot += (-tm[0] * delta.x + tm[1] * delta.y) * (PAN_SENS * max(dist, 0.25))
  else
    dist *= exp((delta.y - delta.x) * ZOOM_SENS)
  dist = nav_dist(dist * exp(-wheel * WHEEL_SENS))
  tm[3] = pivot - tm[2] * dist
  get_cur_cam_entity() |> set("transform", tm)


def snap_view(var tm : float3x4&; dist : float; dir : float3)
  let pivot = tm[3] + tm[2] * dist
  let q = quat_from_unit_arc(tm[2], dir)
  let oldQ = math::quat(tm)
  tm = float3x4(compose(pivot - dir * dist, quat_mul(q, oldQ), float3(1.0)))


def center_on_selection(var tm : float3x4&; dist : float)
  var pos = float3(0.0)
  var posCount = 0
  query() <| $ [es(REQUIRE=daeditor__selected)] (transform : float3x4 const?; animchar : AnimcharBaseComponent const?)
    if animchar != null
      var box : bbox3f
      assume tree = *animchar.nodeTree
      geomtree_calcWorldBox(tree, box)
      pos += (box.bmin.xyz + box.bmax.xyz) * 0.5 + geomtree_getWtmOfs(tree)
      posCount += 1
    elif transform != null
      pos += (*transform)[3]
      posCount += 1

  if posCount > 0
    tm[3] = pos / float(posCount) - tm[2] * dist


def DasHelpMarker(desc : string)
  imgui::TextDisabled("(?)")
  if imgui::IsItemHovered()
    imgui::BeginTooltip()
    imgui::PushTextWrapPos(imgui::GetFontSize() * 35.0)
    imgui::TextUnformatted(desc)
    imgui::PopTextWrapPos()
    imgui::EndTooltip()


[imgui_window(name="Camera", group="Editor")]
def daeditor_camera_window()
  var tm = get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM
  var dist = nav_dist()
  var changed = imguizmo_view_manipulate(tm, dist, 128.0)

  // view cube faces as buttons: label plus the direction the camera ends up looking along
  let axisNames <- fixed_array("+X##ed_cam", "-X##ed_cam", "+Z##ed_cam", "-Z##ed_cam", "Top##ed_cam", "Bottom##ed_cam")
  let axisDirs <- fixed_array(float3(1.0, 0.0, 0.0), float3(-1.0, 0.0, 0.0), float3(0.0, 0.0, 1.0), float3(0.0, 0.0, -1.0),
    float3(0.0, -1.0, 0.0), float3(0.0, 1.0, 0.0))
  for i in range(length(axisNames))
    if i > 0
      imgui::SameLine()
    if imgui::Button(axisNames[i])
      snap_view(tm, dist, axisDirs[i])
      changed = true

  if imgui::Button("Center on selection##ed_cam")
    center_on_selection(tm, dist)
    changed = true

  imgui::SetNextItemWidth(120.0)
  let dMin = MIN_DIST
  let dMax = MAX_DIST
  imgui::DragScalar("pivot distance##ed_cam", ImGuiDataType.Float, safe_addr(dist), 0.05, safe_addr(dMin), safe_addr(dMax), "%.2f", ImGuiSliderFlags.None)
  imgui::SameLine()
  DasHelpMarker("Freecam navigation while the overlay is active, over empty viewport space:\nAlt+LMB rotate\nAlt+MMB pan\nAlt+RMB / wheel zoom\nDrag the cube to orbit, click a face to align with an axis.")

  nav_dist(dist)
  if changed
    get_cur_cam_entity() |> set("transform", tm)
