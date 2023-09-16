#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "riGen/riGenRender.h"
#include "riGen/riGenExtra.h"
#include "riGen/riRotationPalette.h"

#include <stdio.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_shaderBlock.h>
#include <fx/dag_leavesWind.h>
#include <3d/dag_render.h>
#include <3d/dag_drv3d_platform.h>
#include <math/dag_frustum.h>
#include <generic/dag_carray.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_adjpow2.h>
#include <render/dxtcompress.h>
#include <shaders/dag_postFxRenderer.h>
#include <memory/dag_framemem.h>
#include <render/bcCompressor.h>
#include <3d/dag_drv3dCmd.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/array.h>

#define LOGLEVEL_DEBUG _MAKE4C('RGEN')

struct ColorDepthTexturePair
{
  Texture *colorTex = nullptr;
  Texture *depthTex = nullptr;

  void createTextures(uint32_t size)
  {
    colorTex = d3d::create_tex(NULL, size, size, TEXCF_RTARGET, 1, "rtTempTex");
    d3d_err(colorTex);
    createDepth(size);
  }

  void createDepth(uint32_t size)
  {
    depthTex = d3d::create_tex(NULL, size, size, TEXFMT_DEPTH24 | TEXCF_RTARGET, 1, "rtTempTexDepth");
    d3d_err(depthTex);
  }
};

static ColorDepthTexturePair g_lowres_tex;
static ColorDepthTexturePair g_fullres_tex;
static TEXTUREID g_fullres_texid = BAD_TEXTUREID;
static shaders::OverrideStateId g_zfunc_state_id;

static eastl::array<eastl::unique_ptr<BcCompressor>, 3> g_compressors; // 3 frame carousell
static int g_current_compressor = 0;

enum
{
  PHASE_NEW = 0,
  PHASE_LOW_PASS_RENDER = 1,
  PHASE_LOW_PASS_FEEDBACK = 2,
  PHASE_HIGH_PASS_RENDER = 3,
  PHASE_COMPRESSION = 4,
  PHASE_READY = 5,
};

enum
{
  RET_ERR = 0,
  RET_NEXT_PHASE = 1,
  RET_ALL_DONE = 2,
};

namespace rendinst
{
void closeImpostorShadowTempTex()
{
  del_d3dres(g_lowres_tex.colorTex);
  del_d3dres(g_lowres_tex.depthTex);
  del_d3dres(g_fullres_tex.depthTex);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(g_fullres_texid, g_fullres_tex.colorTex);

  for (auto &i : g_compressors)
    i.reset(nullptr);

  g_zfunc_state_id.reset();
}
} // namespace rendinst

