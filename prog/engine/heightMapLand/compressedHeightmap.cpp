// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightMapLand/dag_compressedHeightmap.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_staticTab.h>
#include <debug/dag_assert.h>
#include <math/dag_bits.h>
#include <memory.h>
#include <string.h>
#include <supp/dag_alloca.h>

constexpr uint8_t max_block_shift = 5; // if more than we would need to load two cache line for one cell
size_t CompressedHeightmap::calc_data_size_needed(uint32_t w, uint32_t h, uint8_t &block_shift, uint32_t hrb_subsz)
{
  G_ASSERTF(w >= hrb_subsz * 2 && h >= hrb_subsz * 2, "w,h=%dx%d hrb_subsz=%d", w, h, hrb_subsz);
  block_shift = min(block_shift, max_block_shift);
  uint32_t block_mask = (1 << block_shift) - 1;
  uint32_t blocksW = (w + block_mask) >> block_shift, blocksH = (h + block_mask) >> block_shift;
  uint32_t max_level = hrb_subsz ? __bsf(w / hrb_subsz) : 0;

  return ((blocksW * blocksH * ((1 << (block_shift * 2)) + sizeof(BlockInfo)) + 0xF) & ~0xFu) +
         sHierGridOffsets[max_level] * sizeof(HeightRangeBlock);
}

void CompressedHeightmap::recomputeHierHeightRangeBlocks()
{
  if (!htRangeBlocksLevels)
    return;

  HeightRangeBlock *hrb = getHtRangeBlocksLevData(htRangeBlocksLevels - 1);
  unsigned hrb_stride = getHtRangeBlocksLevStride(htRangeBlocksLevels - 1);
  unsigned step = getW() >> htRangeBlocksLevels, last_bx = getW() / step - 1, last_by = getH() / step - 1;
  for (unsigned y = 0; y <= last_by; y++)
    for (unsigned x = 0; x <= last_bx; x++)
    {
      uint16_t max_ht = 0, min_ht = uint16_t(~0u);
      iteratePixels(x * step, y * step, x < last_bx ? step + 1 : step, y < last_by ? step + 1 : step,
        [&](uint32_t, uint32_t, uint16_t ht) { min_ht = min(min_ht, ht), max_ht = max(max_ht, ht); });

      auto &dest = hrb[hrb_stride * (y / 2) + x / 2];
      unsigned idx = (y & 1) * 2 + (x & 1);
      dest.hMin[idx] = min_ht;
      dest.hMax[idx] = max_ht;
    }
  for (unsigned lev = htRangeBlocksLevels - 1; lev > 0; lev--)
  {
    hrb = getHtRangeBlocksLevData(lev);
    hrb_stride = getHtRangeBlocksLevStride(lev);
    HeightRangeBlock *dest_hrb = getHtRangeBlocksLevData(lev - 1);
    for (unsigned sy = 0; sy < hrb_stride; sy += 2)
      for (unsigned sx = 0; sx < hrb_stride; sx += 2, dest_hrb++)
        for (unsigned i = 0; i < 4; i++)
        {
          const auto &src = hrb[hrb_stride * (sy + i / 2) + sx + (i & 1)];
          dest_hrb->hMin[i] = min(min(src.hMin[0], src.hMin[1]), min(src.hMin[2], src.hMin[3]));
          dest_hrb->hMax[i] = max(max(src.hMax[0], src.hMax[1]), max(src.hMax[2], src.hMax[3]));
        }
  }
}

