require ecs
require ecs.safe
require ecs.common
require AnimV20
require DagorSystem
require DagorMaterials
require DagorTexture3D
require DagorMath
require player

def apply_chevron_data(chevronData : Object;
                       var animchar_render : AnimcharRendComponent&;
                       var chevron_background_d_tex : SharedTex&;
                       var chevron_background_ao_tex : SharedTex&;
                       var chevron_mask_tex : SharedTex&;
                       var chevron_frame_d_tex : SharedTex&;
                       var chevron_frame_n_tex : SharedTex&;
                       var chevron_frame_ao_tex : SharedTex&;
                       var chevron_figure_d_tex : SharedTex&;
                       var chevron_figure_n_tex : SharedTex&;
                       var chevron_figure_ao_tex : SharedTex&)
  let desc = get_ecs_object(chevronData, "chevronDesc")
  if desc == null
    return
  let backgroungTexName = get_string(desc, "backgroungTexName", "")
  let maskTexName = get_string(desc, "maskTexName", "")
  let frameTexName = get_string(desc, "frameTexName", "")
  let figureTexName = get_string(desc, "figureTexName", "")
  let figureOffset = get_Point2(desc, "figureOffset") ?? float2(0.0, 0.0)
  let frameColorTint = get_Point4(desc, "frameColorTint") ?? float4(1.0, 1.0, 1.0, 1.0)
  let figureColorTint = get_Point4(desc, "figureColorTint") ?? float4(1.0, 1.0, 1.0, 1.0)

  get_tex_gameres(chevron_background_d_tex, "{backgroungTexName}_d")
  get_tex_gameres(chevron_background_ao_tex, "{backgroungTexName}_ao")
  get_tex_gameres(chevron_mask_tex, "{maskTexName}")
  get_tex_gameres(chevron_frame_d_tex, "{frameTexName}_d")
  get_tex_gameres(chevron_frame_n_tex, "{frameTexName}_n")
  get_tex_gameres(chevron_frame_ao_tex, "{frameTexName}_ao")
  get_tex_gameres(chevron_figure_d_tex, "{figureTexName}_d")
  get_tex_gameres(chevron_figure_n_tex, "{figureTexName}_n")
  get_tex_gameres(chevron_figure_ao_tex, "{figureTexName}_ao")

  recreate_material(animchar_render, "dynamic_chevron") <| $(mat)
    mat |> set_texture(0, chevron_background_d_tex.getTexId)
    mat |> set_texture(1, chevron_background_ao_tex.getTexId)
    mat |> set_texture(2, chevron_mask_tex.getTexId)
    mat |> set_texture(3, chevron_frame_d_tex.getTexId)
    mat |> set_texture(4, chevron_frame_n_tex.getTexId)
    mat |> set_texture(5, chevron_frame_ao_tex.getTexId)
    mat |> set_texture(6, chevron_figure_d_tex.getTexId)
    mat |> set_texture(7, chevron_figure_n_tex.getTexId)
    mat |> set_texture(8, chevron_figure_ao_tex.getTexId)

  let figureOffsetVarId = get_shader_variable_id("figure_offset", false)
  let frameColorTintVarId = get_shader_variable_id("frame_color_tint", false)
  let figureColorTintVarId = get_shader_variable_id("figure_color_tint", false)
  recreate_material(animchar_render, "dynamic_chevron") <| $(mat)
    mat |> set_color4_param(figureOffsetVarId, Color4(float4(figureOffset.x, figureOffset.y, 0.0, 0.0)))
    mat |> set_color4_param(frameColorTintVarId, Color4(frameColorTint))
    mat |> set_color4_param(figureColorTintVarId, Color4(figureColorTint))


