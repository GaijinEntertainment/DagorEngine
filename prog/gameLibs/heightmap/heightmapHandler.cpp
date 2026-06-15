// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapHandler.h>
#include <heightmap/heightmapMetricsCalc.h>
#include <heightMapLand/dag_hmlGetHeight.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathUtils.h>
#include <shaders/dag_shaders.h>
#include <math/dag_adjpow2.h>
#include <memory/dag_physMem.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <util/dag_convar.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_lockTexture.h>
#include <drv/3d/dag_info.h>

#include <perfMon/dag_statDrv.h>
#include <heightmap/simpleHeightmapRenderer.h>

// for NV 551 workaround
CONSOLE_BOOL_VAL("hmap", nvidia_551_workaround, true);
CONSOLE_INT_VAL("hmap", metrics_maxCalcLevel, 14, 2, 128);
CONSOLE_INT_VAL("hmap", metrics_minCalcLevel, 2, 0, 8);
CONSOLE_INT_VAL("hmap", metrics_dim, 3, 3, 5);

// Used for dynamic terraforms
#define ENABLE_RENDER_HMAP_MODIFICATION    1
#define ENABLE_RENDER_HMAP_SUB_TESSELATION 1

#define GLOBAL_VARS_LIST         \
  VAR(heightmap_region)          \
  VAR(tex_hmap_low_samplerstate) \
  VAR(hmapMain)                  \
  VAR(tex_hmap_low)              \
  VAR(world_to_hmap_low)         \
  VAR(heightmap_scale)           \
  VAR(tex_hmap_inv_sizes)        \
  VAR(heightmap_texels)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

static uint32_t select_hmap_tex_fmt()
{
  if (d3d::get_texformat_usage(TEXFMT_L16) & d3d::USAGE_VERTEXTEXTURE)
    return TEXFMT_L16;
  if (d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_VERTEXTEXTURE)
    return TEXFMT_R32F;
  if (d3d::get_texformat_usage(TEXFMT_R16F) & d3d::USAGE_VERTEXTEXTURE) // better fast, than accurate
    return TEXFMT_R16F;

  logerr("no suitable vertex texture found");
  return TEXFMT_L16;
}

static int get_mip_levels(const IPoint2 &hmapWidth) { return min(get_log2i(hmapWidth.x), get_log2i(hmapWidth.y)) + 1; }

void HeightmapHandler::setVars()
{
  if (!renderData)
    return;
  Color4 c = mirror ? Color4(-(128 << 20), -(128 << 20), 128 << 20, 128 << 20)
                    : Color4(worldBox2[0].x, worldBox2[0].y, worldBox2[1].x, worldBox2[1].y);
  ShaderGlobal::set_float4(heightmap_regionVarId, c);
  ShaderGlobal::set_float4(world_to_hmap_lowVarId,
    Color4(1.0f / worldSize.x, 1.0f / worldSize.y, -worldPosOfs.x / worldSize.x, -worldPosOfs.y / worldSize.y));

  ShaderGlobal::set_float4(tex_hmap_inv_sizesVarId,
    Color4(1.0f / hmapWidth.x, 1.0f / hmapWidth.y, 1.0f / hmapWidth.x, 1.0f / hmapWidth.y));

  ShaderGlobal::set_float4(heightmap_scaleVarId, Color4(hScale, hMin, hScale, hMin));
  ShaderGlobal::set_texture(hmapMainVarId, renderData->heightmap);
  ShaderGlobal::set_texture(tex_hmap_lowVarId, renderData->heightmap);
  ShaderGlobal::set_sampler(tex_hmap_low_samplerstateVarId, renderData->heightmapSampler);
}