bool CompressedHeightmap::updateHierHeightRangeBlocksForPoint(unsigned px, unsigned py, uint16_t new_ht)
{
  if (!htRangeBlocksLevels)
    return false;
  G_ASSERT(px < getW() && py < getH());
  HeightRangeBlock *hrb = getHtRangeBlocksLevData(htRangeBlocksLevels - 1);
  unsigned hrb_stride = getHtRangeBlocksLevStride(htRangeBlocksLevels - 1);
  unsigned step = getW() >> htRangeBlocksLevels;
  unsigned bx = px / step, bx0 = bx > 0 && (px % step) == 0 ? bx - 1 : bx, bx1 = bx + 1;
  unsigned by = py / step, by0 = by > 0 && (py % step) == 0 ? by - 1 : by, by1 = by + 1;
  bool changed = false;
  for (unsigned y = by0; y < by1; y++)
    for (unsigned x = bx0; x < bx1; x++)
    {
      auto &dest = hrb[hrb_stride * (y / 2) + x / 2];
      unsigned idx = (y & 1) * 2 + (x & 1);
      if (dest.hMin[idx] > new_ht)
        dest.hMin[idx] = new_ht, changed = true;
      if (dest.hMax[idx] < new_ht)
        dest.hMax[idx] = new_ht, changed = true;
    }
  if (!changed || htRangeBlocksLevels < 2)
    return false;

  for (unsigned lev = htRangeBlocksLevels - 1; lev > 0; lev--)
  {
    bx0 >>= 1, bx1 = (bx1 + 1) >> 1;
    by0 >>= 1, by1 = (by1 + 1) >> 1;
    hrb = getHtRangeBlocksLevData(lev - 1);
    hrb_stride = getHtRangeBlocksLevStride(lev - 1);
    for (unsigned y = by0; y < by1; y++)
      for (unsigned x = bx0; x < bx1; x++)
      {
        auto &dest = hrb[hrb_stride * (y / 2) + x / 2];
        unsigned idx = (y & 1) * 2 + (x & 1);
        if (dest.hMin[idx] > new_ht)
          dest.hMin[idx] = new_ht;
        if (dest.hMax[idx] < new_ht)
          dest.hMax[idx] = new_ht;
      }
  }
  return true;
}

bool CompressedHeightmap::recomputeHierHeightRangeBlocksForRect(unsigned rx, unsigned ry, unsigned rw, unsigned rh)
{
  if (!htRangeBlocksLevels)
    return false;

  HeightRangeBlock *hrb = getHtRangeBlocksLevData(htRangeBlocksLevels - 1);
  unsigned hrb_stride = getHtRangeBlocksLevStride(htRangeBlocksLevels - 1);
  unsigned step = getW() >> htRangeBlocksLevels;
  if (rx > 0 && (rx % step) == 0)
    rx--, rw++;
  if (ry > 0 && (ry % step) == 0)
    ry--, rh++;

  unsigned last_bx = getW() / step - 1, last_by = getH() / step - 1;
  unsigned bx0 = rx / step, bx1 = (rx + rw + step - 1) / step;
  unsigned by0 = ry / step, by1 = (ry + rh + step - 1) / step;
  bool updated = false;
  for (unsigned y = by0; y < by1; y++)
    for (unsigned x = bx0; x < bx1; x++)
    {
      uint16_t max_ht = 0, min_ht = uint16_t(~0u);
      iteratePixels(x * step, y * step, x < last_bx ? step + 1 : step, y < last_by ? step + 1 : step,
        [&](uint32_t, uint32_t, uint16_t ht) { min_ht = min(min_ht, ht), max_ht = max(max_ht, ht); });

      auto &dest = hrb[hrb_stride * (y / 2) + x / 2];
      unsigned idx = (y & 1) * 2 + (x & 1);
      if (dest.hMin[idx] == min_ht && dest.hMax[idx] == max_ht)
        continue;
      dest.hMin[idx] = min_ht;
      dest.hMax[idx] = max_ht;
      updated = true;
    }
  if (!updated)
    return false;

  for (unsigned lev = htRangeBlocksLevels - 1; lev > 0; lev--)
  {
    bx0 = (bx0 >> 1) & ~1u, bx1 = (((bx1 + 1) >> 1) + 1) & ~1u;
    by0 = (by0 >> 1) & ~1u, by1 = (((by1 + 1) >> 1) + 1) & ~1u;
    hrb = getHtRangeBlocksLevData(lev);
    hrb_stride = getHtRangeBlocksLevStride(lev);
    HeightRangeBlock *dest_hrb = getHtRangeBlocksLevData(lev - 1) + (by0 / 2) * getHtRangeBlocksLevStride(lev - 1) + bx0 / 2;
    unsigned dest_add_stride = (hrb_stride - bx1 + bx0) / 2;
    for (unsigned sy = by0; sy < by1; sy += 2, dest_hrb += dest_add_stride)
      for (unsigned sx = bx0; sx < bx1; sx += 2, dest_hrb++)
        for (unsigned i = 0; i < 4; i++)
        {
          const auto &src = hrb[hrb_stride * (sy + i / 2) + sx + (i & 1)];
          dest_hrb->hMin[i] = min(min(src.hMin[0], src.hMin[1]), min(src.hMin[2], src.hMin[3]));
          dest_hrb->hMax[i] = max(max(src.hMax[0], src.hMax[1]), max(src.hMax[2], src.hMax[3]));
        }
  }
  return updated;
}

