// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/dof/dof_ps.h>
#include <daECS/core/componentType.h>

void set_dof_blend_depth_tex(TEXTUREID tex);

void setFadeMul(float mul);

void postfx_bind_additional_textures_from_registry(dafg::Registry &registry);
void postfx_bind_additional_textures_from_namespace(dafg::NameSpaceRequest &ns);
void postfx_read_additional_textures_from_registry(dafg::Registry &registry);

ECS_DECLARE_RELOCATABLE_TYPE(DepthOfFieldPS)