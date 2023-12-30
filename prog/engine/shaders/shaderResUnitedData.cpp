#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_sort.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>

namespace unitedvdata
{
static constexpr int MAX_IB_SIZE = (256 << 20) - (4 << 10); // BufChunk::sz field is 28 bits only
static constexpr int ALIGN_PAGE_SIZE = 4096;
enum
{
  ALIGN_PAGE = ALIGN_PAGE_SIZE - 1,
  ALIGN_64K = 0xFFFF,
  ALIGN_1M = 0xFFFFF
};

static int get_inst_count(DynamicRenderableSceneLodsResource *r)
{
  int refs = 0;
  for (const auto *rx = r->getFirstOriginal(); rx; rx = rx->getNextClone())
    refs += rx->getInstanceRefCount();
  return refs;
}
static int get_inst_count(RenderableInstanceLodsResource *) { return 1; }
} // namespace unitedvdata

using namespace unitedvdata;

static int dbg_level = 0;

unitedvdata::BufConfig::BufConfig() :
  ibSz{MAX_IB_SIZE},
  ibMinAdd{ALIGN_64K + 1},
  vbMinAdd{ALIGN_1M + 1},
  ibMaxAdd{65536 << 10},
  vbMaxAdd{65536 << 10},
  ibAddPromille{100},
  vbAddPromille{100}
{
  for (int i = BufPool::IDX_VB_START; i < MAX_VBIDX_CNT; i++)
    vbSz[i] = 64 << 20;
}

unitedvdata::BufConfig::BufConfig(const DataBlock &hints_blk, int maxVbSize) : BufConfig()
{
  ibSz = hints_blk.getInt("IB", MAX_IB_SIZE >> 10) << 10;
  debug("  IB=%dK", ibSz >> 10);

  char pnm[5];
  for (int i = BufPool::IDX_VB_START; i < MAX_VBIDX_CNT; i++)
  {
    snprintf(pnm, sizeof(pnm), "VB%d", i);
    int vbVal = hints_blk.getInt(pnm, maxVbSize >> 10);
    vbSz[i] = vbVal << 10;
    debug("  %s=%dK", pnm, vbVal);
    if (!vbVal)
      break;
  }

  ibMinAdd = hints_blk.getInt("ibMinAdd", (ALIGN_64K + 1) >> 10) << 10;
  ibMaxAdd = hints_blk.getInt("ibMaxAdd", 65536) << 10;
  ibAddPromille = hints_blk.getInt("ibAddPromille", 100);

  vbMinAdd = hints_blk.getInt("vbMinAdd", (ALIGN_1M + 1) >> 10) << 10;
  vbMaxAdd = hints_blk.getInt("vbMaxAdd", 65536) << 10;
  vbAddPromille = hints_blk.getInt("vbAddPromille", 100);

  debug("  ibAdd(%%/min/max)=%.1f%% / %dK / %dK, vbAdd(%%/min/max)=%.1f%% / %dK / %dK", ibAddPromille / 10.f, ibMinAdd >> 10,
    ibMaxAdd >> 10, vbAddPromille / 10.f, vbMinAdd >> 10, vbMaxAdd >> 10);
}

template <class RES>
void ShaderResUnitedVdata<RES>::setHints(const DataBlock &hints_blk)
{
  BufConfig localHints{hints_blk, buf.maxVbSize};
  std::lock_guard lock{hintsMutex};
  hints = localHints;
}

template <class RES>
BufConfig ShaderResUnitedVdata<RES>::getHints() const
{
  std::lock_guard lock{hintsMutex};
  return hints;
}

template <class TAB>
static void check_coalesce2(TAB &used_chunks, BufChunk &c)
{
  for (BufChunk &c2 : used_chunks)
    if (&c != &c2 && c.vbIdx == c2.vbIdx)
    {
      if (c.end() == c2.ofs)
        c.sz += c2.sz;
      else if (c.ofs == c2.end())
      {
        c.sz += c2.sz;
        c.ofs = c2.ofs;
      }
      else
        continue;
      erase_items(used_chunks, &c2 - used_chunks.data(), 1);
      return;
    }
}
template <class TAB>
void unitedvdata::BufChunk::add_chunk(TAB &used_chunks, const BufChunk &new_chunk)
{
#if DAGOR_DBGLEVEL > 0
  // validate new_chunk (assuming that chunks in used_chunks don't overlap)
  for (BufChunk &c : used_chunks)
    if (c.vbIdx == new_chunk.vbIdx)
    {
      G_ASSERTF(new_chunk.end() <= c.ofs || new_chunk.ofs >= c.end(),
        "c=used_chunk[%d]=[%d,%d):%d new_chunk=[%d,%d):%d used_chunks.count=%d", &c - used_chunks.data(), c.ofs, c.end(), c.vbIdx,
        new_chunk.ofs, new_chunk.end(), new_chunk.vbIdx, used_chunks.size());
    }
#endif
  for (BufChunk &c : used_chunks)
    if (c.vbIdx == new_chunk.vbIdx)
    {
      if (c.end() == new_chunk.ofs)
        c.sz += new_chunk.sz;
      else if (c.ofs == new_chunk.end())
      {
        c.sz += new_chunk.sz;
        c.ofs = new_chunk.ofs;
      }
      else
        continue;
      check_coalesce2(used_chunks, c);
      return;
    }
  used_chunks.push_back(new_chunk);
}
int unitedvdata::BufChunk::find_chunk(dag::ConstSpan<BufChunk> used_chunks, int req_sz)
{
  for (const BufChunk &c : used_chunks)
    if (c.sz >= req_sz)
      return &c - used_chunks.data();
  return -1;
}
int unitedvdata::BufChunk::find_top_chunk(dag::ConstSpan<BufChunk> used_chunks)
{
  if (used_chunks.size() < 2)
    return used_chunks.size() - 1;
  int top = 0, top_ofs = used_chunks[0].ofs;
  for (int i = 1; i < used_chunks.size(); i++)
    if (used_chunks[i].ofs > top_ofs)
      top = i, top_ofs = used_chunks[i].ofs;
  return top;
}
template <class TAB>
void unitedvdata::BufChunk::cut_chunk(TAB &used_chunks, int chunk_idx, BufChunk &new_chunk, int min_gap)
{
  G_ASSERTF_RETURN(chunk_idx >= 0 && chunk_idx < used_chunks.size(), , "chunk_idx=%d used_chunks.count=%d", chunk_idx,
    used_chunks.size());
  G_ASSERTF_RETURN(used_chunks[chunk_idx].vbIdx == new_chunk.vbIdx && used_chunks[chunk_idx].ofs <= new_chunk.ofs &&
                     new_chunk.end() <= used_chunks[chunk_idx].end(),
    , "used_chunks[%d]=%d,%d{%d}  new_chunk=%d,%d{%d}", chunk_idx, used_chunks[chunk_idx].ofs, used_chunks[chunk_idx].sz,
    used_chunks[chunk_idx].vbIdx, new_chunk.ofs, new_chunk.sz, new_chunk.vbIdx);

  if (new_chunk.end() + min_gap > used_chunks[chunk_idx].end())
    new_chunk.sz = used_chunks[chunk_idx].end() - new_chunk.ofs;
  if (new_chunk.ofs < used_chunks[chunk_idx].ofs + min_gap)
  {
    new_chunk.sz += new_chunk.ofs - used_chunks[chunk_idx].ofs;
    new_chunk.ofs = used_chunks[chunk_idx].ofs;
  }

  G_ASSERTF(new_chunk.ofs == used_chunks[chunk_idx].ofs || new_chunk.end() == used_chunks[chunk_idx].end(),
    "used_chunks[%d]=%d,%d  new_chunk=%d,%d", chunk_idx, used_chunks[chunk_idx].ofs, used_chunks[chunk_idx].sz, new_chunk.ofs,
    new_chunk.sz);
  if (new_chunk.ofs == used_chunks[chunk_idx].ofs)
  {
    used_chunks[chunk_idx].sz -= new_chunk.sz;
    used_chunks[chunk_idx].ofs = new_chunk.end();
  }
  else if (new_chunk.end() == used_chunks[chunk_idx].end())
    used_chunks[chunk_idx].sz -= new_chunk.sz;
  else
    G_ASSERT(0);
  if (!used_chunks[chunk_idx].sz && find_top_chunk(used_chunks) != chunk_idx)
    erase_items(used_chunks, chunk_idx, 1);
}