[es(tag=render, on_appear, REQUIRE=(hasChevron, slot_attach, attachedToParent))]
def dynamic_chevron_apply_es(evt : Event; item__ownerEid : EntityId;
                             var animchar_render : AnimcharRendComponent&;
                             var chevron_background_d_tex : SharedTex&;
                             var chevron_background_ao_tex : SharedTex&;
                             var chevron_mask_tex : SharedTex&;
                             var chevron_frame_d_tex : SharedTex&;
                             var chevron_frame_n_tex : SharedTex&;
                             var chevron_frame_ao_tex : SharedTex&;
                             var chevron_figure_d_tex : SharedTex&;
                             var chevron_figure_n_tex : SharedTex&;
                             var chevron_figure_ao_tex : SharedTex&)
  query(item__ownerEid) <| $ [es(REQUIRE=squad_member__playerEid)] (squad_member__playerEid : EntityId)
    query(squad_member__playerEid) <| $ [es] (chevronData : Object)
      apply_chevron_data(chevronData, animchar_render,
                          chevron_background_d_tex,
                          chevron_background_ao_tex,
                          chevron_mask_tex,
                          chevron_frame_d_tex,
                          chevron_frame_n_tex,
                          chevron_frame_ao_tex,
                          chevron_figure_d_tex,
                          chevron_figure_n_tex,
                          chevron_figure_ao_tex)

[es(tag=render, on_appear, track=item__ownerEid, REQUIRE=hasChevron)]
def dynamic_chevron_apply_menu_char_es(evt : Event;
                                       item__ownerEid : EntityId;
                                       var animchar_render : AnimcharRendComponent&;
                                       var chevron_background_d_tex : SharedTex&;
                                       var chevron_background_ao_tex : SharedTex&;
                                       var chevron_mask_tex : SharedTex&;
                                       var chevron_frame_d_tex : SharedTex&;
                                       var chevron_frame_n_tex : SharedTex&;
                                       var chevron_frame_ao_tex : SharedTex&;
                                       var chevron_figure_d_tex : SharedTex&;
                                       var chevron_figure_n_tex : SharedTex&;
                                       var chevron_figure_ao_tex : SharedTex&)
  if !has(item__ownerEid, "menuChar")
    return
  let checked_eid = get_local_player_eid()
  query(checked_eid) <| $ [es] (chevronData : Object)
    apply_chevron_data(chevronData, animchar_render,
                        chevron_background_d_tex,
                        chevron_background_ao_tex,
                        chevron_mask_tex,
                        chevron_frame_d_tex,
                        chevron_frame_n_tex,
                        chevron_frame_ao_tex,
                        chevron_figure_d_tex,
                        chevron_figure_n_tex,
                        chevron_figure_ao_tex)


[es(tag=render, on_appear, track=chevronData)]
def dynamic_chevron_menu_track_data(evt : Event; chevronData : Object)
  query() <| $ [es(REQUIRE=menuChar)] (attaches_list : EidList)
    for attachedEid in attaches_list
      query(attachedEid) <| $ [es(REQUIRE=hasChevron)] (var animchar_render : AnimcharRendComponent&;
                                                        var chevron_background_d_tex : SharedTex&;
                                                        var chevron_background_ao_tex : SharedTex&;
                                                        var chevron_mask_tex : SharedTex&;
                                                        var chevron_frame_d_tex : SharedTex&;
                                                        var chevron_frame_n_tex : SharedTex&;
                                                        var chevron_frame_ao_tex : SharedTex&;
                                                        var chevron_figure_d_tex : SharedTex&;
                                                        var chevron_figure_n_tex : SharedTex&;
                                                        var chevron_figure_ao_tex : SharedTex&)
        apply_chevron_data(chevronData, animchar_render,
                            chevron_background_d_tex,
                            chevron_background_ao_tex,
                            chevron_mask_tex,
                            chevron_frame_d_tex,
                            chevron_frame_n_tex,
                            chevron_frame_ao_tex,
                            chevron_figure_d_tex,
                            chevron_figure_n_tex,
                            chevron_figure_ao_tex)