CompressedHeightmap CompressedHeightmap::compress(uint8_t *dest, uint32_t dest_size, const uint16_t *ht, uint32_t w, uint32_t h,
  uint8_t block_shift, uint32_t hrb_subsz)
{
  CompressedHeightmap ret;
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(dest_size >= calc_data_size_needed(w, h, block_shift, hrb_subsz), ret, "dest size %d is too small", dest_size);
  G_ASSERTF(((w | h) & (1 << block_shift) - 1) == 0, "todo: process last blocks separately");
  ret.block_width_shift = block_shift;
  ret.block_width = 1 << block_shift;
  ret.block_width_mask = ret.block_width - 1;
  ret.block_size_shift = ret.block_width_shift + ret.block_width_shift;
  ret.bw = w >> ret.block_width_shift;
  ret.bh = h >> ret.block_width_shift;
  ret.fullData = dest;
  ret.blockVariance = ret.fullData + sizeof(BlockInfo) * ret.bw * ret.bh;
  ret.htRangeBlocksLevels = hrb_subsz ? __bsf(w / hrb_subsz) : 0;
  if (ret.htRangeBlocksLevels)
  {
    G_ASSERT((w >> ret.htRangeBlocksLevels) == hrb_subsz);
    ret.htRangeBlocks = (HeightRangeBlock *)(void *)((uintptr_t(ret.blockVariance) + ret.getW() * ret.getH() + 0xF) & ~0xF);
  }

  const uint32_t block_size = 1 << ret.block_size_shift;
  BlockInfo *bInfo = (BlockInfo *)ret.fullData;
  auto vi = ret.blockVariance;
  for (size_t y = 0; y < h; y += ret.block_width, ht += (ret.block_width - 1) * w)
    for (size_t x = 0; x < w; x += ret.block_width, ht += ret.block_width, ++bInfo)
    {
      uint16_t block[1 << (2 * max_block_shift)];
      const uint16_t *cHt = ht;
      uint16_t *dHt = block;
      for (int j = 0; j < ret.block_width; ++j, cHt += w, dHt += ret.block_width)
        memcpy(dHt, cHt, ret.block_width * sizeof(uint16_t));
      auto &di = *bInfo;
      uint16_t mx = 0u, mn = uint16_t(~0u);
      for (int ind = 0; ind < block_size; ++ind)
      {
        mx = max(mx, block[ind]);
        mn = min(mn, block[ind]);
      }
      di.delta = mx - mn;
      di.mn = mn;
      if (di.delta == 0)
      {
        memset(vi, 0, block_size);
        vi += block_size;
      }
      else
      {
        const uint16_t halfDelta = di.delta >> 1;
        for (int ind = 0; ind < block_size; ++ind, ++vi)
        {
          uint32_t encode = uint32_t(uint32_t(block[ind] - mn) * 255 + halfDelta) / di.delta;
          G_ASSERT(encode <= 255);
          *vi = encode;
        }
      }
    }
  ret.recomputeHierHeightRangeBlocks();
  return ret;
}