void HeightmapHandler::initRender(bool clamp, float water_level, float shore_error_meters)
{
  mirror = !clamp;
  renderData.reset(new HeightmapRenderData);

  const int levelCount = get_mip_levels(hmapWidth) - 1; // Last mip is 2x2.
  renderData->texFMT = select_hmap_tex_fmt();
  renderData->heightmap = dag::create_tex(NULL, hmapWidth.x, hmapWidth.y, renderData->texFMT | TEXCF_UPDATE_DESTINATION, levelCount,
    "hmapMain", RESTAG_LAND);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w =
      clamp ? d3d::AddressMode::Clamp : d3d::AddressMode::Mirror;
    renderData->heightmapSampler = d3d::request_sampler(smpInfo);
  }
  d3d_err(renderData->heightmap.getTex2D());

  fillHmapTexturesNeeded = true;

  hmapUploadTex = dag::create_tex(NULL, enabledMipsUpdating ? 3 * HMAP_BSIZE / 2 : HMAP_BSIZE, HMAP_BSIZE,
    renderData->texFMT | TEXCF_WRITEONLY, 1, "hmap_upload_tex_region", RESTAG_LAND);
  lastRegionUpdated_NVworkaround = -1;
  setVars();

  del_it(metrics);
  del_it(metricsRenderer);
  shoreErrorMeters = shore_error_meters;
  metrics = new MetricsErrors();
  const int dimBits = metrics_dim;
  metrics->calc_lod_errors(*this, metrics_minCalcLevel, metrics_maxCalcLevel, 1 << dimBits, water_level, shoreErrorMeters);

  metricsRenderer = new SimpleHeightmapRenderer;
  metricsRenderer->init("heightmap", true, dimBits);
  debug("hmap: initialized with metrics and %f water level, shore error=%f", water_level, shoreErrorMeters);
}

bool HeightmapHandler::loadDump(IGenLoad &loadCb, bool load_render_data, float water_level, float shore_error_meters)
{
  const int skip_mips = clamp(::dgs_get_settings()->getBlockByNameEx("debug")->getInt("skip_hmap_levels", 0), 0, 3);

  if (!HeightmapPhysHandler::loadDump(loadCb, skip_mips))
    return false;

  if (load_render_data)
    initRender(!mirror, water_level, shore_error_meters);
  return true;
}

static void upload_texture_data(uint8_t *dst, int dst_stride, uint32_t dst_frm, const uint16_t *src, IPoint2 src_size)
{
  G_ASSERT(src_size.x <= dst_stride);
  if (dst_frm == TEXFMT_L16)
  {
    for (int y = 0; y < src_size.y; ++y, dst += dst_stride, src += src_size.x)
      memcpy(dst, src, src_size.x * sizeof(uint16_t));
  }
  else if (dst_frm == TEXFMT_R32F)
  {
    for (int y = 0; y < src_size.y; ++y, dst += dst_stride, src += src_size.x)
      for (int x = 0; x < src_size.x; ++x)
        ((float *)dst)[x] = src[x] / float(UINT16_MAX);
  }
  else // if (dst_frm == TEXFMT_R16F)
  {
    for (int y = 0; y < src_size.y; ++y, dst += dst_stride, src += src_size.x)
      for (int x = 0; x < src_size.x; ++x)
        ((uint16_t *)dst)[x] = float_to_half(src[x] / float(UINT16_MAX));
  }
}

static void get_texture_data(eastl::span<uint16_t> dst, const CompressedHeightmap &src, int elements_stride, IPoint2 src_pivot,
  IPoint2 region_width)
{
  for (int y = 0, sy = src_pivot.y; y < region_width.y; ++y, ++sy)
    for (int x = 0, sx = src_pivot.x; x < region_width.x; ++x, ++sx)
      dst[y * elements_stride + x] = src.decodePixelUnsafe(sx, sy);
}

static void get_visual_heights(eastl::span<uint16_t> dst, const ska::flat_hash_map<uint32_t, uint16_t> &visualHeights,
  int elements_stride, IPoint2 src_pivot, int src_pitch, IPoint2 region_width)
{
  // update only mip level 0 with visualHeights
  // for simplicity
  IBBox2 regionBox(src_pivot, src_pivot + region_width - IPoint2::ONE);
  for (auto &iter : visualHeights)
  {
    int index = iter.first;
    IPoint2 cell(index % src_pitch, index / src_pitch);
    if (!(regionBox & cell))
      continue;
    cell -= src_pivot;
    dst[cell.y * elements_stride + cell.x] = iter.second;
  }
}

