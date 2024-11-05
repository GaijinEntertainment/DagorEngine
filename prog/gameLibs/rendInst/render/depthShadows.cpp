// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "riGen/riGenRenderer.h"
#include "riGen/riRotationPalette.h"
#include "render/genRender.h"

#include <stdio.h>
#include <shaders/dag_shaderBlock.h>
#include <fx/dag_leavesWind.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_adjpow2.h>
#include <render/dxtcompress.h>
#include <render/bcCompressor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_lockTexture.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/array.h>
#include <EASTL/fixed_string.h>
#include <dag/dag_vectorSet.h>

#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

using TmpString = eastl::fixed_string<char, 64, false>;

static int numMips = -1;

struct ColorDepthTexturePair
{
  UniqueTex colorTex;
  UniqueTex depthTex;

  void createTextures(const char *name, int index, uint32_t size, int mips)
  {
    colorTex.close();
    TmpString texName{TmpString::CtorSprintf{}, "%sColor%d", name, index};
    int flags = TEXCF_RTARGET;
    if (mips > 1)
      flags |= TEXCF_GENERATEMIPS;
    colorTex = dag::create_tex(nullptr, size, size, flags, mips, texName.c_str());
    createDepth(name, index, size);
  }

  void createDepth(const char *name, int index, uint32_t size)
  {
    depthTex.close();
    TmpString texName{TmpString::CtorSprintf{}, "%sDepth%d", name, index};
    depthTex = dag::create_tex(nullptr, size, size, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, texName.c_str());
  }

  void reset()
  {
    colorTex.close();
    depthTex.close();
  }
};

static dag::Vector<ColorDepthTexturePair> g_lowres_tex;
static dag::Vector<ColorDepthTexturePair> g_fullres_tex;
static shaders::OverrideStateId g_zfunc_state_id;
static unsigned int rendinstGlobalShadowTexSize = 0;
static int shadowBatch = 4;

static eastl::optional<BcCompressor> g_compressor;

namespace rendinst
{
void closeImpostorShadowTempTex()
{
  g_lowres_tex.clear();
  g_fullres_tex.clear();
  g_compressor.reset();

  g_zfunc_state_id.reset();
}
} // namespace rendinst

static bool areImpostorsReady(bool force_update, const TextureIdSet &tex_ids)
{
  if (force_update && is_managed_textures_streaming_load_on_demand())
  {
    prefetch_and_wait_managed_textures_loaded(tex_ids);
    return true;
  }
  else if (!force_update && !prefetch_and_check_managed_textures_loaded(tex_ids))
    return false;

  return true;
}

bool RendInstGenData::RtData::haveNextPoolForShadowImpostors()
{
  for (; nextPoolForShadowImpostors < rtPoolData.size(); ++nextPoolForShadowImpostors)
  {
    if (!rtPoolData[nextPoolForShadowImpostors] || !rtPoolData[nextPoolForShadowImpostors]->hasImpostor())
      continue; // Skip pools without impostors

    return true;
  }

  return false;
}

bool RendInstGenData::RtData::shouldRenderGlobalShadows() { return !globalShadowTask.empty() || haveNextPoolForShadowImpostors(); }

static int compare(const RendInstGenData::RtData::GlobalShadowTask &lhs, const RendInstGenData::RtData::GlobalShadowTask &rhs)
{
  if (lhs.batchIndex != rhs.batchIndex)
    return lhs.batchIndex - rhs.batchIndex;
  if (lhs.phase != rhs.phase)
    return int(lhs.phase) - int(rhs.phase);
  if (lhs.poolNo != rhs.poolNo)
    return lhs.poolNo - rhs.poolNo;
  if (lhs.rotationId != rhs.rotationId)
    return lhs.rotationId - rhs.rotationId;

  return 0;
}

static bool operator<(const RendInstGenData::RtData::GlobalShadowTask &lhs, const RendInstGenData::RtData::GlobalShadowTask &rhs)
{
  return compare(lhs, rhs) < 0;
}

