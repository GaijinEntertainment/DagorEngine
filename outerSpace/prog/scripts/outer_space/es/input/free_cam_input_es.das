require ecs
require DagorInput
require outer_space.es.input.input_events
require danetlibs.renderer.includes.render_events

def private init_actions(var free_cam_input__move : uint16&;
                         var free_cam_input__rotate : uint16&;
                         var free_cam_input__shiftY : uint16&;
                         var free_cam_input__zoomIn : uint16&;
                         var free_cam_input__zoomOut : uint16&;
                         var free_cam_input__turbo : uint16&;
                         var free_cam_input__speedUp : uint16&;
                         var free_cam_input__speedDown : uint16&;
                         var free_cam_input__speedReset : uint16&;
                         var free_cam_input__bank : uint16&)
  free_cam_input__move = get_action_handle("Camera.Move", TYPEGRP_STICK)
  free_cam_input__rotate = get_action_handle("Camera.Rotate", TYPEGRP_STICK)
  free_cam_input__shiftY = get_action_handle("Camera.ShiftY", TYPEGRP_AXIS)
  free_cam_input__turbo = get_action_handle("Camera.Turbo", TYPEGRP_AXIS)
  free_cam_input__zoomIn = get_action_handle("Camera.ZoomIn", TYPEGRP_DIGITAL)
  free_cam_input__zoomOut = get_action_handle("Camera.ZoomOut", TYPEGRP_DIGITAL)
  free_cam_input__speedUp = get_action_handle("Camera.SpeedUp", TYPEGRP_DIGITAL)
  free_cam_input__speedDown = get_action_handle("Camera.SpeedDown", TYPEGRP_DIGITAL)
  free_cam_input__speedReset = get_action_handle("Camera.SpeedReset", TYPEGRP_DIGITAL)
  free_cam_input__bank = get_action_handle("Camera.Bank", TYPEGRP_AXIS)

def private reset_actions(var free_cam_input__move : uint16&;
                          var free_cam_input__rotate : uint16&;
                          var free_cam_input__shiftY : uint16&;
                          var free_cam_input__zoomIn : uint16&;
                          var free_cam_input__zoomOut : uint16&;
                          var free_cam_input__turbo : uint16&;
                          var free_cam_input__speedUp : uint16&;
                          var free_cam_input__speedDown : uint16&;
                          var free_cam_input__speedReset : uint16&;
                          var free_cam_input__bank : uint16&)
  free_cam_input__move = BAD_ACTION_HANDLE
  free_cam_input__rotate = BAD_ACTION_HANDLE
  free_cam_input__shiftY = BAD_ACTION_HANDLE
  free_cam_input__zoomIn = BAD_ACTION_HANDLE
  free_cam_input__zoomOut = BAD_ACTION_HANDLE
  free_cam_input__turbo = BAD_ACTION_HANDLE
  free_cam_input__speedUp = BAD_ACTION_HANDLE
  free_cam_input__speedDown = BAD_ACTION_HANDLE
  free_cam_input__speedReset = BAD_ACTION_HANDLE
  free_cam_input__bank = BAD_ACTION_HANDLE

[es(tag=input)]
def free_cam_input_init_es(evt : EventDaInputInit;
                           var free_cam_input__move : uint16&;
                           var free_cam_input__rotate : uint16&;
                           var free_cam_input__shiftY : uint16&;
                           var free_cam_input__zoomIn : uint16&;
                           var free_cam_input__zoomOut : uint16&;
                           var free_cam_input__turbo : uint16&;
                           var free_cam_input__speedUp : uint16&;
                           var free_cam_input__speedDown : uint16&;
                           var free_cam_input__speedReset : uint16&;
                           var free_cam_input__bank : uint16&)
  if evt.init
    init_actions(free_cam_input__move, free_cam_input__rotate, free_cam_input__shiftY, free_cam_input__zoomIn,
      free_cam_input__zoomOut, free_cam_input__turbo, free_cam_input__speedUp, free_cam_input__speedDown,
      free_cam_input__speedReset, free_cam_input__bank)
  else
    reset_actions(free_cam_input__move, free_cam_input__rotate, free_cam_input__shiftY, free_cam_input__zoomIn,
      free_cam_input__zoomOut, free_cam_input__turbo, free_cam_input__speedUp, free_cam_input__speedDown,
      free_cam_input__speedReset, free_cam_input__bank)

