options persistent_heap

require ecs
require danetlibs.renderer.includes.render_events
require DagorMath
require DagorShaders
require DagorSystem
require daFrameGraph.resource_slot_ecs

[es(tag=render, on_appear, track=screen_night_vision__preset)]
def on_appear_screen_night_vision(evt : Event;
                                  screen_night_vision__preset : string;
                                  var shader_vars__vars : Object&;
                                  screen_night_vision__preset_list : Object;
                                  screen_night_vision__default_preset : Object)

  shader_vars__vars := screen_night_vision__default_preset

  let preset = get_ecs_object(screen_night_vision__preset_list[screen_night_vision__preset])
  if preset != null
    for prop in *preset
      shader_vars__vars |> set(prop.key, prop.value)
  else
    logerr("unexpected screen_night_vision__preset={screen_night_vision__preset}")


def set_night_vision_preset_value(eid : EntityId;
                                  value : bool)
  query(eid) <| $ [es] (var night_vision__isActive : bool&;
                        var night_vision__prevIsActive : bool&)
    night_vision__prevIsActive = night_vision__isActive
    night_vision__isActive = value
    if night_vision__prevIsActive != night_vision__isActive
      find_query() <| $ [es] (var screen_night_vision__enabled : bool&)
        screen_night_vision__enabled = night_vision__isActive
        return true

[es(tag=render, REQUIRE=night_vision__isPreset, after=cockpit_change_active_camera_effects, track=vehicle__night_vision)]
def set_night_vision_preset_vehicle_parameters_es(evt : Event;
                                                  eid : EntityId;
                                                  vehicle__night_vision : bool)
  let value = vehicle__night_vision
  set_night_vision_preset_value(eid, value)

[es(tag=render, REQUIRE=night_vision__isPreset, on_disappear)]
def night_vision_preset_remove_es(evt : Event)
  find_query() <| $ [es] (var screen_night_vision__enabled : bool&)
    screen_night_vision__enabled = false
    return true


def set_fixed_exposure(enabled : bool; exposure : float)
  query() <| $ [es] (var adaptation_override_settings : Object&)
    if enabled
      adaptation_override_settings |> set("adaptation__on", false)
      adaptation_override_settings |> set("adaptation__fixedExposure", exposure)
    else
      adaptation_override_settings |> erase("adaptation__on")
      adaptation_override_settings |> erase("adaptation__fixedExposure")


[resource_slot_ecs(on_appear, track=screen_night_vision__enabled)]
def screen_night_vision_attach(var screenNightVision : NodeHandleWithSlotsAccess&;
                               screen_night_vision__enabled : bool;
                               screen_night_vision__fixed_exposure : float)

  set_fixed_exposure(screen_night_vision__enabled, screen_night_vision__fixed_exposure)

  if !screen_night_vision__enabled
    screenNightVision |> reset
    return

  screenNightVision <- root() |> registerAccess("screen_night_vision", SlotActions(update <- [Update(slot="postfx_input_slot", resource="screen_night_vision_tex", priority=100)])) <| @(slots_state; var registry : Registry)

    registry |> create(slots_state |> resourceToCreateFor("postfx_input_slot"), History.No) |> texture(
      Texture2dCreateInfo(
        creationFlags = TEXFMT_R11G11B10F | TEXCF_RTARGET,
        resolution = TextureResolution2(autoRes = registry |> getResolution`2("post_fx"))
      )
    )

    registry |> requestRenderPass |> color(fixed_array(slots_state |> resourceToCreateFor("postfx_input_slot")))

    registry |> read(slots_state |> resourceToReadFrom("postfx_input_slot")) |> texture |> atStage(Stage.POST_RASTER) |> bindToShaderVar("frame_tex")

    registry |> read("depth_after_transparency") |> texture |> atStage(Stage.POST_RASTER) |> bindToShaderVar("depth_gbuf")

    return <- @ <|
      query() <| $ [es] (screen_night_vision__shader : PostFxRenderer)
        screen_night_vision__shader |> render