static inline int get_top_chunk_ofs(dag::ConstSpan<BufChunk> freeChunks)
{
  int top_idx = BufChunk::find_top_chunk(freeChunks);
  return top_idx >= 0 ? freeChunks[top_idx].ofs : -1;
}
static inline int get_top_chunk_size(dag::ConstSpan<BufChunk> freeChunks)
{
  int top_idx = BufChunk::find_top_chunk(freeChunks);
  return top_idx >= 0 ? freeChunks[top_idx].sz : 0;
}

static inline int align_buf_size(int sz, bool vbuf, int abs_max_size, const BufConfig &hints)
{
  const int al = (vbuf ? ALIGN_1M : ALIGN_64K);
  const int min_add = vbuf ? hints.vbMinAdd : hints.ibMinAdd;
  const int max_add = vbuf ? hints.vbMaxAdd : hints.ibMaxAdd;
  const int add_promille = vbuf ? hints.vbAddPromille : hints.ibAddPromille;
  G_ASSERTF(!abs_max_size || sz <= abs_max_size, "sz=%d abs_max_size=%d", sz, abs_max_size);
  sz =
    min(((sz + clamp(int(int64_t(sz) * add_promille / 1000) + min_add, min_add, max_add)) + al) & ~al, sz * 2 + min_add + ALIGN_PAGE);
  if (abs_max_size)
    sz = min(sz, abs_max_size);
  return sz & ~ALIGN_PAGE;
}

static bool allocBufChunk(BufChunk &out_c, Tab<BufChunk> &free_chunks, int stride, int req_sz, int min_gap)
{
  using namespace unitedvdata;
  int c_idx = BufChunk::find_chunk(free_chunks, req_sz + stride - 1);
  if (c_idx < 0)
    return false;
  out_c = free_chunks[c_idx];
  out_c.sz = ((out_c.ofs + stride - 1) / stride * stride - out_c.ofs) + req_sz;
  BufChunk::cut_chunk(free_chunks, c_idx, out_c, min_gap);
  return true;
}

BufChunk unitedvdata::BufPool::allocChunkForStride(int s, int req_avail_sz, const BufConfig &hints)
{
  if (EASTL_UNLIKELY(!sbuf[IDX_IB] && pool.empty()))
  {
    pool.push_back(PoolSize());
    if (!allocatePool(0, hints.ibSz))
      return BufChunk();
  }

  BufChunk c;
  if (s == IDX_IB)
  {
    allocBufChunk(c, freeChunks[IDX_IB], 2, req_avail_sz, 3 * 2);
    return c;
  }

  for (int i = IDX_VB_START; i < sbuf.size(); i++)
    if (allocBufChunk(c, freeChunks[i], s, req_avail_sz, 3 * s))
      return c;
  if (allowRebuild)
    for (int i = IDX_VB_START; i < sbuf.size(); i++)
      if (sbuf[i] && pool[i].getSize() + req_avail_sz < maxVbSize) // force rebuild if we have under-max sized VB while alloc failed
        return BufChunk();

  if (sbuf.size() >= sbuf.capacity())
    return BufChunk();

  int idx = sbuf.size();
  int buf_sz = hints.vbSz[idx];
  if (!buf_sz)
    return BufChunk();

  sbuf.push_back(nullptr);
  pool.push_back(PoolSize());
  if (!allocatePool(idx, buf_sz))
    return BufChunk();

  allocBufChunk(c, freeChunks[idx], s, req_avail_sz, 3 * s);
  return c;
}

bool unitedvdata::BufPool::arrangeVdata(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, BufChunkTab &out_c, Sbuffer *ib, bool can_fail,
  const BufConfig &hints, int *vbShortage, int *ibShortage)
{
  StaticTab<unsigned, MAX_CHUNK_CNT> usedSize;
  StaticTab<char, MAX_CHUNK_CNT> usedStride;
  usedSize.push_back(0);
  usedStride.push_back(0);
  out_c.resize(0);

  bool has_vdata = false;
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd && smvd->getGlobVDataCount() > 0)
    {
      if (smvd->getGlobVData(0)->indices == (void *)(intptr_t)-1) // skip processed (duplicates)
        continue;
      if (getIB() && smvd->getGlobVData(0)->indices == getIB()) // skip processed (duplicates)
        continue;

      for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
      {
        GlobalVertexData &vd = *smvd->getGlobVData(i);
        G_ASSERT_CONTINUE(vd.cflags & VDATA_SRC_ONLY);
        G_ASSERT_CONTINUE(vd.cflags & VDATA_I16);

        if (vd.indices != ib && vd.indices != (void *)(intptr_t)-2 && vd.indices != (void *)(intptr_t)-1)
        {
          // previously allocated tight vb/ib for single res, release it here
          if (vd.indices && vd.iOfs == 0)
            del_d3dres(vd.indices);
          if (vd.vb && vd.vOfs == 0)
            del_d3dres(vd.vb);
        }

        int idx = usedStride.size() - 1;
        if (idx < 0 || usedStride[idx] != vd.vstride)
        {
          idx = usedStride.size();
          usedStride.push_back(vd.vstride);
          usedSize.push_back(0);
        }
        usedSize[IDX_IB] += vd.iCnt * 2 + ((vd.iCnt & 1) ? 2 : 0); // 4-byte alignment of data in IB
        usedSize[idx] += vd.vCnt * vd.vstride;

        vd.indices = (Ibuffer *)(void *)(intptr_t)-2; // mark as processed (first pass)
      }
      has_vdata = true;
    }
  if (!has_vdata)
    return false;

  for (int i = 0; i < usedStride.size(); i++)
  {
    BufChunk c = allocChunkForStride(usedStride[i], usedSize[i], hints);
    if (can_fail && !c.sz)
    {
      if (vbShortage && ibShortage)
      {
        *vbShortage = *ibShortage = 0;
        for (int i = 0; i < usedStride.size(); i++)
          (usedStride[i] == IDX_IB ? *ibShortage : *vbShortage) += usedSize[i];
        for (const BufChunk &c : out_c)
          (c.vbIdx == IDX_IB ? *ibShortage : *vbShortage) -= c.sz;
      }
      releaseBufChunk(out_c, false);
      resetVdataBufPointers(smvd_list);
      out_c.clear(); // report failed allocation
      return true;
    }
    G_ASSERTF(c.sz, "allocChunkForStride(%d, %d) failed, c={ofs=%d sz=%d vbIdx=%d} top.chunk={ofs=%d sz=%d}\n%s", usedStride[i],
      usedSize[i], c.ofs, c.sz, c.vbIdx, get_top_chunk_ofs(freeChunks[i]), get_top_chunk_size(freeChunks[i]), getStatStr());
    usedSize[i] = c.vbIdx;
    out_c.push_back(c);
  }

  int c_idx = 0;
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd && smvd->getGlobVDataCount() > 0)
    {
      if (smvd->getGlobVData(0)->indices != (void *)(intptr_t)-2) // second pass
        continue;

      for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
      {
        GlobalVertexData &vd = *smvd->getGlobVData(i);
        G_ASSERT_CONTINUE(vd.cflags & VDATA_SRC_ONLY);

        if (vd.getStride() != usedStride[c_idx])
          c_idx++;
        G_ASSERTF(vd.getStride() == usedStride[c_idx], "vd.getStride()=%d usedStride[%d]=%d", vd.getStride(), c_idx,
          usedStride[c_idx]);

        vd.vbIdx = usedSize[c_idx];
        vd.indices = (Ibuffer *)(void *)(intptr_t)-1; // mark as processed
      }
    }
  return true;
}
void unitedvdata::BufPool::getSeparateChunks(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, int first_lod, dag::Span<BufChunk> c,
  BufChunkTab &out_c1, BufChunkTab &out_c2)
{
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd && smvd->getGlobVDataCount() > 0)
    {
      if (smvd->getGlobVData(0)->indices != getIB()) // skip unarranged
        continue;

      for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
      {
        GlobalVertexData &vd = *smvd->getGlobVData(i);
        if (vd.getLodIndex() >= first_lod)
        {
          BufChunk::add_chunk(out_c1, BufChunk(vd.iOfs * 2, ((vd.iCnt + 1) & ~1) * 2, IDX_IB));
          BufChunk::add_chunk(out_c1, BufChunk(vd.vOfs * vd.vstride, vd.vCnt * vd.vstride, vd.vbIdx));
        }
      }
      smvd->applyFirstLodsSkippedCount(first_lod);
    }

  out_c2 = c;
  for (int i = 0; i < out_c1.size(); i++)
    for (int j = 0; j < out_c2.size(); j++)
      if (out_c1[i].vbIdx != out_c2[j].vbIdx)
        continue;
      else if (out_c1[i].ofs <= out_c2[j].ofs && out_c2[j].end() <= out_c1[i].end())
      {
        erase_items(out_c2, j, 1);
        j--;
      }
      else if (out_c2[j].ofs <= out_c1[i].ofs && out_c1[i].end() <= out_c2[j].end())
      {
        static constexpr int MIN_GAP = 64;
        if (out_c2[j].ofs + MIN_GAP > out_c1[i].ofs || out_c1[i].end() + MIN_GAP > out_c2[j].end())
          BufChunk::cut_chunk(out_c2, j, out_c1[i], MIN_GAP);
        else
        {
          BufChunk rest_c = out_c2[j];
          out_c2[j].sz = out_c1[i].ofs - out_c2[j].ofs;
          rest_c.sz = rest_c.end() - out_c1[i].end();
          rest_c.ofs = out_c1[i].end();
          out_c2.push_back(rest_c);
        }
      }
      else if (out_c2[j].ofs <= out_c1[i].ofs && out_c1[i].ofs <= out_c2[j].end())
        out_c2[j].sz = out_c1[i].ofs - out_c2[j].ofs;
      else if (out_c2[j].ofs <= out_c1[i].end() && out_c1[i].end() <= out_c2[j].end())
      {
        out_c2[j].sz -= out_c1[i].end() - out_c2[j].ofs;
        out_c2[j].ofs = out_c1[i].end();
      }
  /*
  debug("lod=%d c=%d c1=%d c2=%d", first_lod, c.size(), out_c1.size(), out_c2.size());
  for (int i = 0; i < c.size(); i ++)
    debug("  c[%d] ofs=%d sz=%d vbIdx=%d", i, c[i].ofs, c[i].sz, c[i].vbIdx);
  for (int i = 0; i < out_c1.size(); i ++)
    debug("  n[%d] ofs=%d sz=%d vbIdx=%d", i, out_c1[i].ofs, out_c1[i].sz, out_c1[i].vbIdx);
  for (int i = 0; i < out_c2.size(); i ++)
    debug("  r[%d] ofs=%d sz=%d vbIdx=%d", i, out_c2[i].ofs, out_c2[i].sz, out_c2[i].vbIdx);
  */
}

