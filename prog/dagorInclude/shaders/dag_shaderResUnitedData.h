//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driver.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <shaders/dag_shaderMesh.h>
#include <util/dag_delayedAction.h>
#include <mutex>
#include <util/dag_multicastEvent.h>


class DataBlock;

namespace unitedvdata
{
static constexpr int MAX_VBIDX_CNT = 16;
static constexpr int MAX_CHUNK_CNT = 32;

struct BufConfig
{
  BufConfig();
  BufConfig(const DataBlock &hints_blk, int maxVbSize);
  union
  {
    unsigned ibSz;
    unsigned vbSz[MAX_VBIDX_CNT];
  };
  unsigned ibMinAdd, vbMinAdd;
  unsigned ibMaxAdd, vbMaxAdd;
  unsigned ibAddPromille, vbAddPromille;
  bool exactSize = false;
  bool dumpResListOnAddResFail = true;
};

struct BufChunk
{
  unsigned ofs = 0, sz : 28, vbIdx : 4;
  BufChunk() : ofs(0), sz(0), vbIdx(0) {}
  BufChunk(unsigned o, unsigned s, unsigned i) : ofs(o), sz(s), vbIdx(i) { G_ASSERTF(i == vbIdx, "i=%d", i); }

  unsigned end() const { return ofs + sz; }

  template <class TAB>
  static void add_chunk(TAB &used_chunks, const BufChunk &new_chunk);
  static int find_top_chunk(dag::ConstSpan<BufChunk> used_chunks);
  static int find_chunk(dag::ConstSpan<BufChunk> used_chunks, int req_sz);
  template <class TAB>
  static void cut_chunk(TAB &used_chunks, int chunk_idx, BufChunk &new_chunk, int min_gap);
};

typedef StaticTab<BufChunk, MAX_CHUNK_CNT> BufChunkTab;

struct BufPool
{
  enum
  {
    IDX_IB = 0,
    IDX_VB_START = 1
  };
  struct PoolSize
  {
    int getUsed() const { return interlocked_acquire_load(used); }
    int getSize() const { return interlocked_acquire_load(size); }
    void setUsed(int u) { interlocked_release_store(used, u); }
    void setSize(int s) { interlocked_release_store(size, s); }
    void incUsed(int u) { interlocked_add(used, u); }
    void decUsed(int u) { interlocked_add(used, -u); }

  protected:
    int used = 0, size = 0;
  };

  StaticTab<Sbuffer *, MAX_VBIDX_CNT> sbuf;
  StaticTab<PoolSize, MAX_VBIDX_CNT> pool;
  Tab<BufChunk> freeChunks[MAX_CHUNK_CNT];
  int maxVbSize = 64 << 20;
  bool allowRebuild = true, allowDelRes = false;
  char nameChar;
  std::mutex updateMutex;

  BufPool(char nc) : nameChar(nc) { sbuf.push_back(nullptr); } // pre-alloc for IB
  BufChunk allocChunkForStride(int stride, int req_avail_sz, const BufConfig &hints);
  bool arrangeVdata(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, BufChunkTab &out_c, Sbuffer *ib, bool can_fail,
    const BufConfig &hints, int *vbShortage = nullptr, int *ibShortage = nullptr);
  void resetVdataBufPointers(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list);
  void getSeparateChunks(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, int first_lod, dag::Span<BufChunk> c, BufChunkTab &out_c1,
    BufChunkTab &out_c2);
  void releaseBufChunk(dag::ConstSpan<BufChunk> ctab, bool update_used)
  {
    for (const BufChunk &c : ctab)
    {
      if (update_used)
        pool[c.vbIdx].decUsed(c.sz);
      BufChunk::add_chunk(freeChunks[c.vbIdx], c);
    }
  }
  static const char *calcUsedSizeStr(dag::ConstSpan<BufChunk> ctab, String &stor);

  bool allocateBuffer(int idx, size_t size, const char *name_fmt = "%cunited_%cb#%x");
  bool allocatePool(int idx, size_t hint_sz, const BufConfig &hints);
  bool createSbuffers(const BufConfig &hints, bool tight = false);
  void clear();

  String getStatStr() const;
  int getVbCount() const { return sbuf.size() > IDX_VB_START ? sbuf.size() - IDX_VB_START : 0; }
  Ibuffer *getIB() const { return sbuf[IDX_IB]; }
};
} // namespace unitedvdata

template <class RES>
class ShaderResUnitedVdata
{
public:
  using ResType = RES;

  ShaderResUnitedVdata();

  void setMaxVbSize(int max_sz);
  void setDelResAllowed(bool allow);
  void setSepTightVdataAllowed(bool allow) { allowSepTightVdata = allow; }
  void setRebuildAllowed(bool allow) { buf.allowRebuild = allow; }

  bool addRes(dag::Span<RES *> res);
  bool addRes(RES *res) { return addRes(make_span(&res, 1)); }
  bool delRes(RES *res);

  bool reloadRes(RES *res);
  void downgradeRes(RES *res, int upper_lod);
  void discardUnusedResToFreeReqMem();

  void releaseUnusedBuffers();
  void clear();
  void stopPendingJobs();
  void rebuildBuffersAfterReset();

  void onBeforeD3dReset()
  {
    stopPendingJobs();
    appendMutex.lock();
  }
  void onAfterD3dReset()
  {
    appendMutex.unlock();
    rebuildBuffersAfterReset();
  }

  Ibuffer *getIB() const { return buf.getIB(); }
  Vbuffer *getVB(uint32_t vb_idx) const { return buf.sbuf[vb_idx]; }

