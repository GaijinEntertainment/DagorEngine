// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/shLimits.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_stcode.h>
#include "shadersBinaryData.h"
#include "shStateBlk.h"
#include "shStateBlock.h"
#include "mapBinarySearch.h"
#include "shBindumpsPrivate.h"
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <startup/dag_restart.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdio.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_set.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_threadSafety.h>
#include "shaders/sh_vars.h"
#include <supp/dag_alloca.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_driverDesc.h>
#include <3d/tql.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_linkedListOfShadervars.h>

//

const char *ShaderMaterial::loadingStirngInfo = NULL;
#if DAGOR_DBGLEVEL > 0
#include <shaders/dag_shaderDbg.h>
#include <startup/dag_globalSettings.h>
namespace shaderbindump
{
extern void dump_shader_var_names();
}
#endif

namespace shaderbindump
{
uint32_t get_generation() { return shBinDumpOwner().generation; }
} // namespace shaderbindump

// set sting for error info while loading material. don't forget to clear it after loading!
void ShaderMaterial::setLoadingString(const char *s) { loadingStirngInfo = s; }

// get stirng for error info. return NULL, if no error string set
const char *ShaderMaterial::getLoadingString() { return loadingStirngInfo; }


static float shadersGlobalTime = 0.f;
static double shadersGlobalTimeDouble = 0.0;

float get_shader_global_time() { return shadersGlobalTime; }

void set_shader_global_time(float t)
{
  shadersGlobalTime = t;
  shadersGlobalTimeDouble = t;
}

void advance_shader_global_time(float dt)
{
  shadersGlobalTimeDouble += double(dt);
  shadersGlobalTime = float(shadersGlobalTimeDouble);
}


static float frac(float v) { return v - floorf(v); }

real get_shader_global_time_phase(float period, float offset)
{
  if (period < VERY_SMALL_NUMBER)
    return shadersGlobalTime;
  else
    return frac((shadersGlobalTime + offset) / period); // The maximum time value is limited by the conversion from int, a lot of
                                                        // precision to calculate in floats.
}

void ShaderGlobal::reset_textures(bool removed_tex_only)
{
  auto &dumpOwner = shBinDumpOwner();
  auto &globalData = shGlobalData();
  auto &state = globalData.globVarsState;
  const auto &vars = shBinDump().globVars;

  for (int i = 0, e = state.size(); i < e; i++)
  {
    const auto type = vars.getType(i);

    if (type == SHVT_TEXTURE)
    {
      auto &tex = state.get<shaders_internal::Tex>(i);
      if (tex.isTextureManaged())
      {
        if (removed_tex_only)
          if (get_managed_texture_name(tex.texId) || get_managed_texture_refcount(tex.texId) <= 0)
            continue;
        release_managed_tex(tex.texId);
      }
      else
      {
#if DAGOR_DBGLEVEL > 0
        auto oldIt = globalData.globResourceVarPointerUsageMap.find(tex.tex);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(tex.tex);
          }
        }
#endif
      }
      tex.texId = BAD_TEXTUREID;
      tex.tex = nullptr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = 0;
    }
    else if (type == SHVT_BUFFER)
    {
      auto &buf = state.get<shaders_internal::Buf>(i);
      if (buf.isBufferManaged())
      {
        if (removed_tex_only)
          if (get_managed_res_name(buf.bufId) || get_managed_res_refcount(buf.bufId) <= 0)
            continue;
        release_managed_res(buf.bufId);
      }
      else
      {
#if DAGOR_DBGLEVEL > 0
        auto oldIt = globalData.globResourceVarPointerUsageMap.find(buf.buf);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(buf.buf);
          }
        }
#endif
      }
      buf.bufId = BAD_D3DRESID;
      buf.buf = nullptr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = 0;
    }
  }
}

void ShaderGlobal::reset_stale_vars()
{
  auto &state = shGlobalData().globVarsState;
  const auto &vars = shBinDump().globVars;
  for (int i = 0, e = state.size(); i < e; i++)
    if (vars.getType(i) == SHVT_TLAS)
      state.set<RaytraceTopAccelerationStructure *>(i, nullptr);
}

void ShaderGlobal::sync_managed_resource(TEXTUREID id, D3dResource *old_res_ptr)
{
  if (id == BAD_TEXTUREID)
    return;

  auto &globalData = shGlobalData();
  auto &state = globalData.globVarsState;
  const auto &vars = shBinDump().globVars;

  for (int i = 0, e = state.size(); i < e; i++)
  {
    auto type = vars.getType(i);
    if (type == SHVT_TEXTURE)
    {
      auto &tex = state.get<shaders_internal::Tex>(i);

      if (tex.isTextureManaged() && tex.texId != id)
        continue;

      if (tex.isTextureUnmanaged() && (!old_res_ptr || tex.tex != old_res_ptr))
        continue;

      auto *actualTexPtr = (BaseTexture *)D3dResManagerData::getD3dRes(id);

#if DAGOR_DBGLEVEL > 0
      if (tex.isTextureUnmanaged())
      {

        auto oldIt = globalData.globResourceVarPointerUsageMap.find(tex.tex);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(tex.tex);
          }
        }

        if (actualTexPtr)
        {
          auto &newCounter = globalData.globResourceVarPointerUsageMap[actualTexPtr];
          ++newCounter;
        }
      }
#endif

      tex.tex = actualTexPtr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = static_cast<uint8_t>(tex.tex != nullptr);
    }
    else if (type == SHVT_BUFFER)
    {
      auto &buf = state.get<shaders_internal::Buf>(i);

      if (buf.isBufferManaged() && buf.bufId != id)
        continue;

      if (buf.isBufferUnmanaged() && (!old_res_ptr || buf.buf != old_res_ptr))
        continue;

      auto *actualBufPtr = (Sbuffer *)D3dResManagerData::getD3dRes(id);

#if DAGOR_DBGLEVEL > 0
      if (buf.isBufferUnmanaged())
      {

        auto oldIt = globalData.globResourceVarPointerUsageMap.find(buf.buf);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(buf.buf);
          }
        }

        if (actualBufPtr)
        {
          auto &newCounter = globalData.globResourceVarPointerUsageMap[actualBufPtr];
          ++newCounter;
        }
      }

