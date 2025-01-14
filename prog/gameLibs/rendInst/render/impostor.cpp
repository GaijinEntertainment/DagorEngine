// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/impostor.h"

#include <rendInst/impostorTextureMgr.h>
#include "riGen/riRotationPalette.h"
#include "render/genRender.h"

#include <render/bcCompressor.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_gpuConfig.h>
#include <math/dag_TMatrix4more.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <fx/dag_leavesWind.h>
#include <shaders/dag_shaderBlock.h>


namespace rendinst::render
{
static int sourceTexVarId[rendinst::render::IMP_COUNT] = {-1, -1, -1};
static int texelSizeVarId = -1;
static int texIndVid = -1;
int baked_impostor_multisampleVarId = -1;
int vertical_impostor_slicesVarId = -1;
int dynamicImpostorViewXVarId = -1;
int dynamicImpostorViewYVarId = -1;
bool impostorPreshadowNeedUpdate = false;

static int impostor_tex_count = rendinst::render::IMP_COUNT;
static uint32_t impostor_texformats[rendinst::render::IMP_COUNT] = {TEXCF_SRGBREAD | TEXCF_SRGBWRITE, 0, 0};
static E3DCOLOR impostor_clear_color[rendinst::render::IMP_COUNT] = {0x00000000, 0x00FFFFFF, 0xFFFFFFFF};
UniqueTex impostorColorTexture[rendinst::render::IMP_COUNT];
PostFxRenderer *postfxBuildMip = nullptr;
static shaders::UniqueOverrideStateId impostorShadowOverride;

static uint32_t MIN_DYNAMIC_IMPOSTOR_TEX_SIZE = 32;
static uint32_t MAX_DYNAMIC_IMPOSTOR_TEX_SIZE = 256;

static const auto compIPoint2 = [](const IPoint2 &a, const IPoint2 &b) { return a.y < b.y || a.x < b.x; };
static eastl::vector_map<IPoint2, UniqueTex, decltype(compIPoint2)> impostorDepthTextures(compIPoint2);

static constexpr uint32_t ROTATION_PALETTE_TM_VB_STRIDE = 3; // 3 float4
} // namespace rendinst::render

static int impostorShadowXVarId = -1;
static int impostorShadowYVarId = -1;
static int impostorShadowZVarId = -1;
static int impostorShadowVarId = -1;
static int worldViewPosVarId = -1;

int impostorLastMipSize = 1;

static constexpr int IMPOSTOR_MAX_ASPECT_RATIO = 2;
static constexpr int IMPOSTOR_NOAUTO_MIPS = 5;

static unsigned int dynamicImpostorsPerFrame = 100;

void rendinst::set_billboards_vertical(bool is_vertical) { rendinst::render::vertical_billboards = is_vertical; }

void initImpostorsTempTextures()
{
  int texWidth = rendinst::render::MAX_DYNAMIC_IMPOSTOR_TEX_SIZE;
  int texHeight = texWidth * IMPOSTOR_MAX_ASPECT_RATIO;

  if (rendinst::render::use_color_padding && !rendinst::render::impostorColorTexture[0].getTex2D())
  {
    for (int i = 0; i < rendinst::render::impostor_tex_count; ++i)
    {
      rendinst::render::impostorColorTexture[i] = dag::create_tex(nullptr, texWidth, texHeight,
        TEXCF_RTARGET | rendinst::render::impostor_texformats[i], 1, String(0, "colorImpostorTexture%i", i).str());
      rendinst::render::impostorColorTexture[i]->texfilter(TEXFILTER_POINT);
      rendinst::render::impostorColorTexture[i]->texaddru(TEXADDR_CLAMP);
      rendinst::render::impostorColorTexture[i]->texaddrv(TEXADDR_CLAMP);
    }
  }
}

static inline uint32_t get_format(const char *fmt)
{
  const bool srgb = strstr(fmt, "srgb") != 0;
  if (strstr(fmt, "argb8") != 0)
    return srgb ? (TEXCF_SRGBREAD | TEXCF_SRGBWRITE) : 0;
  else if (strstr(fmt, "dxt1") != 0)
    return (srgb ? TEXCF_SRGBREAD : 0) | TEXFMT_DXT1;
  else if (strstr(fmt, "dxt5") != 0)
    return (srgb ? TEXCF_SRGBREAD : 0) | TEXFMT_DXT5;
  else if (strstr(fmt, "ati1n") != 0)
    return TEXFMT_ATI1N;
  else if (strstr(fmt, "ati2n") != 0)
    return TEXFMT_ATI2N;
  return 0;
}