void HeightmapHandler::fillHmapTextures()
{
  TIME_D3D_PROFILE(hmap_fillHmapTextures);

  IPoint2 quadWidth = hmapWidth / 2;
  SmallTab<uint16_t, TmpmemAlloc> tempMem;
  clear_and_resize(tempMem, 3 * quadWidth.x * quadWidth.y / 2);

  // Break update into 4 quads in order avoid exceeding device's Texture2D dimension limits.
  for (int i = 0; i < 4; ++i)
  {
    String uploadTempName(128, "hmap_upload_tex_quad_%d", i);
    UniqueTex uploadTemp =
      dag::create_tex(NULL, 3 * quadWidth.x / 2, quadWidth.y, renderData->texFMT | TEXCF_WRITEONLY, 1, uploadTempName, RESTAG_LAND);
    if (!uploadTemp)
    {
      logerr("failed to create uploadTemp during fillHmapTextures, skipping");
      continue;
    }

    IPoint2 quadPivot = IPoint2::ZERO;
    if (i % 2 != 0)
      quadPivot.x += quadWidth.x;
    if (i / 2 != 0)
      quadPivot.y += quadWidth.y;

    if (auto lock = lock_texture<ImageRawBytes>(uploadTemp.getTex2D(), 0, TEXLOCK_WRITE))
      fillHmapRegionDetailed(quadPivot, quadWidth, true, uploadTemp.getTex2D(), eastl::move(lock), tempMem);
    else
      logerr("hmap: Could not lock %s during fillHmapTextures! Part of hmap will be missing!", uploadTempName);
  }
}

bool HeightmapHandler::fillHmapRegion(int region_index, bool NVworkaround_applyOnNextFrame)
{
  TIME_D3D_PROFILE(hmap_fillHmapRegion);

  G_ASSERT(region_index >= 0);
  const int regionStride = hmapWidth.x / HMAP_BSIZE;
  const IPoint2 regionPivot = IPoint2((region_index % regionStride) * HMAP_BSIZE, (region_index / regionStride) * HMAP_BSIZE);
  const IPoint2 regionWidth = IPoint2(HMAP_BSIZE, HMAP_BSIZE);

  // Keep in mind that above log2(HMAP_BSIZE) + 1 = log2(32) + 1 = 6th mip level heights won't update.
  auto tryLock = lock_texture<ImageRawBytes>(hmapUploadTex.getTex2D(), 0, TEXLOCK_WRITE | TEXLOCK_DISCARD);
  if (tryLock)
  {
    alignas(16) carray<uint16_t, 3 * HMAP_BSIZE * HMAP_BSIZE / 2> tempMem;
    fillHmapRegionDetailed(regionPivot, regionWidth, enabledMipsUpdating, hmapUploadTex.getTex2D(), eastl::move(tryLock), tempMem,
      NVworkaround_applyOnNextFrame);
    return true;
  }
  else
  {
    logwarn("hmap: Could not lock hmap_upload_tex_region (aka hmapUploadTex). Skipping fillHmapRegion for now.");
    return false;
  }
}