static int get_optional_buffer_flags()
{
  const Driver3dDesc &drvDsc = d3d::get_driver_desc();
  // This is a SM5.0 feature and is used in CS.
  // that should be available if big buffer views are possible
  uint32_t optionalFlags = drvDsc.shaderModel >= 5.0_sm ? SBCF_MISC_ALLOW_RAW : 0;
  if (drvDsc.issues.hasSmallSampledBuffers)
    optionalFlags = SBCF_MISC_STRUCTURED;

  return optionalFlags;
}

namespace unitedvdata
{

template <size_t N>
static size_t round_up(size_t x)
{
  G_STATIC_ASSERT((N & (N - 1)) == 0); // make sure N is a power of two
  return (x + (N - 1)) & ~(N - 1);
}

} // namespace unitedvdata

bool unitedvdata::BufPool::allocateBuffer(int idx, size_t size, const char *name)
{
  G_ASSERTF(!sbuf[idx] && (!freeChunks[idx].size() || allowRebuild), "sbuf[%d]=%p freeChunks[%d].count=%d allowRebuild=%d", idx,
    sbuf[idx], idx, freeChunks[idx].size(), allowRebuild);

  const bool isIb = idx == IDX_IB;
  const int flags = (isIb ? SBCF_BIND_INDEX : SBCF_BIND_VERTEX) | SBCF_BIND_SHADER_RES | get_optional_buffer_flags();


  Sbuffer *candidate = d3d::create_sbuffer(4, round_up<4>(size) / 4, flags, 0, name);

  if (candidate)
  {
    pool[idx].setSize(size);
    if (!allowRebuild)
      freeChunks[idx].push_back(BufChunk(0, size, idx));
    sbuf[idx] = candidate;
  }

  return candidate != nullptr;
}

bool unitedvdata::BufPool::allocatePool(int idx, size_t hint_sz)
{
  G_ASSERT(!sbuf[idx]);

  const bool isIb = idx == IDX_IB;
  const size_t maxSize = isIb ? MAX_IB_SIZE : maxVbSize;

  G_ASSERTF_AND_DO(hint_sz <= maxSize, hint_sz = 0, "hint_sz=%d", idx, hint_sz);

  hint_sz = round_up<ALIGN_PAGE_SIZE>(hint_sz);
  G_ASSERTF(pool[idx].getUsed() == 0, "pool[%d].used=%d", idx, pool[idx].getUsed());

  if (!allowRebuild)
  {
    // when rebuild is not allowed, we try to pre-allocate more memory for each buffer
    if (isIb)
      hint_sz = min(max(hint_sz, hint_sz ? size_t(1 << 20) : size_t(32 << 20)), maxSize);
    else
      hint_sz = min(max(hint_sz, idx == IDX_VB_START ? (8 << 20) : (size_t)pool[idx - 1].getSize() * 2), maxSize);
  }
  else
  {
    pool[idx].setSize(hint_sz);
    freeChunks[idx].push_back(BufChunk(0, hint_sz, idx));
    return true;
  }

  return allocateBuffer(idx, hint_sz, isIb ? "united_ib" : "united_vb");
}

bool unitedvdata::BufPool::createSbuffers(const BufConfig &hints, bool tight)
{
  for (int i = 0; i < pool.size(); i++)
    if (!sbuf[i])
    {
      int top = BufChunk::find_top_chunk(freeChunks[i]);
      G_ASSERTF(top >= 0, "freeChunks[i].size()=%d top=%d", freeChunks[i].size(), top);
      BufChunk &tc = freeChunks[i][top];
      bool allocated = false;
      if (tight)
        allocated = allocateBuffer(i, tc.ofs, i == IDX_IB ? "tight_ib" : "tight_vb");
      else
        allocated =
          allocateBuffer(i, align_buf_size(allowDelRes ? tc.end() : tc.ofs, i >= IDX_VB_START, i == IDX_IB ? 0 : maxVbSize, hints),
            i == IDX_IB ? "united_ib" : "united_vb");
      tc.sz = pool[i].getSize() - tc.ofs;
      if (!allocated)
        return false;
    }
  return true;
}
void unitedvdata::BufPool::resetVdataBufPointers(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list)
{
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd)
      for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
      {
        GlobalVertexData &vd = *smvd->getGlobVData(i);
        if (vd.indices == getIB() || vd.indices == (void *)(intptr_t)-2 || vd.indices == (void *)(intptr_t)-1)
        {
          vd.vb = nullptr;
          vd.indices = nullptr;
          vd.vbIdx = 0;
        }
      }
}
void unitedvdata::BufPool::clear()
{
  for (Sbuffer *b : sbuf)
    del_d3dres(b);
  pool.resize(0);
  sbuf.resize(1);
  sbuf[IDX_IB] = nullptr;
  for (Tab<BufChunk> &t : freeChunks)
    t.clear();
}
const char *unitedvdata::BufPool::calcUsedSizeStr(dag::ConstSpan<unitedvdata::BufChunk> ctab, String &stor)
{
  if (!ctab.size())
    return "";
  unsigned ib_sum = 0, vb_sum = 0;
  for (const BufChunk &c : ctab)
    (c.vbIdx == IDX_IB ? ib_sum : vb_sum) += c.sz;
  stor.printf(0, "[%4dK+%4dK=%5dK, %2dc]", ib_sum >> 10, vb_sum >> 10, (ib_sum + vb_sum) >> 10, ctab.size());
  return stor;
}
String unitedvdata::BufPool::getStatStr() const
{
  String s;
  int64_t ib_total = 0, vb_total = 0;
  for (int i = 0; i < pool.size(); i++)
  {
    s.aprintf(0, "%s[%d]=%dK(%dK) ", i == IDX_IB ? "ib" : "vb", i, pool[i].getUsed() >> 10, pool[i].getSize() >> 10);
    (i == IDX_IB ? ib_total : vb_total) += pool[i].getUsed();
  }
  if (allowDelRes)
  {
    s += " Avail: ";
    for (int i = 0; i < pool.size(); i++)
    {
      int sum = 0;
      int max_c = 0;
      for (const BufChunk &c : freeChunks[i])
      {
        sum += c.sz;
        if (max_c < c.sz)
          max_c = c.sz;
      }
      if (sum > 0)
        s.aprintf(0, "%s[%d]=%dK:%d{max=%dK} ", i == IDX_IB ? "ib" : "vb", i, sum >> 10, freeChunks[i].size(), max_c >> 10);
    }
  }
  s.aprintf(0, " (total %dM+%dM=%dM) ", ib_total >> 20, vb_total >> 20, (ib_total + vb_total) >> 20);
  return s;
}