  int getResCount() const { return resList.size(); }

  void buildStatusStrNoLock(String &out_str, bool full_res_list, bool (*resolve_res_name)(String &nm, const RES *r) = nullptr);
  void buildStatusStr(String &out_str, bool full_res_list, bool (*resolve_res_name)(String &nm, const RES *r) = nullptr)
  {
    std::lock_guard<std::mutex> scopedLock(appendMutex);
    buildStatusStrNoLock(out_str, full_res_list, resolve_res_name);
  }
  void dumpMemBlocks(String *out_str_summary = nullptr);
  int getPendingReloadResCount() const { return interlocked_acquire_load(pendingVdataReloadResCount); }
  int getFailedReloadResCount() const { return failedVdataReloadResList.size(); }
  void setHints(const DataBlock &hints_blk);

  // This even is triggered whenever a resource was changed due to
  // vertex buffer defrag and things like RElem::bv and RElem::si are no
  // longer the same as before.
  inline static MulticastEvent<void(const RES *, bool)> on_mesh_relems_updated;

  typedef void (*enumRElemFct)(const RES *);
  void enumRElems(enumRElemFct enum_cb);

  inline int getRequestLodsByDistanceFrames() const { return requestLodsByDistanceFrames; }

protected:
  unitedvdata::BufPool buf;
  PtrTab<RES> resList;
  std::mutex appendMutex;
  bool allowSepTightVdata = false;
  volatile int pendingRebuildCount = 0;
  Tab<SmallTab<unitedvdata::BufChunk, MidmemAlloc>> resUsedChunks;
  int64_t maxIbTotalUsed = 0, maxVbTotalUsed = 0;
  int reloadJobMgrId = -1;
  volatile int vbSizeToFree = 0, ibSizeToFree = 0;
  int uselessDiscardAttempts = 0;
  Tab<RES *> failedVdataReloadResList;
  volatile int pendingVdataReloadResCount = 0;
  unitedvdata::BufConfig hints;
  mutable std::mutex hintsMutex;
  int noDiscardFrames = 120;
  int keepIfNeedLodFrames = 600;
  int requestLodsByDistanceFrames = 60;

  unitedvdata::BufConfig getHints() const;

  void rebuildUnitedVdata(dag::Span<RES *> res, bool in_d3d_reset);
  static void updateVdata(RES *r, unitedvdata::BufPool &buf, Tab<int> &dviOfs_stor, Tab<uint8_t> &buf_stor,
    dag::ConstSpan<unitedvdata::BufChunk> c);

  static void prepareVdataBaseOfs(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, Tab<int> &dviOfs);
  static void updateVdata(dag::ConstSpan<Ptr<ShaderMatVdata>> smvd_list, unitedvdata::BufPool &buf, Tab<uint8_t> &buf_stor,
    dag::ConstSpan<unitedvdata::BufChunk> c);
  static void rebaseElemOfs(const RES *r, dag::ConstSpan<int> dviOfs);

  inline dag::ConstSpan<unitedvdata::BufChunk> getBufChunks(int i) const
  {
    return buf.allowDelRes ? make_span_const(resUsedChunks[i]) : dag::ConstSpan<unitedvdata::BufChunk>();
  }

  void updateLocalMaximum(bool out_peak_to_debug);
  void discardUnusedResToFreeReqMemNoLock(bool forced);

  struct UpdateModelCtx
  {
    Ptr<ShaderMatVdata> tmp_smvd[4];
    unitedvdata::BufChunkTab cPrev;
    Tab<int> dviOfs;
    Ptr<RES> res = nullptr;
    unsigned reqLod = 16;
  };
  struct ReleaseUpdateDA final : public DelayedAction, public UpdateModelCtx
  {
    ShaderResUnitedVdata *unitedVdata;
    ReleaseUpdateDA *prev = nullptr;
    static inline ReleaseUpdateDA *slist_tail = nullptr;
    ReleaseUpdateDA(UpdateModelCtx &&ctx, ShaderResUnitedVdata *uvd) : UpdateModelCtx(eastl::move(ctx)), unitedVdata(uvd)
    {
      prev = slist_tail;
      slist_tail = this;
    }
    ~ReleaseUpdateDA()
    {
      for (ReleaseUpdateDA **pcur = &slist_tail; *pcur; pcur = &(*pcur)->prev) // Erase from slist
        if (*pcur == this)
        {
          *pcur = prev;
          return;
        }
      G_ASSERT(0); // Can't happen - DA wasn't found in slist?
    }
    void performAction() override { unitedVdata->releaseUpdateJob(*this); }
    void ceaseUpdateJob() { UpdateModelCtx::res.get() ? unitedVdata->ceaseUpdateJob(*this) : (void)0; }
  };
  inline void initUpdateJob(UpdateModelCtx &ctx, RES *r);
  inline void doUpdateJob(UpdateModelCtx &ctx);
  inline void releaseUpdateJob(UpdateModelCtx &ctx);
  inline void ceaseUpdateJob(UpdateModelCtx &ctx)
  {
    G_VERIFY(interlocked_decrement(pendingVdataReloadResCount) >= 0);
    ctx.res->setResLoadingFlag(false);
    ctx.res = nullptr;
  }
};

class DynamicRenderableSceneLodsResource;
class RenderableInstanceLodsResource;

namespace unitedvdata
{
extern ShaderResUnitedVdata<DynamicRenderableSceneLodsResource> dmUnitedVdata;
extern int dmUnitedVdataUsed;

extern ShaderResUnitedVdata<RenderableInstanceLodsResource> riUnitedVdata;
} // namespace unitedvdata
