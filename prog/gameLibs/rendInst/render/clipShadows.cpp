// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "riGen/riRotationPalette.h"
#include "render/genRender.h"

#include <stdio.h>
#include <shaders/dag_shaderBlock.h>
#include <fx/dag_leavesWind.h>
#include <generic/dag_carray.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bounds2.h>
#include <render/bcCompressor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_lock.h>
#include <EASTL/optional.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_resetDevice.h>


#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

static constexpr float CLIPMAP_SHADOW_SPHERE_SCALE = 1.5f;

static int removeRotationVarId = -1;
static float rendinstShadowScale = 1;

static Ptr<ShaderMaterial> rendinstShadowsToClipmapShaderMaterial;
static Ptr<ShaderElement> rendinstShadowsToClipmapShaderElem;
static Vbuffer *rendinstShadowsToClipmapVb = nullptr;
static Ibuffer *rendinstShadowsToClipmapIb = nullptr;

namespace rendinst::render
{
int clipmapShadowScaleVarId = -1;
static Point2 offsetSunDistMin(0, 0), offsetSunDistMax(0, 0);
static PostFxRenderer blurRenderer;
static int blurOffset01VarId = -1, blurOffset23VarId = -1, blurOffset45VarId = -1, blurOffset67VarId = -1, sourceTexVarId = -1;
static bool areBuffersFilled = false;

void release_clipmap_shadows();
void allocate_clipmap_shadows();

void closeClipmapShadows()
{
  areBuffersFilled = false;
  rendinstShadowsToClipmapShaderElem = nullptr;
  rendinstShadowsToClipmapShaderMaterial = nullptr;
  del_d3dres(rendinstShadowsToClipmapVb);
  del_d3dres(rendinstShadowsToClipmapIb);
  release_clipmap_shadows();
}

static float blurRadiusTm = 0.5, blurRadiusPos = 0.05;

static const unsigned quadSize = 4 * sizeof(Point3); // replace to short!
static const unsigned count = 1;                     // TODO: do we need to support count > 1?
static const unsigned size = count * quadSize;

static bool is_clipmap_shadows_renderable() { return rendinstShadowsToClipmapShaderMaterial && areBuffersFilled; }

static void fill_buffers()
{
  areBuffersFilled = false;

  void *data;
  if (!rendinstShadowsToClipmapVb->lock(0, size, &data, VBLOCK_WRITEONLY))
  {
    if (!d3d::is_in_device_reset_now())
      logerr("can't lock shadows to clipmap vertex buffer");
    return;
  }
  // error handling is not implemented, but this is ancient code anyways
  G_ASSERT(data != nullptr);
  Point3 *vertices = (Point3 *)data;
  for (int i = 0; i < count; ++i, vertices += 4) // -V1008
  {
    vertices[0] = Point3(1.f, 1.f, i);
    vertices[1] = Point3(-1.f, 1.f, i);
    vertices[2] = Point3(-1.f, -1.f, i);
    vertices[3] = Point3(1.f, -1.f, i);
  }

  rendinstShadowsToClipmapVb->unlock();

  uint16_t *indices;
  if (!rendinstShadowsToClipmapIb->lock(0, 6 * sizeof(uint16_t) * count, &indices, VBLOCK_WRITEONLY))
  {
    if (!d3d::is_in_device_reset_now())
      logerr("can't lock shadows to clipmap index buffer");
    return;
  }


  for (int i = 0; i < count; ++i, indices += 6) // -V1008
  {
    indices[0] = 0 + i * 4;
    indices[1] = 1 + i * 4;
    indices[2] = 2 + i * 4;
    indices[3] = 0 + i * 4;
    indices[4] = 2 + i * 4;
    indices[5] = 3 + i * 4;
  }

  rendinstShadowsToClipmapIb->unlock();

  areBuffersFilled = true;
}

static void clip_shadows_after_reset_device(bool)
{
  if (rendinstShadowsToClipmapVb)
    fill_buffers();
}

REGISTER_D3D_AFTER_RESET_FUNC(clip_shadows_after_reset_device);

void initClipmapShadows()
{
  rendinstShadowsToClipmapShaderMaterial = new_shader_material_by_name_optional("rendinst_shadows_to_clipmap");
  if (!rendinstShadowsToClipmapShaderMaterial)
  {
    logwarn("no clipmap shadows");
    return;
  }

  areBuffersFilled = false;

  removeRotationVarId = ::get_shader_glob_var_id("remove_rotation");
  clipmapShadowScaleVarId = ::get_shader_glob_var_id("clipmap_shadow_scale", true);
  G_ASSERT(rendinstShadowsToClipmapShaderMaterial);
  rendinstShadowsToClipmapShaderElem = rendinstShadowsToClipmapShaderMaterial->make_elem();
  debug("rendinst clip shadows tex instancing count is %d", count);

  rendinstShadowsToClipmapVb = d3d::create_vb(size, 0, "rendinstShadowsToClipmapVb");
  G_ASSERT(rendinstShadowsToClipmapVb != nullptr);

  rendinstShadowsToClipmapIb = d3d::create_ib(6 * sizeof(uint16_t) * count, 0, "rendinstShadowsToClipmapIb");
  fill_buffers();

  blurOffset01VarId = get_shader_variable_id("blur_offset_0_1");
  blurOffset23VarId = get_shader_variable_id("blur_offset_2_3");
  blurOffset45VarId = get_shader_variable_id("blur_offset_4_5");
  blurOffset67VarId = get_shader_variable_id("blur_offset_6_7");
  sourceTexVarId = ::get_shader_glob_var_id("source_tex");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(::get_shader_glob_var_id("source_tex_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  allocate_clipmap_shadows();
}

static UniqueTex blurTemp, rtTemp;
static carray<BcCompressor *, 3> g_compressors; // 3 frame carousell
static int g_current_compressor = 0;
static int numMips = 1, rendinstClipmapShadowTexSize = 64;
static Point3 sunForShadow(0, -1, 0);
static float maxHeight = 0;
static unsigned shadow_rt_texFmt = 0, shadow_texFmt = 0;

void allocate_clipmap_shadows()
{
  rendinstClipmapShadowTexSize = ::dgs_get_game_params()->getBlockByNameEx("rendinstShadows")->getInt("clipmapTexSize", 64);

  rendinstClipmapShadowTexSize = get_closest_pow2(rendinstClipmapShadowTexSize);
  numMips = get_log2i_of_pow2(rendinstClipmapShadowTexSize) - 2; // last mip is set

  for (int i = 0; i < g_compressors.static_size; ++i)
  {
    BcCompressor *compr = nullptr;

    if (BcCompressor::isAvailable(BcCompressor::COMPRESSION_BC4))
    {
#if !_TARGET_APPLE // does not work by unknown reasons, needed a research
      compr = new BcCompressor(BcCompressor::COMPRESSION_BC4, numMips, rendinstClipmapShadowTexSize, rendinstClipmapShadowTexSize, 1,
        "bc4_compressor");

      if (!compr->isValid())
        del_it(compr);
#endif
    }

    g_compressors[i] = compr;
  }

  if (g_compressors[0])
  {
    shadow_texFmt = TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION;
  }
  else
  {
    int usage = d3d::USAGE_RTARGET | d3d::USAGE_AUTOGENMIPS;
    if ((d3d::get_texformat_usage(TEXFMT_L8, RES3D_TEX) & usage) == usage)
      shadow_texFmt = TEXFMT_L8;
    else
      shadow_texFmt = TEXFMT_DEFAULT;

    shadow_texFmt |= TEXCF_RTARGET | TEXCF_GENERATEMIPS;
  }


  if ((d3d::get_texformat_usage(TEXFMT_L8, RES3D_TEX) & d3d::USAGE_RTARGET))
    shadow_rt_texFmt = TEXFMT_L8;
  else
    shadow_rt_texFmt = TEXFMT_DEFAULT;

  // fixme if supported

  blurTemp.close();
  blurTemp = dag::create_tex(nullptr, rendinstClipmapShadowTexSize, rendinstClipmapShadowTexSize, shadow_rt_texFmt | TEXCF_RTARGET, 1,
    "clipmapshadow_blurTempTex");
  d3d_err(blurTemp.getTex2D());
  blurTemp.getTex2D()->disableSampler();
  rtTemp.close();
  rtTemp = dag::create_tex(nullptr, rendinstClipmapShadowTexSize, rendinstClipmapShadowTexSize, shadow_rt_texFmt | TEXCF_RTARGET, 1,
    "clipmapshadow_rtTempTex");
  d3d_err(rtTemp.getTex2D());
  rtTemp.getTex2D()->disableSampler();

  blurRenderer.init("blur");
}

void set_sun_dir(const Point3 &sunDir0)
{
  sunForShadow = sunDir0;
  rendinstShadowScale = -1.f / sunForShadow.y;
  rendinstShadowScale *= CLIPMAP_SHADOW_SPHERE_SCALE;

  // debug("rendinstShadowScale=%f", rendinstShadowScale);
}

void release_clipmap_shadows()
{
  ShaderGlobal::set_texture_fast(sourceTexVarId, BAD_TEXTUREID);
  blurTemp.close();
  rtTemp.close();
  blurRenderer.clear();

  for (int i = 0; i < g_compressors.static_size; ++i)
    del_it(g_compressors[i]);
}

bool render_clipmap_shadow_pool(rendinst::render::RtPoolData &pool, RenderableInstanceResource *sourceScene, bool for_sli,
  bool keep_texture, uint32_t impostor_buffer_offset)
{
  if (!for_sli)
  {
    if (pool.rendinstClipmapShadowTex && !keep_texture)
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(pool.rendinstClipmapShadowTexId, pool.rendinstClipmapShadowTex);

    if (!pool.rendinstClipmapShadowTex)
    {
      char name[64];
      _snprintf(name, sizeof(name), "ri_cli_sh_%p", &pool);
      name[sizeof(name) - 1] = 0;
      //

      pool.rendinstClipmapShadowTex =
        d3d::create_tex(nullptr, rendinstClipmapShadowTexSize, rendinstClipmapShadowTexSize, shadow_texFmt, numMips, name);

      pool.rendinstClipmapShadowTex->disableSampler();
      pool.rendinstClipmapShadowTexId = register_managed_tex(name, pool.rendinstClipmapShadowTex);
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      ShaderGlobal::set_sampler(::get_shader_glob_var_id("rendinst_shadow_tex_samplerstate", true), d3d::request_sampler(smpInfo));
    }
  }

  Texture *targetTex = rtTemp.getTex2D();
  TEXTUREID targetTexId = rtTemp.getTexId();

  d3d::set_render_target(targetTex, 0);
  d3d::clearview(CLEAR_TARGET, 0, 1.f, 0);

  G_ASSERT(sourceScene);

  maxHeight = max(pool.sphereRadius, maxHeight);
  // Set new matrices.
  float sphereRadius = pool.sphereRadius * rendinstShadowScale;

  Point3 eye = Point3(0.f, 2.0f * sphereRadius, 0.f);

  TMatrix4_vec4 viewMatrix = ::matrix_look_at_lh(eye, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, -1.f));

  TMatrix4_vec4 skew = TMatrix4(1.f, 0.f, 0.f, 0.f, -sunForShadow.x / sunForShadow.y, 1.f, -sunForShadow.z / sunForShadow.y, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

  viewMatrix = skew * viewMatrix;

  d3d::settm(TM_VIEW, &viewMatrix);
  TMatrix itm = orthonormalized_inverse(tmatrix(viewMatrix));
  itm.orthonormalize();

  float r = sphereRadius, b = -sphereRadius;
  float l = -sphereRadius, t = sphereRadius;
  if (pool.impostor.shadowPoints.size())
  {
    vec4f impostorMax = v_make_vec4f(MIN_REAL, MIN_REAL, MIN_REAL, 0);
    vec4f impostorMin = v_make_vec4f(MAX_REAL, MAX_REAL, MAX_REAL, 0);
    vec4f ground = v_make_vec4f(MIN_REAL, 0, MIN_REAL, 0);
    vec4f col0 = v_make_vec4f(itm.getcol(0).x, itm.getcol(0).y, itm.getcol(0).z, 1);
    vec4f col1 = v_make_vec4f(itm.getcol(1).x, itm.getcol(1).y, itm.getcol(1).z, 1);
    mat44f view;
    static_assert(sizeof(mat44f) == sizeof(TMatrix4_vec4));
    memcpy(&view, &viewMatrix, sizeof(view));

    for (unsigned int pointNo = 0; pointNo < pool.impostor.shadowPoints.size(); pointNo += 2)
    {
      vec4f localPoint = pool.impostor.shadowPoints[pointNo];
      vec4f delta = pool.impostor.shadowPoints[pointNo + 1];
      localPoint = v_add(localPoint, v_mul(v_splat_x(delta), col0));
      localPoint = v_add(localPoint, v_mul(v_splat_y(delta), col1));
      localPoint = v_max(localPoint, ground);
      vec4f rotPoint = v_mat44_mul_vec3p(view, localPoint);
      // impostorSize = v_max(impostorSize, v_abs(rotPoint));
      impostorMin = v_min(impostorMin, rotPoint);
      impostorMax = v_max(impostorMax, rotPoint);
    }
    l = v_extract_x(impostorMin);
    b = v_extract_y(impostorMin);
    r = v_extract_x(impostorMax);
    t = v_extract_y(impostorMax);
  }
  pool.clipShadowWk = (r - l) / 2.0f;
  pool.clipShadowHk = (t - b) / 2.0f;
  pool.clipShadowOrigX = -(r + l) / 2.0;
  pool.clipShadowOrigY = -(t + b) / 2.0;

  pool.clipShadowWk = clamp(pool.clipShadowWk, 0.f, sphereRadius); // Limit to shadow sphere that is scaled with light direction.
  pool.clipShadowHk = clamp(pool.clipShadowHk, 0.f, sphereRadius);
  pool.clipShadowOrigX = clamp(pool.clipShadowOrigX, -sphereRadius, sphereRadius);
  pool.clipShadowOrigY = clamp(pool.clipShadowOrigY, -sphereRadius, sphereRadius);

  LeavesWindEffect::setNoAnimShaderVars(itm.getcol(0), itm.getcol(1), itm.getcol(2));

  ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId,
    eastl::to_underlying(rendinst::RenderPass::ImpostorShadow)); // rendinst_render_pass_impostor_shadow.
  rendinst::render::setCoordType(pool.hasImpostor() ? rendinst::render::COORD_TYPE_POS : rendinst::render::COORD_TYPE_TM);

  TMatrix4 proj = matrix_ortho_off_center_lh(l, r, b, t, 0, 2.f * sphereRadius);
  proj._33 = 0;
  proj._43 = 0;

  d3d::settm(TM_PROJ, &proj);

  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(rendinst::render::rendinstSceneBlockId, ShaderGlobal::LAYER_SCENE);

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinst::render::oneInstanceTmVb);
  rendinst::render::RiShaderConstBuffers cb;
  cb.setBBoxZero();
  cb.setOpacity(0, 1);
  cb.setCrossDissolveRange(0);
  cb.setBoundingSphere(0, 0, 1, 1, 0);
  cb.setInstancing(pool.hasImpostor() ? 3 : 0, pool.hasImpostor() ? 1 : 3, 0, impostor_buffer_offset);
  cb.flushPerDraw();
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

  ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();
  RenderStateContext context;
  for (auto &elem : mesh->getAllElems())
  {
    if (!elem.e)
      continue;

    SWITCH_STATES()

    d3d_err(elem.drawIndTriList());
  }

  rendinst::render::endRenderInstancing();
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);


