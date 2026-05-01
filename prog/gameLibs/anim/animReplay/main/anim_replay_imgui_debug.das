options no_aot
options persistent_heap

require ecs
require strings
require DagorConsole
require DagorDataBlock
require DagorEditor
require DagorImgui
require DagorSystem
require imgui
require imgui/imgui_boost
require daslib/enum_trait
require daslib/safe_addr
require daslib/static_let
require %gameLibs/anim/animReplay/imports/anim_replay_common
require %gameLibs/anim/animReplay/imports/anim_replay_events


let ORIGINAL_PIXELS_PER_SECOND = 200.f // with 1.0 zoom

def get_time_step_on_timeline(time_region : float)
  // pow() with floats is very inaccurate in das
  let timeStep = float(pow(10lf, double(floor(log(time_region) / log(10f)))))
  let r = time_region / timeStep
  if r < 2.f
    return timeStep * 0.2f
  elif r < 5.f
    return timeStep * 0.5f
  else
    return timeStep

def draw_timeline_labels(var draw_list : imgui::ImDrawList?;
                         top_pixel : float;
                         time_to_pixel : float2;
                         time_from : float;
                         time_to : float;
                         replay_time : float)
  let timeRegion = time_to - time_from
  let timeStep = get_time_step_on_timeline(timeRegion)
  let from = time_from - time_from % timeStep
  var numLabels = ceil((time_to - from) / timeStep)
  if from + numLabels * timeStep <= replay_time
    numLabels += 1f // will see the part of last label even if line is not visible

  let linesColor = imgui::GetColorU32(imgui::GetStyle().Colors[int(ImGuiCol.PlotLines)])
  let lineY = top_pixel
  let smallLineSize = 10.f
  let mediumLineSize = 15.f
  let largeLineSize = 20.f
  let labelSpacingY = imgui::GetStyle().ItemSpacing.y
  let subStep = 0.1f * timeStep
  for i in range(numLabels)
    let t = from + float(i) * timeStep
    let lineX = t * time_to_pixel.x + time_to_pixel.y
    let lineBottom = float2(lineX, lineY + largeLineSize)
    *draw_list |> AddLine(float2(lineX, lineY), lineBottom, linesColor)
    let timeLabel = "{t:g} s"
    let timeLabelSize = imgui::CalcTextSize(timeLabel)
    let labelPos = lineBottom + float2(t > 0f ? -timeLabelSize.x * 0.5f : 0f, labelSpacingY)
    imgui::SetCursorScreenPos(labelPos)
    imgui::Text(timeLabel)
    for j in range(1, 10)
      let subLineX = (t + float(j) * subStep) * time_to_pixel.x + time_to_pixel.y
      let lineSize = j == 5 ? mediumLineSize : smallLineSize
      *draw_list |> AddLine(float2(subLineX, lineY), float2(subLineX, lineY + lineSize), linesColor)

def replay_character_panel(anim_replay__animchars : Array)
  imgui::BeginChild("Replay Character Panel", float2(100f, 0f), ImGuiChildFlags.ResizeX | ImGuiChildFlags.Borders)

  static_let <|
    var selectedReplayAnimchar = 0
    var selectedAnimcharEid = INVALID_ENTITY_ID

  init_entity_object_editor()
  query(selectedAnimcharEid) <| $ [es(REQUIRE_NOT=daeditor__selected)] ()
    entity_object_editor_updateObjectsList()
    entity_object_editor_selectEntity(selectedAnimcharEid, true)

  imgui::Text("Selected character")
  var charactersList : array<string>
  for animcharChildComp, idx in anim_replay__animchars, count()
    let animcharData = get_ecs_object(animcharChildComp)
    let originalTemplate = string(*get_ecs_string(*animcharData, "originalTemplate"))
    let initialPos = (*get_ecs_Point3List(*animcharData, "position"))[0]
    push(charactersList, "{idx + 1}. {originalTemplate} at pos ({initialPos})")
  push(charactersList, "None")
  imgui::SetNextItemWidth(imgui::GetContentRegionAvail().x)
  Combo("##Selected character", safe_addr(selectedReplayAnimchar), charactersList)
  if selectedReplayAnimchar < length(anim_replay__animchars)
    let animcharData = get_ecs_object(anim_replay__animchars[selectedReplayAnimchar])
    let eid = *get_Eid(animcharData, "eid")
    if selectedAnimcharEid != eid
      entity_object_editor_selectEntity(selectedAnimcharEid, false)
      entity_object_editor_selectEntity(eid, true)
      selectedAnimcharEid = eid
  else
    entity_object_editor_selectEntity(selectedAnimcharEid, false)
    selectedAnimcharEid = INVALID_ENTITY_ID
  query() <| $ [es(REQUIRE=anim_replay_camera)] (var camera__target : EntityId&)
    camera__target = selectedAnimcharEid
  imgui::EndChild()
  return selectedReplayAnimchar