#endif

      buf.buf = actualBufPtr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = static_cast<uint8_t>(buf.buf != nullptr);
    }
  }
}

void ShaderGlobal::reset_from_vars(TEXTUREID id)
{
  if (id == BAD_TEXTUREID)
    return;

  auto *resPtr = D3dResManagerData::getD3dRes(id);

  auto &globalData = shGlobalData();
  auto &state = globalData.globVarsState;
  const auto &vars = shBinDump().globVars;

  for (int i = 0, e = state.size(); i < e; i++)
  {
    auto type = vars.getType(i);
    if (type == SHVT_TEXTURE)
    {
      auto &tex = state.get<shaders_internal::Tex>(i);

      G_ASSERTF(!(tex.texId == id && tex.tex != resPtr),
        "inconsistent texture shader var binding. bound resource ptr: %p | managed resource ptr: %p", tex.tex, resPtr);

      if (tex.isTextureManaged())
      {
        if (tex.texId != id)
          continue;
        if (get_managed_texture_refcount(id) > 0)
          release_managed_tex(id);
      }
      else
      {
        if (tex.tex != resPtr)
          continue;
#if DAGOR_DBGLEVEL > 0
        auto oldIt = globalData.globResourceVarPointerUsageMap.find(tex.tex);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(tex.tex);
          }
        }
#endif
      }

      tex.texId = BAD_TEXTUREID;
      tex.tex = nullptr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = 0;
    }
    else if (type == SHVT_BUFFER)
    {
      auto &buf = state.get<shaders_internal::Buf>(i);

      G_ASSERTF(!(buf.bufId == id && buf.buf != resPtr),
        "inconsistent buffer shader var binding. bound resource ptr: %p | managed resource ptr: %p", buf.buf, resPtr);

      if (buf.isBufferManaged())
      {
        if (buf.bufId != id)
          continue;
        if (get_managed_res_refcount(id) > 0)
          release_managed_res(id);
      }
      else
      {
        if (buf.buf != resPtr)
          continue;
#if DAGOR_DBGLEVEL > 0
        auto oldIt = globalData.globResourceVarPointerUsageMap.find(buf.buf);
        if (oldIt != globalData.globResourceVarPointerUsageMap.end())
        {
          if (--oldIt->second <= 0)
          {
            globalData.globResourceVarPointerUsageMap.erase(buf.buf);
          }
        }
#endif
      }
      buf.bufId = BAD_D3DRESID;
      buf.buf = nullptr;

      int iid = globalData.globVarIntervalIdx[i];
      if (iid >= 0)
        globalData.globIntervalNormValues[iid] = 0;
    }
  }
}

//

struct ShaderVDeclVsd
{
  uint32_t hash = 0; // keep hash so we can faster select
  Tab<VSDTYPE> vsd;
  ShaderVDeclVsd() = default;
  ShaderVDeclVsd(IMemAlloc *a) : vsd(a) {}
  ShaderVDeclVsd(const ShaderVDeclVsd &) = default;
  bool operator==(const ShaderVDeclVsd &a) const { return a.hash == hash && a.vsd.size() == vsd.size() && mem_eq(vsd, a.vsd.data()); }
  bool operator<(const ShaderVDeclVsd &a) const
  {
    if (hash != a.hash)
      return hash < a.hash;
    if (vsd.size() != a.vsd.size())
      return vsd.size() < a.vsd.size();
    return memcmp(vsd.data(), a.vsd.data(), data_size(vsd)) < 0;
  }
};

struct ShaderVDecl
{
  VDECL vdecl = BAD_VDECL;
  ShaderVDecl(VDECL vd) : vdecl(vd) {}
  ~ShaderVDecl()
  {
    if (vdecl != BAD_VDECL)
      d3d::delete_vdecl(vdecl);
  }
  EA_NON_COPYABLE(ShaderVDecl)
  ShaderVDecl(ShaderVDecl &&other) : vdecl(other.vdecl) { other.vdecl = BAD_VDECL; }
  ShaderVDecl &operator=(ShaderVDecl &&a)
  {
    eastl::swap(vdecl, a.vdecl);
    return *this;
  }
};

static eastl::vector_map<ShaderVDeclVsd, ShaderVDecl> shader_vdecl_map;
static WinCritSec shader_vdecl_map_mutex;
struct ShaderVdeclAutoLock
{
  ShaderVdeclAutoLock() { shader_vdecl_map_mutex.lock(); }
  ~ShaderVdeclAutoLock() { shader_vdecl_map_mutex.unlock(); }
  EA_NON_COPYABLE(ShaderVdeclAutoLock)
};