static void init_impostor_shadow_temp_tex(bool use_compression)
{
  if (numMips < 0)
  {
    const DataBlock *rendinstShadowsBlock = ::dgs_get_game_params()->getBlockByNameEx("rendinstShadows");
    rendinstGlobalShadowTexSize = rendinstShadowsBlock->getInt("globalTexSize", 256);
    numMips = max((int)1, (int)get_log2i_of_pow2(rendinstGlobalShadowTexSize) - 2); // last mip is set
    shadowBatch = rendinstShadowsBlock->getInt("shadowBatch", shadowBatch);
  }

  if (g_lowres_tex.empty())
  {
    rendinstGlobalShadowTexSize = get_closest_pow2(rendinstGlobalShadowTexSize);

    g_compressor.reset();
    if (use_compression && BcCompressor::isAvailable(BcCompressor::COMPRESSION_BC4))
    {
      g_compressor.emplace(BcCompressor::COMPRESSION_BC4, numMips, rendinstGlobalShadowTexSize, rendinstGlobalShadowTexSize, 1,
        "bc4_compressor");

      if (!g_compressor->isValid())
        g_compressor.reset();
    }

    g_lowres_tex.resize(shadowBatch);
    g_fullres_tex.resize(shadowBatch);
    for (int i = 0; i < shadowBatch; ++i)
    {
      g_lowres_tex[i].createTextures("rtTempLowres", i, max(rendinstGlobalShadowTexSize / 4, (unsigned)64), 1);
      if (use_compression)
        g_fullres_tex[i].createTextures("rtTempFullres", i, rendinstGlobalShadowTexSize, g_compressor ? numMips : 1);
      else
        g_fullres_tex[i].createDepth("rtTempFullres", i, rendinstGlobalShadowTexSize);
    }
  }

  if (!g_zfunc_state_id)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_FUNC);
    state.zFunc = CMPF_LESSEQUAL;
    g_zfunc_state_id = shaders::overrides::create(state);
  }
}

static RendInstGenData::RtData::GlobalShadowRet inc_rotation(RendInstGenData::RtData::GlobalShadowTask &task,
  const rendinst::gen::RotationPaletteManager::Palette &rotation_palette)
{
  task.rotationId++;

  if (task.rotationId >= rotation_palette.count)
  {
    task.phase = RendInstGenData::RtData::GlobalShadowPhase::READY;
    return RendInstGenData::RtData::GlobalShadowRet::DONE;
  }

  task.phase = RendInstGenData::RtData::GlobalShadowPhase::LOW_PASS;
  return RendInstGenData::RtData::GlobalShadowRet::WAIT_NEXT_FRAME;
}

static void prepare_pool_for_render_global_shadows(rendinst::render::RtPoolData &pool,
  const rendinst::gen::RotationPaletteManager::Palette &rotation_palette, bool use_compression)
{
  if (pool.rendinstGlobalShadowTex)
    return;

  TmpString name{TmpString::CtorSprintf{}, "ri_glo_sh_%p", &pool};

  int flags = 0;
  int mips = numMips;
  if (!use_compression)
  { // Render direct into texture
    flags = TEXFMT_R8 | TEXCF_RTARGET;
    mips = 1;
  }
  else if (g_compressor)
    flags = TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION;
  else
  {
#if _TARGET_IOS || _TARGET_TVOS || _TARGET_ANDROID
    // Mip levels on mobile are calculated on cpu
    flags = TEXFMT_R8 | TEXCF_UPDATE_DESTINATION;
#else
    // Upload bc4 from cpu
    flags = TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION;
#endif
  }

  pool.rendinstGlobalShadowTex =
    dag::create_array_tex(rendinstGlobalShadowTexSize, rendinstGlobalShadowTexSize, rotation_palette.count, flags, mips, name.c_str());

  if (pool.rendinstGlobalShadowTex)
  {
    pool.rendinstGlobalShadowTex->texaddr(TEXADDR_BORDER);
    pool.rendinstGlobalShadowTex->texbordercolor(0);
  }
}