int RendInstGenData::RtData::renderRendinstGlobalShadowsToTextures(const Point3 &sun_dir_0, bool force_update, bool use_compression)
{
  for (; nextPoolForShadowImpostors < rtPoolData.size(); ++nextPoolForShadowImpostors)
  {
    if (!rtPoolData[nextPoolForShadowImpostors])
      continue;

    if (rtPoolData[nextPoolForShadowImpostors]->hasImpostor())
      break;
  }

  if (nextPoolForShadowImpostors >= rtPoolData.size())
    return RET_ALL_DONE;

  if (force_update && is_managed_textures_streaming_load_on_demand())
    prefetch_and_wait_managed_textures_loaded(riImpTexIds);
  else if (!force_update && !prefetch_and_check_managed_textures_loaded(riImpTexIds))
    return RET_NEXT_PHASE;

  static unsigned int rendinstGlobalShadowTexSize =
    ::dgs_get_game_params()->getBlockByNameEx("rendinstShadows")->getInt("globalTexSize", 256);
  static int numMips = max((int)0, (int)get_log2i_of_pow2(rendinstGlobalShadowTexSize) - 2); // last mip is set

  if (!g_lowres_tex.colorTex)
  {
    rendinstGlobalShadowTexSize = get_closest_pow2(rendinstGlobalShadowTexSize);

    g_lowres_tex.createTextures(max(rendinstGlobalShadowTexSize / 4, (unsigned)64));

    if (use_compression)
    {
      g_fullres_tex.createTextures(rendinstGlobalShadowTexSize);
      g_fullres_texid = ::register_managed_tex("rtTempTex2", g_fullres_tex.colorTex);

      for (int i = 0; i < g_compressors.size(); ++i)
      {
        BcCompressor *compr = nullptr;

        if (BcCompressor::isAvailable(BcCompressor::COMPRESSION_BC4))
        {
          compr = new BcCompressor(BcCompressor::COMPRESSION_BC4, 1, rendinstGlobalShadowTexSize, rendinstGlobalShadowTexSize, 1,
            "bc4_compressor");

          if (compr->getCompressionType() == BcCompressor::COMPRESSION_ERR)
            del_it(compr);
        }

        g_compressors[i].reset(compr);
      }
    }
    else
      g_fullres_tex.createDepth(rendinstGlobalShadowTexSize);
  }

  d3d::GpuAutoLock gpu_lock;
  if (!g_zfunc_state_id)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_FUNC);
    state.zFunc = CMPF_LESSEQUAL;
    g_zfunc_state_id = shaders::overrides::create(state);
  }

  int poolNo = nextPoolForShadowImpostors;
  rendinstgenrender::RtPoolData &pool = *rtPoolData[poolNo];

  const auto finalizeCurrentPoolRendering = [&]() {
    pool.shadowImpostorUpdatePhase = PHASE_READY;
    nextPoolForShadowImpostors++;

    return nextPoolForShadowImpostors < rtPoolData.size() ? RET_NEXT_PHASE : RET_ALL_DONE;
  };

  if (!pool.rendinstGlobalShadowTex && (g_compressors[0] && !force_update || !use_compression))
  {
    char name[64];
    _snprintf(name, sizeof(name), "ri_glo_sh_%p", &pool);
    name[sizeof(name) - 1] = 0;

    const int flags = use_compression ? TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION : TEXCF_RTARGET | TEXFMT_R8;

    pool.rendinstGlobalShadowTex = d3d::create_tex(nullptr, rendinstGlobalShadowTexSize, rendinstGlobalShadowTexSize, flags, 1, name);
    if (pool.rendinstGlobalShadowTex)
    {
      pool.rendinstGlobalShadowTex->texaddr(TEXADDR_BORDER);
      pool.rendinstGlobalShadowTex->texbordercolor(0);
    }
    pool.rendinstGlobalShadowTexId = ::register_managed_tex(name, pool.rendinstGlobalShadowTex);
  }

  if (pool.shadowImpostorUpdatePhase == PHASE_NEW || pool.shadowImpostorUpdatePhase == PHASE_READY)
    pool.shadowImpostorUpdatePhase = PHASE_LOW_PASS_RENDER;

  if (pool.shadowImpostorUpdatePhase == PHASE_LOW_PASS_RENDER || pool.shadowImpostorUpdatePhase == PHASE_HIGH_PASS_RENDER)
  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    shaders::overrides::set(g_zfunc_state_id);

    mat44f ident;
    v_mat44_ident(ident);
    d3d::settm(TM_WORLD, ident);

    rendinstgenrender::RiShaderConstBuffers cb;
    cb.setBBoxZero();
    cb.setBoundingSphere(0, 0, 1, 1, 0);

    ShaderGlobal::set_int_fast(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::ImpostorShadow));
    vec4f sunDirV = v_norm3(v_make_vec4f(sun_dir_0.x, sun_dir_0.y, sun_dir_0.z, 0));
    vec4f ground = v_make_vec4f(MIN_REAL, 0, MIN_REAL, 0);

    rendinstgenrender::startRenderInstancing();
    d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

    G_ASSERT(riRes[poolNo]);
    RenderableInstanceResource *sourceScene = riResLodScene(poolNo, 0);

    // Set new matrices.
    mat44f view;
    mat44f viewitm;

    // vec4f eye = v_mul(sunDirV, v_splats(pool.sphereRadius));
    viewitm.col2 = sunDirV;
    viewitm.col0 = v_norm3(v_cross3(V_C_UNIT_0100, viewitm.col2));
    viewitm.col1 = v_cross3(viewitm.col2, viewitm.col0);

    if (pool.shadowImpostorUpdatePhase == PHASE_LOW_PASS_RENDER)
    {
      bbox3f shadowbox;
      v_bbox3_init_empty(shadowbox);
      for (unsigned int pointNo = 0; pointNo < pool.impostor.shadowPoints.size(); pointNo += 2)
      {
        vec4f localPoint = pool.impostor.shadowPoints[pointNo];
        vec4f delta = pool.impostor.shadowPoints[pointNo + 1];
        localPoint = v_add(localPoint, v_mul(v_splat_x(delta), viewitm.col0));
        localPoint = v_add(localPoint, v_mul(v_splat_y(delta), viewitm.col1));
        localPoint = v_max(localPoint, ground);
        v_bbox3_add_pt(shadowbox, localPoint);
      }
      pool.impostor.shadowSphCenterY = v_extract_y(v_mul(v_add(shadowbox.bmin, shadowbox.bmax), V_C_HALF));
    }

    vec4f center = v_make_vec4f(0, pool.impostor.shadowSphCenterY, 0, 1);
    viewitm.col3 = v_sub(center, v_mul(sunDirV, v_splats(pool.sphereRadius)));
    v_mat44_orthonormal_inverse43_to44(view, viewitm);

    if (pool.shadowImpostorUpdatePhase == PHASE_LOW_PASS_RENDER)
    {
      vec4f impostorSize = v_zero();
      for (unsigned int pointNo = 0; pointNo < pool.impostor.shadowPoints.size(); pointNo += 2)
      {
        vec4f localPoint = pool.impostor.shadowPoints[pointNo];
        vec4f delta = pool.impostor.shadowPoints[pointNo + 1];
        localPoint = v_add(localPoint, v_mul(v_splat_x(delta), viewitm.col0));
        localPoint = v_add(localPoint, v_mul(v_splat_y(delta), viewitm.col1));
        localPoint = v_max(localPoint, ground);
        vec4f rotPoint = v_mat44_mul_vec3p(view, localPoint);
        impostorSize = v_max(impostorSize, v_abs(rotPoint));
      }

      pool.impostor.shadowImpostorWd = v_extract_x(impostorSize); //+pool.impostor.maxFacingLeavesDelta;
      pool.impostor.shadowImpostorHt = v_extract_y(impostorSize); //+pool.impostor.maxFacingLeavesDelta;
    }

    DECL_ALIGN16(Point3_vec4, col[3]);
    v_st(&col[0].x, viewitm.col0);
    v_st(&col[1].x, viewitm.col1);
    v_st(&col[2].x, viewitm.col2);
    LeavesWindEffect::setNoAnimShaderVars(col[0], col[1], col[2]);


    d3d::settm(TM_VIEW, view);
    rendinstgenrender::setCoordType(riPosInst[poolNo] ? rendinstgenrender::COORD_TYPE_POS : rendinstgenrender::COORD_TYPE_TM);

    d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, rendinstgenrender::oneInstanceTmVb);

    if (pool.hasImpostor())
      cb.setInstancing(3, 1,
        rendinstgen::get_rotation_palette_manager()->getImpostorDataBufferOffset({layerIdx, poolNo}, pool.impostorDataOffsetCache));
    else
      cb.setInstancing(0, 3, 0);
    cb.flushPerDraw();

    ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();

    ColorDepthTexturePair targetPair;
    if (pool.shadowImpostorUpdatePhase == PHASE_HIGH_PASS_RENDER)
    {
      if (use_compression)
        targetPair = g_fullres_tex;
      else
        targetPair = ColorDepthTexturePair{pool.rendinstGlobalShadowTex, g_fullres_tex.depthTex};
    }
    else
      targetPair = g_lowres_tex;

    Texture *targetTex = targetPair.colorTex;

    d3d::set_render_target(RenderTarget{targetPair.depthTex, 0, 0}, false, {{targetPair.colorTex, 0, 0}});

    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0xFF000000, 1.f, 0);

    mat44f orthoProj;
    v_mat44_make_ortho(orthoProj, 2.f * pool.impostor.shadowImpostorWd, 2.f * pool.impostor.shadowImpostorHt, 0,
      2.f * pool.sphereRadius);

    d3d::settm(TM_PROJ, orthoProj);
    ShaderGlobal::setBlock(rendinstgenrender::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    bool needToSetBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE) == -1;
    if (needToSetBlock)
      ShaderGlobal::setBlock(rendinstgenrender::rendinstSceneBlockId, ShaderGlobal::LAYER_SCENE);

    RenderStateContext context;
    for (auto &elem : mesh->getAllElems())
    {
      if (!elem.e)
        continue;
      SWITCH_STATES()
      d3d_err(elem.drawIndTriList());
    }

    shaders::overrides::reset();

    rendinstgenrender::endRenderInstancing();
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    if (needToSetBlock)
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

    if (pool.shadowImpostorUpdatePhase == PHASE_LOW_PASS_RENDER && !force_update)
    {
      int stride = 0;
      if (targetTex->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
        targetTex->unlockimg();
      else
        return RET_ERR;
    }

    if (pool.shadowImpostorUpdatePhase == PHASE_HIGH_PASS_RENDER && !use_compression)
    {
      d3d::resource_barrier({pool.rendinstGlobalShadowTex, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
      return finalizeCurrentPoolRendering();
    }

    pool.shadowImpostorUpdatePhase++;

    if (!force_update) // for force_update - locking with syslock
      return RET_NEXT_PHASE;
  }

  if (pool.shadowImpostorUpdatePhase == PHASE_LOW_PASS_FEEDBACK)
  {
    Texture *targetTex = g_lowres_tex.colorTex;
    char *data;
    int stride;
    int lockFlags = force_update ? TEXLOCK_READ : (TEXLOCK_READ | TEXLOCK_NOSYSLOCK);
    if (targetTex->lockimg((void **)&data, stride, 0, lockFlags) && data)
    {
      TextureInfo info;
      targetTex->getinfo(info, 0);
      int y1;
      char *ptrTop = data, *ptrBottom = data + (info.h - 1) * stride;
      for (y1 = 0; y1 < info.h / 2; ++y1, ptrTop += stride, ptrBottom -= stride)
      {
        int x;
        for (x = 0; x < info.w; ++x)
          if (((((unsigned *)ptrTop)[x]) & 0x0000FF00) != 0 || (((unsigned *)ptrBottom)[x] & 0x0000FF00) != 0)
            break;
        if (x != info.w)
          break;
      }
      int x;
      for (x = 0; x < info.w / 2; ++x)
      {
        char *ptrLeft = data + x * 4, *ptrRight = data + (info.w - 1 - x) * 4;
        int y2;
        for (y2 = 0; y2 < info.h; ++y2, ptrRight += stride, ptrLeft += stride)
          if (((*((unsigned *)ptrLeft)) & 0x0000FF00) != 0 || ((*((unsigned *)ptrRight)) & 0x0000FF00) != 0)
            break;
        if (y2 != info.h)
          break;
      }
      // debug("impostor%d, can skip %d lines and %d columns!", poolNo, y1,x );
      if (y1 > 1)
        pool.impostor.shadowImpostorHt *= (info.h * 2. - (y1 * 4.f - 2.f)) / float(info.h * 2.f); // 2 pixels guard
      if (x > 1)
        pool.impostor.shadowImpostorWd *= (info.w * 2.f - (x * 4.f - 2.f)) / float(info.w * 2.f); // 2 pixels guard
      targetTex->unlockimg();

      pool.shadowImpostorUpdatePhase++;
      return RET_NEXT_PHASE;
    }
    else
    {
      return force_update ? RET_ERR : RET_NEXT_PHASE;
    }
  }

  // fast GPU compression
  if (pool.shadowImpostorUpdatePhase == PHASE_COMPRESSION && g_compressors[0] && !force_update)
  {
    BcCompressor *compr = g_compressors[g_current_compressor].get();

    compr->update(g_fullres_texid);
    compr->copyTo(pool.rendinstGlobalShadowTex, 0, 0);

    g_current_compressor = (g_current_compressor + 1) % g_compressors.size();
  }

  // slow CPU compression (fallback)
  if (pool.shadowImpostorUpdatePhase == PHASE_COMPRESSION && (!g_compressors[0] || force_update))
  {
    char *data;
    int stride;

    Texture *targetTex = g_fullres_tex.colorTex;
    if (targetTex->lockimg((void **)&data, stride, 0, TEXLOCK_READ) && data)
    {
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(pool.rendinstGlobalShadowTexId, pool.rendinstGlobalShadowTex);
      int texSize = rendinstGlobalShadowTexSize;

      char name[64];
      _snprintf(name, sizeof(name), "ri_glo_sh_%p", &pool);
      name[sizeof(name) - 1] = 0;

#if _TARGET_IOS || _TARGET_TVOS || _TARGET_ANDROID
      pool.rendinstGlobalShadowTex = convert_to_custom_dxt_texture(texSize, texSize, 0, data, stride, numMips, MODE_R8, 1, name);
#else
      pool.rendinstGlobalShadowTex =
        convert_to_bc4_texture(texSize, texSize, TEXCF_UPDATE_DESTINATION, data, stride, numMips, 1, name);
#endif

      if (pool.rendinstGlobalShadowTex)
      {
        pool.rendinstGlobalShadowTex->texaddr(TEXADDR_BORDER);
        pool.rendinstGlobalShadowTex->texbordercolor(0);
      }
      targetTex->unlockimg();

      pool.rendinstGlobalShadowTexId = ::register_managed_tex(name, pool.rendinstGlobalShadowTex);
    }
  }

  return finalizeCurrentPoolRendering();
}

void rendinst::startUpdateRIGenGlobalShadows()
{
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskDS)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->rtData->nextPoolForShadowImpostors = 0;
    for (int i = 0; i < rgl->rtData->rtPoolData.size(); ++i)
      if (rgl->rtData->rtPoolData[i])
        rgl->rtData->rtPoolData[i]->shadowImpostorUpdatePhase = PHASE_NEW;
  }
}

bool rendinst::renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update, bool use_compression)
{
  TIME_D3D_PROFILE(ri_global_shadows);

  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskDS)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);

    if (force_update)
    {
      rgl->rtData->nextPoolForShadowImpostors = 0;

      while (rgl->rtData->nextPoolForShadowImpostors < rgl->rtData->rtPoolData.size())
      {
        int result = rgl->rtData->renderRendinstGlobalShadowsToTextures(sunDir0, true, use_compression);
        if (result == RET_ERR)
          return false;
      }

      rgl->rtData->nextPoolForShadowImpostors = 0;
    }
    else
    {
      if (rgl->rtData->renderRendinstGlobalShadowsToTextures(sunDir0, false, use_compression) != RET_ALL_DONE)
        return false;
    }
  }
  return true;
}