void dynrender::convert_channels_to_vsd(const CompiledShaderChannelId *ch, int numch, Tab<VSDTYPE> &out_vsd)
{
  int streamId = -1;
  for (int c = 0; c < numch; ++c)
  {
    if (ch[c].streamId != streamId)
    {
      if (ch[c].streamId < streamId)
        logerr("we should not decrease stream number. Was stream %d, now %d", streamId, ch[c].streamId);
      streamId = ch[c].streamId;
      out_vsd.push_back(VSD_STREAM(streamId));
    }
    int r = -1, t = -1;
    switch (ch[c].vbu)
    {
      case SCUSAGE_POS:
#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
        r = VSDR_POS;
#elif _TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3 | _TARGET_ANDROID
        r = (ch[c].vbui == 0 ? VSDR_POS : VSDR_POS2);
#else
        !error unsupported platform
#endif
        break;
      case SCUSAGE_NORM:
#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
        r = VSDR_NORM;
#elif _TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3 | _TARGET_ANDROID
        r = (ch[c].vbui == 0 ? VSDR_NORM : VSDR_NORM2);
#else
        !error unsupported platform
#endif
        break;
      case SCUSAGE_VCOL: r = (ch[c].vbui == 0 ? VSDR_DIFF : VSDR_SPEC); break;
      case SCUSAGE_TC:
        if (ch[c].vbui >= 0 && ch[c].vbui < 8)
          r = VSDR_TEXC0 + ch[c].vbui;
        if (ch[c].vbui >= 8 && ch[c].vbui < 15)
          r = VSDR_TEXC8 + ch[c].vbui - 8;
        break;
    }
    if (r < 0)
      DAG_FATAL("unknown shader channel %d,%d", ch[c].vbu, ch[c].vbui);
    switch (ch[c].t)
    {
      case SCTYPE_FLOAT1: t = VSDT_FLOAT1; break;
      case SCTYPE_FLOAT2: t = VSDT_FLOAT2; break;
      case SCTYPE_FLOAT3: t = VSDT_FLOAT3; break;
      case SCTYPE_FLOAT4: t = VSDT_FLOAT4; break;
      case SCTYPE_E3DCOLOR: t = VSDT_E3DCOLOR; break;
      case SCTYPE_UBYTE4: t = VSDT_UBYTE4; break;
      case SCTYPE_SHORT2: t = VSDT_SHORT2; break;
      case SCTYPE_SHORT4: t = VSDT_SHORT4; break;
      case SCTYPE_USHORT2N: t = VSDT_USHORT2N; break;
      case SCTYPE_USHORT4N: t = VSDT_USHORT4N; break;
      case SCTYPE_SHORT2N: t = VSDT_SHORT2N; break;
      case SCTYPE_SHORT4N: t = VSDT_SHORT4N; break;
      case SCTYPE_UDEC3: t = VSDT_UDEC3; break;
      case SCTYPE_DEC3N: t = VSDT_DEC3N; break;
      case SCTYPE_HALF2: t = VSDT_HALF2; break;
      case SCTYPE_HALF4: t = VSDT_HALF4; break;
      case SCTYPE_UINT1: t = VSDT_UINT1; break;
      case SCTYPE_UINT2: t = VSDT_UINT2; break;
      case SCTYPE_UINT3: t = VSDT_UINT3; break;
      case SCTYPE_UINT4: t = VSDT_UINT4; break;
    }
    if (t < 0)
      DAG_FATAL("unknown shader channel type %d", ch[c].t);
    out_vsd.push_back(VSD_REG(r, t));
  }

  out_vsd.push_back(VSD_END);
}


static VDECL internal_add_shader_vdecl_from_vsd(ShaderVDeclVsd &vsd)
{
  // look for vdecl with our vsd
  {
    ShaderVdeclAutoLock lock;
    auto it = shader_vdecl_map.find(vsd);
    if (it != shader_vdecl_map.end())
      return it->second.vdecl;
  }
  VDECL vdecl = d3d::create_vdecl(vsd.vsd.data()); // we release lock here, so there is no any chance of deadlock
  shader_vdecl_map_mutex.lock();
  auto it = shader_vdecl_map.lower_bound(vsd);
  if (it != shader_vdecl_map.end())
    if (DAGOR_UNLIKELY(it->first == vsd)) // we look up again, as it could be already added
    {
      VDECL ret = it->second.vdecl;
      shader_vdecl_map_mutex.unlock();
      d3d::delete_vdecl(vdecl);
      return ret;
    }
  shader_vdecl_map.emplace_hint(it, vsd, vdecl);
  shader_vdecl_map_mutex.unlock();
  return vdecl;
}


static VDECL add_shader_vdecl(const CompiledShaderChannelId *ch, int numch, dag::Span<VSDTYPE> add_vsd)
{
  ShaderVDeclVsd vsd(framemem_ptr());
  dynrender::convert_channels_to_vsd(ch, numch, vsd.vsd);
  if (add_vsd.size())
  {
    vsd.vsd.pop_back();
    append_items(vsd.vsd, add_vsd.size(), add_vsd.data());
    vsd.vsd.push_back(VSD_END);
  }
  vsd.hash = mem_hash_fnv1((const char *)vsd.vsd.data(), data_size(vsd.vsd)); // probably make hasher based on words, not bytes
  return internal_add_shader_vdecl_from_vsd(vsd);                             // Note: not move b/c of framemem
}