void initImpostorsGlobals()
{
  impostorShadowXVarId = ::get_shader_variable_id("impostor_shadow_x", true);
  impostorShadowYVarId = ::get_shader_variable_id("impostor_shadow_y", true);
  impostorShadowZVarId = ::get_shader_variable_id("impostor_shadow_z", true);
  impostorShadowVarId = ::get_shader_variable_id("impostor_shadow", true);
  worldViewPosVarId = ::get_shader_glob_var_id("world_view_pos");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Compare;
    ShaderGlobal::set_sampler(get_shader_variable_id("impostor_shadow_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const bool compatibilityMode = ::dgs_get_settings()->getBlockByNameEx("video")->getBool("compatibilityMode", false);
  rendinst::render::impostor_tex_count = graphics->getInt("impostorTexCount", compatibilityMode ? 1 : rendinst::render::IMP_COUNT);

  rendinst::render::impostor_texformats[0] = get_format(graphics->getStr("rendinstImpostorTex0", "argb8_srgb"));
  for (int i = 1; i < rendinst::render::impostor_tex_count; ++i)
  {
    rendinst::render::impostor_texformats[i] = get_format(graphics->getStr(String(0, "rendinstImpostorTex%d", i).c_str(), "argb8"));
  }
  for (int i = 0; i < rendinst::render::impostor_tex_count; ++i)
  {
    rendinst::render::impostor_clear_color[i] =
      graphics->getE3dcolor(String(0, "rendinstImpostorClearColor%d", i).c_str(), rendinst::render::impostor_clear_color[i]);
  }

  rendinst::render::sourceTexVarId[0] = ::get_shader_variable_id("source_tex0");
  rendinst::render::sourceTexVarId[1] = ::get_shader_variable_id("source_tex1", true);
  rendinst::render::sourceTexVarId[2] = ::get_shader_variable_id("source_tex2", true);
  rendinst::render::texelSizeVarId = ::get_shader_variable_id("texel_size");
  rendinst::render::texIndVid = ::get_shader_variable_id("rendinst_impostor_tex_index", true);
  rendinst::render::dynamicImpostorViewXVarId = ::get_shader_glob_var_id("impostor_view_x");
  rendinst::render::dynamicImpostorViewYVarId = ::get_shader_glob_var_id("impostor_view_y");
  rendinst::render::baked_impostor_multisampleVarId = ::get_shader_variable_id("baked_impostor_multisample", true);
  rendinst::render::vertical_impostor_slicesVarId = ::get_shader_variable_id("vertical_impostor_slices", true);

  rendinst::render::postfxBuildMip = create_postfx_renderer("generate_imp_mipmaps");

  shaders::OverrideState state;
  state.set(shaders::OverrideState::CULL_NONE | shaders::OverrideState::Z_BIAS | shaders::OverrideState::Z_FUNC);
  state.zBias = 0.00025;
  state.slopeZBias = 0.7;
  state.zFunc = CMPF_LESSEQUAL;
  state.colorWr = 0;
  rendinst::render::impostorShadowOverride.reset(shaders::overrides::create(state));

  if (!rendinst::render::postfxBuildMip)
    rendinst::render::use_color_padding = false;


  initImpostorsTempTextures();

  init_impostor_texture_mgr();
}

void termImpostorsGlobals()
{
  close_impostor_texture_mgr();

  shaders::overrides::destroy(rendinst::render::impostorShadowOverride);
  del_it(rendinst::render::postfxBuildMip);
  for (int i = 0; i < rendinst::render::impostor_tex_count; ++i)
  {
    rendinst::render::impostorColorTexture[i].close();
  }
  rendinst::render::impostorDepthTextures.clear();
}

static int get_impostor_max_tex_size(const DataBlock &graphics)
{
  const int defTexSize = 256;
  const int highMemMaxTexSize = graphics.getInt("impostorMaxTexSize", defTexSize);
  const int lowMemMaxTexSize = graphics.getInt("impostorMaxTexSizeForLowMem", highMemMaxTexSize);

  const bool isHighMemUsage = graphics.getStr("impostorMemUsage", "high") == String("high");
  return isHighMemUsage ? highMemMaxTexSize : lowMemMaxTexSize;
}

void RendInstGenData::RtData::initImpostors()
{
  uint32_t impMinTexSize = 32;
  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const DataBlock *video = ::dgs_get_settings()->getBlockByNameEx("video");
  const bool compatibilityMode = video->getBool("compatibilityMode", false);
  rendinst::render::MIN_DYNAMIC_IMPOSTOR_TEX_SIZE = graphics->getInt("impostorMinTexSize", impMinTexSize);
  rendinst::render::MAX_DYNAMIC_IMPOSTOR_TEX_SIZE = get_impostor_max_tex_size(*graphics);

  numImpostorsCount = 0;
  if (rendinst::render::dynamic_impostor_texture_const_no < 0)
    return;
  float impostorScreenMul = graphics->getReal("impostorScreenMul", 6.3f);

  dynamicImpostorsPerFrame = ::dgs_get_settings()->getBlockByNameEx("debug")->getInt("dynamicImpostorsPerFrame", 2);

  Tab<Point3_vec4> points(tmpmem);
  Tab<BBox3> shadowPoints(tmpmem);
  for (unsigned int poolNo = 0; poolNo < rtPoolData.size(); poolNo++)
  {
    if (!rtPoolData[poolNo])
      continue;
    rendinst::render::RtPoolData &pool = *rtPoolData[poolNo];

    G_ASSERT(riRes[poolNo] != nullptr);
    if (riResLodCount(poolNo) <= 1)
      continue;

    if (riRes[poolNo]->hasImpostor())
    {
      numImpostorsCount++;
      G_ASSERTF(riPosInst[poolNo], "'%s': only pos instanced impostors are supported. Perhaps level re-export or daBuild required",
        riResName[poolNo]);
      // Calculate optimal RT size.

      G_ASSERTF(riResLodCount(poolNo) >= 2, "'%s': no LOD before dynamic impostor", riResName[poolNo]);
      pool.impostor.tex.resize(rendinst::render::impostor_tex_count);

      pool.impostor.maxFacingLeavesDelta = 0.f;

      float minY = MAX_REAL, maxY = MIN_REAL;
      float bboxAspectRatio = 1;
      if (const RenderableInstanceLodsResource::ImpostorData *imp = riRes[poolNo]->getImpostorData())
      {
        points.resize(0);
        points.resize(imp->convexPoints().size());
        for (int i = 0; i < points.size(); ++i)
          points[i] = imp->convexPoints()[i];

        shadowPoints.resize(0);
        shadowPoints.reserve(imp->shadowPoints().size() + imp->convexPoints().size());
        shadowPoints = imp->shadowPoints();

        pool.impostor.maxFacingLeavesDelta = imp->maxFacingLeavesDelta;
        pool.cylinderRadius = imp->cylRad;
        minY = imp->minY;
        maxY = imp->maxY;
        bboxAspectRatio = imp->bboxAspectRatio;
      }
      else
      {
        const BBox3 &lbb = riRes[poolNo]->bbox;
        minY = max(lbb[0].y, 0.f);
        maxY = lbb[1].y;
        bboxAspectRatio = safediv(lbb.width().y, max(lbb.width().x, lbb.width().z));

        points.resize(8);
        float cylRadSq = 0;
        for (int bp = 0; bp < 8; bp++)
        {
          points[bp].set(lbb[bp & 1].x, lbb[(bp >> 1) & 1].y, lbb[(bp >> 2) & 1].z);
          cylRadSq = max(cylRadSq, points[bp].x * points[bp].x + points[bp].z * points[bp].z);
        }
        pool.cylinderRadius = sqrtf(cylRadSq);
        logwarn("riGen without prebuilt impostor data: %s, using estimate", riResName[poolNo]);
      }

      for (int i = 0; i < points.size(); ++i)
        shadowPoints.push_back(BBox3(points[i], Point3(0, 0, 0)));

      clear_and_resize(pool.impostor.points, points.size());
      G_ASSERT(points.size());
      clear_and_resize(pool.impostor.shadowPoints, shadowPoints.size() * 2);
      for (int i = 0; i < points.size(); ++i)
        *((Point4 *)(char *)&pool.impostor.points[i]) = Point4(points[i].x, points[i].y, points[i].z, 1.f);
      for (int i = 0; i < shadowPoints.size(); ++i)
      {
        *((Point4 *)(char *)&pool.impostor.shadowPoints[i * 2]) =
          Point4(shadowPoints[i][0].x, shadowPoints[i][0].y, shadowPoints[i][0].z, 1.f);
        *((Point4 *)(char *)&pool.impostor.shadowPoints[i * 2 + 1]) =
          Point4(shadowPoints[i][1].x, shadowPoints[i][1].y, shadowPoints[i][1].z, 0.f);
      }
      if (pool.impostor.points.size() < 2 && pool.impostor.maxFacingLeavesDelta < 0.00000001f)
      {
        logerr("invalid impostor! probably stub driver");
        pool.impostor.delImpostor();
      }

      pool.impostor.impostorSphCenterY = 0.5f * (minY + maxY);
      pool.impostor.shadowSphCenterY = 0.5f * (minY + maxY);
      pool.sphCenterY = 0.5f * (minY + maxY);
      debug("max leaves delta = %g", pool.impostor.maxFacingLeavesDelta);

      float distance = riResLodCount(poolNo) > 1 ? pool.lodRange[riResLodCount(poolNo) - 2] : 0.f;

      if (distance > rendinstMaxLod0Dist)
        distance = rendinstMaxLod0Dist;
      distance = get_trees_range(distance);

      float radius = riRes[poolNo]->bound0rad;

      int screenWidth, screenHeight;
      d3d::get_render_target_size(screenWidth, screenHeight, nullptr, 0);

      int rtSize = (int)(screenWidth * impostorScreenMul * radius / max(1.f, distance));

      rtSize = get_bigger_pow2(rtSize);

      int maximumRTSize = rendinst::render::MAX_DYNAMIC_IMPOSTOR_TEX_SIZE;
      rtSize = clamp(rtSize, (int)rendinst::render::MIN_DYNAMIC_IMPOSTOR_TEX_SIZE, maximumRTSize);
      int rtSizeY = rtSize;
      if (bboxAspectRatio > 1.75)
        rtSizeY = rtSize * IMPOSTOR_MAX_ASPECT_RATIO; // trees are more likely to be taller, then wider

      pool.flags &= rendinst::render::RtPoolData::HAS_TRANSITION_LOD;
      pool.hasUpdatedShadowImpostor = false;

      if (riRes[poolNo]->isBakedImpostor())
        continue;

      // Create RT.
      char name[64];

      _snprintf(name, sizeof(name), "impostor_color_RT_for_%d_%p", poolNo, this);
      name[sizeof(name) - 1] = 0;
      unsigned int texflags = TEXCF_RTARGET;

      const unsigned int texflagsColor = texflags | rendinst::render::impostor_texformats[0];

      pool.impostor.renderMips = IMPOSTOR_NOAUTO_MIPS;
      pool.impostor.numColorTexMips = pool.impostor.renderMips;
      pool.impostor.tex[0].close();

      pool.impostor.tex[0] = dag::create_tex(nullptr, rtSize, rtSizeY, texflagsColor, pool.impostor.renderMips, name);
      d3d_err(pool.impostor.tex[0].getBaseTex());

      // pure bilinear filtering of impostors gives ~1 msec gain on PS3
      pool.impostor.tex[0].getBaseTex()->texaddr(TEXADDR_CLAMP);
      pool.impostor.tex[0].getBaseTex()->texlod(0);
      if (compatibilityMode)
      {
        pool.impostor.tex[0].getBaseTex()->setAnisotropy(1);
        add_anisotropy_exception(pool.impostor.tex[0].getTexId());
      }


      // rescale rtSize for AO/normal
      rtSize =
        clamp(rtSize, (int)rendinst::render::MIN_DYNAMIC_IMPOSTOR_TEX_SIZE, (int)rendinst::render::MAX_DYNAMIC_IMPOSTOR_TEX_SIZE);
      rtSizeY = rtSize;
      if (bboxAspectRatio > 1.75)
        rtSizeY = rtSize * IMPOSTOR_MAX_ASPECT_RATIO; // trees are more likely to be taller, then wider

      for (int i = 1; i < pool.impostor.tex.size(); ++i)
      {
        _snprintf(name, sizeof(name), "impostor_RT%d_for_%d_%p", i, poolNo, this);
        name[sizeof(name) - 1] = 0;

        pool.impostor.tex[i].close();
        const uint32_t fmt = rendinst::render::impostor_texformats[i];
        pool.impostor.tex[i] = dag::create_tex(nullptr, rtSize, rtSizeY, texflags | fmt, pool.impostor.renderMips, name);
        d3d_err(pool.impostor.tex[i].getBaseTex());
        pool.impostor.tex[i].getBaseTex()->texaddr(TEXADDR_CLAMP);
        pool.impostor.tex[i].getBaseTex()->texlod(0);
        pool.impostor.tex[i].getBaseTex()->setAnisotropy(1);
        add_anisotropy_exception(pool.impostor.tex[i].getTexId());
      }
      pool.impostor.renderMips = max(1, pool.impostor.renderMips);

      debug("%d: impostor %dx%d  normalizedRatio = %g", poolNo, rtSize, rtSizeY, bboxAspectRatio);
    }
  } // for every pool
}

static bool fill_palette_vb(int rotation_count, const rendinst::gen::RotationPaletteManager::PaletteEntry *rotations)
{
  if (rendinst::render::rotationPaletteTmVb)
  {
    if (auto lockedBuf = lock_sbuffer<float>(rendinst::render::rotationPaletteTmVb, 0, 0, VBLOCK_WRITEONLY))
    {
      for (int i = 0; i < rotation_count + 1; ++i)
      {
        TMatrix rot;
        if (i > 0)
          rot = rendinst::gen::RotationPaletteManager::get_tm(rotations[i - 1]);
        else
          rot.identity();
        for (int j = 0; j < 12; ++j)
          lockedBuf[i * 12 + j] = rot.getcol(j % 4)[j / 4];
      }
    }
    else
      return false; // Could not lock buffer
  }
  return true;
}

bool RendInstGenData::RtData::updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0, int paletteId, const UniqueTex &depth_atlas)
{
  TIME_D3D_PROFILE(update_impostors_preshadow);
  int paletteOffset = 0;
  TMatrix instanceRotation;
  instanceRotation.identity();
  if (paletteId != -1)
  {
    const rendinst::gen::RotationPaletteManager::Palette palette =
      rendinst::gen::get_rotation_palette_manager()->getPalette({layerIdx, poolNo});
    // paletteOffset doesn't include the offset obtained from the palette rotation manager,
    // because it's used to index into rendinst::render::rotationPaletteTmVb,
    // which only stores the current palette + the identity rotation at the beginning
    paletteOffset = (1 + paletteId) * rendinst::render::ROTATION_PALETTE_TM_VB_STRIDE;
    instanceRotation = rendinst::gen::RotationPaletteManager::get_tm(palette, paletteId);
  }

  static const int IMPOSTOR_SHADOW_SIZE = 128;

  rendinst::render::RtPoolData &pool = *rtPoolData[poolNo];
  vec4f sunDirV = v_norm3(v_make_vec4f(sunDir0.x, sunDir0.y, sunDir0.z, 0));

  int sourceLodNo = riResLodCount(poolNo) - 2;
  G_ASSERT(sourceLodNo >= 0);

  rendinst::render::RiShaderConstBuffers cb;
  cb.setBBoxZero();

  if (!pool.shadowImpostorTexture)
  {
    // TODO use one global texture instead
    String name(100, "shadowImpostorTexture%d_%d", poolNo, layerIdx);
    pool.shadowImpostorTexture =
      d3d::create_tex(nullptr, IMPOSTOR_SHADOW_SIZE, IMPOSTOR_SHADOW_SIZE, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, name.str());
    //= d3d::create_tex(nullptr, IMPOSTOR_SHADOW_SIZE, IMPOSTOR_SHADOW_SIZE, TEXCF_RTARGET, 1, "shadowImpostorTexture");
    d3d_err(pool.shadowImpostorTexture);
    pool.shadowImpostorTextureId = register_managed_tex(name.str(), pool.shadowImpostorTexture);
  }
  // d3d::set_render_target(0, (Texture*)rendinst::render::shadowImpostorTexture, 0, true);
  // d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 1.f, 0);
  d3d::set_render_target(0, (Texture *)nullptr, 0);
  d3d::set_depth(pool.shadowImpostorTexture, DepthAccess::RW);
  d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
  mat44f view;
  mat44f viewitm;

#if _TARGET_C1 | _TARGET_C2

#endif

  // vec4f eye = v_mul(sunDirV, v_splats(pool.sphereRadius));
  viewitm.col2 = sunDirV;
  viewitm.col0 = v_norm3(v_cross3(V_C_UNIT_0100, viewitm.col2));
  viewitm.col1 = v_cross3(viewitm.col2, viewitm.col0);

  vec4f center = v_make_vec4f(0, pool.impostor.impostorSphCenterY, 0, 1);
  viewitm.col3 = v_sub(center, v_mul(sunDirV, v_splats(pool.sphereRadius * 2)));
  v_mat44_orthonormal_inverse43_to44(view, viewitm);

  mat44f model;
  v_mat44_make_from_43cu_unsafe(model, instanceRotation.m[0]);
  mat44f view_model;
  v_mat44_mul43(view_model, view, model);
  bbox3f shadowbox;
  v_bbox3_init(shadowbox, view_model, riResBb[poolNo]);
  TMatrix4 orthoProj = matrix_ortho_off_center_lh(v_extract_x(shadowbox.bmin), v_extract_x(shadowbox.bmax),
    v_extract_y(shadowbox.bmin), v_extract_y(shadowbox.bmax), 0, 4.f * pool.sphereRadius);
  d3d::settm(TM_VIEW, view);
  d3d::settm(TM_PROJ, &orthoProj);

  TMatrix view34;
  v_mat_43cu_from_mat44(&view34[0][0], view);

  TMatrix4 globtm;
  d3d::calcglobtm(view34, orthoProj, globtm);
  TMatrix4_vec4 shadowMatrix = globtm * screen_to_tex_scale_tm();
  pool.shadowCol0 = Color4(shadowMatrix.m[0][0], shadowMatrix.m[1][0], shadowMatrix.m[2][0], shadowMatrix.m[3][0]);
  pool.shadowCol1 = Color4(shadowMatrix.m[0][1], shadowMatrix.m[1][1], shadowMatrix.m[2][1], shadowMatrix.m[3][1]);
  pool.shadowCol2 = Color4(shadowMatrix.m[0][2], shadowMatrix.m[1][2], shadowMatrix.m[2][2], shadowMatrix.m[3][2]);
  ShaderGlobal::set_color4(impostorShadowXVarId, pool.shadowCol0);
  ShaderGlobal::set_color4(impostorShadowYVarId, pool.shadowCol1);
  ShaderGlobal::set_color4(impostorShadowZVarId, pool.shadowCol2);


  ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId,
    eastl::to_underlying(rendinst::RenderPass::ToShadow)); // rendinst_render_pass_impostor_color.

  pool.impostor.currentHalfWidth = pool.impostor.currentHalfHeight = 1;
  ShaderGlobal::set_color4(worldViewPosVarId, Color4(0, 0, 0, 0));

  pool.setDynamicImpostorBoundingSphere(cb);
  cb.setOpacity(0.f, 1.f);
  cb.setCrossDissolveRange(0);

  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(rendinst::render::rendinstDepthSceneBlockId, ShaderGlobal::LAYER_SCENE);

  cb.setInstancing(paletteOffset, 3, 0, 0);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
  cb.flushPerDraw();
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);
  shaders::overrides::set(rendinst::render::impostorShadowOverride);


  GlobalVertexData *currentVertexData = nullptr;
  RenderableInstanceResource *sourceScene = riResLodScene(poolNo, sourceLodNo);
  // RenderableInstanceResource *sourceScene = riResLodScene(poolNo, 0);
  ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();
  dag::Span<ShaderMesh::RElem> elems = mesh->getElems(mesh->STG_opaque, mesh->STG_atest);
  for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
  {
    ShaderMesh::RElem &elem = elems[elemNo];

    if (!elem.e)
      continue;

    if (elem.vertexData != currentVertexData)
    {
      currentVertexData = elem.vertexData;
      currentVertexData->setToDriver();
    }

    if (!elem.e->setStates(0, true))
      continue;

    d3d_err(elem.drawIndTriList());
#if _TARGET_C1 | _TARGET_C2

#endif
  }
  shaders::overrides::reset();

  d3d::resource_barrier({pool.shadowImpostorTexture, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (paletteId >= 0 && riRes[poolNo]->isBakedImpostor() && get_impostor_texture_mgr())
  {
    ImpostorTextureManager::ShadowGenInfo shadowInfo;
    shadowInfo.impostor_shadow_x = pool.shadowCol0;
    shadowInfo.impostor_shadow_y = pool.shadowCol1;
    shadowInfo.impostor_shadow_z = pool.shadowCol2;
    shadowInfo.impostor_shadowID = pool.shadowImpostorTextureId;
    if (!get_impostor_texture_mgr()->update_shadow(riRes[poolNo], shadowInfo, layerIdx, poolNo, paletteId, depth_atlas))
      return false;
  }
  return true;
}

bool RendInstGenData::RtData::updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0)
{
  // Shadow textures are rarely recalculated.
  // On an vdata-reload event it's pointless to render shadows with low res data just to override them a few frames later
  if (riRes[poolNo]->getQlBestLod() > riRes[poolNo]->getQlMinAllowedLod())
    return false;
  if (riRes[poolNo]->isBakedImpostor() &&
      (!prefetch_and_check_managed_texture_loaded(riRes[poolNo]->getImpostorTextures().albedo_alpha, true) ||
        !riRes[poolNo]->getImpostorParams().preshadowEnabled))
    return false;
  if (!riPaletteRotation[poolNo] || !rendinst::gen::get_rotation_palette_manager()->hasPalette({layerIdx, poolNo}))
    return updateImpostorsPreshadow(poolNo, sunDir0, -1, {});
  const rendinst::gen::RotationPaletteManager::Palette palette =
    rendinst::gen::get_rotation_palette_manager()->getPalette({layerIdx, poolNo});

  UniqueTex depthAtlas = get_impostor_texture_mgr()->renderDepthAtlasForShadow(riRes[poolNo]);
  G_ASSERT_RETURN(depthAtlas, false);

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, nullptr);
  bool updated = fill_palette_vb(palette.count, palette.rotations);
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinst::render::rotationPaletteTmVb);
  if (!updated)
    return false;
  for (int i = 0; i < palette.count; ++i)
    if (!updateImpostorsPreshadow(poolNo, sunDir0, i, depthAtlas))
      return false;
  return true;
}

