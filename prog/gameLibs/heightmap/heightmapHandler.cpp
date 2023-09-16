#include <heightmap/heightmapHandler.h>
#include <heightMapLand/dag_hmlGetHeight.h>
#include <heightMapLand/dag_hmlTraceRay.h>
#include <ioSys/dag_baseIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <3d/dag_drv3d.h>
#include <math/dag_bounds2.h>
#include <math/dag_frustum.h>
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

#include <perfMon/dag_statDrv.h>


// Used for dynamic terraforms
#define ENABLE_RENDER_HMAP_MODIFICATION    1
#define ENABLE_RENDER_HMAP_SUB_TESSELATION 1


static int hmapLdetailVarId = -1;

static int heightmapTexVarId = -1, world_to_hmap_lowVarId = -1, tex_hmap_inv_sizesVarId = -1;
static int heightmap_scaleVarId = -1;

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
static int select_hmap_tex_levels(const IPoint2 &hmapWidth) { return min(get_log2i(hmapWidth.x), get_log2i(hmapWidth.y)); }

#define PHYS_MEM_THRESHOLD_SIZE (32 << 20)

static inline void *alloc_hmap_data(size_t sz)
{
#if _TARGET_STATIC_LIB
  using namespace dagor_phys_memory;
  if (sz >= PHYS_MEM_THRESHOLD_SIZE)
    return alloc_phys_mem(sz, PM_ALIGN_PAGE, PM_PROT_CPU_ALL, /*cpu_cached*/ true);
  else
#endif
    return tmpmem->tryAlloc(sz);
}

static inline void free_hmap_data(void *ptr, size_t sz)
{
#if _TARGET_STATIC_LIB
  if (sz >= PHYS_MEM_THRESHOLD_SIZE)
    dagor_phys_memory::free_phys_mem(ptr);
  else
#endif
    memfree(ptr, tmpmem);
}

bool HeightmapHandler::loadDump(IGenLoad &loadCb, bool load_render_data, GlobalSharedMemStorage *sharedMem)
{
  const int skip_mips = clamp(::dgs_get_settings()->getBlockByNameEx("debug")->getInt("skip_hmap_levels", 0), 0, 3);

  if (!HeightmapPhysHandler::loadDump(loadCb, sharedMem, skip_mips))
    return false;

  hmapTData.init(1 << max(hmapDimBits - 2, 2),
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("hmapTessCellSize", hmapCellSize));

  if (load_render_data)
  {
    heightmapTexVarId = get_shader_variable_id("tex_hmap_low");
    world_to_hmap_lowVarId = get_shader_variable_id("world_to_hmap_low");
    tex_hmap_inv_sizesVarId = get_shader_variable_id("tex_hmap_inv_sizes", true);
    ShaderGlobal::set_color4(world_to_hmap_lowVarId,
      Color4(1.0f / worldSize.x, 1.0f / worldSize.y, -worldPosOfs.x / worldSize.x, -worldPosOfs.y / worldSize.y));

    ShaderGlobal::set_color4(tex_hmap_inv_sizesVarId,
      Color4(1.0f / hmapWidth.x, 1.0f / hmapWidth.y, 1.0f / hmapWidth.x, 1.0f / hmapWidth.y));

    heightmap_scaleVarId = get_shader_variable_id("heightmap_scale");
    ShaderGlobal::set_color4(heightmap_scaleVarId, Color4(hScale, hMin, hScale, hMin));

    renderData.reset(new HeightmapRenderData);
    uint32_t texFMT = select_hmap_tex_fmt();
    int level_count = select_hmap_tex_levels(hmapWidth);
    debug("heightmap tex format = 0x%X", texFMT);
    renderData->heightmap = UniqueTex(
      dag::create_tex(NULL, hmapWidth.x, hmapWidth.y, texFMT | TEXCF_MAYBELOST | TEXCF_UPDATE_DESTINATION, level_count, "hmapMain"),
      "land_heightmap_tex");
    renderData->heightmap->texaddr(TEXADDR_CLAMP);
    d3d_err(renderData->heightmap.getTex2D());

    renderData->diamondSubDiv = (texFMT == TEXFMT_L16 && (d3d::get_texformat_usage(texFMT) & d3d::USAGE_FILTER));
    debug("use diamond sub div = %d", renderData->diamondSubDiv);

    hmapBuffer = dag::create_tex(NULL, HMAP_BSIZE, HMAP_BSIZE, texFMT | TEXCF_DYNAMIC, 1, "hmap_buffer");
    hmapBuffer.getTex2D()->texaddr(TEXADDR_CLAMP);

    fillHmapTextures();

    renderer.setRenderClip(&worldBox2);
  }
  return true;
}