void close_vdecl()
{
  ShaderVdeclAutoLock lock;
  debug("%s shader_vdecl_map.size() = %d", __FUNCTION__, shader_vdecl_map.size());
  shader_vdecl_map.clear();
}

//

int shader_channel_type_size(int t)
{
  switch (t)
  {
    case SCTYPE_FLOAT1: return 4;
    case SCTYPE_FLOAT2: return 4 * 2;
    case SCTYPE_FLOAT3: return 4 * 3;
    case SCTYPE_FLOAT4: return 4 * 4;
    case SCTYPE_E3DCOLOR: return 4;
    case SCTYPE_UBYTE4: return 4;
    case SCTYPE_UDEC3:
    case SCTYPE_DEC3N: return 4;
    case SCTYPE_HALF2:
    case SCTYPE_USHORT2N:
    case SCTYPE_SHORT2N:
    case SCTYPE_SHORT2: return 2 * 2;
    case SCTYPE_HALF4:
    case SCTYPE_USHORT4N:
    case SCTYPE_SHORT4N:
    case SCTYPE_SHORT4: return 2 * 4;
  }
  return 0;
}

//

//************************************************************************
//* low-level dynamic shader rendering
//************************************************************************
namespace dynrender
{
// add vdecl or get existing for specified channel set.
VDECL addShaderVdecl(const CompiledShaderChannelId *ch, int numch, int, dag::Span<VSDTYPE> add_vsd)
{
  return add_shader_vdecl(ch, numch, add_vsd);
}

// get stride for channel set. return -1, if failed
int getStride(const CompiledShaderChannelId *ch, int numch)
{
  if (!ch)
    return -1;
  int s = 0;
  for (int i = 0; i < numch; ++i)
    s += shader_channel_type_size(ch[i].t);
  return s;
}

void RElem::setStates(bool indexed) const
{
  setBuffers(indexed);
  shElem->setStates();
}

void RElem::setBuffers(bool indexed) const
{
  G_ASSERT(vb);
  G_ASSERT(vDecl != BAD_VDECL);
  G_ASSERT(shElem);
  G_ASSERT(stride >= 0);

  if (!ib)
    indexed = false;

  if (indexed)
    d3d_err(d3d::setind(ib));

  d3d_err(d3d::setvdecl(vDecl));
  d3d_err(d3d::setvsrc(0, vb, stride));
}

// set ib, vb, vdecl & render this element
void RElem::render(bool indexed, int base_vertex_index) const
{
  setBuffers(indexed);

  if (indexed)
  {
    shElem->render(minVert, numVert, startIndex, numPrim, base_vertex_index);
  }
  else
  {
    shElem->render(minVert, 0, RELEM_NO_INDEX_BUFFER, numPrim);
  }
}

} // namespace dynrender

namespace shaders_bindump_reload_fence
{

// Sync works as follows
//
// register_job_manager_requiring_shaders_bindump and load_shaders_bindump_with_fence are mutually excluded, so we only have to
// organize sync for managers that were registered before (re)load was initiated.
//
// load_shaders_bindump_with_fence also protects itself with a JobManagersSyncPoint which
// 1) Places a special FenceJob on each registered mgr and waits for all of them to reach this job
//    (allReachedFenceEvent +notReachedFenceCount) in constructor
// 2) The mgrs then sync-wait inside FenceJob for the reload to finish (reloadDoneEvent + reloadDone)
// 3) When reload is done, JobManagersSyncPoint notifies fences and managers resume work.
//
// In order to avoid deadlocks when re-entering load_shaders_bindump_with_fence, each JobManagersSyncPoint spawns
// it's own block of events/counters SyncPointSharedState which is only destroyed when all FenceJobs and sync point
// are done with it, and concurrently with that it is ok to call load_shaders_bindump_with_fence again,
// which will place more fences and use different events/counters.


static struct Context
{
  OSReadWriteLock bindumpReloadLock;
  eastl::vector_set<int> waitingJobManagerIds DAG_TS_GUARDED_BY(bindumpReloadLock);
} g_ctx{};

class JobManagersSyncPoint
{
  struct SyncPointSharedState
  {
    os_event_t reloadDoneEvent{};
    dag::AtomicFlag reloadDone{};

    os_event_t allReachedFenceEvent{};
    dag::AtomicInteger<int> notReachedFenceCount;

    dag::AtomicInteger<int> rc;

    SyncPointSharedState(int fence_count, int ref_count) : notReachedFenceCount{fence_count}, rc{ref_count}
    {
      os_event_create(&allReachedFenceEvent);
      os_event_create(&reloadDoneEvent, nullptr, true);
    }

    ~SyncPointSharedState()
    {
      os_event_destroy(&allReachedFenceEvent);
      os_event_destroy(&reloadDoneEvent);
    }

    friend void release(SyncPointSharedState *&hnd)
    {
      G_ASSERT(hnd);
      if (hnd->rc.sub_fetch(1) == 0)
        delete hnd;
      hnd = nullptr;
    }
  };

  struct FenceJob : public cpujobs::IJob
  {
    SyncPointSharedState *state;

    explicit FenceJob(SyncPointSharedState *state) : state{state} {}

