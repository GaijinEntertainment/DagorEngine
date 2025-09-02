// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapPhysHandler.h>
#include <heightMapLand/dag_hmlGetHeight.h>
#include <heightMapLand/dag_hmlTraceRay.h>
#include <ioSys/dag_baseIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathUtils.h>
#include <util/dag_finally.h>
#include <math/dag_adjpow2.h>
#include <memory/dag_physMem.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <supp/dag_alloca.h>

#define PHYS_MEM_THRESHOLD_SIZE (32 << 20) // 4K^2*sizeof(uint16_t)

static inline void *alloc_hmap_data(size_t sz, bool failable = false)
{
#if !_TARGET_PC_TOOLS_BUILD
  using namespace dagor_phys_memory;
  if (void *p = (sz >= PHYS_MEM_THRESHOLD_SIZE) ? alloc_phys_mem(sz, PM_ALIGN_PAGE, PM_PROT_CPU_ALL, /*cpu_cached*/ true) : NULL)
    return p;
#endif
  return (failable ? tmpmem->tryAlloc(sz) : tmpmem->alloc(sz));
}

static inline void free_hmap_data(void *ptr, size_t sz)
{
#if !_TARGET_PC_TOOLS_BUILD
  if (sz >= PHYS_MEM_THRESHOLD_SIZE && dagor_phys_memory::free_phys_mem(ptr, /*ignore_unknown*/ true))
    return;
#endif
  memfree(ptr, tmpmem);
}

void HeightmapPhysHandler::finalizeLoad()
{
  uint16_t minH16 = 65535, maxH16 = 0;
  compressed.iterateBlocks(0, 0, compressed.bw, compressed.bh, [&](uint32_t, uint32_t, uint16_t mn, uint16_t mx) {
    minH16 = min(mn, minH16);
    maxH16 = max(mx, maxH16);
  });
  worldSize = Point2(hmapWidth.x * hmapCellSize, hmapWidth.y * hmapCellSize);
  worldBox[0] = Point3(worldPosOfs.x, hMin + minH16 * hScaleRaw, worldPosOfs.y);
  worldBox[1] = Point3(worldPosOfs.x + worldSize.x, hMin + maxH16 * hScaleRaw, worldPosOfs.y + worldSize.y);
  worldBox2 = BBox2(Point2::xz(worldBox[0]), Point2::xz(worldBox[1]));
  // worldBox[0] = Point3(worldPosOfs.x, hMin, worldPosOfs.y);
  // worldBox[1] = Point3(worldPosOfs.x+worldSize.x, hMin+hScale, worldPosOfs.y+worldSize.y);
  vecbox = v_ldu_bbox3(worldBox);

  invElemSize = v_splats(1.0f / hmapCellSize);
  v_worldOfsxzxz = v_make_vec4f(worldPosOfs.x, worldPosOfs.y, worldPosOfs.x, worldPosOfs.y);
}

void HeightmapPhysHandler::initRaw(const uint16_t *raw, float cell, uint16_t w, uint16_t h, float hmin, float hscale,
  const Point2 &ofs, uint8_t blockShift, uint8_t hrbSubSz)
{
  close();

  hmapCellSize = cell;
  hMin = hmin;
  hScale = hscale;
  hScaleRaw = hscale / 65535.0;
  worldPosOfs = ofs;
  hmapWidth.x = w;
  hmapWidth.y = h;
  excludeBounding = IBBox2{{0, 0}, {0, 0}};

  const size_t allocatedDataSize = CompressedHeightmap::calc_data_size_needed(hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);

  hmap_data = alloc_hmap_data(allocatedDataSize, /*failable*/ true);
  if (!hmap_data)
  {
    logerr("%s failed to allocate %dK", __FUNCTION__, allocatedDataSize >> 10);
    return;
  }
  compressed = CompressedHeightmap::compress((uint8_t *)hmap_data, allocatedDataSize, //
    raw, hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);

  finalizeLoad();
}

