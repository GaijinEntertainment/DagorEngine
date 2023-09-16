#include <heightMapLand/dag_compressedHeightmap.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_staticTab.h>
#include <debug/dag_assert.h>
#include <memory.h>
#include <string.h>

constexpr uint8_t max_block_shift = 5; // if more than we would need to load two cache line for one cell
size_t CompressedHeightmap::calc_data_size_needed(uint32_t w, uint32_t h, uint8_t &block_shift)
{
  block_shift = min(block_shift, max_block_shift);
  uint32_t block_mask = (1 << block_shift) - 1;
  uint32_t blocksW = (w + block_mask) >> block_shift, blocksH = (h + block_mask) >> block_shift;
  return blocksW * blocksH * ((1 << (block_shift * 2)) + sizeof(BlockInfo));
}

CompressedHeightmap CompressedHeightmap::compress(uint8_t *dest, uint32_t dest_size, const uint16_t *ht, uint32_t w, uint32_t h,
  uint8_t block_shift)
{
  CompressedHeightmap ret;
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(dest_size >= calc_data_size_needed(w, h, block_shift), ret, "dest size %d is too small", dest_size);
  G_ASSERTF(((w | h) & (1 << block_shift) - 1) == 0, "todo: process last blocks separately");
  ret.block_width_shift = block_shift;
  ret.block_width = 1 << block_shift;
  ret.block_width_mask = ret.block_width - 1;
  ret.block_size_shift = ret.block_width_shift + ret.block_width_shift;
  ret.bw = w >> ret.block_width_shift;
  ret.bh = h >> ret.block_width_shift;
  ret.fullData = dest;
  ret.blockVariance = ret.fullData + sizeof(BlockInfo) * ret.bw * ret.bh;

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
  return ret;
}

CompressedHeightmap CompressedHeightmap::init(uint8_t *data, uint32_t data_size, uint32_t w, uint32_t h, uint8_t block_shift)
{
  CompressedHeightmap ret;
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(data_size >= calc_data_size_needed(w, h, block_shift), ret, "dest size %d is too small", data_size);
  G_ASSERTF(((w | h) & (1 << block_shift) - 1) == 0, "todo: process last blocks separately");
  ret.block_width_shift = block_shift;
  ret.block_width = 1 << block_shift;
  ret.block_width_mask = ret.block_width - 1;
  ret.block_size_shift = ret.block_width_shift + ret.block_width_shift;
  ret.bw = w >> ret.block_width_shift;
  ret.bh = h >> ret.block_width_shift;
  ret.fullData = data;
  ret.blockVariance = ret.fullData + sizeof(BlockInfo) * ret.bw * ret.bh;
  return ret;
}

static void unpack_chunk(CompressedHeightmap &hmap, IGenLoad *crd, unsigned chunk_idx, unsigned blocks_per_chunk, bool single_chunk,
  uint32_t *cOfs = nullptr, Tab<uint8_t> *cdata = nullptr, unsigned fmt = 0)
{
  unsigned b0 = 0, b1 = hmap.bw * hmap.bh, uncompressed_size = /*headers*/ hmap.blockVariance - hmap.fullData; // single chunk setup
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
    zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(*crd, compressed_size, uncompressed_size);
  else // if (fmt == btag_compr::ZSTD || fmt == btag_compr::UNSPECIFIED)
    zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(*crd, compressed_size);

  if (chunk_idx == 0) // single chunk (whole data) or block info only
    zcrd->read(hmap.fullData, uncompressed_size);
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

  if (chunk_cnt && threadpool::get_num_workers())
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
  G_ASSERTF_RETURN(block_shift <= max_block_shift, ret, "block width %d doesn't make sense, two lines of caches will be touched",
    1 << block_shift);
  G_ASSERTF_RETURN(data_size >= calc_data_size_needed(w, h, block_shift), ret, "dest size %d is too small", data_size);
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
  return ret;
}