    const char *getJobName(bool &) const override { return "FenceJob"; }

    void doJob() override
    {
      if (state->notReachedFenceCount.sub_fetch(1) == 0)
        os_event_set(&state->allReachedFenceEvent);

      while (!state->reloadDone.test())
        os_event_wait_noreset(&state->reloadDoneEvent, 1);

      release(state);
    }
    void releaseJob() override { delete this; }
  };

  SyncPointSharedState *state{nullptr};

public:
  JobManagersSyncPoint() DAG_TS_REQUIRES(g_ctx.bindumpReloadLock)
  {
#if DAGOR_DBGLEVEL > 0
    int64_t currentThreadId = get_current_thread_id();
    if (eastl::find_if(g_ctx.waitingJobManagerIds.cbegin(), g_ctx.waitingJobManagerIds.cend(), [currentThreadId](int id) {
          return cpujobs::get_job_manager_thread_id(id) == currentThreadId;
        }) != g_ctx.waitingJobManagerIds.cend())
    {
      G_ASSERTF(0, "Trying to reload shaders bindump from job mgr %d which relies on bindump, this would be a deadlock",
        currentThreadId);
    }
#endif

    // Fence counter is g_ctx.waitingJobManagerIds.size() since there is one FenceJob per mgr,
    // but rc is that + 1 cause the syncpoint itself holds a ref too
    const int registeredJobMgrCount = int(g_ctx.waitingJobManagerIds.size());
    state = new SyncPointSharedState{registeredJobMgrCount, registeredJobMgrCount + 1};

    for (int jobMgrId : g_ctx.waitingJobManagerIds)
      G_VERIFY(cpujobs::add_job(jobMgrId, new FenceJob{state}));

    while (state->notReachedFenceCount.load() > 0)
      os_event_wait(&state->allReachedFenceEvent, 1);
  }

  ~JobManagersSyncPoint()
  {
    state->reloadDone.test_and_set();
    os_event_set(&state->reloadDoneEvent);

    release(state);
  }
};

} // namespace shaders_bindump_reload_fence

void register_job_manager_requiring_shaders_bindump(int job_mgr_id)
{
  using namespace shaders_bindump_reload_fence;
  ScopedLockWriteTemplate lock{g_ctx.bindumpReloadLock};
  g_ctx.waitingJobManagerIds.insert(job_mgr_id);
}

bool load_shaders_bindump_with_fence(const char *src_filename, d3d::shadermodel::Version shader_model_version)
{
  using namespace shaders_bindump_reload_fence;
  G_ASSERT_RETURN(src_filename, false);

  ScopedLockWriteTemplate lock{g_ctx.bindumpReloadLock};
  JobManagersSyncPoint syncPoint{};
  return load_shaders_bindump(src_filename, shader_model_version);
}

static void build_shaderdump_filename(char *fname, const char *src_filename, d3d::shadermodel::Version shader_model_version)
{
  const auto getFormatForStubDriver = [] {
    const char *ret = "%s.%s.shdump.bin";
#if _TARGET_PC
    if (::dgs_get_settings()->getBlockByNameEx("video")->getBlockByNameEx("stub")->getBool("enableBindlessAndRt", false))
    {
#if _TARGET_PC_WIN
      ret = "%sDX12.%s.shdump.bin";
#elif _TARGET_PC_LINUX
      ret = "%sSpirV.bindless.%s.shdump.bin";
#else
      ret = "%sMTL.bindless.%s.shdump.bin";
#endif
    }
#endif
    return ::dgs_get_settings()->getBlockByNameEx("video")->getBlockByNameEx("stub")->getStr("shaderDumpOverride", ret);
  };

  auto verStr = d3d::as_ps_string(shader_model_version);

  size_t sizeof_fname = sizeof(char) * DAGOR_MAX_PATH;

  auto format =
    d3d::get_driver_code()
      .map(d3d::xboxOne, "%sDX12x.%s.shdump.bin")
      .map(d3d::scarlett, "%sDX12xs.%s.shdump.bin")
      .map(d3d::dx12, "%sDX12.%s.shdump.bin")
      .map(d3d::vulkan, d3d::get_driver_desc().caps.hasBindless ? "%sSpirV.bindless.%s.shdump.bin" : "%sSpirV.%s.shdump.bin")
      .map(d3d::ps4, "%sPS4.%s.shdump.bin")
      .map(d3d::ps5, "%sPS5.%s.shdump.bin")
      .map(d3d::metal, d3d::get_driver_desc().caps.hasBindless ? "%sMTL.bindless.%s.shdump.bin" : "%sMTL.%s.shdump.bin")
      .map(d3d::stub, getFormatForStubDriver)
      .map(d3d::any, "%s.%s.shdump.bin");

  _snprintf(fname, sizeof_fname, format, src_filename, verStr);
  fname[sizeof_fname - 1] = 0;
}