bool HeightmapPhysHandler::loadDump(IGenLoad &loadCb, GlobalSharedMemStorage *sharedMem, int skip_mips)
{
  hmapCellSize = loadCb.readReal() * (1 << skip_mips);
  hMin = loadCb.readReal();
  hScale = loadCb.readReal();
  hScaleRaw = hScale / 65535.0;
  worldPosOfs.x = loadCb.readReal();
  worldPosOfs.y = loadCb.readReal();
  int width_version = loadCb.readInt();
  int version = width_version >> HMAP_WIDTH_BITS;
  hmapWidth.x = (width_version & HMAP_WIDTH_MASK) >> skip_mips;
  hmapWidth.y = (loadCb.readInt() >> skip_mips);
  excludeBounding[0].x = loadCb.readInt();
  excludeBounding[0].y = loadCb.readInt();
  excludeBounding[1].x = loadCb.readInt();
  excludeBounding[1].y = loadCb.readInt();
  uint32_t chunkSz = (version == (int)HmapVersion::HMAP_CBLOCK_DELTAC_VER) ? loadCb.readInt() : 0x403;
  uint8_t blockShift = chunkSz & 0xFF;
  uint8_t hrbSubSz = (chunkSz & 0xF00) ? (1 << ((chunkSz >> 8) & 0xF)) : 0;
  const size_t allocatedDataSize = CompressedHeightmap::calc_data_size_needed(hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);

  GlobalSharedMemStorage *sm = sharedMem;
  bool own_data = false;
  int t0_msec = 0;
  if (sm && (hmap_data = sm->findPtr(loadCb.getTargetName(), _MAKE4C('HMAP'))) != NULL)
  {
    int dump_sz = (int)sm->getPtrSize(hmap_data);
    G_ASSERT(allocatedDataSize <= dump_sz);
    logmessage(_MAKE4C('SHMM'), "reusing HMAP dump from shared mem: : %p, %dK, '%s'", hmap_data, dump_sz >> 10,
      loadCb.getTargetName());
    compressed = CompressedHeightmap::init((uint8_t *)hmap_data, allocatedDataSize, hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);
  }
  else
  {
    if (sm)
    {
      hmap_data = sm->allocPtr(loadCb.getTargetName(), _MAKE4C('HMAP'), allocatedDataSize);
      if (hmap_data)
        logmessage(_MAKE4C('SHMM'), "allocated HMAP dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d)", hmap_data,
          allocatedDataSize >> 10, loadCb.getTargetName(), ((uint64_t)sm->getMemUsed()) >> 10, ((uint64_t)sm->getMemSize()) >> 10,
          sm->getRecUsed());
      else
        logmessage(_MAKE4C('SHMM'),
          "failed to allocate HMAP dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d); "
          "falling back to conventional allocator",
          hmap_data, allocatedDataSize >> 10, loadCb.getTargetName(), ((uint64_t)sm->getMemUsed()) >> 10,
          ((uint64_t)sm->getMemSize()) >> 10, sm->getRecUsed());
    }
    if (!hmap_data)
    {
      hmap_data = alloc_hmap_data(allocatedDataSize, /*failable*/ true);
      if (!hmap_data)
      {
        logerr("%s failed to allocate %dK", __FUNCTION__, allocatedDataSize >> 10);
        return false;
      }
      own_data = true;
    }

    t0_msec = get_time_msec();

    int orig_width = hmapWidth.x << skip_mips, orig_height = hmapWidth.y << skip_mips;
    if (version == (int)HmapVersion::HMAP_CBLOCK_DELTAC_VER)
    {
      if (!skip_mips)
      {
        compressed =
          CompressedHeightmap::init((uint8_t *)hmap_data, allocatedDataSize, hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);
        CompressedHeightmap::loadData(compressed, loadCb, chunkSz & ~0xFFFu);
      }
      else
      {
        size_t hdataSize = CompressedHeightmap::calc_data_size_needed(orig_width, orig_height, blockShift, hrbSubSz);
        uint8_t *height_data = (uint8_t *)alloc_hmap_data(hdataSize);
        compressed = CompressedHeightmap::init(height_data, hdataSize, orig_width, orig_height, blockShift, hrbSubSz);
        CompressedHeightmap::loadData(compressed, loadCb, chunkSz & ~0xFFFu);
        compressed = CompressedHeightmap::downsample((uint8_t *)hmap_data, allocatedDataSize, hmapWidth.x, hmapWidth.y, compressed);
        free_hmap_data(height_data, hdataSize);
      }
    }
    else
    {
      unsigned uncompressed_size = hmapWidth.x * hmapWidth.y * sizeof(uint16_t);
      uint16_t *const heightmapData = (uint16_t *)alloc_hmap_data(uncompressed_size), *heightmapDataOrig = nullptr;
      Finally freeHeightmapData([&] { free_hmap_data(heightmapData, hmapWidth.x * hmapWidth.y * sizeof(uint16_t)); });
      if (skip_mips)
      {
        uncompressed_size = orig_width * orig_height * sizeof(uint16_t);
        heightmapDataOrig = (uint16_t *)alloc_hmap_data(uncompressed_size);
      }

      unsigned fmt = 0;
      loadCb.beginBlock(&fmt);
      IGenLoad *zcrd_p = NULL;
      if (fmt == btag_compr::OODLE)
        zcrd_p = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(loadCb, loadCb.getBlockRest(), uncompressed_size);
      else if (fmt == btag_compr::ZSTD)
        zcrd_p = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(loadCb, loadCb.getBlockRest());
      else
        zcrd_p = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(loadCb, loadCb.getBlockRest());
      IGenLoad &zcrd = *zcrd_p;

      zcrd.read(!skip_mips ? heightmapData : heightmapDataOrig, uncompressed_size);
      zcrd.ceaseReading();
      zcrd.~IGenLoad();

      if (version == (int)HmapVersion::HMAP_DELTA_COMPRESSION_VER)
      {
        uint16_t *hmap = skip_mips ? heightmapDataOrig : heightmapData;
        for (int y = 0; y < orig_height; ++y, hmap += orig_width)
          for (int x = 1; x < orig_width; ++x)
            hmap[x] = (uint16_t)(int(hmap[x]) + hmap[x - 1]); //-V522
      }

      if (skip_mips)
      {
        uint16_t *hmap = heightmapData;
        for (int y = 0; y < hmapWidth.y; ++y)
          for (int x = 0; x < hmapWidth.x; ++x)
          {
            int average = 0, size = 1 << skip_mips;
            for (int mip_y = 0; mip_y < size; ++mip_y)
              for (int mip_x = 0; mip_x < size; ++mip_x)
                average += heightmapDataOrig[(x + y * orig_width) * size + mip_x + mip_y * orig_width]; //-V522
            *hmap++ = average >> (2 * skip_mips);
          }
        free_hmap_data(eastl::exchange(heightmapDataOrig, nullptr), uncompressed_size);
      }

      compressed = CompressedHeightmap::compress((uint8_t *)hmap_data, allocatedDataSize, //
        heightmapData, hmapWidth.x, hmapWidth.y, blockShift, hrbSubSz);
      G_ASSERT(compressed);

#if DAGOR_DBGLEVEL > 0
      double mse = 0;
      uint16_t maxErr = 0;
      compressed.iterateBlocksPixels(0, 0, compressed.bw, compressed.bh, [&](uint32_t x, uint32_t y, const uint16_t v) {
        const uint16_t err = abs(heightmapData[x + y * hmapWidth.x] - v);
        maxErr = max(maxErr, err);
        mse += double(uint32_t(err) * err);
      });
      mse /= double(hmapWidth.y * hmapWidth.x);
      mse = sqrt(mse);
      extern void report_heightmap_error(uint32_t bl, uint16_t max_err, double mse, float scale);
      report_heightmap_error(compressed.block_width, maxErr, mse, hScaleRaw);
#endif
      loadCb.endBlock();
    }
  }
  if (!own_data)
    hmap_data = NULL;

  finalizeLoad();
  unsigned grid_res = compressed.getBestHtRangeBlocksResolution(), grid_step = compressed.getW() / grid_res;
  debug("heightmap of size %dx%d at %@, minH=%g(real=%g) maxH=%g(real=%g)  memSz=%dK (%s), hier-grid %dx%d,L%d (blocks of %dx%d)"
        " [fmt=%d:%dK skip=%d] decoded for %d ms",
    hmapWidth.x, hmapWidth.y, worldPosOfs, hMin, worldBox[0].y, hMin + hScale, worldBox[1].y, compressed.dataSizeCurrent() >> 10,
    hmap_data ? "owned" : "shared", grid_res, grid_res, compressed.htRangeBlocksLevels, grid_step, grid_step, version, chunkSz >> 10,
    skip_mips, t0_msec ? get_time_msec() - t0_msec : 0);
  return true;
}

