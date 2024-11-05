// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <render/renderEvent.h>
#include <ecs/core/entityManager.h>
#include <render/priorityManagedShadervar.h>
#include <main/water.h>
#include <ecs/render/updateStageRender.h>

ECS_TAG(render)
ECS_AFTER(update_water_level_values_es)
static void underwater_update_shadervars_es(
  const UpdateStageInfoBeforeRender &, float underwater_postfx__vignette_strength, Point3 underwater_postfx__chromatic_aberration)
{
  bool hasPostfx = []() {
    auto wr = static_cast<WorldRenderer *>(get_world_renderer());
    return wr && wr->hasFeature(FeatureRenderFlags::POSTFX);
  }();
  if (hasPostfx)
  {
    static constexpr int CHROMATIC_ABERRATION_PRIORITY = 2;
    static constexpr int VIGNETTE_PRIORITY = 2;
    static int vignette_strengthVarId = get_shader_variable_id("vignette_strength");
    static int chromatic_aberration_paramsVarId = get_shader_variable_id("chromatic_aberration_params");
    if (is_underwater())
    {
      PriorityShadervar::set_color4(chromatic_aberration_paramsVarId, CHROMATIC_ABERRATION_PRIORITY,
        Point4::xyz0(underwater_postfx__chromatic_aberration));
      PriorityShadervar::set_real(vignette_strengthVarId, VIGNETTE_PRIORITY, underwater_postfx__vignette_strength);
    }
    else
    {
      PriorityShadervar::clear(chromatic_aberration_paramsVarId, CHROMATIC_ABERRATION_PRIORITY);
      PriorityShadervar::clear(vignette_strengthVarId, VIGNETTE_PRIORITY);
    }
  }
}