// Only invalidate global state for the main shaderdump
static void init_global_systems_after_main_dump_reload(ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData *gdata)
{
  auto *dump = dest.getDump();
  G_UNUSED(gdata);

  // init block state
  shaderbindump::blockStateWord = 0;
  static const char *null_name[shaderbindump::MAX_BLOCK_LAYERS] = {"!frame", "!scene", "!obj"};
  for (int i = 0; i < shaderbindump::MAX_BLOCK_LAYERS; i++)
  {
    int id = mapbinarysearch::bfindStrId(dump->blockNameMap, null_name[i]);
    if (id < 0)
      shaderbindump::nullBlock[i] = nullptr;
    else
    {
      shaderbindump::nullBlock[i] = &dump->blocks[id];
      shaderbindump::blockStateWord |= shaderbindump::nullBlock[i]->uidVal;
    }
  }

  shaderbindump::reset_interval_binds();

  shadervars::resolve_shadervars();
  IShaderBindumpReloadListener::resolveAll(); // @TODO: separate subscriptions for different dumps?
  extern void reset_shaders_logerr_info();
  reset_shaders_logerr_info();

  d3d::driver_command(Drv3dCommand::REGISTER_SHADER_DUMP, &dest, (void *)dest.name.str());
}

static void shutdown_global_systems_before_main_dump_unload() { d3d::driver_command(Drv3dCommand::REGISTER_SHADER_DUMP); }

static void clear_shaders_bindump(ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData *gdata, bool is_main)
{
  if (is_main)
    shutdown_global_systems_before_main_dump_unload();
  dest.clear();
  if (gdata)
    gdata->clear();
}

bool load_shaders_bindump_asset(ShadersBinDumpAssetData &dump_asset, char const *src_filename,
  d3d::shadermodel::Version shader_model_version)
{
  dump_asset.fullName.resize(DAGOR_MAX_PATH);
  build_shaderdump_filename(dump_asset.fullName.data(), src_filename, shader_model_version);
  dump_asset.fullName.updateSz();

  // FIXME
  // vromfs_get_file_data cause error for miniui shaders, this is workaround to avoid it
  const size_t inplace_load_min_size_threshold = 1 << 20;

  VromReadHandle dump_data = ::vromfs_get_file_data(dump_asset.fullName.str());
  if (dump_data.data() && data_size(dump_data) > inplace_load_min_size_threshold)
  {
    debug("[SH] Loading precompiled shaders from VROMFS::'%s'...[%p]", dump_asset.fullName.str(), dump_data.data());
    dump_asset.memCrd.emplace(dump_data.data(), data_size(dump_data));
    dump_asset.dumpFileSz = data_size(dump_data);
    dump_asset.mmapedLoad = false;
  }
  else
  {
#if _TARGET_ANDROID
    if (!android_get_asset_data(dump_asset.fullName.str(), dump_asset.assetData))
      clear_and_shrink(dump_asset.assetData);
    if (dump_asset.assetData.data())
    {
      dump_asset.memCrd.emplace(dump_asset.assetData.data(), data_size(dump_asset.assetData));
      dump_asset.dumpFileSz = data_size(dump_asset.assetData);
      dump_asset.mmapedLoad = false;
    }
    else
#endif
    {
      dump_asset.fileCrd.emplace(dump_asset.fullName.str(), DF_IGNORE_MISSING | DF_READ);
      if (dump_asset.fileCrd->fileHandle)
        dump_asset.dumpFileSz = df_length(dump_asset.fileCrd->fileHandle);
      else
        dump_asset.fileCrd.reset();
    }
  }

  return dump_asset.valid();
}