  // String fname1(128, "clipshadowimpostor%02d.tga", poolNo);
  // save_rt_image_as_tga(targetTex, fname1);


  // Blur shadow.

  float blurRadius = pool.hasImpostor() ? blurRadiusPos : blurRadiusTm;

  unsigned int texSize = rendinstClipmapShadowTexSize;
  d3d::set_render_target(blurTemp.getTex2D(), 0);
  ShaderGlobal::set_color4(blurOffset01VarId,
    Color4(-7.f * blurRadius / texSize, 0.5f / texSize, -5.f * blurRadius / texSize, 0.5f / texSize));
  ShaderGlobal::set_color4(blurOffset23VarId,
    Color4(-3.f * blurRadius / texSize, 0.5f / texSize, -1.f * blurRadius / texSize, 0.5f / texSize));
  ShaderGlobal::set_color4(blurOffset45VarId,
    Color4(1.f * blurRadius / texSize, 0.5f / texSize, 3.f * blurRadius / texSize, 0.5f / texSize));
  ShaderGlobal::set_color4(blurOffset67VarId,
    Color4(5.f * blurRadius / texSize, 0.5f / texSize, 7.f * blurRadius / texSize, 0.5f / texSize));
  ShaderGlobal::set_texture_fast(sourceTexVarId, targetTexId);
  blurRenderer.render();