static void copy_texture_data(uint8_t *dst, const uint16_t *src, uint32_t texFMT, int stride, IPoint2 src_pivot, int src_pitch,
  IPoint2 region_width)
{
  src += src_pivot.y * src_pitch + src_pivot.x;
  if (texFMT == TEXFMT_L16)
  {
    for (int y = 0; y < region_width.y; ++y, dst += stride, src += src_pitch)
      memcpy(dst, src, region_width.x * sizeof(uint16_t));
  }
  else if (texFMT == TEXFMT_R32F)
  {
    for (int y = 0; y < region_width.y; ++y, dst += stride, src += src_pitch)
      for (int x = 0; x < region_width.x; ++x)
        ((float *)dst)[x] = src[x] / float(UINT16_MAX);
  }
  else // if (texFMT == TEXFMT_R16F)
  {
    for (int y = 0; y < region_width.y; ++y, dst += stride, src += src_pitch)
      for (int x = 0; x < region_width.x; ++x)
        ((uint16_t *)dst)[x] = float_to_half(src[x] / float(UINT16_MAX));
  }
}

static void copy_texture_data(uint8_t *dst, const CompressedHeightmap &src, uint32_t texFMT, int stride, IPoint2 src_pivot,
  IPoint2 region_width)
{
  if (texFMT == TEXFMT_L16)
  {
    for (int y = 0, sy = src_pivot.y; y < region_width.y; ++y, dst += stride, ++sy)
      for (int x = 0, sx = src_pivot.x; x < region_width.x; ++x, ++sx)
        ((uint16_t *)dst)[x] = src.decodePixelUnsafe(sx, sy);
  }
  else if (texFMT == TEXFMT_R32F)
  {
    for (int y = 0, sy = src_pivot.y; y < region_width.y; ++y, dst += stride, ++sy)
      for (int x = 0, sx = src_pivot.x; x < region_width.x; ++x, ++sx)
        ((float *)dst)[x] = src.decodePixelUnsafe(sx, sy) / float(UINT16_MAX);
  }
  else // if (texFMT == TEXFMT_R16F)
  {
    for (int y = 0, sy = src_pivot.y; y < region_width.y; ++y, dst += stride, ++sy)
      for (int x = 0, sx = src_pivot.x; x < region_width.x; ++x, ++sx)
        ((uint16_t *)dst)[x] = float_to_half(src.decodePixelUnsafe(sx, sy) / float(UINT16_MAX));
  }
}

static void copy_visual_heights(uint8_t *dst, const ska::flat_hash_map<uint32_t, uint16_t> &visualHeights, uint32_t texFMT, int stride,
  IPoint2 src_pivot, int src_pitch, IPoint2 region_width)
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
    if (texFMT == TEXFMT_L16)
      *reinterpret_cast<uint16_t *>(dst + cell.y * stride + cell.x * sizeof(uint16_t)) = iter.second;
    else if (texFMT == TEXFMT_R32F)
      *reinterpret_cast<float *>(dst + cell.y * stride + cell.x * sizeof(float)) = iter.second / float(UINT16_MAX);
    else // if (texFMT == TEXFMT_R16F)
      *reinterpret_cast<uint16_t *>(dst + cell.y * stride + cell.x * sizeof(uint16_t)) =
        float_to_half(iter.second / float(UINT16_MAX));
  }
}


void HeightmapHandler::fillHmapTextures() { fillHmapRegion(-1); }