template <class RES>
void ShaderResUnitedVdata<RES>::prepareVdataBaseOfs(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, Tab<int> &dviOfs)
{
  dviOfs.clear();
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd && smvd->getGlobVDataCount() > 0)
    {
      dag::Span<GlobalVertexData> vdata(smvd->getGlobVData(0), smvd->getGlobVDataCount());
      int base = append_items(dviOfs, vdata.size() * 2);
      for (int i = 0; i < vdata.size(); i++)
        dviOfs[base + i * 2 + 0] = vdata[i].vOfs, dviOfs[base + i * 2 + 1] = vdata[i].iOfs;
    }
}

template <class RES>
void ShaderResUnitedVdata<RES>::updateVdata(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, unitedvdata::BufPool &buf,
  Tab<uint8_t> &buf_stor, dag::ConstSpan<BufChunk> chunks)
{
  StaticTab<int, MAX_CHUNK_CNT * 3> start_end_stride_triples;
  int bufUsed[MAX_VBIDX_CNT];
  if (!chunks.size())
  {
    for (int i = 0; i < buf.pool.size(); i++)
      bufUsed[i] = buf.pool[i].getUsed();
  }
  else
  {
    StaticTab<char, MAX_CHUNK_CNT> usedStride;

    for (ShaderMatVdata *smvd : smvd_list)
      if (smvd && smvd->getGlobVDataCount() > 0)
        for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
        {
          GlobalVertexData &vd = *smvd->getGlobVData(i);
          if (!usedStride.size() || usedStride.back() != vd.vstride)
            usedStride.push_back(vd.vstride);
        }
    G_ASSERTF(usedStride.size() + 1 == chunks.size(), "usedStride.count=%d chunks.count=%d", usedStride.size(), chunks.size());
    start_end_stride_triples.resize(chunks.size() * 3);
    for (int i = 0; i < chunks.size(); i++)
    {
      start_end_stride_triples[i * 3 + 0] = chunks[i].ofs;
      start_end_stride_triples[i * 3 + 1] = chunks[i].end();
      start_end_stride_triples[i * 3 + 2] = i == 0 ? 2 : usedStride[i - 1];
    }
  }

  buf.updateMutex.lock();
  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd && smvd->getGlobVDataCount() > 0)
      smvd->unpackBuffersTo(make_span(buf.sbuf), chunks.size() ? nullptr : bufUsed, make_span(start_end_stride_triples), buf_stor);

  if (!chunks.size())
  {
    for (int i = 0; i < buf.pool.size(); i++)
    {
      buf.pool[i].setUsed(bufUsed[i]);
      G_ASSERTF(buf.pool[i].getUsed() <= buf.pool[i].getSize(), "i=%d used=%d size=%d", i, buf.pool[i].getUsed(),
        buf.pool[i].getSize());
    }
  }
  else
    for (int i = 0; i < chunks.size(); i++)
    {
      buf.pool[chunks[i].vbIdx].incUsed(chunks[i].sz);
      G_ASSERTF(start_end_stride_triples[i * 3 + 0] <= start_end_stride_triples[i * 3 + 1] &&
                  start_end_stride_triples[i * 3 + 0] >= start_end_stride_triples[i * 3 + 1] - start_end_stride_triples[i * 3 + 2] * 4,
        "chunk[%d] vbIdx=%d ofs=%d sz=%d written_sz=%d stride=%d", i, chunks[i].vbIdx, chunks[i].ofs, chunks[i].sz,
        start_end_stride_triples[i * 3 + 0] - chunks[i].ofs, start_end_stride_triples[i * 3 + 2]);
    }
  buf.updateMutex.unlock();

  for (ShaderMatVdata *smvd : smvd_list)
    if (smvd)
      smvd->clearVdataSrc();
}

template <class RES>
void ShaderResUnitedVdata<RES>::rebaseElemOfs(const RES *r, dag::ConstSpan<int> dviOfs)
{
  Tab<dag::ConstSpan<ShaderMesh::RElem>> relems;
  RES::lockClonesList();
  for (const RES *rx = r->getFirstOriginal(); rx; rx = rx->getNextClone())
    // we can utilize rx->getUsedLods() only when all smvd"s return areLodsSplit()=1; otherwise we must use rx->lods
    // when we add some smvd's vdata to pool we must process here ALL relems that reference that vdata
    for (const typename RES::Lod &lod : rx->getUsedLods())
    {
      lod.getAllElems(relems);

      for (dag::ConstSpan<ShaderMesh::RElem> &r_list : relems)
        for (const ShaderMesh::RElem &e : r_list)
        {
          int dvi_base = 0;
          for (const ShaderMatVdata *smvd : r->getSmvd())
            if (smvd && smvd->getGlobVDataCount() > 0)
            {
              dag::Span<GlobalVertexData> vdata(smvd->getGlobVData(0), smvd->getGlobVDataCount());
              if (e.vertexData >= vdata.data() && e.vertexData < vdata.data() + vdata.size())
              {
                int i = dvi_base + (e.vertexData - vdata.data()) * 2;
                const_cast<ShaderMesh::RElem &>(e).si += e.vertexData->iOfs - dviOfs[i + 1];
                const_cast<ShaderMesh::RElem &>(e).baseVertex += e.vertexData->vOfs - dviOfs[i + 0];
              }
              dvi_base += vdata.size() * 2;
            }
        }
    }
  RES::unlockClonesList();
  on_mesh_relems_updated.fire(r);
}

template <class RES>
void ShaderResUnitedVdata<RES>::updateVdata(RES *r, unitedvdata::BufPool &buf, Tab<int> &dviOfs_stor, Tab<uint8_t> &buf_stor,
  dag::ConstSpan<unitedvdata::BufChunk> c)
{
  prepareVdataBaseOfs(r->getSmvd(), dviOfs_stor);
  updateVdata(r->getSmvd(), buf, buf_stor, c);
  rebaseElemOfs(r, dviOfs_stor);
}

template <class RES>
static int cmp_src_file_and_ofs(RES *const *a, RES *const *b)
{
  if (!a[0]->getSmvd().size() || !b[0]->getSmvd().size())
    return int(a[0] - b[0]);
  return ShaderMatVdata::cmp_src_file_and_ofs(*a[0]->getSmvd()[0], *b[0]->getSmvd()[0]);
}