  d3d::set_render_target(g_compressors[0] ? targetTex : pool.rendinstClipmapShadowTex, 0);
  ShaderGlobal::set_color4(blurOffset01VarId,
    Color4(0.5f / texSize, -7.f * blurRadius / texSize, 0.5f / texSize, -5.f * blurRadius / texSize));
  ShaderGlobal::set_color4(blurOffset23VarId,
    Color4(0.5f / texSize, -3.f * blurRadius / texSize, 0.5f / texSize, -1.f * blurRadius / texSize));
  ShaderGlobal::set_color4(blurOffset45VarId,
    Color4(0.5f / texSize, 1.f * blurRadius / texSize, 0.5f / texSize, 3.f * blurRadius / texSize));
  ShaderGlobal::set_color4(blurOffset67VarId,
    Color4(0.5f / texSize, 5.f * blurRadius / texSize, 0.5f / texSize, 7.f * blurRadius / texSize));
  ShaderGlobal::set_texture_fast(sourceTexVarId, blurTemp.getTexId());
  blurRenderer.render();

  if (g_compressors[0])
  {
    BcCompressor *compr = g_compressors[g_current_compressor];

    for (int i = 0; i < numMips; ++i)
    {
      compr->updateFromMip(targetTexId, i, i);
      compr->copyToMip(pool.rendinstClipmapShadowTex, i, 0, 0, i);
    }

    g_current_compressor = (g_current_compressor + 1) % g_compressors.static_size;
  }
  else
  {
    pool.rendinstClipmapShadowTex->generateMips();
  }
  // String fname(128, "clipshadowimpostor%02d_blurred.tga", poolNo);
  // save_rt_image_as_tga(targetTex, fname);
  return true;
}

