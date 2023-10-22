#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <render/scopeRenderTarget.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_statDrv.h>
#include "shadowDepthScroller.h"

#define GLOBAL_VARS_LIST  \
  VAR(tile_read_depth_2d) \
  VAR(tile_change_depth_scale_ofs)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

#define GLOBAL_CONST_LIST VAR(tile_change_depth_source_tex)

#define VAR(a) static int a##_const_no = -1;
GLOBAL_CONST_LIST
#undef VAR

bool ShadowDepthScroller::translateDepth(Texture *tex, int layer, float scale, float ofs)
{
  if (ofs == 0 && scale == 1.0f) // we compare for exact!
    return false;
  if (!readDepthPS || !writeDepthPS)
    return false;
  SCOPE_RENDER_TARGET;
  if (scale + ofs <= 0.0f || ofs >= 1.0f) // no point in any translation. Everything is totally incorrect
  {
    d3d_err(d3d::set_render_target(0, (Texture *)NULL, 0));
    if (layer >= 0)
      d3d_err(d3d::set_depth(tex, layer, DepthAccess::RW));
    else
      d3d_err(d3d::set_depth(tex, DepthAccess::RW));
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 1.f, 0);
    return true;
  }
  TIME_D3D_PROFILE(st_shadows_change_depth);
  shaders::overrides::set(scrollStateId);
  ShaderGlobal::set_color4(tile_change_depth_scale_ofsVarId, scale, ofs, (float)layer, 0);
  ShaderGlobal::set_int(tile_read_depth_2dVarId, (layer < 0) ? 1 : 0);
  TextureInfo tinfo;
  tex->getinfo(tinfo);
  for (int y = 0; y < tinfo.h; y += tileSizeH)
    for (int x = 0; x < tinfo.w; x += tileSizeW)
    {
      const uint32_t addr = x | (y << 16);
      d3d::set_immediate_const(STAGE_PS, &addr, 1);
      d3d_err(d3d::set_render_target(tileTex.getTex2D(), 0, 0));
      d3d::set_depth(NULL, DepthAccess::RW);
      d3d::set_tex(STAGE_PS, tile_change_depth_source_tex_const_no, tex, false);
      readDepthPS->render();
      d3d::resource_barrier({tileTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d_err(d3d::set_render_target(0, (Texture *)NULL, 0));
      if (layer >= 0)
        d3d_err(d3d::set_depth(tex, layer, DepthAccess::RW));
      else
        d3d_err(d3d::set_depth(tex, DepthAccess::RW));
      d3d::set_tex(STAGE_PS, tile_change_depth_source_tex_const_no, tileTex.getTex2D(), false);
      d3d::setview(x, y, min<int>(tinfo.w - x, tileSizeW), min<int>(tinfo.h - y, tileSizeH), 0, 1);
      writeDepthPS->render();
      // fixme: I don't understand resource barriers logic. Probably there should be no resource barrier in loop on frontFrame tex, as
      // we only write independent tiles d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX,
      // 0, 0});
    }
  d3d::resource_barrier({tex, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  ShaderElement::invalidate_cached_state_block();
  shaders::overrides::reset();
  return true;
}

void ShadowDepthScroller::init()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

#define VAR(a)                                              \
  {                                                         \
    int tmp = get_shader_variable_id(#a "_const_no", true); \
    if (tmp >= 0)                                           \
      a##_const_no = ShaderGlobal::get_int_fast(tmp);       \
  }
  GLOBAL_CONST_LIST
#undef VAR

  readDepthPS = eastl::make_unique<PostFxRenderer>();
  readDepthPS->init("tile_read_depth_ps");

  writeDepthPS = eastl::make_unique<PostFxRenderer>();
  writeDepthPS->init("tile_write_depth_ps");

  if (readDepthPS->getElem() == nullptr || writeDepthPS->getElem() == nullptr)
  {
    readDepthPS.reset();
    writeDepthPS.reset();
    logwarn("tile_read_depth_ps/tile_write_depth_ps shaders are missing");
    return;
  }
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  scrollStateId = shaders::overrides::create(state);

  const char *name = "static_shadow_depth_tile_patch";
  tileTex.set(d3d::create_tex(NULL, tileSizeW, tileSizeH, TEXCF_RTARGET | TEXFMT_L16, 1, name), name);
  tileTex.getTex()->texfilter(TEXFILTER_POINT);
  tileTex.getTex()->texaddr(TEXADDR_CLAMP);
  tileTex.setVar();
}
