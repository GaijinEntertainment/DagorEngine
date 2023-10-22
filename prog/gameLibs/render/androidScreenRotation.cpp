#include "render/androidScreenRotation.h"
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_drv3d.h>
#include <perfMon/dag_statDrv.h>

namespace
{
PostFxRenderer pass;
bool enabled = false;
int android_screen_rotationVarId = -1;
intptr_t bbBindingIdx = 15;
} // anonymous namespace

void android_screen_rotation::init()
{
  if (enabled)
    return;

  enabled = ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBool("preRotation", false);
  if (enabled)
  {
    pass.init("android_screen_rotate");
    android_screen_rotationVarId = get_shader_variable_id("android_screen_rotation");
    bbBindingIdx = ShaderGlobal::get_int_fast(get_shader_variable_id("android_internal_backbuffer_binding_reg"));
  }
  debug("android_screen_rotation: %s", enabled ? "enabled" : "disabled");
}

void android_screen_rotation::shutdown()
{
  if (enabled)
    pass.clear();
}

void android_screen_rotation::onFrameEnd()
{
  if (!enabled)
    return;

  TIME_D3D_PROFILE(android_screen_rotate);

  // bind empty targets, to avoid problems with shadow state
  d3d::set_render_target();
  d3d::set_render_target(0, nullptr, 0);
  int angle = d3d::driver_command(DRV3D_COMMAND_PRE_ROTATE_PASS, (void *)bbBindingIdx, nullptr, nullptr);
  ShaderGlobal::set_int(android_screen_rotationVarId, angle);
  // do transform only when needed
  if (angle)
  {
    d3d::set_render_target();
    d3d::set_depth(nullptr, DepthAccess::SampledRO);
    d3d::clearview(CLEAR_DISCARD_TARGET, E3DCOLOR(0), 0, 0);
    pass.render();
  }
}