void start_clipmap_shadows() {}

void end_clipmap_shadows()
{
  Point3 offsetSunDist = -sunForShadow * maxHeight / max(0.01f, fabsf(sunForShadow.y));
  rendinst::render::offsetSunDistMax = Point2(max(offsetSunDist.x, 0.f), max(offsetSunDist.z, 0.f));
  rendinst::render::offsetSunDistMin = Point2(min(offsetSunDist.x, 0.f), min(offsetSunDist.z, 0.f));
}

}; // namespace rendinst::render


bool RendInstGenData::RtData::renderRendinstClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update)
{
  if (force_update && is_managed_textures_streaming_load_on_demand())
    prefetch_and_wait_managed_textures_loaded(riImpTexIds);
  else if (!force_update && !prefetch_and_check_managed_textures_loaded(riImpTexIds))
    return false;

  d3d::GpuAutoLock gpu_lock;
  rendinst::render::set_sun_dir(sunDir0);
  rendinst::render::start_clipmap_shadows();

  // Save RT and matrices.
  mat44f ident;
  v_mat44_ident(ident);
  d3d::settm(TM_WORLD, ident);

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  for (; nextPoolForClipmapShadows < rtPoolData.size(); nextPoolForClipmapShadows++)
  {
    int poolNo = nextPoolForClipmapShadows;
    if (!rtPoolData[poolNo])
      continue;

    uint32_t impostorBufferOffset = rtPoolData[poolNo]->hasImpostor()
                                      ? rendinst::gen::get_rotation_palette_manager()->getImpostorDataBufferOffset({layerIdx, poolNo},
                                          rtPoolData[poolNo]->impostorDataOffsetCache)
                                      : 0;

    rendinst::render::render_clipmap_shadow_pool(*rtPoolData[poolNo], riResLodScene(poolNo, 0), for_sli, !force_update,
      impostorBufferOffset);

    if (!force_update)
    {
      nextPoolForClipmapShadows++;
      break;
    }
  }

  // Restore RT and matrices.

  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  return force_update ? true : nextPoolForClipmapShadows >= rtPoolData.size();
}

