// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <shaders/dag_shaderVar.h>
#include <render/renderSettings.h>

static ShaderVariableInfo gtao_screen_radiusVarId("gtao_screen_radius", true);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__gtao_screen_radius)
static void gtao_screen_radius_settings_es(const ecs::Event &, float render_settings__gtao_screen_radius)
{
  ShaderGlobal::set_float(gtao_screen_radiusVarId, render_settings__gtao_screen_radius);
}