void HeightmapHandler::fillHmapRegionDetailed(IPoint2 region_pivot, IPoint2 region_width, bool update_mips, BaseTexture *upload_tex,
  LockedImageRawBytes upload_texlock, const eastl::span<uint16_t> temp_mem, bool NVworkaround_applyOnNextFrame)
{
  const int levelCount = min<int>(update_mips ? get_mip_levels(region_width) : 1, renderData->heightmap.getTex2D()->level_count());
  const int elementsStride = levelCount > 1 ? 3 * region_width.x / 2 : region_width.x;

  auto getFlatMipPivot = [](int mip, IPoint2 texSize) -> IPoint2 {
    // Starting from mip1, we place mip on the right of the previous if mip index is odd number, bellow otherwise.
    // For example, if mip0 is 32*32, origin of mip0=(0,0), mip1=(32, 0), mip2=(32, 16), mip3=(40, 16), mip4=(40, 20), mip5=(42, 20).
    G_ASSERT_RETURN(mip >= 0, IPoint2(0, 0));

    // Geometric Progression Sum for R=1/4 is Sn = a0*(1 - 1/4^n) / (3/4) = a0*(4^n - 1) / 3*4^(n-1) = a0*(2^(2*n) - 1) / 3*2^(2*(n-1))
    // For n = 0, we make sure Sn = getNumerator(0) / getDenominator(0) = 0 and avoid UB, thus max operation at "getDenominator".
    auto getNumerator = [](int n) -> uint32_t { return (1 << (2 * n)) - 1; };
    auto getDenominator = [](int n) -> uint32_t { return 3 * (1 << (2 * max(n - 1, 0))); };

    int rightSteps = (mip + 1) / 2;
    int downSteps = mip / 2;

    IPoint2 a0 = IPoint2(texSize.x, texSize.y / 2);
    return IPoint2(((uint32_t)a0.x * getNumerator(rightSteps)) / getDenominator(rightSteps),
      ((uint32_t)a0.y * getNumerator(downSteps)) / getDenominator(downSteps));
  };

  // Get mip0 without visual heights.
  get_texture_data(temp_mem, compressed, elementsStride, region_pivot, region_width);

  // Create mip-chain where we skip visual heights.
  IPoint2 prevRegionFlatMipPivot = getFlatMipPivot(0, region_width);
  for (int l = 1; l < levelCount; ++l)
  {
    auto [w, h] = region_width >> l;
    IPoint2 regionFlatMipPivot = getFlatMipPivot(l, region_width);

    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
      {
        unsigned sum = 0;
        sum += temp_mem[(prevRegionFlatMipPivot.y + 2 * y) * elementsStride + (prevRegionFlatMipPivot.x + 2 * x)];
        sum += temp_mem[(prevRegionFlatMipPivot.y + 2 * y) * elementsStride + (prevRegionFlatMipPivot.x + 2 * x + 1)];
        sum += temp_mem[(prevRegionFlatMipPivot.y + 2 * y + 1) * elementsStride + (prevRegionFlatMipPivot.x + 2 * x)];
        sum += temp_mem[(prevRegionFlatMipPivot.y + 2 * y + 1) * elementsStride + (prevRegionFlatMipPivot.x + 2 * x + 1)];
        temp_mem[(regionFlatMipPivot.y + y) * elementsStride + (regionFlatMipPivot.x + x)] = static_cast<uint16_t>(sum / 4);
      }

    prevRegionFlatMipPivot = regionFlatMipPivot;
  }

  // Get mip0 visual heights.
  get_visual_heights(temp_mem, visualHeights, elementsStride, region_pivot, hmapWidth.x, region_width);

  // Copy temp_mem to upload_tex.
  upload_texture_data(upload_texlock.get(), upload_texlock.getByteStride(), renderData->texFMT, temp_mem.data(),
    IPoint2(elementsStride, region_width.y));
  upload_texlock.close();

  // Copy upload_tex regions to proper hmap mips.
  for (int l = 0; l < levelCount && !NVworkaround_applyOnNextFrame; ++l)
  {
    auto [w, h] = region_width >> l;
    IPoint2 regionMipPivot = region_pivot >> l;
    IPoint2 regionFlatMipPivot = getFlatMipPivot(l, region_width);
    renderData->heightmap->updateSubRegion(upload_tex, 0, regionFlatMipPivot.x, regionFlatMipPivot.y, 0, w, h, 1, l, regionMipPivot.x,
      regionMipPivot.y, 0);
  }

  terrainStateVersion++;
}