void RendInstGenData::RtData::applyImpostorRange(int ri_idx, const DataBlock *ri_ovr, float cell_size)
{
  G_ASSERT(riRes[ri_idx]->hasImpostor());
  auto &ranges = rtPoolData[ri_idx]->lodRange;
  const bool hasTransitionLod = rtPoolData[ri_idx]->hasTransitionLod();
  const int impostorLodNum = riResLodCount(ri_idx) - 1;
  const int transitionLodNum = impostorLodNum - 1;
  const int lastMeshLodNum = hasTransitionLod ? transitionLodNum - 1 : transitionLodNum;
  auto defaultTransitionLodRange = riResLodRange(ri_idx, transitionLodNum, nullptr);
  const float transitionRange = hasTransitionLod ? defaultTransitionLodRange - riResLodRange(ri_idx, lastMeshLodNum, nullptr) : 0.f;
  G_ASSERT(lastMeshLodNum >= 0);
  for (int lodI = 0; lodI < riResLodCount(ri_idx); lodI++)
    ranges[lodI] = riResLodRange(ri_idx, lodI, ri_ovr);
  if (rendinst::render::use_tree_lod0_offset)
  {
    ranges[lastMeshLodNum] += cell_size;
    if (hasTransitionLod)
    {
      ranges[transitionLodNum] += cell_size;
      defaultTransitionLodRange += cell_size;
    }
  }
  if (hasTransitionLod)
  {
    G_ASSERT(transitionRange > 0);
    // Override setting does not know about transition lod,
    // so need to apply it to impostor lod instead.
    if (defaultTransitionLodRange != ranges[transitionLodNum])
      ranges[impostorLodNum] = ranges[transitionLodNum];
    Tab<ShaderMaterial *> mats;
    riRes[ri_idx]->lods[transitionLodNum].scene->gatherUsedMat(mats);
    for (auto mat : mats)
      riRes[ri_idx]->setImpostorTransitionRange(mat, ranges[lastMeshLodNum], transitionRange);

    ranges[transitionLodNum] = ranges[lastMeshLodNum] + transitionRange;
  }
}