CompressedHeightmap CompressedHeightmap::init(uint8_t *data, uint32_t data_size, uint32_t w, uint32_t h, uint8_t block_shift,
  uint32_t hrb_subsz)
{
  CompressedHeightmap ret;
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(data_size >= calc_data_size_needed(w, h, block_shift, hrb_subsz), ret, "dest size %d is too small", data_size);
  G_ASSERTF(((w | h) & (1 << block_shift) - 1) == 0, "todo: process last blocks separately");
  ret.block_width_shift = block_shift;
  ret.block_width = 1 << block_shift;
  ret.block_width_mask = ret.block_width - 1;
  ret.block_size_shift = ret.block_width_shift + ret.block_width_shift;
  ret.bw = w >> ret.block_width_shift;
  ret.bh = h >> ret.block_width_shift;
  ret.fullData = data;
  ret.blockVariance = ret.fullData + sizeof(BlockInfo) * ret.bw * ret.bh;
  ret.htRangeBlocksLevels = hrb_subsz ? __bsf(w / hrb_subsz) : 0;
  if (ret.htRangeBlocksLevels)
  {
    G_ASSERT((w >> ret.htRangeBlocksLevels) == hrb_subsz);
    ret.htRangeBlocks = (HeightRangeBlock *)(void *)((uintptr_t(ret.blockVariance) + ret.getW() * ret.getH() + 0xF) & ~0xF);
  }
  return ret;
}

static void unpack_chunk(CompressedHeightmap &hmap, IGenLoad *crd, unsigned chunk_idx, unsigned blocks_per_chunk, bool single_chunk,
  uint32_t *cOfs = nullptr, Tab<uint8_t> *cdata = nullptr, unsigned fmt = 0)
{
  unsigned b0 = 0, b1 = hmap.bw * hmap.bh, uncompressed_size = /*headers*/ hmap.blockVariance - hmap.fullData; // single chunk setup
  unsigned hrb_data_sz =
    chunk_idx == 0 && hmap.htRangeBlocksLevels ? hmap.fullData + hmap.dataSizeCurrent() - (uint8_t *)hmap.htRangeBlocks : 0;
  if (chunk_idx > 0) // reading variance data in other chunks (1..)
  {
    b0 = (chunk_idx - 1) * blocks_per_chunk;
    b1 = min(b1, chunk_idx * blocks_per_chunk);
    uncompressed_size = 0;
  }
  else if (!single_chunk) // when several chunks used we read only block info in first chunk
    b1 = 0;
  uncompressed_size += (b1 - b0) << hmap.block_size_shift;
  auto *vBegin = hmap.blockVariance + (b0 << hmap.block_size_shift), *vEnd = vBegin + ((b1 - b0) << hmap.block_size_shift);

  if (cOfs)
    spin_wait_no_profile([&]() { return interlocked_acquire_load(*cOfs) <= chunk_idx; });

  unsigned compressed_size = 0;
  if (cdata)
    crd = new (alloca(sizeof(InPlaceMemLoadCB)), _NEW_INPLACE) InPlaceMemLoadCB(cdata->data(), compressed_size = cdata->size());
  else
  {
    crd->beginBlock(&fmt);
    compressed_size = crd->getBlockRest();
  }

  IGenLoad *zcrd = NULL;
  if (fmt == btag_compr::OODLE)
    zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(*crd, compressed_size, uncompressed_size + hrb_data_sz);
  else // if (fmt == btag_compr::ZSTD || fmt == btag_compr::UNSPECIFIED)
    zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(*crd, compressed_size);

  if (chunk_idx == 0) // single chunk (whole data) or block info only
  {
    zcrd->read(hmap.fullData, uncompressed_size);
    if (hrb_data_sz)
      zcrd->read(hmap.htRangeBlocks, hrb_data_sz);
  }
  else // variance data
    zcrd->read(vBegin, vEnd - vBegin);
  zcrd->ceaseReading();
  zcrd->~IGenLoad();
  if (cdata)
  {
    clear_and_shrink(*cdata);
    crd->~IGenLoad();
  }
  else
    crd->endBlock();

  // decode delta-compression
  for (auto *b = vBegin; b < vEnd;)
  {
    auto *be = b + (1 << hmap.block_size_shift);
    uint8_t last = *b;
    for (b++; b < be; b++)
      *b = last = uint8_t(*b + last);
  }
}