bool RendInstGenData::notRenderedClipmapShadowsBBox(BBox2 &box, int cascadeNo)
{
  G_ASSERT(cascadeNo < CellRtData::CLIPMAP_SHADOW_CASCADES);
  ScopedLockRead lock(rtData->riRwCs);

  dag::ConstSpan<int> ld = rtData->loaded.getList();
  bbox3f bbox;
  v_bbox3_init_empty(bbox);
  uint32_t flag = (RendInstGenData::CellRtData::CLIPMAP_SHADOW_RENDERED << cascadeNo);
  bool hasNotRendered = false;
  for (auto ldi : ld)
  {
    const RendInstGenData::CellRtData *crt_ptr = cells[ldi].isReady();
    if (!crt_ptr)
      continue;
    const RendInstGenData::CellRtData &crt = *crt_ptr;
    if (!(crt.cellStateFlags & flag))
    {
      v_bbox3_add_box(bbox, crt.bbox[0]);
      hasNotRendered = true;
    }
  }
  if (!hasNotRendered)
    return false;
  box[0] = Point2(v_extract_x(bbox.bmin), v_extract_z(bbox.bmin));
  box[1] = Point2(v_extract_x(bbox.bmax), v_extract_z(bbox.bmax));
  return true;
}

bool RendInstGenData::notRenderedStaticShadowsBBox(BBox3 &box)
{
  static eastl::optional<BBox3> notRenderedBox;

  ScopedLockRead lock(rtData->riRwCs);

  dag::ConstSpan<int> ld = rtData->loaded.getList();
  bbox3f bbox;
  v_bbox3_init_empty(bbox);

  bool hasNotRendered = false;
  for (auto ldi : ld)
  {
    RendInstGenData::CellRtData *crt_ptr = cells[ldi].isReady();
    if (!crt_ptr)
      continue;
    RendInstGenData::CellRtData &crt = *crt_ptr;
    if (!(crt.cellStateFlags & RendInstGenData::CellRtData::STATIC_SHADOW_RENDERED))
    {
      crt.cellStateFlags |= RendInstGenData::CellRtData::STATIC_SHADOW_RENDERED;
      v_bbox3_add_box(bbox, crt.bbox[0]);
      v_bbox3_add_box(bbox, crt.pregenRiExtraBbox); //==
      hasNotRendered = true;
    }
  }

  v_stu_bbox3(box, bbox);

  // Not actually update the static shadow, only when the camera is
  // settled. While zooming with a rifle scope, the transition spammed
  // the static shadow update, resulting in a serious FPS drop.
  if (hasNotRendered)
  {
    if (notRenderedBox.has_value())
      notRenderedBox.value() += box;
    else
      notRenderedBox = box;
    return false;
  }
  if (notRenderedBox.has_value())
  {
    box = notRenderedBox.value();
    notRenderedBox.reset();
    return true;
  }

  return hasNotRendered;
}

