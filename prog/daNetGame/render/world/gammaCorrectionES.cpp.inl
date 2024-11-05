// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <3d/dag_render.h>
#include <render/renderSettings.h>

ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__gamma_correction, render_settings__is_hdr_enabled)
static void gamma_correction_es(const ecs::Event &, bool render_settings__is_hdr_enabled, float render_settings__gamma_correction)
{
  set_gamma_shadervar(render_settings__is_hdr_enabled ? 1.0f : render_settings__gamma_correction);
}