bool CompressedHeightmap::loadData(CompressedHeightmap &hmap, IGenLoad &crd, unsigned chunk_sz)
{
  uint32_t blocks_per_chunk = chunk_sz >> hmap.block_size_shift;
  int chunk_cnt = chunk_sz ? (hmap.bw * hmap.bh + blocks_per_chunk - 1) / blocks_per_chunk : 0;
  uint32_t cOfs = 0;

  if (chunk_cnt && threadpool::get_num_workers() >= 2)
  {
    struct UnpackChunkJob final : cpujobs::IJob
    {
      CompressedHeightmap &hmap;
      uint32_t &cOfs;
      Tab<uint8_t> cdata;
      uint32_t cidx, blocksPerChunk, fmt;

      UnpackChunkJob(CompressedHeightmap &h, uint32_t ci, uint32_t bpc, uint32_t &o, uint32_t f) :
        hmap(h), cidx(ci), blocksPerChunk(bpc), cOfs(o), fmt(f)
      {}
      const char *getJobName(bool &) const override { return "UnpackChunkJob"; }
      virtual void doJob() override { unpack_chunk(hmap, nullptr, cidx, blocksPerChunk, false, &cOfs, &cdata, fmt); }
    };

    uint32_t queuePos = 0, fmt = 0;
    StaticTab<UnpackChunkJob, 32 + 1> jobs;
    for (unsigned chunk_idx = 0; chunk_idx <= chunk_cnt; chunk_idx++)
    {
      crd.beginBlock(&fmt);
      auto &j = jobs.push_back(UnpackChunkJob(hmap, chunk_idx, blocks_per_chunk, cOfs, fmt));
      threadpool::add(&j, threadpool::PRIO_HIGH, queuePos, threadpool::AddFlags::IgnoreNotDone);

      j.cdata.resize(crd.getBlockRest());
      crd.readTabData(j.cdata);
      interlocked_increment(cOfs);
      crd.endBlock();
      threadpool::wake_up_all();
    }
    threadpool::barrier_active_wait_for_job(&jobs.back(), threadpool::PRIO_HIGH, queuePos);
    for (auto &j : jobs)
      threadpool::wait(&j);
  }
  else // serial chunks loading
    for (unsigned chunk_idx = 0; chunk_idx <= chunk_cnt; chunk_idx++)
      unpack_chunk(hmap, &crd, chunk_idx, blocks_per_chunk, chunk_cnt == 0);
  return true;
}