static void impostorMipSRVBarrier(rendinst::render::RtPoolData &pool, int mip)
{
  for (int j = 0; j < pool.impostor.tex.size(); ++j)
  {
    UniqueTex &tex = pool.impostor.tex[j];
    BaseTexture *pTex = tex.getBaseTex();
    d3d::resource_barrier({pTex, RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)(mip), 1});
  }
}

void renderImpostorMips(rendinst::render::RtPoolData &pool, int currentRenderMips)
{
  PostFxRenderer *pFx = rendinst::render::postfxBuildMip;
  if (!pFx)
  {
    for (int j = 0; j < pool.impostor.tex.size(); ++j)
      if (pool.impostor.tex[j].getBaseTex())
        pool.impostor.tex[j].getBaseTex()->generateMips();

    return;
  }

  static int rendinstPaddingVarId = ::get_shader_variable_id("rendinst_padding", true);
  static int maxTranslucancyVarId = ::get_shader_variable_id("rendinst_max_translucancy", true);

  ShaderGlobal::set_int(rendinstPaddingVarId, rendinst::render::use_color_padding);
  ShaderGlobal::set_int(maxTranslucancyVarId, 1);

  // set states
  for (int j = 0; j < pool.impostor.tex.size(); ++j)
  {
    if (pool.impostor.tex[j].getBaseTex())
      pool.impostor.tex[j].getBaseTex()->texfilter(TEXFILTER_POINT);
  }

  // copy image to other mips
  for (int mip = 1; mip < currentRenderMips; ++mip)
  {
    int src_mip = pool.impostor.baseMip + mip - 1;
    int dst_mip = pool.impostor.baseMip + mip;

    impostorMipSRVBarrier(pool, src_mip);

    for (int j = 0; j < pool.impostor.tex.size(); ++j)
    {
      UniqueTex &tex = pool.impostor.tex[j];
      BaseTexture *pTex = tex.getBaseTex();
      if (pTex)
      {
        d3d::set_render_target(j, pTex, dst_mip);
        ShaderGlobal::set_texture(rendinst::render::sourceTexVarId[j], tex.getTexId());
        pTex->texmiplevel(src_mip, src_mip);
      }
    }

    UniqueTex &tex = pool.impostor.tex[0];
    TextureInfo ti;
    tex.getBaseTex()->getinfo(ti, src_mip);
    ShaderGlobal::set_color4(rendinst::render::texelSizeVarId, 1.f, 1.f, 1.f / ti.w, 1.f / ti.h);
    ShaderGlobal::set_int(rendinst::render::texIndVid, -1);

    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    pFx->render();
  }

  // restore levels
  int lastMip = min(pool.impostor.numColorTexMips - 1, get_last_mip_idx(pool.impostor.tex[0].getBaseTex(), impostorLastMipSize));
  lastMip = max(0, lastMip);

  for (int j = 0; j < pool.impostor.tex.size(); ++j)
  {
    if (pool.impostor.tex[j].getBaseTex())
    {
      pool.impostor.tex[j].getBaseTex()->texmiplevel(pool.impostor.baseMip, lastMip);
      pool.impostor.tex[j].getBaseTex()->texfilter(TEXFILTER_DEFAULT);
    }
  }
}

