// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_preRotation.h>

#if _TARGET_ANDROID

#include <util/dag_stdint.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_renderTarget.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <math/dag_color.h>

static ShaderVariableInfo pre_rotation_var("pre_rotation", true);
static const Color4 pre_rotation_identity(1, 0, 0, 1);

static int cur_angle = 0;
static int cur_shader_angle = 0;
static Color4 cur_rotation_col = pre_rotation_identity;

static bool is_backbuffer(BaseTexture *tex) { return tex && tex == d3d::get_backbuffer_tex(); }

void prerotation::refresh()
{
  cur_angle = d3d::driver_command(Drv3dCommand::GET_SWAPCHAIN_PRE_ROTATION);
  const Color4 cols[4] = {pre_rotation_identity, Color4(0, -1, 1, 0), Color4(-1, 0, 0, -1), Color4(0, 1, -1, 0)};
  cur_rotation_col = cols[(cur_angle / 90) & 3];
}

int prerotation::frame_angle() { return cur_angle; }

int prerotation::angle_for_target(BaseTexture *dst) { return is_backbuffer(dst) ? cur_angle : 0; }

void prerotation::set_shader_var(int a)
{
  G_ASSERTF(a == 0 || a == cur_angle, "prerotation angle %d does not match current %d", a, cur_angle);
  G_ASSERTF(a == 0 || VariableMap::isVariablePresent(pre_rotation_var.get_var_id()),
    "Trying to set non-zero prerotation with missing var in shaders");
  cur_shader_angle = a;
  ShaderGlobal::set_float4(pre_rotation_var, a ? cur_rotation_col : pre_rotation_identity);
}

int prerotation::get_current_angle() { return cur_shader_angle; }

prerotation::Scope::Scope(BaseTexture *dst)
{
  savedAngle = cur_shader_angle;
  set_shader_var(angle_for_target(dst));
}
prerotation::Scope::~Scope() { set_shader_var(savedAngle); }

#endif // _TARGET_ANDROID