RendInstGenData::RtData::GlobalShadowRet RendInstGenData::RtData::renderGlobalShadow(GlobalShadowTask &task, const Point3 &sun_dir_0,
  bool force_update, bool use_compression)
{
  G_ASSERT_RETURN(task.phase != GlobalShadowPhase::READY, GlobalShadowRet::DONE);
  rendinst::render::RtPoolData &pool = *rtPoolData[task.poolNo];
  auto rotationPalette = rendinst::gen::get_rotation_palette_manager()->getPalette({layerIdx, task.poolNo});

  prepare_pool_for_render_global_shadows(pool, rotationPalette, use_compression);

  if (task.phase == GlobalShadowPhase::HIGH_PASS)
  {
    int lockFlags = force_update ? TEXLOCK_READ : (TEXLOCK_READ | TEXLOCK_NOSYSLOCK);
    if (auto lockedTex = lock_texture_ro(g_lowres_tex[task.batchIndex].colorTex.getTex2D(), 0, lockFlags))
    {
      const uint8_t *data = lockedTex.get();
      const int stride = lockedTex.getByteStride();
      TextureInfo info;
      g_lowres_tex[task.batchIndex].colorTex->getinfo(info, 0);
      int y1;
      const uint8_t *ptrTop = data, *ptrBottom = data + (info.h - 1) * stride;
      for (y1 = 0; y1 < info.h / 2; ++y1, ptrTop += stride, ptrBottom -= stride)
      {
        int x;
        for (x = 0; x < info.w; ++x)
          if (((reinterpret_cast<const unsigned *>(ptrTop)[x]) & 0x0000FF00) != 0 ||
              (reinterpret_cast<const unsigned *>(ptrBottom)[x] & 0x0000FF00) != 0)
            break;
        if (x != info.w)
          break;
      }
      int x;
      for (x = 0; x < info.w / 2; ++x)
      {
        const uint8_t *ptrLeft = data + x * 4, *ptrRight = data + (info.w - 1 - x) * 4;
        int y2;
        for (y2 = 0; y2 < info.h; ++y2, ptrRight += stride, ptrLeft += stride)
          if (((*reinterpret_cast<const unsigned *>(ptrLeft)) & 0x0000FF00) != 0 ||
              ((*reinterpret_cast<const unsigned *>(ptrRight)) & 0x0000FF00) != 0)
            break;
        if (y2 != info.h)
          break;
      }

      if (y1 > 1)
        pool.impostor.shadowImpostorSizes[task.rotationId].y *= (info.h * 2. - (y1 * 4.f - 2.f)) / float(info.h * 2.f); // 2 pixels
                                                                                                                        // guard
      if (x > 1)
        pool.impostor.shadowImpostorSizes[task.rotationId].x *= (info.w * 2.f - (x * 4.f - 2.f)) / float(info.w * 2.f); // 2 pixels
                                                                                                                        // guard
    }
    else
      return force_update ? GlobalShadowRet::ERR : GlobalShadowRet::WAIT_NEXT_FRAME;
  }

  Texture *colorTex = nullptr;
  Texture *depthTex = nullptr;
  if (task.phase == GlobalShadowPhase::HIGH_PASS)
  {
    depthTex = g_fullres_tex[task.batchIndex].depthTex.getTex2D();
    if (use_compression)
      colorTex = g_fullres_tex[task.batchIndex].colorTex.getTex2D();
    else
      colorTex = pool.rendinstGlobalShadowTex.getArrayTex();
  }
  else
  {
    colorTex = g_lowres_tex[task.batchIndex].colorTex.getTex2D();
    depthTex = g_lowres_tex[task.batchIndex].depthTex.getTex2D();
  }
  G_ASSERT_RETURN(colorTex, GlobalShadowRet::ERR);
  G_ASSERT_RETURN(depthTex, GlobalShadowRet::ERR);

  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    shaders::overrides::set(g_zfunc_state_id);

    mat44f ident;
    v_mat44_ident(ident);
    d3d::settm(TM_WORLD, ident);

    rendinst::render::RiShaderConstBuffers cb;
    cb.setBBoxZero();
    cb.setBoundingSphere(0, 0, 1, 1, 0);

    ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::ImpostorShadow));
    Point3 sunDir = sun_dir_0;
    {
      auto rot = rotationPalette.rotations[task.rotationId];

      float cosReverseRot = rot.cosY;
      float sinReverseRot = -rot.sinY; // angle applied to sun dir has to be reversed, so sin is inverted

      Point3 rotatedSunDir;
      rotatedSunDir.x = sunDir.x * cosReverseRot - sunDir.z * sinReverseRot;
      rotatedSunDir.y = sunDir.y;
      rotatedSunDir.z = sunDir.z * cosReverseRot + sunDir.x * sinReverseRot;
      sunDir = rotatedSunDir;
    }
    vec4f sunDirV = v_norm3(v_make_vec4f(sunDir.x, sunDir.y, sunDir.z, 0));
    vec4f ground = v_make_vec4f(MIN_REAL, 0, MIN_REAL, 0);

    rendinst::render::startRenderInstancing();
    d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

    G_ASSERT(riRes[task.poolNo]);
    RenderableInstanceResource *sourceScene = riResLodScene(task.poolNo, 0);

    // Set new matrices.
    mat44f view;
    mat44f viewitm;

    // vec4f eye = v_mul(sunDirV, v_splats(pool.sphereRadius));
    viewitm.col2 = sunDirV;
    viewitm.col0 = v_cross3(V_C_UNIT_0100, viewitm.col2);
    vec3f col0_len = v_length3(viewitm.col0);
    if (v_extract_x(col0_len) > 2e-6f)
      viewitm.col0 = v_div(viewitm.col0, col0_len);
    else
      viewitm.col0 = v_norm3(v_cross3(V_C_UNIT_0010, viewitm.col2));
    viewitm.col1 = v_cross3(viewitm.col2, viewitm.col0);

    if (task.phase == GlobalShadowPhase::LOW_PASS)
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

    if (task.phase == GlobalShadowPhase::LOW_PASS)
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

      pool.impostor.shadowImpostorSizes[task.rotationId] = Point2(v_extract_x(impostorSize), //+pool.impostor.maxFacingLeavesDelta;
        v_extract_y(impostorSize)                                                            //+pool.impostor.maxFacingLeavesDelta;
      );
    }

    DECL_ALIGN16(Point3_vec4, col[3]);
    v_st(&col[0].x, viewitm.col0);
    v_st(&col[1].x, viewitm.col1);
    v_st(&col[2].x, viewitm.col2);
    LeavesWindEffect::setNoAnimShaderVars(col[0], col[1], col[2]);


    d3d::settm(TM_VIEW, view);
    rendinst::render::setCoordType(riPosInst[task.poolNo] ? rendinst::render::COORD_TYPE_POS : rendinst::render::COORD_TYPE_TM);

    d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinst::render::oneInstanceTmVb);

    if (pool.hasImpostor())
      cb.setInstancing(3, 1, 0,
        rendinst::gen::get_rotation_palette_manager()->getImpostorDataBufferOffset({layerIdx, task.poolNo},
          pool.impostorDataOffsetCache));
    else
      cb.setInstancing(0, 3, 0, 0);
    cb.flushPerDraw();

    ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();

    if (task.phase == GlobalShadowPhase::HIGH_PASS && !use_compression)
    {
      d3d::set_depth(depthTex, DepthAccess::RW);
      d3d::set_render_target(0, colorTex, task.rotationId, 0);
    }
    else
      d3d::set_render_target(RenderTarget{depthTex, 0, 0}, DepthAccess::RW, {{colorTex, 0, 0}});

    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0xFF000000, 1.f, 0);

    mat44f orthoProj;
    Point2 shadowSize = pool.impostor.shadowImpostorSizes[task.rotationId];
    v_mat44_make_ortho(orthoProj, 2.f * shadowSize.x, 2.f * shadowSize.y, 0, 2.f * pool.sphereRadius);

    d3d::settm(TM_PROJ, orthoProj);
    ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    bool needToSetBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE) == -1;
    if (needToSetBlock)
      ShaderGlobal::setBlock(rendinst::render::rendinstSceneBlockId, ShaderGlobal::LAYER_SCENE);

    RenderStateContext context;
    for (auto &elem : mesh->getAllElems())
    {
      if (!elem.e)
        continue;
      SWITCH_STATES()
      d3d_err(elem.drawIndTriList());
    }

    shaders::overrides::reset();

    rendinst::render::endRenderInstancing();
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    if (needToSetBlock)
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

    if (task.phase == GlobalShadowPhase::LOW_PASS)
    {
      // Request driver for reading texture without lock right now
      // We will read this texture later in one of next frames
      int stride;
      if (!force_update && colorTex->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
        colorTex->unlockimg();

      task.phase = GlobalShadowPhase::HIGH_PASS;
      return GlobalShadowRet::WAIT_NEXT_FRAME;
    }

    if (task.phase == GlobalShadowPhase::HIGH_PASS && !use_compression)
    {
      d3d::resource_barrier({pool.rendinstGlobalShadowTex.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
      return inc_rotation(task, rotationPalette);
    }
  }

  if (g_compressor)
  {
    // Fast GPU compression
    g_fullres_tex[task.batchIndex].colorTex->generateMips();
    for (int mip = 0; mip < numMips; ++mip)
    {
      g_compressor->updateFromMip(g_fullres_tex[task.batchIndex].colorTex.getTexId(), mip, mip);
      g_compressor->copyToMip(pool.rendinstGlobalShadowTex.getArrayTex(), mip + task.rotationId * numMips, 0, 0, mip);
    }
  }
  else
  {
    // Slow CPU compression (fallback)
    if (auto lockedTex = lock_texture_ro(colorTex, 0, TEXLOCK_READ))
    {
      const char *data = reinterpret_cast<const char *>(lockedTex.get());
      const int stride = lockedTex.getByteStride();

      if (pool.rendinstGlobalShadowTex)
      {
#if _TARGET_IOS || _TARGET_TVOS || _TARGET_ANDROID
        const int mode = MODE_R8;
#else
        const int mode = MODE_BC4;
#endif
        Texture *tex = convert_to_custom_dxt_texture(rendinstGlobalShadowTexSize, rendinstGlobalShadowTexSize,
          TEXCF_UPDATE_DESTINATION, data, stride, numMips, mode, 1, "converttmp");
        int destBaseMip = task.rotationId * numMips;
        for (int i = 0; i < numMips; i++)
          pool.rendinstGlobalShadowTex->updateSubRegion(tex, i, 0, 0, 0, rendinstGlobalShadowTexSize >> i,
            rendinstGlobalShadowTexSize >> i, 1, destBaseMip + i, 0, 0, 0);
        tex->destroy();
      }
    }
  }

  return inc_rotation(task, rotationPalette);
}

void rendinst::startUpdateRIGenGlobalShadows()
{
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskDS)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->rtData->nextPoolForShadowImpostors = 0;
    rgl->rtData->globalShadowTask.clear();
  }
}

