// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texMgr.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_carray.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_consts.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_TMatrix4D.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_statDrv.h>
#include <render/viewVecs.h>
#include <render/bigLightsShadows.h>
#include <render/set_reprojection.h>
#include <drv/3d/dag_resetDevice.h>

#define MAX_COUNT 4
#define GLOBAL_VARS_LIST        \
  VAR(shadow_frame)             \
  VAR(big_light_reset_temporal) \
  VAR(big_shadows_tex)          \
  VAR(big_shadows_cnt)          \
  VAR(big_shadows_prev_tex)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR
static int big_light_posVarIds[MAX_COUNT] = {-1, -1, -1, -1};

class BigLightsShadows
{
public:
  TMatrix4 prevGlobTm = TMatrix4::IDENT, prevProjTm = TMatrix4::IDENT;
  Point4 prevViewVecLT;
  Point4 prevViewVecRT;
  Point4 prevViewVecLB;
  Point4 prevViewVecRB;
  DPoint3 prevWorldPos;
  unsigned int frame = 0;
  unsigned resetGen = 0;
  carray<UniqueTex, 3> targetTex;

  int w = 0, h = 0;
  uint32_t maxCnt = 0;
  PostFxRenderer render_big_light_shadows, temporal_big_light_shadows;
  void render(const DPoint3 *world_pos);

  void init(int w, int h, unsigned int max_lights_count, const char *prefix);
  void close();
  void render(const DPoint3 *world_pos, const Point4 *pos_rad, uint32_t cnt);
  const UniqueTex &get() const;
  void setTex(int reg);
};

void BigLightsShadows::setTex(int reg) { d3d::settex(reg, get().getTex2D()); }

void BigLightsShadows::init(int w_, int h_, unsigned int maxCnt_, const char *prefix)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
  if (maxCnt_ > MAX_COUNT)
    logerr("we currently support no more than 4 big lights shadows");
  maxCnt = min(maxCnt_, (uint32_t)MAX_COUNT);
  w = w_;
  h = h_;
  const uint32_t fmt = maxCnt == 1 ? TEXFMT_L8 : maxCnt == 2 ? TEXFMT_R8G8 : TEXFMT_DEFAULT;
  for (int i = 0; i < MAX_COUNT; ++i)
    big_light_posVarIds[i] = get_shader_variable_id(String(0, "big_light_pos_rad_%d", i), true);

  for (int i = 0; i < targetTex.size(); ++i)
  {
    targetTex[i].close();
    String name(128, "%s_big_lights_shadows_tex_%d", prefix ? prefix : "", i);
    targetTex[i] = dag::create_tex(NULL, w, h, fmt | TEXCF_RTARGET, 1, name);
    targetTex[i].getTex2D()->texbordercolor(0xFFFFFFFF);
    targetTex[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }


  render_big_light_shadows.init("render_big_light_shadows");
  temporal_big_light_shadows.init("temporal_big_light_shadows");
  prevGlobTm = TMatrix4::IDENT;
  prevProjTm = TMatrix4::IDENT;
  prevViewVecLT = Point4(0, 0, 0, 0);
  prevViewVecRT = Point4(0, 0, 0, 0);
  prevViewVecLB = Point4(0, 0, 0, 0);
  prevViewVecRB = Point4(0, 0, 0, 0);
  prevWorldPos = DPoint3(0, 0, 0);
}

void BigLightsShadows::close()
{
  for (auto &i : targetTex)
    i.close();
}

void BigLightsShadows::render(const DPoint3 *world_pos, const Point4 *pos_rad, uint32_t cnt)
{
  cnt = min(cnt, maxCnt);
  TIME_D3D_PROFILE(big_shadows_total);
  SCOPE_RENDER_TARGET;
  const uint32_t gen = get_d3d_reset_counter();
  for (int i = 0; i < cnt; ++i)
    ShaderGlobal::set_color4(big_light_posVarIds[i], P4D(pos_rad[i]));
  for (int i = cnt; i < MAX_COUNT; ++i)
    ShaderGlobal::set_color4(big_light_posVarIds[i], 0, 0, 0, -1);
  ShaderGlobal::set_int(big_shadows_cntVarId, cnt);

  ShaderGlobal::set_real(big_light_reset_temporalVarId, resetGen != gen ? 2 : 0);
  resetGen = gen;
  const uint32_t current = frame & 1;
  frame++;
  frame &= ((1 << 23) - 1);

  ShaderGlobal::set_int(shadow_frameVarId, frame);

  TMatrix viewTm;
  TMatrix4 projTm;
  d3d::gettm(TM_VIEW, viewTm);
  d3d::gettm(TM_PROJ, &projTm);
  set_reprojection(viewTm, projTm, prevProjTm, prevWorldPos, prevGlobTm, prevViewVecLT, prevViewVecRT, prevViewVecLB, prevViewVecRB,
    world_pos);
  if (cnt == 0)
  {
    d3d::set_render_target(targetTex[current].getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 1.0, 0);
  }
  else
  {
    {
      TIME_D3D_PROFILE(big_shadows_render);
      d3d::set_render_target(targetTex[2].getTex2D(), 0);
      d3d::clearview(CLEAR_DISCARD, 0xFFFFFFFF, 1.0, 0);
      render_big_light_shadows.render();
      d3d::resource_barrier({targetTex[2].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }

    {
      TIME_D3D_PROFILE(big_shadows_temporal);
      ShaderGlobal::set_texture(big_shadows_prev_texVarId, targetTex[1 - current].getTexId());
      ShaderGlobal::set_texture(big_shadows_texVarId, targetTex[2].getTexId());
      d3d::set_render_target(targetTex[current].getTex2D(), 0);
      d3d::clearview(CLEAR_DISCARD, 0xFFFFFFFF, 1.0, 0);
      temporal_big_light_shadows.render();
      // end of blur
    }
  }
  d3d::resource_barrier({targetTex[current].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  ShaderGlobal::set_texture(big_shadows_texVarId, targetTex[current].getTexId());
}

const UniqueTex &BigLightsShadows::get() const { return targetTex[1 - (frame & 1)]; }


BigLightsShadows *create_big_lights_shadows(int w, int h, unsigned int maxCnt, const char *prefix)
{
  BigLightsShadows *r = new BigLightsShadows;
  r->init(w, h, maxCnt, prefix);
  return r;
}

void destroy_big_lights_shadows(BigLightsShadows *&r)
{
  ShaderGlobal::set_color4(big_light_posVarIds[0], 0, 10000, 0, -1);
  ShaderGlobal::set_int(big_shadows_cntVarId, 0);
  del_it(r);
}

void render_big_lights_shadows(BigLightsShadows *r, const Point4 *pos_rad, uint32_t cnt, const DPoint3 *world_pos)
{
  if (r)
    r->render(world_pos, pos_rad, cnt);
}

void set_big_lights_shadows(BigLightsShadows *r, int reg)
{
  if (r)
    r->setTex(reg);
}