bool HeightmapPhysHandler::repackCompressedData(uint8_t use_hrb_subsz)
{
  unsigned w = compressed.bw * compressed.block_width, h = compressed.bh * compressed.block_width;
  uint8_t block_shift = compressed.block_size_shift;

  // validate use_hrb_subsz
  if (w != h)
    use_hrb_subsz = 0;
  else if (use_hrb_subsz * 2 > w)
    use_hrb_subsz = w / 2;
  uint8_t hrb_subsz_bits = use_hrb_subsz > 1 ? __bsf(use_hrb_subsz) : 0;
  if (!hrb_subsz_bits)
    use_hrb_subsz = 0;
  else if ((1 << hrb_subsz_bits) != use_hrb_subsz)
  {
    logerr("bad use_hrb_subsz=%d, must be > 1 and pow-of-2 (hrb_subsz_bits=%d)", use_hrb_subsz, hrb_subsz_bits);
    use_hrb_subsz = hrb_subsz_bits = 0;
  }

  if (use_hrb_subsz == (w >> compressed.htRangeBlocksLevels))
    return false;

  // unpack to plain hmap
  Tab<uint16_t> hmap16;
  hmap16.resize(w * h);
  compressed.iterateBlocksPixels(0, 0, compressed.bw, compressed.bh,
    [&](uint32_t x, uint32_t y, const uint16_t v) { hmap16[y * w + x] = v; });

  // reallocate storage and re-compress hmap
  const size_t allocatedDataSize = CompressedHeightmap::calc_data_size_needed(w, h, block_shift, use_hrb_subsz);
  close();
  hmap_data = alloc_hmap_data(allocatedDataSize);

  compressed = CompressedHeightmap::compress((uint8_t *)hmap_data, allocatedDataSize, hmap16.data(), w, h, block_shift, use_hrb_subsz);
  return true;
}