template <class RES>
void ShaderResUnitedVdata<RES>::rebuildUnitedVdata(dag::Span<RES *> res, bool in_d3d_reset)
{
  int64_t reft = ref_time_ticks();
  G_ASSERT_RETURN(is_main_thread() || !allowSepTightVdata, );

  sort(res, &cmp_src_file_and_ofs<RES>);
  Sbuffer *prev_ib = buf.getIB();

  int64_t ib_total = 0, vb_total = 0, res_added = res.size() - resList.size();
  for (int i = 0; i < buf.pool.size(); i++)
    (i == buf.IDX_IB ? ib_total : vb_total) -= buf.pool[i].getUsed();
  for (RES *r : resList)
    buf.resetVdataBufPointers(r->getSmvd());
  buf.clear();

  resList.clear();
  resList.reserve(res.size());
  if (buf.allowDelRes)
  {
    resUsedChunks.clear();
    resUsedChunks.reserve(res.size());
  }

  unitedvdata::BufConfig local_hints = getHints();

  for (RES *r : res)
  {
    BufChunkTab c;
    if (!buf.arrangeVdata(r->getSmvd(), c, prev_ib, false, local_hints))
      continue;

    resList.push_back(r);
    if (buf.allowDelRes)
      resUsedChunks.push_back() = c;
  }

  if (!buf.createSbuffers(local_hints))
    logerr("unitedVdata<%s>: failed allocate one or more pre-reserved buffers (%d VBs, %s)", RES::getStaticClassName(),
      buf.getVbCount(), buf.getStatStr());

  Tab<int> dviOfs;
  Tab<uint8_t> buf_stor;
  dviOfs.reserve(8 * 2);
  for (int i = 0; i < resList.size(); i++)
    updateVdata(resList[i], buf, dviOfs, buf_stor, getBufChunks(i));

  ShaderMatVdata::closeTlsReloadCrd();
  int t_usec = get_time_usec(reft);
  char fctx[4 << 10];
  for (int i = 0; i < buf.pool.size(); i++)
    (i == buf.IDX_IB ? ib_total : vb_total) += buf.pool[i].getUsed();
  logmessage((!in_d3d_reset && (t_usec > 2000000 || (t_usec > 20000 && is_main_thread()))) ? LOGLEVEL_ERR : LOGLEVEL_WARN,
    "unitedVdata<%s>: rebuild for %d res, %d VBs, %s (+%d res for [%dK+%dK]) for %d usec%s", RES::getStaticClassName(), resList.size(),
    buf.getVbCount(), buf.getStatStr(), res_added, ib_total >> 10, vb_total >> 10, t_usec, dgs_get_fatal_context(fctx, sizeof(fctx)));
}

template <class RES>
inline void ShaderResUnitedVdata<RES>::initUpdateJob(UpdateModelCtx &ctx, RES *r)
{
  G_ASSERTF_RETURN(find_value_idx(resList, r) >= 0, , "res=%p resList=%d", r, resList.size());
  G_ASSERT_RETURN(r->getSmvd().size() < 4, );
  ctx.res = r;
  ctx.res->setResLoadingFlag(true);
  ctx.reqLod = r->getQlReqLodEff();
  interlocked_increment(pendingVdataReloadResCount);
}
template <class RES>
inline void ShaderResUnitedVdata<RES>::doUpdateJob(UpdateModelCtx &ctx)
{
  dag::ConstSpan<Ptr<ShaderMatVdata>> res_smvd = ctx.res->getSmvd();
  G_ASSERT_RETURN(res_smvd.size() < 4, );

  int prev_lod = ctx.res->getQlBestLod();
  ctx.reqLod = ctx.res->getQlReqLodEff();
  if (ctx.reqLod >= prev_lod)
  {
    // std::lock_guard<std::mutex> scopedLock(appendMutex);
    // if (ctx.reqLod > prev_lod)
    //   downgradeRes(ctx.res, ctx.reqLod);
    ceaseUpdateJob(ctx);
    return;
  }

  for (int i = 0; i < res_smvd.size(); i++)
    ctx.tmp_smvd[i] = ShaderMatVdata::make_tmp_copy(res_smvd[i], ctx.reqLod);

  BufChunkTab c_new;
  bool already_in_failed = find_value_idx(failedVdataReloadResList, ctx.res) >= 0;
  unitedvdata::BufConfig local_hints = getHints();

  {
    std::lock_guard<std::mutex> scopedLock(appendMutex);
    int vb_to_free = 0, ib_to_free = 0;

    while (ctx.reqLod < prev_lod)
      if (buf.arrangeVdata(make_span_const(ctx.tmp_smvd, res_smvd.size()), c_new, buf.getIB(), true, local_hints, &vb_to_free,
            &ib_to_free) &&
          c_new.size())
        break;
      else if (ctx.reqLod + 1 != prev_lod)
      {
        ctx.reqLod++;
        for (int i = 0; i < res_smvd.size(); i++)
          ctx.tmp_smvd[i] = ShaderMatVdata::make_tmp_copy(res_smvd[i], ctx.reqLod);
      }
      else
        break;

    if (!c_new.size())
    {
      if (!already_in_failed)
      {
        interlocked_add(vbSizeToFree, vb_to_free);
        interlocked_add(ibSizeToFree, ib_to_free);
        failedVdataReloadResList.push_back(ctx.res);
      }
      ceaseUpdateJob(ctx);
      return;
    }

    if (already_in_failed)
      erase_item_by_value(failedVdataReloadResList, ctx.res);

    int idx = find_value_idx(resList, ctx.res);
    if (idx < 0)
    {
      buf.releaseBufChunk(c_new, false);
      ceaseUpdateJob(ctx);
      return;
    }
    ctx.cPrev = resUsedChunks[idx];
    resUsedChunks[idx] = c_new;
  }

  ctx.dviOfs.reserve(8 * 2);
  prepareVdataBaseOfs(make_span_const(ctx.tmp_smvd, res_smvd.size()), ctx.dviOfs);

  if (buf.allowRebuild && !buf.createSbuffers(local_hints))
    logerr("unitedVdata<%s>: failed allocate one or more pre-reserved buffers (%d VBs, %s)", RES::getStaticClassName(),
      buf.getVbCount(), buf.getStatStr());
  Tab<uint8_t> buf_stor;
  updateVdata(make_span_const(ctx.tmp_smvd, res_smvd.size()), buf, buf_stor, c_new);
  ShaderMatVdata::closeTlsReloadCrd();
}
template <class RES>
inline void ShaderResUnitedVdata<RES>::releaseUpdateJob(UpdateModelCtx &ctx)
{
  if (!ctx.res) // Was it already ceased?
    return;

  if (!ctx.dviOfs.size()) // doUpdateJob() was not called
  {
    // std::lock_guard<std::mutex> scopedLock(appendMutex);
    ceaseUpdateJob(ctx);
    return;
  }

  d3d::GpuAutoLock render_lock;
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  int idx = find_value_idx(resList, ctx.res);
  if (idx < 0)
  {
    buf.releaseBufChunk(ctx.cPrev, true);
    ceaseUpdateJob(ctx);
    return;
  }

  int prev_lod = ctx.res->getQlBestLod();
  {
    dag::ConstSpan<Ptr<ShaderMatVdata>> res_smvd = ctx.res->getSmvd();
    if (ctx.reqLod >= ctx.res->lods.size())
    {
      logerr("ctx.reqLod=%d while ctx.res->lods.size()=%d", ctx.reqLod, ctx.res->lods.size());
      ctx.reqLod = ctx.res->lods.size() - 1;
    }
    for (RES *rx = ctx.res->getFirstOriginal(); rx; rx = rx->getNextClone())
      rx->applyQlBestLod(ctx.reqLod);
    for (int i = 0; i < res_smvd.size(); i++)
      if (res_smvd[i])
        ShaderMatVdata::update_vdata_from_tmp_copy(res_smvd[i], ctx.tmp_smvd[i]);
    rebaseElemOfs(ctx.res, ctx.dviOfs);
    buf.releaseBufChunk(ctx.cPrev, true);
  }

  if (dbg_level > 0)
    debug("unitedVdata<%s>: reload res[%d] (%d:%d, %d, %d) (%d total), %d VBs, %s", RES::getStaticClassName(), idx,
      ctx.res->getQlReqLodEff(), ctx.res->getQlReqLFU(), ctx.reqLod, prev_lod, resList.size(), buf.getVbCount(), buf.getStatStr());

  ceaseUpdateJob(ctx);
}