void RendInstGenData::RtData::updateImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm,
  const mat44f &proj_tm)
{
  if (rendinst::render::dynamic_impostor_texture_const_no < 0 || !rtPoolData.size())
    return;

  // Save RT and matrices.
  d3d::GpuAutoLock gpuLock;
  SCOPE_RENDER_TARGET;
  SCOPE_VIEWPORT;
  SCOPE_VIEW_PROJ_MATRIX;

  vec4f ground = v_make_vec4f(MIN_REAL, 0, MIN_REAL, MIN_REAL);

  int currentCycle = dynamicImpostorToUpdateNo / rtPoolData.size();
  if (currentCycle != oldImpostorCycle)
  {
    oldImpostorCycle = currentCycle;
    oldViewImpostorDir = viewImpostorDir;
  }
  Point3 dir = view_itm.getcol(2), up, left;
  if (rendinst::render::vertical_billboards)
    dir = normalize(Point3(dir.x, 0, dir.z));

  get_up_left(up, left, view_itm);
  viewImpostorDir = v_make_vec4f(dir.x, dir.y, dir.z, 0);
  viewImpostorUp = v_ldu(&up.x);

  int updateImpostorsPerFrame = dynamicImpostorsPerFrame;
  bool isSmallChange =
    v_test_vec_x_gt(v_dot3_x(viewImpostorDir, oldViewImpostorDir), v_splats(0.9961946980917455)); // cos(DTOR(5)) == 0.9961946980917455
  if (isSmallChange)
  {
    if (--bigChangePoolNo > 0)
      isSmallChange = false;
  }
  else
    bigChangePoolNo = numImpostorsCount / updateImpostorsPerFrame; // we update x impostors per frame if change is big

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  rendinst::render::RiShaderConstBuffers cb;
  cb.setBBoxZero();

  rendinst::render::startRenderInstancing();
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, nullptr);
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, rendinst::render::rotationPaletteTmVb);
  d3d::set_immediate_const(STAGE_VS, ZERO_PTR<uint32_t>(), 1);

  int numUpdated = 0;
  uint32_t updatePoolNo = dynamicImpostorToUpdateNo;

  for (unsigned int numCheckedPools = 0; numCheckedPools < rtPoolData.size() && numUpdated < updateImpostorsPerFrame;
       updatePoolNo++, numCheckedPools++)
  {
    int poolNo = updatePoolNo % rtPoolData.size();

    if (!rtPoolData[poolNo] || !rtPoolData[poolNo]->hasImpostor())
      continue;
    rendinst::render::RtPoolData &pool = *rtPoolData[poolNo];
    RenderableInstanceLodsResource *res = riRes[poolNo];
    if (!pool.hadVisibleImpostor)
    {
      // we skip updating completely invisible impostor
      continue;
    }
    int sourceLodNo = riResLodCount(poolNo) - 2;
    G_ASSERT(sourceLodNo >= 0);

    numUpdated++;
    float unscaledRange = pool.lodRange[sourceLodNo];
    float range = get_trees_range(unscaledRange);
    d3d::set_render_target(); // Remove other MRTs
    bool hasShadow = shadowDistance > range;
    // debug("impostor range = %g, original range =%g shadowD=%g", range, pool.lodRange[sourceLodNo], shadowDistance);
    if (hasShadow && !pool.hasUpdatedShadowImpostor)
      pool.hasUpdatedShadowImpostor = updateImpostorsPreshadow(poolNo, sunDir0);

    // Set new matrices.
    const float impostorZnear = 1.f;
    float distanceToImpostor = max(max(range, 250.f), riRes[poolNo]->bsphRad * 4.f);

    DECL_ALIGN16(Point4, sphereCenter) = Point4(0.f, pool.sphCenterY, 0.f, 1.f);

    float zfar = distanceToImpostor + impostorZnear + riRes[poolNo]->bsphRad;

    DECL_ALIGN16(Point4, cameraPos);
    mat44f viewtm, viewitm;

    cameraPos = sphereCenter - distanceToImpostor * as_point4(&viewImpostorDir);
    viewitm.col3 = v_ld(&cameraPos.x);
    viewitm.col2 = viewImpostorDir;
    viewitm.col0 = v_norm3(v_cross3(viewImpostorUp, viewitm.col2));
    viewitm.col1 = v_cross3(viewitm.col2, viewitm.col0);
    v_mat44_orthonormal_inverse43_to44(viewtm, viewitm);

    d3d::settm(TM_VIEW, viewtm);
    DECL_ALIGN16(Point3_vec4, col[3]);
    v_st(&col[0].x, viewitm.col0);
    v_st(&col[1].x, viewitm.col1);
    v_st(&col[2].x, viewitm.col2);
    LeavesWindEffect::setNoAnimShaderVars(col[0], col[1], col[2]);

    vec4f impostorSize = v_zero();

    for (unsigned int pointNo = 0; pointNo < pool.impostor.shadowPoints.size(); pointNo += 2)
    {
      vec4f localPoint = pool.impostor.shadowPoints[pointNo];
      vec4f delta = pool.impostor.shadowPoints[pointNo + 1];
      localPoint = v_add(localPoint, v_mul(v_splat_x(delta), viewitm.col0));
      localPoint = v_add(localPoint, v_mul(v_splat_y(delta), viewitm.col1));
      localPoint = v_max(localPoint, ground);
      vec4f rotPoint = v_mat44_mul_vec3p(viewtm, localPoint);
      impostorSize = v_max(impostorSize, v_abs(rotPoint));
    }

    pool.impostor.currentHalfWidth = v_extract_x(impostorSize);
    pool.impostor.currentHalfHeight = v_extract_y(impostorSize);

    if (res->isBakedImpostor())
    {
      // if preshadow baking is failed, we need to try again next frame
      if (!hasShadow || pool.hasUpdatedShadowImpostor)
        updatePoolNo++;
      break;
    }

    mat44f proj;
    v_mat44_make_persp(proj, distanceToImpostor / (pool.impostor.currentHalfWidth),
      distanceToImpostor / (pool.impostor.currentHalfHeight), impostorZnear, zfar);
    int currentRenderMips = pool.impostor.renderMips;
    if (currentRenderMips > 1)
    {
      vec4f screenImpostorHalfDiag =
        v_mat44_mul_vec4(proj_tm, v_make_vec4f(pool.impostor.currentHalfWidth, pool.impostor.currentHalfHeight, range, 1.f));
      screenImpostorHalfDiag = v_div(screenImpostorHalfDiag, v_splat_w(screenImpostorHalfDiag));

      vec4f screenImpostorSizeVec = v_max(screenImpostorHalfDiag, v_splat_y(screenImpostorHalfDiag));
      float screenImpostorRadius = v_extract_x(screenImpostorSizeVec);

      // screenImpostorRadius is equal to radius of impostor's circumcircle in screen space
      // debug("poolNo=%d screen impostor Radius = %g at = %g", poolNo, screenImpostorRadius, range);
      int screenW, screenH;
      d3d::get_render_target_size(screenW, screenH, nullptr);
      int screenSize = max(screenW, screenH);

      int screenPixels = max((int)(screenImpostorRadius * screenSize), 1) + 1;
      TextureInfo tinfo;
      pool.impostor.tex[0].getBaseTex()->getinfo(tinfo);
      int impostorPixels = min(tinfo.w, tinfo.h);

      pool.impostor.baseMip = 0;
      pool.impostor.baseMip = impostorPixels <= screenPixels ? 0 : get_log2i(impostorPixels / screenPixels);

      pool.impostor.baseMip = max(0, min(pool.impostor.renderMips - 1, pool.impostor.baseMip));
    }
    else
      pool.impostor.baseMip = 0;

    int lastMip = min(pool.impostor.numColorTexMips - 1, get_last_mip_idx(pool.impostor.tex[0].getBaseTex(), impostorLastMipSize));
    lastMip = max(0, lastMip);
    pool.impostor.baseMip = min(pool.impostor.baseMip, lastMip);
    currentRenderMips = lastMip + 1 - pool.impostor.baseMip;
    for (int j = 0; j < pool.impostor.tex.size(); ++j)
    {
      if (!pool.impostor.tex[j].getBaseTex())
        break;
      pool.impostor.tex[j].getBaseTex()->texmiplevel(pool.impostor.baseMip, lastMip);
    }

    d3d::settm(TM_PROJ, proj);


    ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId,
      eastl::to_underlying(rendinst::RenderPass::ImpostorColor)); // rendinst_render_pass_impostor_color.

    ShaderGlobal::set_color4(worldViewPosVarId, *(Color4 *)&cameraPos.x);

    if (hasShadow)
    {
      ShaderGlobal::set_texture(impostorShadowVarId, pool.shadowImpostorTextureId);
      ShaderGlobal::set_color4(impostorShadowXVarId, pool.shadowCol0);
      ShaderGlobal::set_color4(impostorShadowYVarId, pool.shadowCol1);
      ShaderGlobal::set_color4(impostorShadowZVarId, pool.shadowCol2);
    }

    // required to be set before block, soopengl driver use correct axis flipping
    d3d::set_render_target(pool.impostor.tex[0].getBaseTex(), 0);
    d3d::set_depth(nullptr, DepthAccess::RW);
    ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(rendinst::render::rendinstSceneBlockId, ShaderGlobal::LAYER_SCENE);

    pool.setDynamicImpostorBoundingSphere(cb);
    cb.setOpacity(0.f, 1.f);
    cb.setCrossDissolveRange(0);

    cb.setInstancing(0, 3, 0, 0);
    rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
    cb.flushPerDraw();


    RenderableInstanceResource *sourceScene = riResLodScene(poolNo, sourceLodNo);
    {
      UniqueTex *dstRT[rendinst::render::IMP_COUNT];

      for (int j = 0; j < pool.impostor.tex.size(); ++j)
      {
        if (rendinst::render::use_color_padding)
          dstRT[j] = &rendinst::render::impostorColorTexture[j];
        else
          dstRT[j] = &pool.impostor.tex[j];
      }
      int baseMip = rendinst::render::use_color_padding ? 0 : pool.impostor.baseMip;

      TextureInfo tinfo0;
      dstRT[0]->getBaseTex()->getinfo(tinfo0);
      G_UNUSED(tinfo0);
      // clear
      // begin mrt clear sequnce, with an offset of 1 (first target is not set in this sequence)
      d3d::driver_command(Drv3dCommand::BEGIN_MRT_CLEAR_SEQUENCE, ((uint8_t *)1));
      for (int j = 1; j < pool.impostor.tex.size(); ++j)
      {
        BaseTexture *pTex = dstRT[j]->getBaseTex();
        if (pTex)
        {
          TextureInfo tinfo;
          dstRT[j]->getBaseTex()->getinfo(tinfo);
          G_UNUSED(tinfo);
          G_ASSERT(tinfo.w == tinfo0.w && tinfo.h == tinfo0.h);

          d3d::set_render_target(0, pTex, baseMip);
          d3d::clearview(CLEAR_TARGET, ::grs_draw_wire ? (E3DCOLOR)0xFF000000 : rendinst::render::impostor_clear_color[j], 1.f, 0);
        }
      }
      // reset index of mrt clear sequence to 0, to complete the mrt block
      d3d::driver_command(Drv3dCommand::BEGIN_MRT_CLEAR_SEQUENCE, ((uint8_t *)0));

      // required to be set before block, soopengl driver use correct axis flipping
      d3d::set_render_target(dstRT[0]->getBaseTex(), baseMip);

      auto depthTexIt = rendinst::render::impostorDepthTextures.find(IPoint2(tinfo0.w >> baseMip, tinfo0.h >> baseMip));
      if (depthTexIt == rendinst::render::impostorDepthTextures.end())
      {
        depthTexIt =
          rendinst::render::impostorDepthTextures.emplace(IPoint2(tinfo0.w >> baseMip, tinfo0.h >> baseMip), UniqueTex()).first;
        String name(0, "impostorDepth%dx%d", tinfo0.w >> baseMip, tinfo0.h >> baseMip);
        depthTexIt->second =
          dag::create_tex(nullptr, tinfo0.w >> baseMip, tinfo0.h >> baseMip, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, name.str());
      }
      d3d::set_depth(depthTexIt->second.getTex2D(), d3d::get_driver_code().is(d3d::metal) ? DepthAccess::SampledRO : DepthAccess::RW);

      unsigned impostor_clear_color = (::grs_draw_wire ? 0xFF000000 : 0) | rendinst::render::impostor_clear_color[0];
      if (d3d::get_driver_code().is(!d3d::metal)) // clear color/depth for non-METAL here or it is not cleared at all!
        d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, impostor_clear_color, 0.f, 0);
      for (int j = 1; j < pool.impostor.tex.size(); ++j)
        if (dstRT[j]->getBaseTex())
        {
          d3d::set_render_target(j, dstRT[j]->getBaseTex(), baseMip);
        }

      // Do not split renderpass into 2 passes by doing clear before set_render_target
      // The cleared depth is not preserved between them!
      if (d3d::get_driver_code().is(d3d::metal))
        d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, impostor_clear_color, 0.f, 0);
      // mrt clear sequence complete, continue as normal
      d3d::driver_command(Drv3dCommand::END_MRT_CLEAR_SEQUENCE, (uint8_t *)1);