def showAnimcharInfo(animchar_data : Object; cur_time : float; timestamps : FloatList)
  if is_animchar_dead(animchar_data, cur_time)
    return
  unsafe
    let position & = *get_ecs_Point3List(animchar_data, "position")
    sample_timestamps(cur_time, timestamps) <| $(prevIdx; nextIdx)
      let speed = length(position[nextIdx] - position[prevIdx]) / (timestamps[nextIdx] - timestamps[prevIdx])
      imgui::Text("Character speed: {speed} m/s")

def debug_button_with_popup_menu(align_right : bool)
  static_let <|
    var buttonSize = 0f
  if align_right
    imgui::Dummy(float2(imgui::GetContentRegionAvail().x - buttonSize, 1))
    imgui::SameLine(0f, 0f)
  if imgui::Button("Debug...")
    imgui::OpenPopup("DebugMenu")
  buttonSize = imgui::GetItemRectSize().x
  if (imgui::BeginPopup("DebugMenu"))
    if imgui::MenuItem("Select anim_replay entity")
      query() <| $ [es(REQUIRE=anim_replay__animchars)] (eid : EntityId)
        entity_object_editor_selectEntity(eid, true)
    query() <| $ [es] (var anim_replay__disableMMTransitionBlend : bool&)
      imgui::MenuItem("Disable blend on MM transitions", "", unsafe(addr(anim_replay__disableMMTransitionBlend)))
    imgui::EndPopup()

def set_time_speed(speed : float)
  var commandFound = false
  dgs_get_settings() |> datablock_get_block("animReplay") <| $(blk)
    let timeSpeedCommand = blk |> datablock_getStr("timeSpeedCommand", "")
    if !empty(timeSpeedCommand)
      commandFound = true
      console_command("{timeSpeedCommand} {speed}")
  if !commandFound
    logerr("missing 'animReplay/timeSpeedCommand:t=' parameter in settings")

[es(on_appear, REQUIRE=anim_replay__animchars)]
def anim_replay_pause_game(evt : Event)
  set_time_speed(0f)

[es(on_disappear, REQUIRE=anim_replay__animchars)]
def anim_replay_resume_game(evt : Event)
  set_time_speed(1f)

def imgui_disableable_button(button_text : string;  button_condition : bool)
  if !button_condition
    imgui::PushStyleColor(imgui::ImGuiCol.Text, ImGuiStyle().Colors[int(ImGuiCol.TextDisabled)])
    imgui::PushStyleColor(imgui::ImGuiCol.ButtonHovered, ImGuiStyle().Colors[int(ImGuiCol.Button)])
  let isButtonPressed = imgui::Button(button_text)
  if !button_condition
    imgui::PopStyleColor()
    imgui::PopStyleColor()
  return button_condition && isButtonPressed

def to_enum(val : int; defVal : auto(E)) : E
  for en in each_enum(defVal)
    if val == int(en)
      return en
  return defVal