void HeightmapHandler::close()
{
  del_it(metrics);
  del_it(metricsRenderer);
  hmapUploadTex.close();
  ShaderGlobal::set_texture(hmapMainVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tex_hmap_lowVarId, BAD_TEXTUREID);
  ShaderGlobal::set_float4(world_to_hmap_lowVarId, Color4(10e+3, 10e+3, 10e+10, 10e+10)); // 1mm^2 at (-10000Km, -10000Km)
  renderData.reset();
  renderer.close();
  heightmapHeightCulling.reset();
  HeightmapPhysHandler::close();
}

void HeightmapHandler::afterDeviceReset()
{
  renderer.close();
  renderer.init("heightmap", "", "hmap_tess_factor", false, hmapDimBits);
}

bool HeightmapHandler::init(int dim_bits)
{
  hmapDimBits =
    dim_bits <= 1 ? ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("heightmapDimBits", default_patch_bits) : dim_bits;

  if (!renderer.init("heightmap", "", "hmap_tess_factor", false, hmapDimBits))
    return false;

  return true;
}

void HeightmapHandler::setMaxUpwardDisplacement(float v)
{
  G_ASSERT_RETURN(v >= 0.0f, );
  maxUpwardDisplacement = v;
  if (heightmapHeightCulling)
    heightmapHeightCulling->setUpDisplacement(v);
}

void HeightmapHandler::setMaxDownwardDisplacement(float v)
{
  G_ASSERT_RETURN(v >= 0.0f, );
  maxDownwardDisplacement = v;
  if (heightmapHeightCulling)
    heightmapHeightCulling->setDownDisplacement(v);
}

void HeightmapHandler::invalidateCulling(const IBBox2 &ib)
{
  if (metrics)
    metrics->updateHeightBounds(*this, ib, false);
  if (!heightmapHeightCulling)
  {
    heightmapHeightCulling.reset(new HeightmapHeightCulling);
    heightmapHeightCulling->init(this);
  }
  else
    heightmapHeightCulling->updateMinMaxHeights(this, ib);
}

void HeightmapHandler::makeBookKeeping()
{
  if (!heightmapHeightCulling)
    invalidateCulling(IBBox2{{0, 0}, {hmapWidth.x - 1, hmapWidth.y - 1}});
  if (fillHmapTexturesNeeded)
  {
    fillHmapTextures();
    heightChangesIndex.clear();
    fillHmapTexturesNeeded = false;
  }
  if (pushHmapModificationOnPrepare)
    prepareHmapModificaton();
}

bool HeightmapHandler::prepare(const Point3 &world_pos)
{
  makeBookKeeping();
#if DAGOR_DBGLEVEL > 0
  // dev-only: react to runtime changes of the tessellation console vars. metrics/metricsRenderer are
  // created by initRender before any handler reaches prepare() - assert that invariant rather than
  // lazily creating them.
  G_ASSERT_RETURN(metricsRenderer, false);
  const int dimBits = metrics_dim;
  if (metricsRenderer->getDimBits() != dimBits)
  {
    del_it(metricsRenderer);
    metricsRenderer = new SimpleHeightmapRenderer();
    metricsRenderer->init("heightmap", true, dimBits);
  }
  metrics->calc_lod_errors(*this, metrics_minCalcLevel, metrics_maxCalcLevel, 1 << dimBits, metrics->water_level, shoreErrorMeters);
#endif

  preparedOriginPos = world_pos;
  setVars();
  float distanceToSwitch = hmapDistanceMul * max(worldSize.x, worldSize.y);
  if (lengthSq(getClippedOrigin(world_pos) - world_pos) > distanceToSwitch * distanceToSwitch)
    return false;

  return true;
}

