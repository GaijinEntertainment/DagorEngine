// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderDynamicCube.h"

#include <math/dag_hlsl_floatx.h>

#define INSIDE_RENDERER
#include "private_worldRenderer.h"
#include "gbufferConsts.h"

#include <render/dag_cur_view.h>
#include <render/deferredRenderer.h>
#include <math/dag_cube_matrix.h>
#include "global_vars.h"
#include "render/renderEvent.h"
#include <ecs/core/entityManager.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>


#define RENDER_DYNAMIC_CUBE_VARS VAR(tonemap_on)

#define VAR(a) static int a##VarId = -1;
RENDER_DYNAMIC_CUBE_VARS
#undef VAR

constexpr float z_near = 0.25, z_far = 400, ri_zf = 75.f;

struct ScopedNoExposure
{
  ScopedNoExposure() { g_entity_mgr->broadcastEventImmediate(RenderSetExposure{false}); }
  ~ScopedNoExposure() { g_entity_mgr->broadcastEventImmediate(RenderSetExposure{true}); }
};

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
RenderDynamicCube::RenderDynamicCube() { RENDER_DYNAMIC_CUBE_VARS; }
#undef VAR

RenderDynamicCube::~RenderDynamicCube() { close(); }

void RenderDynamicCube::close()
{
  shadedTarget.close();
  target.reset();
  copy.clear();
}

void RenderDynamicCube::init(int cube_size)
{
  close();
  cubeSize = cube_size;

  d3d::set_esram_layout(L"Probes cube");
  shadedTarget =
    dag::create_tex(NULL, cube_size, cube_size, TEXFMT_A16B16G16R16F | TEXCF_RTARGET | TEXCF_ESRAM_ONLY, 1, "dynamic_cube_tex_target");
  copy.init("copy_tex");

  uint32_t cube_gbuf_fmts[NO_MOTION_GBUFFER_RT_COUNT];
  uint32_t cube_gbuf_cnt = 0;
  const char *resolveShader = nullptr;
  auto worldRenderer = static_cast<WorldRenderer *>(get_world_renderer());
  if (worldRenderer->isForwardRender())
  {
    resolveShader = nullptr;
    cube_gbuf_cnt = 1;
    cube_gbuf_fmts[0] = TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_ESRAM_ONLY;
  }
  else if (!worldRenderer->isThinGBuffer())
  {
    resolveShader = NO_MOTION_GBUFFER_RESOLVE_SHADER;
    cube_gbuf_cnt = NO_MOTION_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < cube_gbuf_cnt; ++i)
      cube_gbuf_fmts[i] = FULL_GBUFFER_FORMATS[i] | TEXCF_ESRAM_ONLY;
  }
  else
  {
    resolveShader = THIN_GBUFFER_RESOLVE_SHADER;
    cube_gbuf_cnt = THIN_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < cube_gbuf_cnt; ++i)
      cube_gbuf_fmts[i] = THIN_GBUFFER_RT_FORMATS[i] | TEXCF_ESRAM_ONLY;
  }
  target = eastl::make_unique<DeferredRenderTarget>(resolveShader, "cube", cube_size, cube_size,
    DeferredRT::StereoMode::MonoOrMultipass, 0, cube_gbuf_cnt, cube_gbuf_fmts, TEXFMT_DEPTH32 | TEXCF_ESRAM_ONLY);
  d3d::unset_esram_layout();
}

TMatrix4_vec4 RenderDynamicCube::getGlobTmForFace(int face_num, const Point3 &pos)
{
  TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, face_num);
  cameraMatrix.setcol(3, pos);
  TMatrix view = orthonormalized_inverse(cameraMatrix);

  return TMatrix4(view) * matrix_perspective(1, 1, z_near, ri_zf);
}

void RenderDynamicCube::update(const ManagedTex *cubeTarget, const Point3 &pos, int face_num, RenderMode mode)
{
  if (!isInit())
  {
    logerr("RenderDynamicCube: update without init");
    init(cubeSize <= 0 ? 128 : cubeSize);
  }

  SCOPE_VIEW_PROJ_MATRIX;
  SCOPE_RENDER_TARGET;
  ShaderGlobal::set_color4(world_view_posVarId, pos.x, pos.y, pos.z, 1);
  Driver3dPerspective persp = Driver3dPerspective(1, 1, z_near, z_far, 0, 0);
  TMatrix4 projTm;
  d3d::setpersp(persp, &projTm);
  ShaderGlobal::set_color4(zn_zfarVarId, z_near, z_far, 0, 0);
  const int faceBegin = face_num == -1 ? 0 : face_num;
  const int faceEnd = face_num == -1 ? 6 : face_num + 1;
  WorldRenderer &cb = *((WorldRenderer *)get_world_renderer());
  ScopedNoExposure noExposure;

  if (cb.hasFeature(FeatureRenderFlags::FORWARD_RENDERING))
  {
    ShaderGlobal::set_int(tonemap_onVarId, 0);
  }

  // Calculate the first face for reprojection calculation in cb.startRenderLightProbe/useFogNoScattering
  TMatrix cameraMatrixFirstSide = cube_matrix(TMatrix::IDENT, faceBegin);
  cameraMatrixFirstSide.setcol(3, pos);
  TMatrix viewFirstSide = orthonormalized_inverse(cameraMatrixFirstSide);
  cb.startRenderLightProbe(pos, viewFirstSide, projTm);
  TMatrix itm = ::grs_cur_view.itm;
  for (int i = faceBegin; i < faceEnd; ++i)
  {
    target->setRt();
    TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
    cameraMatrix.setcol(3, pos);
    TMatrix view = orthonormalized_inverse(cameraMatrix);
    d3d::settm(TM_VIEW, view);
    ::grs_cur_view.itm = cameraMatrix;

    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);

    if (mode == RenderMode::WHOLE_SCENE)
    {
      const TMatrix4_vec4 riPerspMatrix = TMatrix4(view) * matrix_perspective(1, 1, z_near, ri_zf);
      Frustum frustum(riPerspMatrix);
      if (face_num < 0)
        cb.prepareLightProbeRIVisibility(reinterpret_cast<const mat44f &>(riPerspMatrix), pos);
      cb.renderLightProbeOpaque(pos, itm, frustum);

      d3d::resource_barrier({target->getRt(0), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (target->getRt(1))
        d3d::resource_barrier({target->getRt(1), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (target->getRt(2))
        d3d::resource_barrier({target->getRt(2), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({target->getDepth(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      target->resolve(shadedTarget.getTex2D(), view, projTm);
    }

    d3d::set_render_target(shadedTarget.getTex2D(), 0); // because of cube depth
    d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);

    // d3d::clearview(CLEAR_ZBUFFER|CLEAR_STENCIL, 0, 0, 0);
    d3d::resource_barrier({target->getDepth(), RB_RO_CONSTANT_DEPTH_STENCIL_TARGET | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    cb.renderLightProbeEnvi(view, projTm, persp);
    // save_rt_image_as_tga(shadedTarget.getTex2D(), String(128, "cube%s.tga", i));

    d3d::set_render_target(cubeTarget->getCubeTex(), i, 0);
    d3d::settex(2, shadedTarget.getTex2D());
    d3d::set_sampler(STAGE_PS, 2, d3d::request_sampler({}));
    d3d::resource_barrier({shadedTarget.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    copy.render();
  }
  cb.endRenderLightProbe();

  if (cb.hasFeature(FeatureRenderFlags::FORWARD_RENDERING))
  {
    ShaderGlobal::set_int(tonemap_onVarId, 1);
  }

  ::grs_cur_view.itm = itm;
}