#if _TARGET_C1 | _TARGET_C2

#endif

      // set viewport
      float viewportPartX = 1.f, viewportPartY = 1.f;
      if (rendinst::render::use_color_padding)
      {
        TextureInfo tinfo;
        pool.impostor.tex[0].getBaseTex()->getinfo(tinfo, baseMip);

        int vpWidth = tinfo.w;
        int vpHeight = tinfo.h;
        d3d::setview(0, 0, vpWidth, vpHeight, 0.f, 1.f);

        TextureInfo tempRTInfo;
        UniqueTex &tex = rendinst::render::impostorColorTexture[0];
        tex.getTex2D()->getinfo(tempRTInfo, 0);

        G_ASSERT(vpWidth <= tempRTInfo.w && vpHeight <= tempRTInfo.h);

        viewportPartX = (float)vpWidth / max((int)tempRTInfo.w, 1);
        viewportPartY = (float)vpHeight / max((int)tempRTInfo.h, 1);
      }

      // draw geometry to baseMip of texture
      GlobalVertexData *currentVertexData = nullptr;
      ShaderMesh *mesh = sourceScene->getMesh()->getMesh()->getMesh();
      dag::Span<ShaderMesh::RElem> elems = mesh->getElems(mesh->STG_opaque, mesh->STG_decal);
      for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
      {
        ShaderMesh::RElem &elem = elems[elemNo];

        if (!elem.e)
          continue;

        if (elem.vertexData != currentVertexData)
        {
          currentVertexData = elem.vertexData;
          currentVertexData->setToDriver();
        }

        if (!elem.e->setStates(0, true))
          continue;

        d3d_err(elem.drawIndTriList());
#if _TARGET_C1 | _TARGET_C2

#endif
      }

      for (int j = 0; j < pool.impostor.tex.size(); ++j)
      {
        BaseTexture *pTex = dstRT[j]->getBaseTex();
        if (pTex)
        {
          d3d::resource_barrier({pTex, RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)baseMip, 1});
        }
      }

      PostFxRenderer *pFx = rendinst::render::postfxBuildMip;
      ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

      // to mip 0
      if (rendinst::render::use_color_padding)
      {
        static int maxTranslucancyVarId = ::get_shader_variable_id("rendinst_max_translucancy");

        for (int j = 0; j < pool.impostor.tex.size(); ++j)
        {
          UniqueTex &tex = pool.impostor.tex[j];
          BaseTexture *pTex = tex.getBaseTex();
          if (pTex)
          {
            d3d::set_render_target(j, pTex, pool.impostor.baseMip);
            ShaderGlobal::set_texture(rendinst::render::sourceTexVarId[j], rendinst::render::impostorColorTexture[j].getTexId());
          }
        }
        TextureInfo ti;
        UniqueTex &tex = rendinst::render::impostorColorTexture[0];
        tex.getBaseTex()->getinfo(ti, 0);
        ShaderGlobal::set_color4(rendinst::render::texelSizeVarId, viewportPartX, viewportPartY, 1.f / ti.w, 1.f / ti.h);
        ShaderGlobal::set_int(maxTranslucancyVarId, 0);
        ShaderGlobal::set_int(rendinst::render::texIndVid, -1);

        pFx->render();
      }

      renderImpostorMips(pool, currentRenderMips);

      // do a full subres barrier to fit all branching above
      // as whatever happens, we should have all targets and their subresources
      // in RO_SRV state before exiting generation
      for (int j = 0; j < pool.impostor.tex.size(); ++j)
      {
        UniqueTex &tex = pool.impostor.tex[j];
        Texture *pTex = tex.getBaseTex();
        d3d::resource_barrier({pTex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      }
    }

    if (hasShadow && isSmallChange)
      numUpdated++;
    if (hasShadow)
      ShaderGlobal::set_texture(impostorShadowVarId, BAD_TEXTUREID);
  }


  dynamicImpostorToUpdateNo = updatePoolNo;

  // Restore RT and matrices.

  rendinst::render::endRenderInstancing();

  if (!numUpdated)
    return; // nothing has been rendered
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::set_color4(worldViewPosVarId, Color4(view_itm.getcol(3).x, view_itm.getcol(3).y, view_itm.getcol(3).z, 1));

  ShaderGlobal::set_int_fast(rendinst::render::rendinstRenderPassVarId,
    eastl::to_underlying(rendinst::RenderPass::Normal)); // rendinst_render_pass_normal.

  ShaderGlobal::setBlock(rendinst::render::globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
}

