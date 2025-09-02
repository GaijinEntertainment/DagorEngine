// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <ecs/render/animCharUtils.h>
#include <shaders/dag_shaders.h>
#include <math/dag_color.h>
#include <daECS/core/componentTypes.h>

#define CAMOUFLAGE_OVERRIDE_VARS    \
  VAR(material_id)                  \
  VAR(camouflage_scale_and_offset)  \
  VAR(solid_fill_color)             \
  VAR(color_multiplier)             \
  VAR(camouflage_tex)               \
  VAR(micro_detail_layer)           \
  VAR(micro_detail_layer_intensity) \
  VAR(micro_detail_layer_uv_scale)  \
  VAR(micro_detail_layer_v_scale)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
CAMOUFLAGE_OVERRIDE_VARS
#undef VAR

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(camouflage_override) // only for use for debug in editor! (don't create materials on track for other cases)
static void dynamic_sheen_camo_override_params_es_event_handler(const ecs::Event &, AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::Array &camouflage_override)
{
  if (camouflage_override.empty() || !VariableMap::isVariablePresent(material_idVarId))
    return;

  for (const auto &material : camouflage_override)
  {
    ecs::Object parameters = material.get<ecs::Object>();
    if (!parameters.empty())
    {
      const int materialId = parameters.getMemberOr(ECS_HASH("material_id"), -1);
      const Point4 *camouflageScaleAndOffset = parameters.getNullable<Point4>(ECS_HASH("camouflage_scale_and_offset"));
      const Point4 *solidFillColor = parameters.getNullable<Point4>(ECS_HASH("solid_fill_color"));
      const float *colorMultiplier = parameters.getNullable<float>(ECS_HASH("color_multiplier"));
      const bool *forceSolidFill = parameters.getNullable<bool>(ECS_HASH("force_solid_fill"));
      const int *microDetailLayer = parameters.getNullable<int>(ECS_HASH("micro_detail_layer"));
      const float *microDetailLayerIntensity = parameters.getNullable<float>(ECS_HASH("micro_detail_layer_intensity"));
      const float *microDetailLayerUvScale = parameters.getNullable<float>(ECS_HASH("micro_detail_layer_uv_scale"));
      const float *microDetailLayerVScale = parameters.getNullable<float>(ECS_HASH("micro_detail_layer_v_scale"));

      if (!camouflageScaleAndOffset && !solidFillColor && !colorMultiplier && !forceSolidFill && !microDetailLayer &&
          !microDetailLayerIntensity && !microDetailLayerUvScale && !microDetailLayerVScale)
        continue;

      recreate_material_with_new_params(animchar_render, "dynamic_sheen_camo", [&](ShaderMaterial *mat) {
        int currentMaterialId = -1;
        mat->getIntVariable(material_idVarId.get_var_id(), currentMaterialId);
        if (materialId != currentMaterialId)
          return;

        if (camouflageScaleAndOffset)
          mat->set_color4_param(camouflage_scale_and_offsetVarId.get_var_id(), Color4::xyzw(*camouflageScaleAndOffset));
        if (solidFillColor)
          mat->set_color4_param(solid_fill_colorVarId.get_var_id(), Color4::xyzw(*solidFillColor));
        if (colorMultiplier)
          mat->set_real_param(color_multiplierVarId.get_var_id(), *colorMultiplier);
        if (forceSolidFill && *forceSolidFill)
          mat->set_texture_param(camouflage_texVarId.get_var_id(), BAD_TEXTUREID);

        if (microDetailLayer)
          mat->set_int_param(micro_detail_layerVarId.get_var_id(), *microDetailLayer);
        if (microDetailLayerIntensity)
          mat->set_real_param(micro_detail_layer_intensityVarId.get_var_id(), *microDetailLayerIntensity);
        if (microDetailLayerUvScale)
          mat->set_real_param(micro_detail_layer_uv_scaleVarId.get_var_id(), *microDetailLayerUvScale);
        if (microDetailLayerVScale)
          mat->set_real_param(micro_detail_layer_v_scaleVarId.get_var_id(), *microDetailLayerVScale);
      });
    }
  }
}