void RendInstGenData::setClipmapShadowsRendered(int cascadeNo)
{
  if (!rendinst::render::is_clipmap_shadows_renderable())
    return;
  G_ASSERT(cascadeNo < CellRtData::CLIPMAP_SHADOW_CASCADES);
  ScopedLockRead lock(rtData->riRwCs);

  dag::ConstSpan<int> ld = rtData->loaded.getList();
  uint32_t flag = (RendInstGenData::CellRtData::CLIPMAP_SHADOW_RENDERED << cascadeNo);
  for (auto ldi : ld)
    if (RendInstGenData::CellRtData *crt_ptr = cells[ldi].isReady())
      crt_ptr->cellStateFlags |= flag;
}

void RendInstGenData::renderRendinstShadowsToClipmap(const BBox2 &region, int newForCascadeNo)
{
  if (!rendinst::render::is_clipmap_shadows_renderable())
    return;

  bbox3f regionBBox;
  regionBBox.bmin =
    v_make_vec4f(region[0].x + rendinst::render::offsetSunDistMin.x, MIN_REAL, region[0].y + rendinst::render::offsetSunDistMin.y, 0);
  regionBBox.bmax =
    v_make_vec4f(region[1].x + rendinst::render::offsetSunDistMax.x, MAX_REAL, region[1].y + rendinst::render::offsetSunDistMax.y, 0);

  //(minX, minZ, maxX, maxZ)
  vec4f regionV = v_perm_xzac(regionBBox.bmin, regionBBox.bmax);
  regionV = v_sub(regionV, world0Vxz);
  regionV = v_max(v_mul(regionV, invGridCellSzV), v_zero());
  regionV = v_min(regionV, lastCellXZXZ);
  vec4i regionI = v_cvt_floori(regionV);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);
  int cellXStride = cellNumW - (regions[2] - regions[0] + 1);
  float grid2worldcellSz = grid2world * cellSz;

  d3d::setind(rendinstShadowsToClipmapIb);
  d3d::setvsrc_ex(0, rendinstShadowsToClipmapVb, 0, sizeof(Point3));
  rendinst::render::startRenderInstancing();
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

  ScopedLockRead lock(rtData->riRwCs);
  uint32_t flag = newForCascadeNo >= 0 ? (RendInstGenData::CellRtData::CLIPMAP_SHADOW_RENDERED << newForCascadeNo) : 0;

  rendinst::render::RiShaderConstBuffers cb;
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rtData->cellsVb.getHeap().getBuf());
  auto currentHeapGen = rtData->cellsVb.getManager().getHeapGeneration();

  for (int z = regions[1], cellI = regions[1] * cellNumW + regions[0]; z <= regions[3]; z++, cellI += cellXStride)
    for (int x = regions[0]; x <= regions[2]; x++, cellI++)
    {
      RendInstGenData::Cell &cell = cells[cellI];
      RendInstGenData::CellRtData *crt_ptr = cell.isReady();
      if (!crt_ptr)
        continue;
      RendInstGenData::CellRtData &crt = *crt_ptr;
      if (crt.cellStateFlags & flag)
        continue;

      if (!v_bbox3_test_box_intersect(crt.bbox[0], regionBBox))
        continue;

      uint8_t ranges[RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV][2];
      int rangesCount = 0;
      if (v_bbox3_test_box_inside(regionBBox, crt.bbox[0]))
      {
        ranges[0][0] = 0;
        ranges[0][1] = RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV - 1;
        rangesCount = 1;
      }
      else
        for (int idx = 1; idx < RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV + 1; ++idx)
        {
          if (v_bbox3_test_box_intersect(crt.bbox[idx], regionBBox))
          {
            if (!rangesCount || ranges[rangesCount - 1][1] != idx - 2)
            {
              ranges[rangesCount][0] = ranges[rangesCount][1] = idx - 1;
              rangesCount++;
            }
            else
              ranges[rangesCount - 1][1] = idx - 1;
          }
        }

      cell_set_encoded_bbox(cb, crt.cellOrigin, grid2worldcellSz, crt.cellHeight);

      for (unsigned int ri_idx = 0; ri_idx < rtData->riRes.size(); ri_idx++)
      {
        if (rendinst::isResHidden(rtData->riResHideMask[ri_idx]))
          continue;
        if (crt.pools[ri_idx].total - crt.pools[ri_idx].avail < 1)
          continue;
        if (!rtData->riRes[ri_idx] || !rtData->rtPoolData[ri_idx] || rendinst::isResHidden(rtData->riResHideMask[ri_idx]))
          continue;
        bool posInst = rtData->riPosInst[ri_idx] ? 1 : 0;
        rendinst::render::RtPoolData &pool = *rtData->rtPoolData[ri_idx];

        if (!pool.rendinstClipmapShadowTex)
          continue;

        rendinst::render::setCoordType(posInst ? rendinst::render::COORD_TYPE_POS : rendinst::render::COORD_TYPE_TM);

        unsigned int stride = RIGEN_STRIDE_B(posInst, rtData->riZeroInstSeeds[ri_idx], perInstDataDwords);
        unsigned int flags = (rtData->riZeroInstSeeds[ri_idx] == 0) && (perInstDataDwords != 0) ? 2 : 0;

        ShaderGlobal::set_texture_fast(rendinst::render::rendinstShadowTexVarId, pool.rendinstClipmapShadowTexId);
        // ShaderGlobal::set_color4_fast(rendinst::render::boundingSphereVarId, 0.f, 0.f, 0.f, pool.sphereRadius *
        // rendinstShadowScale);
        ShaderGlobal::set_color4_fast(rendinst::render::clipmapShadowScaleVarId, pool.clipShadowWk, pool.clipShadowHk,
          pool.clipShadowOrigX, pool.clipShadowOrigY);

        const uint32_t vectorsCnt = posInst ? 1 : 3;
        ShaderGlobal::set_int_fast(removeRotationVarId, posInst ? 0 : 1);
        rendinstShadowsToClipmapShaderElem->setStates(0, true);

        IPoint2 resultRanges[RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV];
        int resultRangesCount = 0;
        for (int rangeI = 0; rangeI < rangesCount; ++rangeI)
        {
          const RendInstGenData::CellRtData::SubCellSlice &scss = crt.getCellSlice(ri_idx, ranges[rangeI][0]);
          const RendInstGenData::CellRtData::SubCellSlice &scse = crt.getCellSlice(ri_idx, ranges[rangeI][1]);
          if (scse.ofs + scse.sz == scss.ofs)
            continue;

          if (!resultRangesCount || resultRanges[resultRangesCount - 1][1] != scss.ofs)
          {
            resultRanges[resultRangesCount][0] = scss.ofs;
            resultRanges[resultRangesCount][1] = scse.ofs + scse.sz;
            resultRangesCount++;
          }
          else
            resultRanges[resultRangesCount - 1][1] = scse.ofs + scse.sz;
        }
        for (int rangeI = 0; rangeI < resultRangesCount; ++rangeI)
        {
          G_ASSERT(crt.cellVbId);
          const uint32_t ofs = resultRanges[rangeI][0];
          if (crt.heapGen != currentHeapGen) // driver is incapable of copy in thread
          {
            updateVb(crt, cellI);
            // updateVb reallocates cellsVb buffer
            d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rtData->cellsVb.getHeap().getBuf());
          }
          auto vbInfo = rtData->cellsVb.get(crt.cellVbId);
          int count = (resultRanges[rangeI][1] - resultRanges[rangeI][0]) / stride;
          uint32_t impostorOffset = pool.hasImpostor() ? rendinst::gen::get_rotation_palette_manager()->getImpostorDataBufferOffset(
                                                           {rtData->layerIdx, int(ri_idx)}, pool.impostorDataOffsetCache)
                                                       : 0;
          cb.setInstancing(vbInfo.offset / RENDER_ELEM_SIZE + (ofs * vectorsCnt) / stride, vectorsCnt, flags, impostorOffset);
          cb.flushPerDraw();
          d3d_err(d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, count));
        }
      }
    }

  rendinst::render::endRenderInstancing();
}