void rendinst::resetRiGenImpostors()
{
  rendinst::render::impostorPreshadowNeedUpdate = true;
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (!rgl->rtData)
      continue;

    ScopedLockRead lock(rgl->rtData->riRwCs);
    rgl->rtData->dynamicImpostorToUpdateNo = 0;
  }
}

void rendinst::updateRIGenImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm, const mat44f &proj_tm)
{
  if (!RendInstGenData::renderResRequired || RendInstGenData::isLoading)
    return;
  const float subCellDistMul = (1.0f / RendInstGenData::SUBCELL_DIV);

  bool needsReset = false;

  if (rendinst::render::impostorPreshadowNeedUpdate)
  {
    initImpostorsTempTextures();
    needsReset = true;
  }

  FOR_EACH_RG_LAYER_DO (rgl)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    float shadowDistOfs = (rendinst::render::per_instance_visibility ? 0 : (0.5f * rgl->grid2world * rgl->cellSz * subCellDistMul));

    if (needsReset)
    {
      rgl->rtData->initImpostors();               // tex2D <-> texArray migration
      rgl->rtData->dynamicImpostorToUpdateNo = 0; // new update cycle
    }

    if (rgl->rtData->dynamicImpostorToUpdateNo < rgl->rtData->rtPoolData.size())
      rgl->rtData->updateImpostors(shadowDistance - shadowDistOfs, sunDir0, view_itm, proj_tm);
  }

  rendinst::render::impostorPreshadowNeedUpdate = false;
}