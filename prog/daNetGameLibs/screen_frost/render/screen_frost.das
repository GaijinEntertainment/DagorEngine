options persistent_heap

require ecs
require danetlibs.screen_frost.render.screen_frost_events
require DagorMath
require DagorShaders
require daBfg.resource_slot_ecs

let
  screenCorruptionDiffuseVarId        = get_shader_variable_id("screen_corruption_diffuse", true)
  screenFrostDiffuseVarId             = get_shader_variable_id("screen_frost_diffuse", true)
  screenFrostRoughnessVarId           = get_shader_variable_id("screen_frost_roughness", true)
  screenFrostOpacityVarId             = get_shader_variable_id("screen_frost_opacity", true)
  screenFrostIntensityVarId           = get_shader_variable_id("screen_frost_intensity", true)
  screenFrostDisplayResolutionVarId   = get_shader_variable_id("screen_frost_display_resolution", true)

[es(tag=render, on_appear)]
def on_appear_screen_frost_renderer(evt : Event;
                                    screen_frost__diffuse, screen_frost__corruption_diffuse : float4;
                                    screen_frost__roughness, screen_frost__opacity : float)
  DagorShaders::set_color4(screenFrostDiffuseVarId, screen_frost__diffuse)
  DagorShaders::set_color4(screenCorruptionDiffuseVarId, screen_frost__corruption_diffuse)
  DagorShaders::set_real(screenFrostRoughnessVarId, screen_frost__roughness)
  DagorShaders::set_real(screenFrostOpacityVarId, screen_frost__opacity)

[es(tag=render, on_appear)]
def on_attach_screen_frost(evt : Event;
                           screen_frost__intensity : float)
  DagorShaders::set_real(screenFrostIntensityVarId, screen_frost__intensity)

[es(tag=render, on_disappear, REQUIRE=screen_frost__intensity)]
def on_detach_screen_frost(evt : Event)
  DagorShaders::set_real(screenFrostIntensityVarId, 0.)

[es(tag=render)]
def screen_frost_set_resolution_es(evt : SetResolutionEvent)
  DagorShaders::set_color4(screenFrostDisplayResolutionVarId, float(evt.postFxResolution.x), float(evt.postFxResolution.y),
    1.f / float(evt.postFxResolution.x), 1.f / float(evt.postFxResolution.y))

[es(tag=render, no_order)]
def screen_frost_update_es(info : UpdateStageInfoAct; screen_frost__maxIntensity, screen_frost__rate : float;
                           var screen_frost__intensity : float&)
  screen_frost__intensity = clamp(screen_frost__intensity + screen_frost__rate * info.dt, 0., screen_frost__maxIntensity)
  DagorShaders::set_real(screenFrostIntensityVarId, screen_frost__intensity)


[resource_slot_ecs]
def register_screen_frost(var screenFrost : NodeHandleWithSlotsAccess&)
  screenFrost <- root() |> registerAccess("screen_frost", [[SlotActions update <- [{Update slot="postfx_input_slot", resource="frame_with_screen_frost", priority=300}] ]]) <| @(slots_state; var registry : Registry)

    registry |> read(slots_state |> resourceToReadFrom("postfx_input_slot")) |> texture |> atStage(Stage POST_RASTER) |> bindToShaderVar("frame_tex")

    registry |> create(slots_state |> resourceToCreateFor("postfx_input_slot"), History No) |> texture(
      [[Texture2dCreateInfo()
        creationFlags = TEXFMT_R11G11B10F | TEXCF_RTARGET,
        resolution = [[TextureResolution2 autoRes=registry |> getResolution`2("main_view")]],
      ]]
    )
    registry |> requestRenderPass |> color([[auto[] slots_state |> resourceToCreateFor("postfx_input_slot")]])

    return <- @ <|
      query() <| $ [es] (screen_frost__shader : PostFxRenderer)
        screen_frost__shader |> render


[es(tag=render, on_appear, REQUIRE=screenFrostEffect)]
def screen_frost_attach(evt : Event&)
  query() <| $ [es] (var screenFrost : NodeHandleWithSlotsAccess&)
    screenFrost |> register_screen_frost


[es(tag=render, on_disappear, REQUIRE=screenFrostEffect)]
def screen_frost_detach(evt : Event&)
  query() <| $ [es] (var screenFrost : NodeHandleWithSlotsAccess&)
    screenFrost |> reset