void HeightmapHandler::prepareHmapModificaton()
{
  G_ASSERTF_RETURN(!fillHmapTexturesNeeded, , "prepareHmapModificaton: Full fillHmapTextures is needed");
#if ENABLE_RENDER_HMAP_MODIFICATION
  const bool applyOnNextFrame = nvidia_551_workaround && d3d::get_driver_desc().issues.hasMultipleCopySubresourceBug;
  if (applyOnNextFrame && lastRegionUpdated_NVworkaround != -1)
  {
    fillHmapRegion(lastRegionUpdated_NVworkaround, false);
    lastRegionUpdated_NVworkaround = -1;
  }
  else if (!heightChangesIndex.empty())
  {
    if (fillHmapRegion(*heightChangesIndex.begin(), applyOnNextFrame))
    {
      lastRegionUpdated_NVworkaround = *heightChangesIndex.begin();
      heightChangesIndex.erase(heightChangesIndex.begin());
    }
  }
#endif
}

void cull_lod_grid3(const MetricsErrors &errors, LodGridCullData &cull_data, const Frustum &frustum, const Occlusion *use_occlusion,
  const HeightmapHandler &h, vec3f vp, const vec4f *bbox2, const HeightmapMetricsQuality &q)
{
  cull_lod_grid3(errors, cull_data, frustum, use_occlusion, h.worldBox[0].y, h.worldBox[1].y, h.worldBox2,
    h.getMaxUpwardDisplacement(), h.getMaxDownwardDisplacement(), h.isMirror(), vp, bbox2, q);
}

void frustumCulling(const MetricsErrors &errors, LodGridCullData &cull_data, const Point3 &world_pos, const Frustum &frustum,
  const Occlusion *occlusion, const HeightmapHandler &handler, const vec4f *bbox2, const HeightmapMetricsQuality &q)
{
  TIME_PROFILE(htmap_frustum_cull2);
  cull_lod_grid3(errors, cull_data, frustum, occlusion, handler, v_ldu(&world_pos.x), bbox2, q);
}


// float hm_scale = 1.0f;

void HeightmapHandler::frustumCulling(LodGridCullData &cull_data, const HeightmapFrustumCullingInfo &fi)
{
  // if (fi.proj == TMatrix4::IDENT)
  // debug_dump_stack();
  // metrics are always built for the rendering heightmap (see initRender / prepare), so the
  // non-metrics distance-based cull is dead. Assert the invariant; on release degrade to empty cull.
  cull_data.eraseAll();
  G_ASSERT_RETURN(metrics, );
  ::frustumCulling(*metrics, cull_data, fi.world_pos, fi.frustum, fi.occlusion, *this, fi.world_bbox_xzs, fi.metrics);
}

void HeightmapHandler::renderCulled(const LodGridCullData &cullData)
{
  if (!cullData.hasPatches())
    return;
  // metrics/metricsRenderer are always built for the rendering heightmap (see initRender / prepare);
  // the plain renderer.render() fallback is dead. Assert the invariant; on release degrade to no-op.
  G_ASSERT_RETURN(metrics && metricsRenderer, );
  G_ASSERT(metrics->dim == metricsRenderer->getDim());
  metricsRenderer->render(cullData, metricsRenderer->getShElem(), metricsRenderer->getDimBits());
}

void HeightmapHandler::renderOnePatch()
{
  mat44f globtm;
  d3d::getglobtm(globtm);
  if (!Frustum(globtm).testBoxB(vecbox.bmin, vecbox.bmax))
    return;
  renderer.renderOnePatch(Point2::xz(worldBox[0]), Point2::xz(worldBox[1]));
}

void HeightmapHandler::setSampler(d3d::SamplerHandle &&s)
{
  if (renderData)
    renderData->heightmapSampler = (d3d::SamplerHandle &&)s;
}

void HeightmapHandler::setTexture(UniqueTex &&u)
{
  if (!renderData)
    return;
  renderData->heightmap = (UniqueTex &&)u;
  if (renderData->heightmap.getTex2D())
  {
    TextureInfo ti;
    renderData->heightmap->getinfo(ti, 0);
    renderData->texFMT = ti.cflg & TEXFMT_MASK;
  }
}