[es(tag=input, on_appear)]
def free_cam_input_appear_es(evt : Event;
                             var free_cam_input__move : uint16&;
                             var free_cam_input__rotate : uint16&;
                             var free_cam_input__shiftY : uint16&;
                             var free_cam_input__zoomIn : uint16&;
                             var free_cam_input__zoomOut : uint16&;
                             var free_cam_input__turbo : uint16&;
                             var free_cam_input__speedUp : uint16&;
                             var free_cam_input__speedDown : uint16&;
                             var free_cam_input__speedReset : uint16&;
                             var free_cam_input__bank : uint16&)
  init_actions(free_cam_input__move, free_cam_input__rotate, free_cam_input__shiftY, free_cam_input__zoomIn,
    free_cam_input__zoomOut, free_cam_input__turbo, free_cam_input__speedUp, free_cam_input__speedDown,
    free_cam_input__speedReset, free_cam_input__bank)

[es(tag=input, on_disappear)]
def free_cam_input_disappear_es(evt : Event;
                                var free_cam_input__move : uint16&;
                                var free_cam_input__rotate : uint16&;
                                var free_cam_input__shiftY : uint16&;
                                var free_cam_input__zoomIn : uint16&;
                                var free_cam_input__zoomOut : uint16&;
                                var free_cam_input__turbo : uint16&;
                                var free_cam_input__speedUp : uint16&;
                                var free_cam_input__speedDown : uint16&;
                                var free_cam_input__speedReset : uint16&;
                                var free_cam_input__bank : uint16&)
  reset_actions(free_cam_input__move, free_cam_input__rotate, free_cam_input__shiftY, free_cam_input__zoomIn,
    free_cam_input__zoomOut, free_cam_input__turbo, free_cam_input__speedUp, free_cam_input__speedDown,
    free_cam_input__speedReset, free_cam_input__bank)

[es(tag=input, before=free_cam_es)]
def free_cam_input_es(info : UpdateStageInfoBeforeRender;
                      camera__input_enabled : bool;
                      camera__active : bool;
                      free_cam_input__move : uint16;
                      free_cam_input__turbo : uint16;
                      free_cam_input__rotate : uint16;
                      free_cam_input__bank : uint16;
                      free_cam_input__shiftY : uint16;
                      var free_cam__move : float2&;
                      var free_cam__turbo : bool&;
                      var free_cam__rotate : float2&;
                      var free_cam__shiftY : float&;
                      var free_cam__bank : float&)

  if !camera__active || !camera__input_enabled
    return

  let move = get_analog_stick_action_state(free_cam_input__move)
  free_cam__move = float2(move.x, move.y)

  let turbo = get_analog_axis_action_state(free_cam_input__turbo)
  free_cam__turbo = turbo.x > 0.

  let rotate = get_analog_stick_action_state(free_cam_input__rotate)
  free_cam__rotate = float2(rotate.x, rotate.y)

  let shiftY = get_analog_axis_action_state(free_cam_input__shiftY)
  free_cam__shiftY = shiftY.x

  let turn = get_analog_axis_action_state(free_cam_input__bank)
  free_cam__bank = turn.x


[es(tag=input, before=free_cam_es, REQUIRE=free_cam)]
def free_cam_input_action_es(evt : EventDaInputActionTriggered;
                             free_cam_input__zoomIn : uint16;
                             free_cam_input__zoomOut : uint16;
                             free_cam_input__speedUp : uint16;
                             free_cam_input__speedDown : uint16;
                             free_cam_input__speedReset : uint16;
                             free_cam__moveSpeedDefault : float;
                             var free_cam__angSpdScale : float2&;
                             var fov : float&;
                             var free_cam__move_speed : float&;
                             free_cam__zoomFactor = 1.5;
                             free_cam__moveSpeedChangeFactor = 1.03;
                             free_cam__max_fov = 90.)
  if evt.action == free_cam_input__zoomIn
    fov /= free_cam__zoomFactor
    free_cam__angSpdScale /= free_cam__zoomFactor
  elif evt.action == free_cam_input__zoomOut && fov < free_cam__max_fov
    free_cam__angSpdScale *= free_cam__zoomFactor
    fov = min(fov * free_cam__zoomFactor, free_cam__max_fov)
  elif evt.action == free_cam_input__speedUp
    free_cam__move_speed *= pow(free_cam__moveSpeedChangeFactor, 1.)
  elif evt.action == free_cam_input__speedDown
    free_cam__move_speed *= pow(free_cam__moveSpeedChangeFactor, -1.)
  elif evt.action == free_cam_input__speedReset
    free_cam__move_speed = free_cam__moveSpeedDefault

[es(tag=render, on_appear, before=free_cam_update_params_es)]
def free_cam_init_es(evt : Event;
                     free_cam__moveSpeedDefault : float;
                     var free_cam__move_speed : float&)
  free_cam__move_speed = free_cam__moveSpeedDefault