void HeightmapHandler::fillHmapRegion(int region_index)
{
  TIME_D3D_PROFILE(hmap_fillHmapRegion);
  bool isRegion = region_index != -1;
  int regionStride = hmapWidth.x / HMAP_BSIZE;
  const IPoint2 regionPivot =
    isRegion ? IPoint2((region_index % regionStride) * HMAP_BSIZE, (region_index / regionStride) * HMAP_BSIZE) : IPoint2(0, 0);
  const IPoint2 regionWidth = isRegion ? IPoint2(HMAP_BSIZE, HMAP_BSIZE) : IPoint2(hmapWidth.x, hmapWidth.y);

  uint32_t texFMT = select_hmap_tex_fmt(); // as we can't get format from texture now!
  int level_count = renderData->heightmap.getTex2D()->level_count();
  Texture *destTex = isRegion ? hmapBuffer.getTex2D() : renderData->heightmap.getTex2D();

#if _TARGET_XBOX
  const bool useUpdateBufferRegion = isRegion; // it works for full update as well, but needs extra VRAM for the allocation
#else
  const bool useUpdateBufferRegion = false; // TODO: enable it if Xbox works fine
#endif

  if (useUpdateBufferRegion)
  {
    if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex_region(renderData->heightmap.getTex2D(), 0, 0, regionPivot.x,
          regionPivot.y, 0, regionWidth.x, regionWidth.y, 1))
    {
      uint8_t *data = (uint8_t *)d3d::get_update_buffer_addr_for_write(rub);
      int stride = d3d::get_update_buffer_pitch(rub);

      if (!data)
      {
        if (!d3d::is_in_device_reset_now())
          logerr("%s can't lock heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
        return;
      }

      copy_texture_data(data, compressed, texFMT, stride, regionPivot, regionWidth);
      copy_visual_heights(data, visualHeights, texFMT, stride, regionPivot, hmapWidth.x, regionWidth);

      if (!d3d::update_texture_and_release_update_buffer(rub))
      {
        if (!d3d::is_in_device_reset_now())
          logerr("%s can't update heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
        return;
      }
      d3d::release_update_buffer(rub);
    }
    else
    {
      if (!d3d::is_in_device_reset_now())
        logerr("%s can't lock heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
      return;
    }
  }
  else
  {
    int stride;
    if (auto lockedTex = lock_texture<uint8_t>(destTex, stride, 0,
          TEXLOCK_WRITE | (isRegion ? TEXLOCK_DISCARD : (level_count > 1 ? TEXLOCK_DONOTUPDATEON9EXBYDEFAULT : 0))))
    {
      uint8_t *data = lockedTex.get();
      copy_texture_data(data, compressed, texFMT, stride, regionPivot, regionWidth);
      copy_visual_heights(data, visualHeights, texFMT, stride, regionPivot, hmapWidth.x, regionWidth);
    }

    if (isRegion)
      renderData->heightmap->updateSubRegion(hmapBuffer.getTex2D(), 0, 0, 0, 0, regionWidth.x, regionWidth.y, 1, 0, regionPivot.x,
        regionPivot.y, 0);
  }

  // !isRegion means initial (or after reset) filling
  if (!isRegion || enabledMipsUpdating)
  {
    SmallTab<uint16_t, TmpmemAlloc> downsampledData;
    clear_and_resize(downsampledData, (regionWidth.y * regionWidth.x) >> 2);
    for (int l = 1, w = regionWidth.x >> 1, h = regionWidth.y >> 1; min(h, w) > 0 && l < level_count; ++l, w >>= 1, h >>= 1)
    {
      uint16_t *destDownData = downsampledData.data();
      for (int y = 0; y < h; ++y, destDownData += w)
        if (l == 1)
        {
          int sy = regionPivot.y + y * 2;
          for (int x = 0, sx = regionPivot.x; x < w; ++x, sx += 2)
            destDownData[x] = (compressed.decodePixelUnsafe(sx + 0, sy + 0) + compressed.decodePixelUnsafe(sx + 1, sy + 0) +
                                compressed.decodePixelUnsafe(sx + 0, sy + 1) + compressed.decodePixelUnsafe(sx + 1, sy + 1)) /
                              4;
        }
        else
        {
          int srcStride = w << 1;
          uint16_t *srcData = &downsampledData[(y << 1) * srcStride];
          for (int x = 0; x < w; ++x, srcData += 2)
            destDownData[x] = (srcData[0] + srcData[1] + srcData[srcStride] + srcData[srcStride + 1]) / 4;
        }
      destDownData = downsampledData.data();

      if (useUpdateBufferRegion)
      {
        if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex_region(renderData->heightmap.getTex2D(), l, 0,
              regionPivot.x >> l, regionPivot.y >> l, 0, w, h, 1))
        {
          uint8_t *data = (uint8_t *)d3d::get_update_buffer_addr_for_write(rub);
          int stride = d3d::get_update_buffer_pitch(rub);

          if (!data)
          {
            if (!d3d::is_in_device_reset_now())
              logerr("%s can't lock heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
            continue;
          }

          copy_texture_data(data, destDownData, texFMT, stride, IPoint2::ZERO, w, IPoint2(w, h));

          if (!d3d::update_texture_and_release_update_buffer(rub))
          {
            if (!d3d::is_in_device_reset_now())
              logerr("%s can't update heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
            continue;
          }
          d3d::release_update_buffer(rub);
        }
        else
        {
          if (!d3d::is_in_device_reset_now())
            logerr("%s can't lock heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
          continue;
        }
      }
      else
      {
        uint32_t stagingBufferMip = isRegion ? 0 : l;
        int stride;
        if (auto lockedTex = lock_texture<uint8_t>(destTex, stride, stagingBufferMip,
              TEXLOCK_WRITE |
                (isRegion ? TEXLOCK_DISCARD : (l == level_count - 1 ? TEXLOCK_DELSYSMEMCOPY : TEXLOCK_DONOTUPDATEON9EXBYDEFAULT))))
        {
          uint8_t *data = lockedTex.get();

          if (!data)
          {
            if (!d3d::is_in_device_reset_now())
              logerr("%s can't lock heightmapTex '%s'", __FUNCTION__, d3d::get_last_error());
            continue;
          }

          copy_texture_data(data, destDownData, texFMT, stride, IPoint2::ZERO, w, IPoint2(w, h));
        }

        if (isRegion)
          renderData->heightmap->updateSubRegion(hmapBuffer.getTex2D(), stagingBufferMip, 0, 0, 0, w, h, 1, l, regionPivot.x >> l,
            regionPivot.y >> l, 0);
      }
    }
  }
  terrainStateVersion++;
}

void HeightmapHandler::close()
{
  hmapBuffer.close();
  ShaderGlobal::set_texture(heightmapTexVarId, BAD_TEXTUREID);
  renderData.reset();
  renderer.close();
  heightmapHeightCulling.reset();
  HeightmapPhysHandler::close();
}

void HeightmapHandler::afterDeviceReset()
{
  renderer.close();
  renderer.init("heightmap", "", false, hmapDimBits);
}

bool HeightmapHandler::init()
{
  hmapDimBits = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("heightmapDimBits", default_patch_bits);

  if (!renderer.init("heightmap", "", false, hmapDimBits))
    return false;

  hmapLdetailVarId = get_shader_variable_id("hmap_ldetail", true);
  return true;
}

int HeightmapHandler::calcLod(int min_lod, const Point3 &origin_pos, float camera_height) const
{
  int maxLod = lodCount - 1;
  int lod = (length(Point2::xz(getClippedOrigin(origin_pos)) - Point2::xz(origin_pos)) / renderer.getDim()) * 0.5f;
  lod += max(0.f, camera_height) * 0.003f;
  lod = clamp(lod, min(min_lod, maxLod), maxLod);
  return lod;
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
  if (!heightmapHeightCulling)
  {
    heightmapHeightCulling.reset(new HeightmapHeightCulling);
    heightmapHeightCulling->init(this);
  }
  else
    heightmapHeightCulling->updateMinMaxHeights(this, ib);
}

bool HeightmapHandler::prepare(const Point3 &world_pos, float camera_height)
{
  if (!heightmapHeightCulling)
    invalidateCulling(IBBox2{{0, 0}, {hmapWidth.x - 1, hmapWidth.y - 1}});
  preparedOriginPos = world_pos;
  preparedCameraHeight = camera_height;
  ShaderGlobal::set_texture(heightmapTexVarId, renderData->heightmap);
  float distanceToSwitch = hmapDistanceMul * max(worldSize.x, worldSize.y);
  if (lengthSq(getClippedOrigin(world_pos) - world_pos) > distanceToSwitch * distanceToSwitch)
    return false;

#if ENABLE_RENDER_HMAP_MODIFICATION
  if (!heightChangesIndex.empty())
  {
#if _TARGET_APPLE
    // Workaround for terraforming bug
    fillHmapTextures();
    heightChangesIndex.clear();
#else
    fillHmapRegion(*heightChangesIndex.begin());
    heightChangesIndex.erase(heightChangesIndex.begin());
#endif
  }
#endif

  return true;
}

void HeightmapHandler::render(int min_tank_lod)
{
  mat44f globtm;
  d3d::getglobtm(globtm);
  LodGridCullData defaultCullData(framemem_ptr());
  frustumCulling(defaultCullData, preparedOriginPos, preparedCameraHeight, Frustum(globtm), min_tank_lod, NULL, 0);
  renderCulled(defaultCullData);
}

void HeightmapHandler::setVsSampler()
{
  if (hmapLdetailVarId >= 0)
    ShaderGlobal::set_texture(hmapLdetailVarId, renderData->heightmap);
}

void HeightmapHandler::resetVsSampler()
{
  if (hmapLdetailVarId >= 0)
    ShaderGlobal::set_texture(hmapLdetailVarId, BAD_TEXTUREID);
}

// float hm_scale = 1.0f;

void HeightmapHandler::frustumCulling(LodGridCullData &cull_data, const Point3 &world_pos, float camera_height, const Frustum &frustum,
  int min_tank_lod, const Occlusion *occlusion, int lod0subdiv, float lod0scale)
{
  if (!frustum.testBoxB(vecbox.bmin, vecbox.bmax))
  {
    cull_data.eraseAll();
    return;
  }

  const auto hmtd = (ENABLE_RENDER_HMAP_SUB_TESSELATION && hmapTData.hasTesselation()) ? &hmapTData : nullptr;
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  auto doFrustumCull = [&]() {
#else
  TIME_PROFILE(htmap_frustum_cull);
#endif
    int lastLodRepeat =
      max(1, (int)(1 + safediv(max(worldSize.x, worldSize.y), renderer.getDim() * hmapCellSize * lod0scale * (1 << (lodCount - 1)))));

    cull_data.lodGrid.init(lodCount, lodDistance, lod0subdiv, lodDistance * lastLodRepeat); // changing 1,lod0subdiv, 1 to
                                                                                            // 3,lod0subdiv,3 will get much longer
                                                                                            // distance of high quality tesselation
                                                                                            // (helpful  in zoom)
    int lod = calcLod(min_tank_lod, world_pos, camera_height);
    if (lod > 0)
      cull_data.lodGrid.lod0SubDiv = 0;

    // unfortunately, although if we use (1<<(lodGrid.lodsCount-1)) snapping all verices stays, but triangulation pops (because we use
    // diamond subdivision) can be fix, if we will swap subdivision as well.
    float scale = hmapCellSize * (1 << lod) * lod0scale;
    float align = renderer.getDim() * scale;

    // float align = (1<<(lodGrid.lodsCount-1))*hmapCellSize;
    float lod0AreaSize = 0.f;
    Point3 clippedOrigin = getClippedOrigin(world_pos);
    cull_lod_grid(cull_data.lodGrid, cull_data.lodGrid.lodsCount - min_tank_lod, clippedOrigin.x, clippedOrigin.z, scale, scale, align,
      align, worldBox[0].y, worldBox[1].y, &frustum, &worldBox2, cull_data, occlusion, lod0AreaSize, renderer.getDim(), true,
      heightmapHeightCulling.get(), hmtd);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
  };
  if (!hmtd)
  {
    TIME_PROFILE(htmap_frustum_cull);
    doFrustumCull();
  }
  else
  {
    TIME_PROFILE(htmap_tesselated_frustum_cull);
    doFrustumCull();
  }
#endif
}

void HeightmapHandler::renderCulled(const LodGridCullData &cullData)
{
  if (!cullData.patches.size())
    return;
  setVsSampler();
  renderer.render(cullData.lodGrid, cullData);
  resetVsSampler();
}

void HeightmapHandler::renderOnePatch()
{
  mat44f globtm;
  d3d::getglobtm(globtm);
  if (!Frustum(globtm).testBoxB(vecbox.bmin, vecbox.bmax))
    return;
  if (hmapLdetailVarId >= 0)
    ShaderGlobal::set_texture(hmapLdetailVarId, renderData->heightmap);
  renderer.renderOnePatch(Point2::xz(worldBox[0]), Point2::xz(worldBox[1]));
  if (hmapLdetailVarId >= 0)
    ShaderGlobal::set_texture(hmapLdetailVarId, BAD_TEXTUREID);
}