bool HeightmapPhysHandler::checkOrAllocateOwnData()
{
  if (hmap_data)
    return true;

  const size_t allocatedDataSize = compressed.dataSizeCurrent();
  hmap_data = alloc_hmap_data(allocatedDataSize);
  if (!hmap_data)
  {
    logerr("%s failed to allocate %dK", __FUNCTION__, allocatedDataSize >> 10);
    return false;
  }

  memcpy(hmap_data, compressed.fullData, allocatedDataSize);
  logmessage(_MAKE4C('SHMM'), "check and allocate new HMAP dump by copying: : %p, %dK %p", hmap_data, allocatedDataSize >> 10,
    compressed.fullData);
  compressed = CompressedHeightmap::init((uint8_t *)hmap_data, allocatedDataSize, hmapWidth.x, hmapWidth.y,
    compressed.block_width_shift, compressed.htRangeBlocksLevels ? hmapWidth.x >> compressed.htRangeBlocksLevels : 0);
  return true;
}

void HeightmapPhysHandler::close()
{
  if (hmap_data)
  {
    const size_t allocatedDataSize = compressed.dataSizeCurrent();
    free_hmap_data(hmap_data, allocatedDataSize);
    hmap_data = NULL;
  }
}

Point3 HeightmapPhysHandler::getClippedOrigin(const Point3 &origin_pos) const
{
  Point3 clippedOrigin;
  clippedOrigin.x = min(max(worldBox[0].x, origin_pos.x), worldBox[1].x);
  clippedOrigin.y = min(max(worldBox[0].y, origin_pos.y), worldBox[1].y);
  clippedOrigin.z = min(max(worldBox[0].z, origin_pos.z), worldBox[1].z);
  return clippedOrigin;
}


bool HeightmapPhysHandler::getHeightBelow(const Point3 &p, float &ht, Point3 *normal) const
{
  float getHt;
  Point3 getNormal;
  bool ret = getHeight(Point2::xz(p), getHt, normal ? &getNormal : NULL);

  if (ret && getHt < p.y)
  {
    ht = getHt;
    if (normal)
      *normal = getNormal;
    return true;
  }
  return false;
}

bool HeightmapPhysHandler::getHeight(const Point2 &p, float &ht, Point3 *normal) const
{
  if (!(worldBox2 & p))
    return false;
  // return getHeightmapHeight(ipoint2((p-worldPosOfs)/hmapCellSize), ht);
  return get_height_midpoint_heightmap(*this, p, ht, normal);
}

bool HeightmapPhysHandler::getHeightMax(const Point2 &p0, const Point2 &p1, float hmin, float &hmax) const
{
  const BBox3 box(Point3::xVy(p0, hmin), Point3::xVy(p1, VERY_BIG_NUMBER));
  if (!(worldBox & box))
    return false;
  return get_height_max_heightmap(*this, p0, p1, hmin, hmax);
}