// load current binary code to disk. no up-to-date checking performed
static bool load_shaders_bindump(const char *src_filename, d3d::shadermodel::Version shader_model_version,
  ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData *gdata)
{
  // @NOTE: will not be asserted once additional dumps are added
  G_ASSERT(gdata);
  if (dest.getDump())
    G_ASSERT(gdata->backing == &dest);

  bool const isMainDump = &dest == &shBinDumpOwner();

  if (isMainDump)
    G_ASSERT(gdata == &shGlobalData());
  else if (&dest == &shBinDumpExOwner(false))
    G_ASSERT(gdata == &shGlobalDataEx(false));

  if (!shaders_internal::shader_reload_allowed && dest.getDump() && dest->classes.size())
  {
    DAG_FATAL("shader reload is disabled by project");
    return false;
  }

  ShadersBinDumpAssetData dumpAsset{};
  if (!load_shaders_bindump_asset(dumpAsset, src_filename, shader_model_version))
  {
    // rebuild shaders
    debug("[SH] Precompiled shader file '%s' not found", dumpAsset.fullName.str());
    return false;
  }

  if (isMainDump)
    stcode::unload(dest.stcodeCtx);

  shaders_internal::init_stateblocks();

  // flush all driver commands, which potentially can reference old scripted shader bin dump
  d3d::GpuAutoLock gpuLock;

  // have to delete shaders before ruining memory with move because if loading will fail
  // we do dest.initAfterLoad which clears shader arrays without destroying them
  shaders_internal::cleanup_shaders_on_reload(dest);
  // why is it here if we are only reloading dest bindump?
  iterate_all_additional_shader_dumps([](ScriptedShadersBinDumpOwner &owner) { shaders_internal::cleanup_shaders_on_reload(owner); });

  // signal backend that the memory backing shader bytecode is about to be invalidated
  d3d::driver_command(Drv3dCommand::UNLOAD_SHADER_MEMORY);

  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  dest.assertionCtx.close();

  ScriptedShadersBinDumpOwner prev_sh = eastl::move(dest);
  ScriptedShadersGlobalData prev_gdata = eastl::move(*gdata);

  // Transfer cppstcode settings
  dest.stcodeCtx = eastl::move(prev_sh.stcodeCtx);

  clear_shaders_bindump(dest, gdata, isMainDump);

  auto initAfterLoad = [&] {
    gdata->initAfterLoad(&dest, isMainDump);
    dest.initAfterLoad(isMainDump);
    if (isMainDump)
      init_global_systems_after_main_dump_reload(dest, gdata);
    dest.assertionCtx.init(dest);
  };

  // load precached file
  DAGOR_TRY
  {
    debug("[SH] Loading precompiled shaders from '%s'...", dumpAsset.fullName.str());
    if (dest.loadFromFile(dumpAsset.getCrd(), dumpAsset.dumpFileSz, dumpAsset.mmapedLoad))
    {
      initAfterLoad();

      debug("[SH] Precompiled shaders from '%s' loaded OK regs = %d", dumpAsset.fullName.str(), dest->maxRegSize);
      if (dest->maxRegSize > MAX_TEMP_REGS)
        DAG_FATAL("too much shader temp registers %d, current limit is %d", dest->maxRegSize, MAX_TEMP_REGS);

      if (!prev_sh.getDump() || shaders_internal::reload_shaders_materials(dest, gdata, prev_sh, &prev_gdata))
      {
        prev_sh.clear();
        prev_gdata.clear();

        if (isMainDump)
          shglobvars::init_varids_loaded();

        iterate_all_additional_shader_dumps([&gdata](ScriptedShadersBinDumpOwner &owner) { reinit_shaders_bindump(owner, *gdata); });

        return stcode::load(dest.stcodeCtx, dumpAsset.fullName.str());
      }
    }
  }
  DAGOR_CATCH(const IGenLoad::LoadException &e) { debug("[SH] exception while loading %s!", dumpAsset.fullName.str()); }

  clear_shaders_bindump(dest, gdata, isMainDump);

  dest = eastl::move(prev_sh);
  *gdata = eastl::move(prev_gdata);

  if (dest.getDump() && dest->classes.size())
  {
    initAfterLoad();
    iterate_all_additional_shader_dumps([&gdata](ScriptedShadersBinDumpOwner &owner) { reinit_shaders_bindump(owner, *gdata); });
    if (isMainDump)
      stcode::load(dest.stcodeCtx, dumpAsset.fullName.str());
    return false;
  }

  debug("[SH] Invalid '%s' version", dumpAsset.fullName.str());
  DAG_FATAL("Invalid '%s' version - recompile shaders!", dumpAsset.fullName.str());
  return false;
}
static String last_loaded_dump[2];

const char *last_loaded_shaders_bindump(bool sec) { return last_loaded_dump[sec ? 1 : 0].c_str(); }

bool load_shaders_bindump(const char *src_filename, d3d::shadermodel::Version shader_model_version, bool sec_dump_for_exp)
{
  String &dump = last_loaded_dump[sec_dump_for_exp ? 1 : 0];
  if (src_filename)
    dump = src_filename;
  return load_shaders_bindump(dump.c_str(), shader_model_version, shBinDumpExOwner(!sec_dump_for_exp),
    &shGlobalDataEx(!sec_dump_for_exp));
}

static void unload_shaders_bindump(ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData *gdata)
{
  stcode::unload(dest.stcodeCtx);

  // flush all driver commands, which potentially can reference old scripted shader bin dump
  d3d::GpuAutoLock gpuLock;

  // signal backend that the memory backing shader bytecode is about to be invalidated
  d3d::driver_command(Drv3dCommand::UNLOAD_SHADER_MEMORY);

  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  clear_shaders_bindump(dest, gdata, &dest == &shBinDumpOwner());

#if DAGOR_DBGLEVEL > 0
  shaderbindump::resetInvalidVariantMarks(dest.selfHandle);
#endif
}
void unload_shaders_bindump(bool sec_dump_for_exp)
{
  unload_shaders_bindump(shBinDumpExOwner(!sec_dump_for_exp), &shGlobalDataEx(!sec_dump_for_exp));
}

static d3d::shadermodel::Version forceFSH = d3d::smAny;

// return maximum permitted FSH version
d3d::shadermodel::Version getMaxFSHVersion() { return forceFSH; }
void limitMaxFSHVersion(d3d::shadermodel::Version f) { forceFSH = f; }

static bool debugDump = false;

bool load_shaders_debug_bindump(d3d::shadermodel::Version version)
{
#if DAGOR_DBGLEVEL > 0
  if (debugDump)
    return true;

  if (d3d::get_driver_code() != d3d::dx12 || dgs_get_settings()->getBlockByNameEx("video")->getBool("compatibilityMode", false))
  {
    debug("Debug shaderdumps only exist for PC DX12");
    return false;
  }

  static String shadersDir = String(dgs_get_settings()->getBlockByNameEx("debug")->getStr("shadersDir", "compiledShaders")) + ".debug";

  if (load_shaders_bindump(shadersDir + "/game_debug", version))
  {
    debugDump = true;
    return true;
  }
  debug("The debug shaderdump cannot be found, make sure it exists: %s/game_debugDX12.ps%d%d.shdump", shadersDir, version.minor,
    version.major);
#endif
  return false;
}

