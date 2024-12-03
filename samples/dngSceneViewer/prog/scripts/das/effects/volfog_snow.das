require ecs
require ecs.common
require player
require DagorShaders
require daslib.strings_boost

let
  volfog_snow_intensityVarId = get_shader_variable_id("volfog_snow_intensity", true)

[es(tag=gameClient, REQUIRE=shooter_cam, REQUIRE_NOT=camera_volfog_snow__maxIntensity, on_appear)]
def volfog_snow_camera_created_es(evt : Event; eid : EntityId)
  query() <| $ [es] (volfog_snow__intensityByCamera : Object)
    let camTemplate = split_template(getEntityTemplateName(eid))[0]
    let maxIntensity = volfog_snow__intensityByCamera |> get_float(camTemplate) ?? -1.f
    if maxIntensity >= 0.f
      addSubTemplate(eid, "camera_volfog_snow_affect") <| $(init)
        set(init, "camera_volfog_snow__maxIntensity", maxIntensity)

[es(no_order, tag=render)]
def volfog_snow_update(info : UpdateStageInfoAct; volfog_snow__intensitySpeed : float; var volfog_snow__intensity : float&; volfog_snow__maxIntensity = 1f)
  var maxIntensity = volfog_snow__maxIntensity
  query(get_local_player_eid()) <| $ [es] (activeCamEid : EntityId)
    query(activeCamEid) <| $ [es] (camera_volfog_snow__maxIntensity : float)
      maxIntensity = min(maxIntensity, camera_volfog_snow__maxIntensity)

  volfog_snow__intensity = volfog_snow__intensity + volfog_snow__intensitySpeed * info.dt
  volfog_snow__intensity = clamp(volfog_snow__intensity, 0f, maxIntensity)
  DagorShaders::set_real(volfog_snow_intensityVarId, volfog_snow__intensity)