bool HeightmapPhysHandler::traceray(const Point3 &p, const Point3 &dir, real &t, Point3 *normal, bool cull) const
{
  BBox3 rayBox(p, p);
  Point3_vec4 end = p + dir * t;
  rayBox += end;
  if (!(worldBox & rayBox))
    return false;
  if (!v_test_segment_box_intersection(v_ldu(&p.x), v_ld(&end.x), vecbox))
    return false;
  return trace_ray_midpoint_heightmap(*this, p, dir, t, normal, cull);
}
bool HeightmapPhysHandler::rayhitNormalized(const Point3 &p, const Point3 &normDir, real t) const
{
  vec3f pt = v_ldu(&p.x), end = v_madd(v_ldu(&normDir.x), v_splats(t), pt);
  bbox3f rayBox;
  rayBox.bmin = v_min(end, pt);
  rayBox.bmax = v_max(end, pt);
  if (!v_bbox3_test_box_intersect(rayBox, vecbox))
    return false;
  if (!v_test_segment_box_intersection(pt, end, vecbox))
    return false;
  return ray_hit_midpoint_heightmap(*this, p, normDir, t);
}

bool HeightmapPhysHandler::rayUnderHeightmapNormalized(const Point3 &p, const Point3 &normDir, real t) const
{
  BBox3 rayBox(p, p);
  Point3_vec4 end = p + normDir * t;
  rayBox += end;
  if (!(worldBox & rayBox))
    return false;
  if (!v_test_segment_box_intersection(v_ldu(&p.x), v_ld(&end.x), vecbox))
    return false;
  return ray_under_heightmap(*this, p, normDir, t);
}

void HeightmapPhysHandler::fillHeightsInRegion(int regions[4], int cnt, float *heights, float &elemSize, Point2 &gridOfs) const
{
  elemSize = getHeightmapCellSize();
  gridOfs = worldPosOfs + Point2(elemSize * regions[0], elemSize * regions[1]);
  const int rw = regions[2] - regions[0] + 1;
  const int rh = regions[3] - regions[1] + 1;
  compressed.iteratePixels(regions[0], regions[1], min(rw, hmapWidth.x), min(rh, hmapWidth.y),
    [&](uint32_t x, uint32_t y, uint16_t ht) { heights[x - regions[0] + (y - regions[1]) * rw] = ht * hScaleRaw + hMin; });
}

bool HeightmapPhysHandler::getHeightsGrid(bbox3f_cref gbox, float *heights, int cnt, Point2 &gridOfs, float &elemSize, int8_t &w,
  int8_t &h) const
{
  if (!v_bbox3_test_box_intersect(gbox, vecbox))
  {
    w = h = 0;
    return true;
  }

  vec4f worldBboxxzXZ = v_perm_xzac(gbox.bmin, gbox.bmax);
  vec4f regionV = v_sub(worldBboxxzXZ, v_worldOfsxzxz);
  regionV = v_mul(regionV, invElemSize);
  vec4i regionI = v_subi(v_cvt_floori(regionV), V_CI_MASK0011);
  vec4i maxLimit = v_addi(v_make_vec4i(hmapWidth.x, hmapWidth.y, hmapWidth.x, hmapWidth.y), v_set_all_bitsi());
  regionI = v_clampi(regionI, v_zeroi(), maxLimit);
  DECL_ALIGN16(int, regions[4]);
  v_sti(regions, regionI);
  if ((regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1) > cnt)
    return false;
  G_ASSERT(regions[2] - regions[0] < 125 && regions[3] - regions[1] < 125);
  w = regions[2] - regions[0] + 1;
  h = regions[3] - regions[1] + 1;
  fillHeightsInRegion(regions, cnt, heights, elemSize, gridOfs);

  return true;
}

bool HeightmapPhysHandler::getHeightsGrid(const Point4 &origin, float *heights, int w, int h, Point2 &gridOfs, float &elemSize) const
{
  const IPoint2 xzOrigin = posToHmapXZ(Point2::xz(origin));
  int regions[4];
  regions[0] = xzOrigin.x - w / 2;
  regions[1] = xzOrigin.y - h / 2;
  regions[2] = xzOrigin.x + (w - 1) / 2;
  regions[3] = xzOrigin.y + (h - 1) / 2;
  G_ASSERT(regions[2] - regions[0] + 1 == w && regions[3] - regions[1] + 1 == h);
  if (regions[0] < 0 || regions[1] < 0 || regions[2] + 1 >= hmapWidth.x || regions[3] + 1 >= hmapWidth.y)
    return false;

  fillHeightsInRegion(regions, w * h, heights, elemSize, gridOfs);
  return true;
}