void rendinst::startUpdateRIGenClipmapShadows()
{
  if (!rendinstClipmapShadows || !rendinst::render::is_clipmap_shadows_renderable())
    return;

  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskCMS)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->rtData->nextPoolForClipmapShadows = 0;
  }
}

bool rendinst::render::renderRIGenClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update)
{
  if (!rendinstClipmapShadows || !rendinst::render::is_clipmap_shadows_renderable())
    return true;

  TIME_D3D_PROFILE(ri_clipmap_shadows);

  bool succeeded = true;
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskCMS)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);

    if (force_update)
      rgl->rtData->nextPoolForClipmapShadows = 0;

    succeeded = rgl->rtData->renderRendinstClipmapShadowsToTextures(sunDir0, for_sli, force_update);

    if (force_update)
      rgl->rtData->nextPoolForClipmapShadows = 0;

    if (!succeeded)
      break;
  }
  return succeeded;
}

bool rendinst::render::notRenderedClipmapShadowsBBox(BBox2 &box, int cascadeNo)
{
  if (!rendinstClipmapShadows || !rendinst::render::is_clipmap_shadows_renderable())
    return false;

  bool succeeded = false;
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskCMS)
    succeeded = rgl->notRenderedClipmapShadowsBBox(box, cascadeNo) || succeeded;
  return succeeded;
}

