module effect_animation_common shared
require ecs


def begin_effect_animation(eid : EntityId)
  ecs::query(eid) <| $ [es] (effect_animation__activatedTemplateName : string;
                             transform : float3x4 const?)
    addSubTemplate(eid, effect_animation__activatedTemplateName) <| $(var init)
      set(init, "effect_animation__currentStageId", -1)
      if transform != null
        set(init, "effect_animation__transform", *transform)

def stop_effect_animation(eid : EntityId)
  ecs::query(eid) <| $ [es] (effect_animation__activatedTemplateName : string)
    removeSubTemplate(eid, effect_animation__activatedTemplateName)