[imgui_window(name="Anim Replay", group="Anim")]
def anim_replay_imgui_es()
  if IsWindowCollapsed()
    return

  static_let <|
    var isReplayRecording = false
    var recordingEntityNotFound = false
    var deselectedAnimcharEntities : array<EntityId>
    var animcharsRecordingFilter : int
    var recordingSampleRate : int = 30
    var resizeWindowWhenReplayLoaded = false
  let isReplayLoaded = find_query <| $ [es(REQUIRE=anim_replay__animchars)] ()
    return true
  let isReplayLoading = find_query <| $ [es(REQUIRE=anim_replay__loadingAnimchars)] ()
    return true
  let newReplayAllowed = !isReplayLoaded && !isReplayLoading && !isReplayRecording

  if imgui_disableable_button("Start record", newReplayAllowed)
    isReplayRecording = true
    recordingEntityNotFound = false
    broadcastEvent(AnimReplayStartRecord(animcharsFilter = to_enum(animcharsRecordingFilter, AnimcharsRecordingFilter WATCHED_BY_PLAYER),
      sampleRate = recordingSampleRate))

  if typeinfo builtin_module_exists(stddlg)
    imgui::SameLine()
    if imgui_disableable_button("Load...", newReplayAllowed)
      broadcastEvent(AnimReplayOpenLoadFileDialog())
      resizeWindowWhenReplayLoaded = true

  query() <| $ [es] (eid : EntityId;
                     anim_replay__time : float;
                     anim_replay__savedAnimchars : Array)
    if empty(anim_replay__savedAnimchars)
      isReplayRecording = false
      recordingEntityNotFound = true
      destroyEntity(eid)
      return
    if empty(deselectedAnimcharEntities) && animcharsRecordingFilter == int(AnimcharsRecordingFilter DAEDITOR_SELECTED)
      query() <| $ [es(REQUIRE=(daeditor__selected, animchar))] (eid : EntityId)
        push(deselectedAnimcharEntities, eid)
      for animcharEid in deselectedAnimcharEntities
        entity_object_editor_selectEntity(animcharEid, false)
    imgui::SameLine()
    imgui::Text("Recorded time: {anim_replay__time:.3f} s")

  if newReplayAllowed
    imgui::SameLine(0.f, 120.f)
    var animcharFilterList : array<string>
    resize(animcharFilterList, typeinfo enum_length(type<AnimcharsRecordingFilter>))
    animcharFilterList[int(AnimcharsRecordingFilter WATCHED_BY_PLAYER)] = "Watched"
    animcharFilterList[int(AnimcharsRecordingFilter DAEDITOR_SELECTED)] = "Selected"
    animcharFilterList[int(AnimcharsRecordingFilter ALL_MM_CHARACTERS)] = "All MM characters"
    imgui::SetNextItemWidth(120.f)
    if Combo("Target entity", safe_addr(animcharsRecordingFilter), animcharFilterList)
      recordingEntityNotFound = false
    if recordingEntityNotFound
      imgui::SameLine()
      imgui::TextColored(float4(1, 0, 0, 1), "(not found)")
    imgui::SameLine(0.f, 20.f)
    imgui::SetNextItemWidth(30.f)
    if imgui::InputInt("Sample rate", safe_addr(recordingSampleRate), 0, 0)
      recordingSampleRate = max(recordingSampleRate, 1)

  if isReplayLoaded
    imgui::SameLine()
    debug_button_with_popup_menu(true)

  if imgui_disableable_button("Stop record", isReplayRecording)
    isReplayRecording = false
    broadcastEvent(AnimReplayStopRecord())
    resizeWindowWhenReplayLoaded = true

  imgui::SameLine()
  if imgui_disableable_button("Close record", isReplayLoaded)
    query() <| $ [es] (eid : EntityId;
                       anim_replay__animchars : Array)
      for animcharChildComp in anim_replay__animchars
        assume animcharData = *get_ecs_object(animcharChildComp)
        let animcharEid = *get_Eid(animcharData, "eid")
        destroyEntity(animcharEid)
      destroyEntity(eid)
    if !empty(deselectedAnimcharEntities)
      for animcharEid in deselectedAnimcharEntities
        if doesEntityExist(animcharEid)
          entity_object_editor_selectEntity(animcharEid, true)
      clear(deselectedAnimcharEntities)

  if typeinfo builtin_module_exists(stddlg)
    imgui::SameLine()
    if imgui_disableable_button("Save...", isReplayLoaded)
      broadcastEvent(AnimReplayOpenSaveFileDialog())

  static_let <|
    var timelineZoom = 1.f
    var prevMouseX = -1.f
    var playSpeed = 1.f
    var isPlayEnabled = false

  query() <| $ [es] (var anim_replay__currentTime : float&;
                     var anim_replay__prevTime : float&;
                     var anim_replay__runMMSearch : bool&;
                     anim_replay__animchars : Array;
                     anim_replay__timestamps : FloatList)
    if empty(anim_replay__timestamps)
      return
    let replayTime = back(anim_replay__timestamps)
    if replayTime <= 0f
      return
    anim_replay__prevTime = anim_replay__currentTime

    imgui::SameLine(0.f, 80.f)
    if imgui::Button(isPlayEnabled ? "Pause" : "Play") || imgui::IsKeyPressed(ImGuiKey Space, false)
      isPlayEnabled = !isPlayEnabled
      if isPlayEnabled && anim_replay__currentTime == replayTime
        anim_replay__currentTime = 0f
    imgui::SameLine()
    imgui::SetNextItemWidth(40.f)
    imgui::DragFloat("Speed", unsafe(addr(playSpeed)), 0.01f, 0.f, 4.f, "%g", ImGuiSliderFlags.None)
    if isPlayEnabled
      anim_replay__currentTime += imgui::GetIO().DeltaTime * playSpeed;
      if anim_replay__currentTime >= replayTime
        anim_replay__currentTime = replayTime
        isPlayEnabled = false
    imgui::SameLine()
    imgui::Checkbox("Run MM search", unsafe(addr(anim_replay__runMMSearch)))

    let selectedAnimcharIdx = replay_character_panel(anim_replay__animchars)
    imgui::SameLine()
    imgui::BeginChild("Timeline Area", float2(0, 0), ImGuiChildFlags.AutoResizeY)
    let timelineFrameSize = 1.0f
    let timelineAvailableSizeX = imgui::GetContentRegionAvail().x - timelineFrameSize * 2f
    let originalTimelineSize = replayTime * ORIGINAL_PIXELS_PER_SECOND
    timelineZoom = max(timelineZoom, timelineAvailableSizeX / originalTimelineSize)
    let timelineScreenPos = imgui::GetCursorScreenPos()

    let timelineSizeX = originalTimelineSize * timelineZoom
    let timelineSizeY = 50f
    let timelinePadding = 1f
    imgui::PushStyleVar(ImGuiStyleVar.WindowPadding, float2(timelinePadding))
    imgui::SetNextWindowContentSize(float2(timelineSizeX, timelineSizeY))
    imgui::BeginChild("Record Timeline", float2(0f, timelineSizeY + 2f * timelinePadding + imgui::GetStyle().ScrollbarSize),
      ImGuiChildFlags.Borders, ImGuiWindowFlags.HorizontalScrollbar | ImGuiWindowFlags.AlwaysHorizontalScrollbar)
    imgui::Dummy(float2(timelineSizeX, timelineSizeY))
    let isTimelineHovered = imgui::IsItemHovered()
    let leftTopCorner = imgui::GetItemRectMin()

    let timeToPixel = float2((timelineSizeX - 1f) / replayTime, leftTopCorner.x)
    var drawList = imgui::GetWindowDrawList()
    let firstVisiblePixel = timelineScreenPos.x + timelinePadding - leftTopCorner.x
    let visibleTimeFrom = firstVisiblePixel / timeToPixel.x
    let visibleTimeTo = visibleTimeFrom + (timelineAvailableSizeX - 1f) / timeToPixel.x
    draw_timeline_labels(drawList, leftTopCorner.y, timeToPixel, visibleTimeFrom, visibleTimeTo, replayTime)

    let playheadX = anim_replay__currentTime * timeToPixel.x + timeToPixel.y
    let playheadColor = imgui::GetColorU32(imgui::GetStyle().Colors[int(ImGuiCol.ButtonHovered)])
    *drawList |> AddLine(float2(playheadX, leftTopCorner.y), float2(playheadX, leftTopCorner.y + timelineSizeY), playheadColor, 1.5f)
    if isPlayEnabled && (playheadX <= timelineScreenPos.x || timelineScreenPos.x + timelineAvailableSizeX < playheadX)
      imgui::SetScrollX(anim_replay__currentTime * timeToPixel.x)

    let mouse = imgui::GetMousePos()
    if isTimelineHovered
      let mouseReplayProgress = min((mouse.x - leftTopCorner.x) / (timelineSizeX - 1.f), 1.f)
      let mouseTime = mouseReplayProgress * replayTime
      imgui::BeginTooltip()
      imgui::Text("{mouseTime:.3f} s")
      imgui::EndTooltip()
      if imgui::IsMouseDown(ImGuiMouseButton.Left)
        anim_replay__currentTime = mouseTime
        isPlayEnabled = false
      if imgui::IsMouseDown(ImGuiMouseButton.Right)
        let prevScroll = imgui::GetScrollX()
        imgui::SetScrollX(prevScroll - (mouse.x - prevMouseX))
      let mouseWheel = imgui::GetIO().MouseWheel
      if mouseWheel != 0.f
        let zoomScaleStep = 1.1f
        if mouseWheel > 0.f
          timelineZoom *= zoomScaleStep
        else
          timelineZoom /= zoomScaleStep
        let newTimelineSize = originalTimelineSize * timelineZoom
        imgui::SetScrollX(mouseReplayProgress * newTimelineSize - (mouse.x - timelineScreenPos.x))

    imgui::EndChild()
    imgui::PopStyleVar()

    prevMouseX = mouse.x
    if selectedAnimcharIdx < length(anim_replay__animchars)
      let animcharData = get_ecs_object(anim_replay__animchars[selectedAnimcharIdx])
      showAnimcharInfo(*animcharData, anim_replay__currentTime, anim_replay__timestamps)

    imgui::EndChild()

    if resizeWindowWhenReplayLoaded
      let oldWindowSize = imgui::GetWindowSize()
      imgui::SetWindowSize(float2(oldWindowSize.x, 0f))
      resizeWindowWhenReplayLoaded = false