bool rendinst::render::are_impostors_ready_for_depth_shadows()
{
  if (!is_managed_textures_streaming_load_on_demand())
    return true;

  dag::RelocatableFixedVector<TextureIdSet *, 8, true, framemem_allocator> riImpCollections;
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskDS)
    if (!rgl->rtData->riImpTexIds.empty())
      riImpCollections.push_back(&rgl->rtData->riImpTexIds);

  if (riImpCollections.empty())
    return true;

  if (riImpCollections.size() == 1)
    return prefetch_and_check_managed_textures_loaded(*riImpCollections[0]);

  dag::VectorSet<TEXTUREID, eastl::less<TEXTUREID>, framemem_allocator> riImpTexIds;
  for (TextureIdSet *col : riImpCollections)
    riImpTexIds.insert(col->begin(), col->end());

  return prefetch_and_check_managed_textures_loaded(riImpTexIds);
}

static int free_index(const dag::VectorSet<RendInstGenData::RtData::GlobalShadowTask> &tasks)
{
  // Tasks list are sorted by batchIndex
  // Different tasks are rendered by different count of frames - depends on count of rotationId in rotationPalette
  // As result some of batchIndeces can be missed in tasks list
  // This function search for first index i missed in tasks list
  for (int i = 0; i < tasks.size(); ++i)
    if (tasks[i].batchIndex > i)
      return i;

  if (tasks.size() < shadowBatch)
    return tasks.size();

  return -1;
}