bool unload_shaders_debug_bindump(d3d::shadermodel::Version version)
{
#if DAGOR_DBGLEVEL > 0
  if (!debugDump)
    return true;

  static String shadersDir = String(dgs_get_settings()->getBlockByNameEx("debug")->getStr("shadersDir", "compiledShaders")) + ".debug";

  if (load_shaders_bindump(shadersDir + "/game", d3d::shadermodel::Version(5, 0)))
  {
    debugDump = false;
    return true;
  }
  debug("The normal shaderdump cannot be found, make sure it exists: %s/gameDX12.ps%d%d.shdump", shadersDir, version.minor,
    version.major);
#endif
  return false;
}

void dump_shader_statistics()
{
#if DAGOR_DBGLEVEL > 0
  if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("dumpShaderStatistics", false))
  {
    auto game_name = dgs_get_settings()->getStr("contactsGameId");
    String dump_name(64, ".logs~%s/game", game_name);
    char fname[DAGOR_MAX_PATH];
    build_shaderdump_filename(fname, dump_name, d3d::smAny);
    strcat(fname, ".stat");
    dump_shader_statistics(fname);
    debug("Shader statistics has been dumped to %s", fname);
  }
#endif
}

void set_stcode_special_tag_interp(stcode::SpecialBlkTagInterpreter &&interp)
{
  stcode::set_special_tag_interp(::shBinDumpOwner().stcodeCtx, eastl::move(interp));
}

class ShadersRestartProc : public SRestartProc
{
public:
  class ShadersResetOnDriverReset : public SRestartProc
  {
  public:
    virtual const char *procname() { return "ShadersReset"; }
    ShadersResetOnDriverReset() : SRestartProc(RESTART_VIDEODRV) { inited = 1; }

    void startup() { shaders_internal::reload_shaders_materials_on_driver_reset(); }
    void shutdown()
    {
      ShaderGlobal::reset_textures(true);
      ShaderGlobal::reset_stale_vars();
    }
  };
  SimpleString fileName;
  d3d::shadermodel::Version maxFshVer = d3d::smAny;
  ShadersResetOnDriverReset drvResetRp;

  virtual const char *procname() { return "shaders"; }

  ShadersRestartProc() : SRestartProc(RESTART_GAME | RESTART_VIDEO) {}

  void startup()
  {
    tql::reset_texture_from_shader_vars = &ShaderGlobal::reset_from_vars;
    shaders_internal::init_stateblocks();
    ShaderStateBlock::clear();
    if (d3d::smAny == maxFshVer)
      maxFshVer = getMaxFSHVersion();

    if (d3d::smAny == maxFshVer)
    {
      G_ASSERT(d3d::is_inited());
      maxFshVer = d3d::get_driver_desc().shaderModel;
      limitMaxFSHVersion(maxFshVer);
    }

    if (!fileName.empty())
    {
      d3d::shadermodel::Version selectedVersion = d3d::smNone;
      for (auto version : d3d::smAll)
      {
        if (version > maxFshVer)
          continue;
        if (!load_shaders_bindump(fileName, version))
          continue;
        selectedVersion = version;
        break;
      }
      if (d3d::smNone == selectedVersion)
      {
        DAG_FATAL("Cannot find precompiled shaders for this videocard\n"
                  "(while looking for files %s.psXX.shdump.bin)\n"
                  "This can imply that videocard doesn't meet game's requirements\n\n",
          (char *)fileName);
      }
      maxFshVer = selectedVersion;
      limitMaxFSHVersion(maxFshVer);
    }

    add_restart_proc(&drvResetRp);
  }

  void shutdown()
  {
    dump_shader_statistics();

    del_restart_proc(&drvResetRp);
    close_vdecl();

    // reset public varIds
    shaders_internal::close_global_constbuffers();
    shaders_internal::close_stateblocks();
    ShaderStateBlock::clear();
    auto &dumpOwner = shBinDumpOwner();
    auto &globalData = shGlobalData();
    const auto &dump = shBinDump();
    if (dump.varMap.size())
    {
      unsigned vars_resolved = 0, glob_vars_resolved = 0;
      for (int i = 0, ie = VariableMap::getVariablesCount(); i < ie; i++)
        if (globalData.varIndexMap[i] < SHADERVAR_IDX_ABSENT)
        {
          vars_resolved++;
          if (globalData.globvarIndexMap[i] < SHADERVAR_IDX_ABSENT)
            glob_vars_resolved++;
        }

      debug("shaders: resolved %d of %d requested variables (%d vars total), globvars resolved %d of %d total", vars_resolved,
        VariableMap::getVariablesCount(), dump.varMap.size(), glob_vars_resolved, dump.gvMap.size());

#if DAGOR_DBGLEVEL > 0
      if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("dumpUsedShaderVars", false))
        shaderbindump::dump_shader_var_names();
#endif

      G_UNUSED(vars_resolved);
      G_UNUSED(glob_vars_resolved);
    }

    unload_shaders_bindump();

    shaderbindump::g_stub_texture_repo.shutdown();
  }
};

static InitOnDemand<ShadersRestartProc> shaders_rproc;

void startup_shaders(const char *src_filename, d3d::shadermodel::Version shader_model_version)
{
  shaders_rproc.demandInit();
  shaders_rproc->fileName = src_filename;
  shaders_rproc->maxFshVer = shader_model_version;

  IShaderBindumpReloadListener::reportStaticInitDone();

  add_restart_proc(shaders_rproc);
}

void shutdown_shaders() { del_restart_proc(shaders_rproc); }