template <class RES>
bool ShaderResUnitedVdata<RES>::reloadRes(RES *res)
{
  G_ASSERT_RETURN(buf.allowDelRes, false);

  std::lock_guard<std::mutex> scopedLock(appendMutex);
  if (reloadJobMgrId < 0)
  {
    reloadJobMgrId = cpujobs::create_virtual_job_manager(256 << 10, WORKER_THREADS_AFFINITY_MASK, jobMgrName);
    register_job_manager_requiring_shaders_bindump(reloadJobMgrId);
  }
  if (res->getResLoadingFlag())
    return false;
  if ((vbSizeToFree > 0 || ibSizeToFree > 0) && find_value_idx(failedVdataReloadResList, res) >= 0)
    return false;

  struct UpdateModelVdataJob final : public cpujobs::IJob, public UpdateModelCtx
  {
    ShaderResUnitedVdata<RES> *unitedVdata = nullptr;
    UpdateModelVdataJob(ShaderResUnitedVdata<RES> *self, RES *r)
    {
      unitedVdata = self;
      unitedVdata->initUpdateJob(*this, r);
    }
    void doJob() override final { unitedVdata->doUpdateJob(*this); }
    void releaseJob() override final
    {
      // Delay apply one frame if driver defrag is requested (in order to be able to perform it in next present)
#if _TARGET_C1 | _TARGET_C2






#endif
        unitedVdata->releaseUpdateJob(*this);
      delete this;
    }
  };

  UpdateModelVdataJob *j = new UpdateModelVdataJob(this, res);
  if (j->res)
    return cpujobs::add_job(reloadJobMgrId, j);

  delete j;
  return false;
}
template <class RES>
void ShaderResUnitedVdata<RES>::downgradeRes(RES *res, int upper_lod)
{
  if (upper_lod <= res->getQlBestLod() || res->getResLoadingFlag())
    return;

  G_ASSERTF(is_main_thread() || res->getQlReqLFU() + 120 <= dagor_frame_no() || res->getRefCount() <= 2 || get_inst_count(res) == 0,
    "res=%p upper_lod=%d res->getQlBestLod()=%d relLFU=%d rc=%d inst=%d", res, upper_lod, res->getQlBestLod(),
    int(dagor_frame_no() - res->getQlReqLFU()), res->getRefCount(), get_inst_count(res));
  if (upper_lod < res->getQlReqLodEff())
    res->updateReqLod(upper_lod);
  BufChunkTab out_c1, out_c2;
  int idx = find_value_idx(resList, res);
  buf.getSeparateChunks(res->getSmvd(), upper_lod, make_span(resUsedChunks[idx]), out_c1, out_c2);
  resUsedChunks[idx] = out_c1;
  if (upper_lod >= res->lods.size())
  {
    if (upper_lod != RES::qlReqLodInitialValue) // It is an initial value of qlReqLod, meaning that the resource
                                                // was not rendered in any way yet.
      logerr("upper_lod=%d while res->lods.size()=%d", upper_lod, res->lods.size());
    upper_lod = res->lods.size() - 1;
  }
  for (RES *rx = res->getFirstOriginal(); rx; rx = rx->getNextClone())
    rx->applyQlBestLod(upper_lod);

  if (!out_c2.size())
    return;

  updateLocalMaximum(dbg_level >= 0);
  buf.releaseBufChunk(out_c2, true);
  for (BufChunk &c0 : out_c2)
    (c0.vbIdx == BufPool::IDX_IB ? ibSizeToFree : vbSizeToFree) -= c0.sz;
}
template <class RES>
void ShaderResUnitedVdata<RES>::discardUnusedResToFreeReqMem()
{
  if (vbSizeToFree <= 0 && ibSizeToFree <= 0)
  {
    uselessDiscardAttempts = 0;
    return;
  }

  std::lock_guard<std::mutex> scopedLock(appendMutex);
  discardUnusedResToFreeReqMemNoLock(false);
}
template <class RES>
void ShaderResUnitedVdata<RES>::discardUnusedResToFreeReqMemNoLock(bool forced)
{
  int prev_vb_to_free = vbSizeToFree, prev_ib_to_free = ibSizeToFree;

  if (!failedVdataReloadResList.size() && !forced)
  {
    interlocked_release_store(vbSizeToFree, 0);
    interlocked_release_store(ibSizeToFree, 0);
    uselessDiscardAttempts = 0;
    return;
  }

  for (int idx = 0; idx < resList.size(); idx++)
  {
    Ptr<RES> res = resList[idx];
    if (res->lods.size() < 2 || res->getQlBestLod() >= res->lods.size() - 1 || !res->areLodsSplit())
      continue;
    bool discard_at_all = res->getRefCount() <= 2 || get_inst_count(res) == 0; // no instances created
    if (!discard_at_all)
    {
      if (res->getQlReqLFU() + 120 > dagor_frame_no())
        continue;
      if (res->getQlReqLodEff() <= res->getQlBestLod() && res->getQlReqLFU() + 600 > dagor_frame_no())
        continue;
    }

    downgradeRes(res, (discard_at_all || res->getQlReqLFU() + 600 <= dagor_frame_no()) ? res->lods.size() - 1 : res->getQlReqLodEff());
    if (vbSizeToFree <= 0 && ibSizeToFree <= 0)
      break;
  }
  if ((vbSizeToFree > 0 || ibSizeToFree > 0) && is_main_thread()) // here we can safely downgrade to reqLod
    for (int idx = 0; idx < resList.size(); idx++)
    {
      Ptr<RES> res = resList[idx];
      if (res->lods.size() < 2 || res->getQlBestLod() >= res->lods.size() - 1 || !res->areLodsSplit())
        continue;
      if (res->getQlReqLodEff() <= res->getQlBestLod())
        continue;

      downgradeRes(res, res->getQlReqLodEff());
      if (vbSizeToFree <= 0 && ibSizeToFree <= 0)
        break;
    }

  if (prev_vb_to_free <= vbSizeToFree && prev_ib_to_free <= ibSizeToFree)
    uselessDiscardAttempts++;
  if (uselessDiscardAttempts > 30 || (vbSizeToFree <= 0 && ibSizeToFree <= 0))
  {
    failedVdataReloadResList.clear();
    interlocked_release_store(vbSizeToFree, 0);
    interlocked_release_store(ibSizeToFree, 0);
    uselessDiscardAttempts = 0;
  }
}