static void remove_ready(dag::VectorSet<RendInstGenData::RtData::GlobalShadowTask> &task_list)
{
  auto *newEnd = eastl::remove_if(task_list.begin(), task_list.end(),
    [](auto &task) { return task.phase == RendInstGenData::RtData::GlobalShadowPhase::READY; });
  task_list.erase(newEnd, task_list.end());
}

RendInstGenData::RtData::GlobalShadowRet RendInstGenData::RtData::renderRendinstGlobalShadowsToTextures(const Point3 &sun_dir_0,
  bool force_update, bool use_compression)
{
  GlobalShadowRet res = GlobalShadowRet::DONE;
  // Process high pass render
  for (auto &task : globalShadowTask)
  {
    G_ASSERT(task.phase == GlobalShadowPhase::HIGH_PASS);
    res = renderGlobalShadow(task, sun_dir_0, force_update, use_compression);
    if (res == GlobalShadowRet::ERR)
      break;
  }
  remove_ready(globalShadowTask);
  if (res == GlobalShadowRet::ERR)
    return res;

  // Init task
  while (haveNextPoolForShadowImpostors())
  {
    int index = free_index(globalShadowTask);
    if (index < 0)
      break;

    globalShadowTask.insert(GlobalShadowTask{index, GlobalShadowPhase::LOW_PASS, nextPoolForShadowImpostors, 0});
    nextPoolForShadowImpostors += 1;
  }

  if (globalShadowTask.empty())
    return GlobalShadowRet::DONE;

  // Process low pass render
  for (auto &task : globalShadowTask)
  {
    if (task.phase != GlobalShadowPhase::LOW_PASS)
      continue;
    res = renderGlobalShadow(task, sun_dir_0, force_update, use_compression);
    if (res == GlobalShadowRet::ERR)
      break;
  }
  remove_ready(globalShadowTask);
  if (res == GlobalShadowRet::ERR)
    return res;

  if (!globalShadowTask.empty())
    return GlobalShadowRet::WAIT_NEXT_FRAME;

  return haveNextPoolForShadowImpostors() ? GlobalShadowRet::WAIT_NEXT_FRAME : GlobalShadowRet::DONE;
}