void rendinst::render::setClipmapShadowsRendered(int cascadeNo)
{
  if (!rendinstClipmapShadows || !rendinst::render::is_clipmap_shadows_renderable())
    return;
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskCMS)
    rgl->setClipmapShadowsRendered(cascadeNo);
}

void rendinst::render::renderRIGenShadowsToClipmap(const BBox2 &region, int newForCascadeNo)
{
  if (!rendinstClipmapShadows || !rendinst::render::is_clipmap_shadows_renderable())
    return;
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskCMS)
    rgl->renderRendinstShadowsToClipmap(region, newForCascadeNo);
}

float rendinst::getMaxFarplaneRIGen(bool sec_layer)
{
  float range = 1;
  for (int i = sec_layer ? rgPrimaryLayers : 0, ie = sec_layer ? rgLayer.size() : rgPrimaryLayers; i < ie; i++)
    if (rgAttr[i].clipmapShadows && rgLayer[i] && rgLayer[i]->rtData)
      inplace_max(range, rgLayer[i]->rtData->get_range(rgLayer[i]->rtData->rendinstFarPlane));
  return range;
}

float rendinst::getMaxAverageFarplaneRIGen(bool sec_layer)
{
  float range = 1;
  for (int i = sec_layer ? rgPrimaryLayers : 0, ie = sec_layer ? rgLayer.size() : rgPrimaryLayers; i < ie; i++)
    if (rgAttr[i].clipmapShadows && rgLayer[i] && rgLayer[i]->rtData)
      inplace_max(range, rgLayer[i]->rtData->get_range(rgLayer[i]->rtData->averageFarPlane));
  return range;
}