template <class RES>
bool ShaderResUnitedVdata<RES>::addRes(dag::Span<RES *> res)
{
  int64_t reft = ref_time_ticks();
  unitedvdata::BufConfig local_hints = getHints();
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  int prev_res_count = resList.size();
  Tab<RES *> unarranged_res;
  Tab<int> dviOfs;
  Tab<uint8_t> buf_stor;
  int prev_pools = buf.pool.size();
  int vb_to_free = 0, ib_to_free = 0;

  for (RES *r : res)
  {
    BufChunkTab c;
    if (!buf.arrangeVdata(r->getSmvd(), c, buf.getIB(), true, local_hints, &vb_to_free, &ib_to_free))
      continue;
    if (!c.size())
    {
      interlocked_add(vbSizeToFree, vb_to_free);
      interlocked_add(ibSizeToFree, ib_to_free);
      unarranged_res.push_back(r);
      continue;
    }

    resList.push_back(r);

    if (buf.allowDelRes)
      resUsedChunks.push_back() = c;
  }

  if (resList.size() > prev_res_count)
  {
    // just append data to shared buffer, required space is available
    dviOfs.reserve(8 * 2);

    if (buf.allowRebuild && !buf.createSbuffers(local_hints))
      logerr("unitedVdata<%s>: failed allocate one or more pre-reserved buffers (%d VBs, %s)", RES::getStaticClassName(),
        buf.getVbCount(), buf.getStatStr());

    for (int i = prev_res_count; i < resList.size(); i++)
      updateVdata(resList[i], buf, dviOfs, buf_stor, getBufChunks(i));
    if (dbg_level > 0 || buf.pool.size() > prev_pools)
      debug("unitedVdata<%s>: added %d res (%d total), %d VBs, %s for %d usec", RES::getStaticClassName(),
        resList.size() - prev_res_count, resList.size(), buf.getVbCount(), buf.getStatStr(), get_time_usec(reft));
  }

  if (unarranged_res.size() && buf.allowDelRes) // not enough free mem, discard unused res and retry
  {
    debug("unitedVdata<%s>: IB/VB shortage in addRes(%d res), unarranged_res=%d, needIB=%dK, needVB=%dK), trying to discard unused",
      RES::getStaticClassName(), res.size(), unarranged_res.size(), ibSizeToFree >> 10, vbSizeToFree >> 10);
    discardUnusedResToFreeReqMemNoLock(true);
    Tab<RES *> res2(eastl::move(unarranged_res));
    for (RES *r : res2)
    {
      BufChunkTab c;
      if (!buf.arrangeVdata(r->getSmvd(), c, buf.getIB(), true, local_hints))
        continue;
      if (!c.size())
      {
        unarranged_res.push_back(r);
        continue;
      }

      resList.push_back(r);
      if (buf.allowDelRes)
        resUsedChunks.push_back() = c;
    }
  }
  if (unarranged_res.size() == 0)
  {
    ShaderMatVdata::closeTlsReloadCrd();
    return resList.size() > prev_res_count;
  }

  // clean unreferenced res first
  for (int i = 0; i < resList.size(); i++)
    if (resList[i].get() && resList[i]->getRefCount() == 1)
    {
      resList[i] = NULL;
      if (i + 1 < resList.size())
        resList[i] = resList.back();
      resList.pop_back();
      if (buf.allowDelRes)
      {
        if (i + 1 < resUsedChunks.size())
          resUsedChunks[i] = eastl::move(resUsedChunks[resList.size() - 1]);
        resUsedChunks.pop_back();
      }
      i--;
    }

  if (!allowSepTightVdata)
  {
    // we cannot create res with vdata in separate tight buffers, so just rebuild immediately (if allowed)
    G_ASSERTF_RETURN(buf.allowRebuild, false, "unitedVdata<%s> OOM, but allowSepTightVdata=%d and allowRebuild=%d, %s",
      RES::getStaticClassName(), allowSepTightVdata, buf.allowRebuild, buf.getStatStr());

    logwarn("unitedVdata<%s>: starting rebuild (for %d unarranged res), total %d res to gather", RES::getStaticClassName(),
      unarranged_res.size(), resList.size() + unarranged_res.size());
    Tab<RES *> new_res;
    new_res.resize(resList.size());
    for (int i = 0; i < resList.size(); i++)
      new_res[i] = resList[i];
    append_items(new_res, unarranged_res.size(), unarranged_res.data());
    rebuildUnitedVdata(make_span(new_res), false);
    return true;
  }

  // add unarranged res to the end
  prev_res_count = resList.size();
  resList.reserve(unarranged_res.size());
  String tight_stats, tmp_str;
  bool bad_tight_detected = false;
  for (RES *r : unarranged_res)
    if (find_value_idx(resList, r) < 0)
      for (ShaderMatVdata *smvd : r->getSmvd())
        if (smvd && smvd->getGlobVDataCount() > 0 && (!buf.getIB() || smvd->getGlobVData(0)->indices != buf.getIB()))
        {
          resList.push_back(r);

          BufPool tmp_buf; // we shall not clear this temporary buffer since sbuf[] get owned by res later
          BufConfig tmp_hint;
          BufChunkTab c;
          tmp_buf.allowRebuild = true; //-V1048 // mark as pre-reserve only, without creating Sbuffers in arrangeVdata()
          if (tmp_buf.arrangeVdata(r->getSmvd(), c, nullptr, false, tmp_hint))
          {
            if (!buf.allowRebuild)
            {
              tmp_buf.calcUsedSizeStr(c, tmp_str);
              tight_stats += " " + tmp_str;
            }
            if (!tmp_buf.createSbuffers(tmp_hint, true))
              logerr("unitedVdata<%s>: failed to alloc tight buffers (%s)", RES::getStaticClassName(), tight_stats);
            unsigned tb_size = 0;
            for (const BufChunk &c : c)
              tb_size += c.sz;
            if (r->lods.size() > 1 || tb_size < (1 << 20))
              bad_tight_detected = true;
            updateVdata(r, tmp_buf, dviOfs, buf_stor, c);

            // reset vbIdx to mark buffers as not combined
            for (ShaderMatVdata *smvd : r->getSmvd())
              if (smvd)
                for (int i = 0, ie = smvd->getGlobVDataCount(); i < ie; i++)
                  smvd->getGlobVData(i)->vbIdx = 0;
          }
          break;
        }
  if (buf.allowDelRes)
    resUsedChunks.resize(resList.size());
  ShaderMatVdata::closeTlsReloadCrd();
  if (!buf.allowRebuild)
  {
    logmessage(bad_tight_detected ? LOGLEVEL_ERR : LOGLEVEL_WARN,
      "unitedVdata<%s>: arranged %d res in tight buffers:%s; setup *.gameparams.blk unitedVdata.%s{} ! now %d VBs, %s",
      RES::getStaticClassName(), resList.size() - prev_res_count, tight_stats, RES::getStaticClassName(), buf.getVbCount(),
      buf.getStatStr());
    return true;
  }

  // schedule rebuild
  if (interlocked_increment(pendingRebuildCount) == 1)
    debug("unitedVdata<%s>: scheduling rebuild (for %d unarranged res), pendCnt=%d, now %d VBs, %s", RES::getStaticClassName(),
      resList.size() - prev_res_count, pendingRebuildCount, buf.getVbCount(), buf.getStatStr());

  struct RebuildUnitedVdataAction : public DelayedAction
  {
    ShaderResUnitedVdata<RES> *me;
    RebuildUnitedVdataAction(ShaderResUnitedVdata<RES> *_this) : me(_this) {}
    virtual void performAction() override
    {
      if (interlocked_decrement(me->pendingRebuildCount) == 0)
        me->rebuildBuffersAfterReset();
    }
  };
  add_delayed_action(new RebuildUnitedVdataAction(this));
  return true;
}
template <class RES>
void ShaderResUnitedVdata<RES>::rebuildBuffersAfterReset()
{
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  Tab<RES *> new_res;
  new_res.resize(resList.size());
  for (int i = 0; i < resList.size(); i++)
    new_res[i] = resList[i];
  rebuildUnitedVdata(make_span(new_res), true);
}

template <class RES>
void ShaderResUnitedVdata<RES>::setMaxVbSize(int max_sz)
{
  buf.maxVbSize = (max_sz + ALIGN_1M) & ~ALIGN_1M;
  debug("unitedVdata<%s>: set max VB size %dM", RES::getStaticClassName(), buf.maxVbSize >> 20);
}
template <class RES>
void ShaderResUnitedVdata<RES>::setDelResAllowed(bool allow)
{
  G_ASSERTF(resList.size() == 0 || !allow, "resList.size()=%d allow=%d", resList.size(), allow);
  buf.allowDelRes = allow;
  if (!buf.allowDelRes)
    clear_and_shrink(resUsedChunks);
}
template <class RES>
bool ShaderResUnitedVdata<RES>::delRes(RES *res)
{
  G_ASSERT_RETURN(buf.allowDelRes, false);
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  int idx = find_value_idx(resList, res);
  if (idx < 0)
    return false;

  updateLocalMaximum(dbg_level >= 0);
  buf.resetVdataBufPointers(resList[idx]->getSmvd());
  on_mesh_relems_updated.fire(res);
  erase_items(resList, idx, 1);
  buf.releaseBufChunk(resUsedChunks[idx], true);
  erase_items(resUsedChunks, idx, 1);
  if (dbg_level > 0)
    debug("unitedVdata<%s>: removed %d res (%d total), %d VBs, %s", RES::getStaticClassName(), 1, resList.size(), buf.getVbCount(),
      buf.getStatStr());
  return true;
}