int HeightmapPhysHandler::traceDownMultiRay(bbox3f_cref rayBox, vec4f *v_ray_pos_mint, vec4f *v_out_norm, int cnt) const
{

  if (!v_bbox3_test_box_intersect(rayBox, vecbox))
    return 0;
  int ret = 0;
  for (int i = 0; i < cnt; ++i, v_out_norm++, v_ray_pos_mint++)
  {
    float getHt;
    Point3_vec4 &p = *reinterpret_cast<Point3_vec4 *>(v_ray_pos_mint);
    Point3_vec4 normal;
    if (get_height_midpoint_heightmap(*this, Point2::xz(p), getHt, &normal))
    {
      float dist = p.y - getHt;
      // if (getHt < p.y && dist < p.resv)
      if (dist < p.resv) // we don't want be below heightmap
      {
        *v_out_norm = v_ld(&normal.x);
        ret = 1;
        p.resv = dist;
      }
    }
  }
  return ret;
}

bool HeightmapPhysHandler::getHeightmapHeightMinMaxInChunk(const Point2 &pos, const real &chunkSize, real &hmin, real &hmax) const
{
  const Point3 orig = getHeightmapOffset();
  const Point2 pt = (pos - Point2(orig.x, orig.z)) / getHeightmapCellSize();
  Point2 ptf(floorf(pt.x), floorf(pt.y));
  IPoint2 cellStart(int(ptf.x), int(ptf.y));

  ptf = Point2(floorf(pt.x + chunkSize / getHeightmapCellSize()), floorf(pt.y + chunkSize / getHeightmapCellSize()));
  IPoint2 cellEnd(int(ptf.x), int(ptf.y));
  cellEnd.x = min(cellEnd.x, hmapWidth.x - 1);
  cellEnd.y = min(cellEnd.y, hmapWidth.y - 1);

  cellStart.x = max(cellStart.x, 0);
  cellStart.y = max(cellStart.y, 0);

  if (cellEnd.x < cellStart.x || cellEnd.y < cellStart.y)
  {
    hmin = worldBox[0].y;
    hmax = worldBox[1].y;
    return false;
  }

  cellEnd.x = min(cellEnd.x + 1, hmapWidth.x - 1);
  cellEnd.y = min(cellEnd.y + 1, hmapWidth.y - 1);
  uint16_t min_16 = 0xFFFF, max_16 = 0;
  compressed.iteratePixels(cellStart.x, cellStart.y, cellEnd.x - cellStart.x + 1, cellEnd.y - cellStart.y + 1,
    [&](uint32_t, uint32_t, uint16_t h0) {
      min_16 = min(min_16, h0);
      max_16 = max(max_16, h0);
    });
  hmin = min_16 * hScaleRaw + hMin;
  hmax = max_16 * hScaleRaw + hMin;
  return true;
}

void HeightmapPhysHandler::updateBlockMinMax(uint32_t blockId, uint16_t ht, float min_meters_change)
{
  CompressedHeightmap::BlockInfo &block = ((CompressedHeightmap::BlockInfo *)compressed.fullData)[blockId];
  const int cMin = block.getMin(), cMax = block.getMax(), cDelta = block.delta;
  const uint16_t maxDiff = max<int>(cMin - int(ht), int(ht) - cMax);
  const uint16_t addDelta = min<int>(0xFFFF, max<int>(min_meters_change / (hScaleRaw + 1e-6f), maxDiff * 2));
  const int nextMin = max<int>(0, min<int>(ht, cMin - addDelta));
  const int nextMax = min<int>(0xFFFF, max<int>(ht, cMax + addDelta));
  const int nextDelta = nextMax - nextMin;
  const uint32_t blockSize = (1 << compressed.block_size_shift);
  uint8_t *vi = compressed.blockVariance + (blockId << compressed.block_size_shift);
  if (nextDelta == 0)
    memset(vi, 0, blockSize);
  else
  {
    const uint32_t halfDelta = nextDelta >> 1;
    for (uint32_t i = 0, ie = blockSize; i < ie; ++i, ++vi)
      *vi = uint32_t(uint32_t(block.decodeVariance(*vi) - nextMin) * 255 + halfDelta) / nextDelta;
  }
  block = CompressedHeightmap::BlockInfo{uint16_t(nextMin), uint16_t(nextDelta)};
}