bool rendinst::render::renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update, bool use_compression,
  bool free_temp_resources)
{
  TIME_D3D_PROFILE(ri_global_shadows);

  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskDS)
  {
    TextureIdSet texIds = rgl->rtData->riImpTexIds; // riImpTexIds are not mutable, but for a multiframe long wait it is better to pass
                                                    // a copy anyway.
    if (!areImpostorsReady(force_update, texIds))   // prefetch_and_wait_managed_textures_loaded must not be called under GPU lock
                                                  // because it depends on frames advancing. And therefore calling it under the riRwCs
                                                  // lock is a bad idea because it is easily interlocked with the GPU lock.
      return false;

    {
      ScopedLockRead lock(rgl->rtData->riRwCs);

      if (force_update)
      {
        rgl->rtData->nextPoolForShadowImpostors = 0;
        rgl->rtData->globalShadowTask.clear();
      }

      if (rgl->rtData->shouldRenderGlobalShadows())
      {
        d3d::GpuAutoLock gpu_lock;

        init_impostor_shadow_temp_tex(use_compression);

        if (force_update)
        {
          while (rgl->rtData->shouldRenderGlobalShadows())
          {
            RendInstGenData::RtData::GlobalShadowRet result =
              rgl->rtData->renderRendinstGlobalShadowsToTextures(sunDir0, true, use_compression);
            if (result == RendInstGenData::RtData::GlobalShadowRet::ERR)
              return false;
          }

          rgl->rtData->nextPoolForShadowImpostors = 0;
          G_ASSERT(rgl->rtData->globalShadowTask.empty());
        }
        else if (rgl->rtData->renderRendinstGlobalShadowsToTextures(sunDir0, false, use_compression) !=
                 RendInstGenData::RtData::GlobalShadowRet::DONE)
          return false;
      }
    }

    RiGenRenderer::updatePerDrawData(*rgl->rtData, rgl->perInstDataDwords);
  }
  if (free_temp_resources)
    rendinst::closeImpostorShadowTempTex();
  return true;
}