template <class RES>
void ShaderResUnitedVdata<RES>::stopPendingJobs()
{
  if (reloadJobMgrId >= 0)
  {
    int t0 = get_time_msec();
#if _TARGET_C1 | _TARGET_C2 // Currently only PSX has reason to delay update jobs apply


#endif
    cpujobs::reset_job_queue(reloadJobMgrId);
    for (int cnt = 1000; cnt > 0; cnt--)
      if (cpujobs::is_job_manager_busy(reloadJobMgrId))
        sleep_msec(1);
      else
        break;
    cpujobs::reset_job_queue(reloadJobMgrId);
    if (cpujobs::is_job_manager_busy(reloadJobMgrId) || getPendingReloadResCount())
      logwarn("failed to finish pending jobs for %d msec, busy=%d pendReload=%d", get_time_msec() - t0,
        cpujobs::is_job_manager_busy(reloadJobMgrId), getPendingReloadResCount());
  }
}
template <class RES>
void ShaderResUnitedVdata<RES>::releaseUnusedBuffers()
{
  std::lock_guard<std::mutex> scopedLock1(appendMutex);
  std::lock_guard<std::mutex> scopedLock2(buf.updateMutex);
  for (int i = buf.pool.size() - 1; i > 0; i--)
    if (buf.pool[i].getUsed() || buf.freeChunks[i].size() > 1 ||
        (buf.freeChunks[i].size() == 1 && (buf.freeChunks[i][0].ofs != 0 || buf.freeChunks[i][0].sz != buf.pool[i].getSize())))
      break;
    else
    {
      int top = BufChunk::find_top_chunk(buf.freeChunks[i]);
      debug("unitedVdata<%s>: release unused sbuf[%d]=%p, pool.size=%d freeChunks.count=%d lastFree=(%d;%d)",
        RES::getStaticClassName(), i, buf.sbuf[i], buf.pool[i].getSize(), buf.freeChunks[i].size(),
        top >= 0 ? buf.freeChunks[i][top].ofs : -1, top >= 0 ? buf.freeChunks[i][top].sz : 0);
      del_d3dres(buf.sbuf[i]);
      buf.sbuf.pop_back();
      buf.pool.pop_back();
      buf.freeChunks[i].clear();
    }
}
template <class RES>
void ShaderResUnitedVdata<RES>::clear()
{
  stopPendingJobs();
  G_ASSERTF(!getPendingReloadResCount(), "pendingVdataReloadResCount=%d", getPendingReloadResCount());

  std::lock_guard<std::mutex> scopedLock(appendMutex);
  if (buf.getIB())
  {
    if (buf.allowDelRes)
      debug("unitedVdata<%s>: clear (%d res), %d VBs, %s (maximum buf used %dM+%dM=%dM)", RES::getStaticClassName(), resList.size(),
        buf.getVbCount(), buf.getStatStr(), maxIbTotalUsed >> 20, maxVbTotalUsed >> 20, (maxIbTotalUsed + maxVbTotalUsed) >> 20);
    else
      debug("unitedVdata<%s>: clear (%d res), %d VBs, %s", RES::getStaticClassName(), resList.size(), buf.getVbCount(),
        buf.getStatStr());

    for (RES *r : resList)
      buf.resetVdataBufPointers(r->getSmvd());
  }

  buf.clear();
  clear_and_shrink(resList);
  clear_and_shrink(resUsedChunks);
}

template <class RES>
void ShaderResUnitedVdata<RES>::updateLocalMaximum(bool out_peak_to_debug)
{
  int64_t ib_total = 0, vb_total = 0;
  for (int i = 0; i < buf.pool.size(); i++)
    (i == buf.IDX_IB ? ib_total : vb_total) += buf.pool[i].getUsed();

  if (maxIbTotalUsed < ib_total || maxVbTotalUsed < vb_total)
  {
    maxIbTotalUsed = ib_total;
    maxVbTotalUsed = vb_total;
    if (out_peak_to_debug)
      debug("unitedVdata<%s>: local maximum (%d res), %d VBs, %s [%dM+%dM=%dM]", RES::getStaticClassName(), resList.size(),
        buf.getVbCount(), buf.getStatStr(), maxIbTotalUsed >> 20, maxVbTotalUsed >> 20, (maxIbTotalUsed + maxVbTotalUsed) >> 20);
  }
}
template <class RES>
void ShaderResUnitedVdata<RES>::buildStatusStr(String &out_str, bool full_res_list, bool (*resolve_res_name)(String &nm, RES *r))
{
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  out_str.printf(0, "registered %d res, %d VBs, %s", resList.size(), buf.getVbCount(), buf.getStatStr());
  if (buf.allowDelRes)
  {
    updateLocalMaximum(false);
    out_str.aprintf(0, "\nmax buf used [%dM+%dM=%dM] pendingVdataReloadResCount=%d", maxIbTotalUsed >> 20, maxVbTotalUsed >> 20,
      (maxIbTotalUsed + maxVbTotalUsed) >> 20, getPendingReloadResCount());
    if (vbSizeToFree > 0 || ibSizeToFree > 0)
      out_str.aprintf(0, "\nfailedVdataReloadResList=%d vbShortage=%dK ibShortage=%dK uselessDiscardAttempts=%d",
        failedVdataReloadResList.size(), vbSizeToFree >> 10, ibSizeToFree >> 10, uselessDiscardAttempts);
    if (full_res_list)
      out_str += "\nfull registered statistics dumped to debug";
  }

  if (full_res_list)
  {
    String tmp_stor;
    String tmp_nm;
    debug("");
    debug("unitedVdata<%s>: registered %d res, frame=%d", RES::getStaticClassName(), resList.size(), dagor_frame_no());
    for (int i = 0; i < resList.size(); i++)
      if (resList[i]->getQlReqLFU() && resList[i]->lods.size() > 1)
      {
        bool know_name = resolve_res_name ? resolve_res_name(tmp_nm, resList[i]) : false;
        debug("%c%cres[%3d]=%p ldLod=%d (%d lods)  qlReqLod=%u  qlReqLFU=%-5u %s LFU.rel=%-4d%c%*s rc=%d inst=%d",
          resList[i]->getResLoadingFlag() ? '*' : ' ', find_value_idx(failedVdataReloadResList, resList[i]) >= 0 ? '-' : ' ', i,
          resList[i], resList[i]->getQlBestLod(), resList[i]->lods.size(), resList[i]->getQlReqLodEff(), resList[i]->getQlReqLFU(),
          BufPool::calcUsedSizeStr(getBufChunks(i), tmp_stor), dagor_frame_no() - resList[i]->getQlReqLFU(),
          (dagor_frame_no() > resList[i]->getQlReqLFU() + 600 && resList[i]->getQlBestLod() != resList[i]->lods.size() - 1) ||
              (dagor_frame_no() > resList[i]->getQlReqLFU() + 120 && resList[i]->getQlBestLod() < resList[i]->getQlReqLodEff())
            ? '*'
            : ' ',
          know_name ? tmp_nm.length() + 2 : 0, know_name ? tmp_nm.str() : "", resList[i]->getRefCount(), get_inst_count(resList[i]));
      }
    for (int i = 0; i < resList.size(); i++)
      if (!resList[i]->getQlReqLFU() || resList[i]->lods.size() <= 1)
      {
        bool know_name = resolve_res_name ? resolve_res_name(tmp_nm, resList[i]) : false;
        debug("  res[%3d]=%p ldLod=%d (%d lods) %s%*s", i, resList[i], resList[i]->getQlBestLod(), resList[i]->lods.size(),
          BufPool::calcUsedSizeStr(getBufChunks(i), tmp_stor), know_name ? tmp_nm.length() + 2 : 0, know_name ? tmp_nm.str() : "");
      }
  }
}
template <class RES>
void ShaderResUnitedVdata<RES>::dumpMemBlocks(String *out_str_summary)
{
  std::lock_guard<std::mutex> scopedLock(appendMutex);
  if (out_str_summary)
    *out_str_summary = buf.getStatStr();
  debug("");
  debug("unitedVdata<%s>: allocated %d pools", RES::getStaticClassName(), buf.pool.size());
  for (int i = 0; i < buf.pool.size(); i++)
  {
    debug("%s[%d]: bufSz=%dK, %d free chunks", i == buf.IDX_IB ? "IB" : "VB", i, buf.pool[i].getSize() >> 10,
      buf.freeChunks[i].size());
    int sum = 0, max_c = 0, last_ofs = 0, used = 0;
    stlsort::sort(buf.freeChunks[i].data(), buf.freeChunks[i].data() + buf.freeChunks[i].size(),
      [&](auto a, auto b) { return a.ofs < b.ofs; });
    for (const BufChunk &c : buf.freeChunks[i])
    {
      if (c.ofs > last_ofs)
      {
        used += c.ofs - last_ofs;
        debug("    -- used %7d", c.ofs - last_ofs);
      }
      debug("  start=0x%08X end=0x%08X sz=%7d", c.ofs, c.end(), c.sz);
      last_ofs = c.end();
      sum += c.sz;
      if (max_c < c.sz)
        max_c = c.sz;
    }
    if (sum > 0)
      debug("  --- total free %dK (%d), max_chunk=%dK (%d), total used %dK (%dK)", sum >> 10, sum, max_c >> 10, max_c, used >> 10,
        buf.pool[i].getUsed() >> 10);
  }
}

template class ShaderResUnitedVdata<RenderableInstanceLodsResource>;
template class ShaderResUnitedVdata<DynamicRenderableSceneLodsResource>;