CompressedHeightmap CompressedHeightmap::downsample(uint8_t *data, uint32_t data_size, uint32_t w, uint32_t h,
  const CompressedHeightmap &orig)
{
  CompressedHeightmap ret;
  uint8_t block_shift = orig.block_width_shift;
  uint32_t hrb_subsz = orig.htRangeBlocksLevels ? orig.getW() >> orig.htRangeBlocksLevels : 0;
  if (hrb_subsz > w / 2)
    hrb_subsz = w / 2;
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(data_size >= calc_data_size_needed(w, h, block_shift, hrb_subsz), ret, "dest size %d is too small", data_size);
  G_ASSERTF(((w | h) & (1 << block_shift) - 1) == 0, "todo: process last blocks separately");
  ret.block_width_shift = block_shift;
  ret.block_width = 1 << block_shift;
  ret.block_width_mask = ret.block_width - 1;
  ret.block_size_shift = ret.block_width_shift + ret.block_width_shift;
  ret.bw = w >> ret.block_width_shift;
  ret.bh = h >> ret.block_width_shift;

  unsigned f = orig.bw / ret.bw;
  G_ASSERTF_RETURN(f >= 1 && f <= 128 && orig.bw == ret.bw * f && orig.bh == ret.bh * f, ret, //
    "orig.bw=%u ret.bw=%u f=%u", orig.bw, ret.bw, f);

  ret.fullData = data;
  ret.blockVariance = ret.fullData + sizeof(BlockInfo) * ret.bw * ret.bh;
  ret.htRangeBlocksLevels = hrb_subsz ? __bsf(w / hrb_subsz) : 0;
  if (ret.htRangeBlocksLevels)
    ret.htRangeBlocks = (HeightRangeBlock *)(void *)((uintptr_t(ret.blockVariance) + ret.getW() * ret.getH() + 0xF) & ~0xF);

  const uint32_t block_size = 1 << ret.block_size_shift;
  BlockInfo *bInfo = (BlockInfo *)ret.fullData;
  auto vi = ret.blockVariance;
  for (size_t y = 0; y < h; y += ret.block_width)
    for (size_t x = 0; x < w; x += ret.block_width, ++bInfo)
    {
      uint32_t block[1 << (2 * max_block_shift)];
      memset(block, 0, block_size * sizeof(block[0]));
      orig.iteratePixels(x * f, y * f, ret.block_width * f, ret.block_width * f,
        [&](uint32_t ox, uint32_t oy, uint32_t ht) { block[(oy / f - y) * ret.block_width + (ox / f - x)] += ht; });

      auto &di = *bInfo;
      uint16_t mx = 0u, mn = uint16_t(~0u);
      for (int ind = 0; ind < block_size; ++ind)
      {
        block[ind] /= f * f;
        mx = max<uint16_t>(mx, block[ind]);
        mn = min<uint16_t>(mn, block[ind]);
      }
      di.delta = mx - mn;
      di.mn = mn;
      if (di.delta == 0)
      {
        memset(vi, 0, block_size);
        vi += block_size;
      }
      else
      {
        const uint16_t halfDelta = di.delta >> 1;
        for (int ind = 0; ind < block_size; ++ind, ++vi)
        {
          uint32_t encode = uint32_t(uint32_t(block[ind] - mn) * 255 + halfDelta) / di.delta;
          G_ASSERT(encode <= 255);
          *vi = encode;
        }
      }
    }
  ret.recomputeHierHeightRangeBlocks();
  return ret;
}

const unsigned CompressedHeightmap::sHierGridOffsets[15] = {
  0,        // level:  0, max x/y:     0, offset: 0
  1,        // level:  1, max x/y:     1, offset: 1
  5,        // level:  2, max x/y:     3, offset: 1 + 4
  21,       // level:  3, max x/y:     7, offset: 1 + 4 + 16
  85,       // level:  4, max x/y:    15, offset: 1 + 4 + 64
  341,      // level:  5, max x/y:    31, offset: 1 + 4 + 64 + 256
  1365,     // level:  6, max x/y:    63, offset: 1 + 4 + 64 + 256 + 1024
  5461,     // level:  7, max x/y:   127, offset: 1 + 4 + 64 + 256 + 1024 + 4096
  21845,    // level:  8, max x/y:   255, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  87381,    // level:  9, max x/y:   511, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  349525,   // level: 10, max x/y:  1023, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  1398101,  // level: 11, max x/y:  2047, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  5592405,  // level: 12, max x/y:  4095, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  22369621, // level: 13, max x/y:  8191, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
  89478485, // level: 14, max x/y: 16383, offset: 1 + 4 + 64 + 256 + 1024 + 4096 + ...
};
