#include <shaders/shLimits.h>
#include <shaders/dag_shaders.h>
#include "shadersBinaryData.h"
#include "shStateBlk.h"
#include "shStateBlock.h"
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <startup/dag_restart.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdio.h>
#include <EASTL/vector_map.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include "shaders/sh_vars.h"
#if _TARGET_PC_WIN
#include <malloc.h>
#elif defined(__GNUC__)
#include <stdlib.h>
#endif

//

const char *ShaderMaterial::loadingStirngInfo = NULL;
#if DAGOR_DBGLEVEL > 0
#include <shaders/dag_shaderDbg.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
namespace shaderbindump
{
extern void dump_shader_var_names();
}
#endif

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
  auto &vl = shBinDumpRW().globVars;

  for (int i = 0, e = vl.size(); i < e; i++)
  {
    auto type = vl.getType(i);
    if (type == SHVT_TEXTURE)
    {
      if (removed_tex_only)
        if (get_managed_texture_name(vl.getTex(i).texId) || get_managed_texture_refcount(vl.getTex(i).texId) <= 0)
          continue;
      release_managed_tex(vl.getTex(i).texId);
      vl.setTexId(i, BAD_TEXTUREID);
      vl.setTex(i, NULL);

      int iid = shBinDumpOwner().globVarIntervalIdx[i];
      if (iid >= 0)
        shBinDumpOwner().globIntervalNormValues[iid] = 0;
    }
    else if (type == SHVT_BUFFER)
    {
      if (removed_tex_only)
        if (get_managed_res_name(vl.getBuf(i).bufId) || get_managed_res_refcount(vl.getBuf(i).bufId) <= 0)
          continue;
      release_managed_tex(vl.getBuf(i).bufId);
      vl.setBufId(i, BAD_TEXTUREID);
      vl.setBuf(i, NULL);

      int iid = shBinDumpOwner().globVarIntervalIdx[i];
      if (iid >= 0)
        shBinDumpOwner().globIntervalNormValues[iid] = 0;
    }
  }
}
void ShaderGlobal::reset_from_vars(TEXTUREID id)
{
  if (id == BAD_TEXTUREID)
    return;

  auto &vl = shBinDumpRW().globVars;

  for (int i = 0, e = vl.size(); i < e; i++)
  {
    auto type = vl.getType(i);
    if (type == SHVT_TEXTURE)
    {
      if (vl.getTex(i).texId != id)
        continue;
      if (get_managed_texture_refcount(id) > 0)
        release_managed_tex(id);
      vl.setTexId(i, BAD_TEXTUREID);
      vl.setTex(i, NULL);

      int iid = shBinDumpOwner().globVarIntervalIdx[i];
      if (iid >= 0)
        shBinDumpOwner().globIntervalNormValues[iid] = 0;
    }
    else if (type == SHVT_BUFFER)
    {
      if (vl.getBuf(i).bufId != id)
        continue;
      if (get_managed_res_refcount(id) > 0)
        release_managed_res(id);
      vl.setBufId(i, BAD_TEXTUREID);
      vl.setBuf(i, NULL);

      int iid = shBinDumpOwner().globVarIntervalIdx[i];
      if (iid >= 0)
        shBinDumpOwner().globIntervalNormValues[iid] = 0;
    }
  }
}

//

struct ShaderVDeclVsd
{
  uint32_t hash; // keep hash so we can faster select
  Tab<VSDTYPE> vsd;
  bool operator==(const ShaderVDeclVsd &a) { return a.hash == hash && a.vsd.size() == vsd.size() && mem_eq(vsd, a.vsd.data()); }
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
        !error unsuported platform
#endif
        break;
      case SCUSAGE_NORM:
#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
        r = VSDR_NORM;
#elif _TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3 | _TARGET_ANDROID
        r = (ch[c].vbui == 0 ? VSDR_NORM : VSDR_NORM2);
#else
        !error unsuported platform
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
      fatal("unknown shader channel %d,%d", ch[c].vbu, ch[c].vbui);
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
      fatal("unknown shader channel type %d", ch[c].t);
    out_vsd.push_back(VSD_REG(r, t));
  }

  out_vsd.push_back(VSD_END);
}


static VDECL internal_add_shader_vdecl_from_vsd(ShaderVDeclVsd &&vsd)
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
    if (EASTL_UNLIKELY(it->first == vsd)) // we look up again, as it could be already added
    {
      VDECL ret = it->second.vdecl;
      shader_vdecl_map_mutex.unlock();
      d3d::delete_vdecl(vdecl);
      return ret;
    }
  shader_vdecl_map.emplace_hint(it, eastl::move(vsd), vdecl);
  shader_vdecl_map_mutex.unlock();
  return vdecl;
}


static VDECL add_shader_vdecl(const CompiledShaderChannelId *ch, int numch, dag::Span<VSDTYPE> add_vsd)
{

  // build vsd
  ShaderVDeclVsd vsd;
  dynrender::convert_channels_to_vsd(ch, numch, vsd.vsd);
  if (add_vsd.size())
  {
    vsd.vsd.pop_back();
    append_items(vsd.vsd, add_vsd.size(), add_vsd.data());
    vsd.vsd.push_back(VSD_END);
  }
  vsd.hash = mem_hash_fnv1((const char *)vsd.vsd.data(), data_size(vsd.vsd)); // probably make hasher based on words, not bytes

  return internal_add_shader_vdecl_from_vsd(eastl::move(vsd));
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
static volatile int launched_jobs = 0;
static volatile int shaders_reloading = 0;
static dag::Vector<int> job_manager_ids;
static OSSpinlock job_managers_mutex;

struct FenceJob : public cpujobs::IJob
{
  void doJob() override
  {
    interlocked_increment(launched_jobs);
    spin_wait([] { return interlocked_acquire_load(shaders_reloading); });
    interlocked_decrement(launched_jobs);
  }
  void releaseJob() override { delete this; }
};

struct JobManagersFence
{
  JobManagersFence()
  {
    G_ASSERTF(interlocked_acquire_load(shaders_reloading) == 0, "Simultaneous shaders reload from different threads?");
    spin_wait([] { return interlocked_acquire_load(launched_jobs); });
    interlocked_release_store(launched_jobs, 0);
    interlocked_release_store(shaders_reloading, 1);
    uint32_t expectedJobs = 0;
    {
      OSSpinlockScopedLock lock(job_managers_mutex);
      expectedJobs = job_manager_ids.size();
      for (int jobMgrId : job_manager_ids)
        G_VERIFY(cpujobs::add_job(jobMgrId, new FenceJob));
    }
    spin_wait([expectedJobs] { return interlocked_acquire_load(launched_jobs) != expectedJobs; });
  }
  ~JobManagersFence() { interlocked_release_store(shaders_reloading, 0); }
};

static bool check_current_thread()
{
  int64_t currentThreadId = get_current_thread_id();
  OSSpinlockScopedLock lock(job_managers_mutex);
  for (int jobMgrId : job_manager_ids)
    if (cpujobs::get_job_manager_thread_id(jobMgrId) == currentThreadId)
    {
      // Probably you are doing something wrong, but if you really want to reload shaders
      // from jobs, then make sure that you haven't data races with other threads.
      // And don't launch FenceJob for current thread, otherwise it is deadlock.
      return false;
    }
  return true;
}
} // namespace shaders_bindump_reload_fence

void register_job_manager_requiring_shaders_bindump(int job_mgr_id)
{
  OSSpinlockScopedLock lock(shaders_bindump_reload_fence::job_managers_mutex);
  for (int jobId : shaders_bindump_reload_fence::job_manager_ids)
    if (jobId == job_mgr_id)
      return;
  shaders_bindump_reload_fence::job_manager_ids.push_back(job_mgr_id);
}

static void build_shaderdump_filename(char fname[DAGOR_MAX_PATH], const char *src_filename, int max_fshver)
{
  const char *verStr = NULL;
  switch (max_fshver)
  {
    case FSHVER_FFP: verStr = "ps11"; break;
    case FSHVER_NV20: verStr = "ps11"; break;
    case FSHVER_NV25: verStr = "ps13"; break;
    case FSHVER_R200: verStr = "ps14"; break;
    case FSHVER_R300: verStr = "ps20"; break;
    case FSHVER_20B: verStr = "ps2b"; break;
    case FSHVER_20A: verStr = "ps2a"; break;
    case FSHVER_30: verStr = "ps30"; break;
    case FSHVER_40: verStr = "ps40"; break;
    case FSHVER_41: verStr = "ps41"; break;
    case FSHVER_50: verStr = "ps50"; break;
    case FSHVER_60: verStr = "ps60"; break;
    case FSHVER_66: verStr = "ps66"; break;
  }
  G_ASSERT(verStr);

  size_t sizeof_fname = sizeof(char) * DAGOR_MAX_PATH;

  if (d3d::get_driver_code() == _MAKE4C('DX12'))
#if _TARGET_XBOXONE
    _snprintf(fname, sizeof_fname, "%sDX12x.%s.shdump.bin", src_filename, verStr);
#elif _TARGET_SCARLETT
    _snprintf(fname, sizeof_fname, "%sDX12xs.%s.shdump.bin", src_filename, verStr);
#else
    _snprintf(fname, sizeof_fname, "%sDX12.%s.shdump.bin", src_filename, verStr);
#endif
  else if (d3d::get_driver_code() == _MAKE4C('VULK'))
  {
    if (d3d::get_driver_desc().caps.hasBindless)
      _snprintf(fname, sizeof_fname, "%sSpirV.bindless.%s.shdump.bin", src_filename, verStr);
    else
      _snprintf(fname, sizeof_fname, "%sSpirV.%s.shdump.bin", src_filename, verStr);
  }
  else if (d3d::get_driver_code() == _MAKE4C('PS4'))
    _snprintf(fname, sizeof_fname, "%sPS4.%s.shdump.bin", src_filename, verStr);
  else if (d3d::get_driver_code() == _MAKE4C('PS5'))
    _snprintf(fname, sizeof_fname, "%sPS5.%s.shdump.bin", src_filename, verStr);
  else if (d3d::get_driver_code() == _MAKE4C('MTL'))
    _snprintf(fname, sizeof_fname, "%sMTL.%s.shdump.bin", src_filename, verStr);
  else
    _snprintf(fname, sizeof_fname, "%s.%s.shdump.bin", src_filename, verStr);
  fname[sizeof_fname - 1] = 0;
}

// load current binary code to disk. no up-to-date checking performed
static bool load_shaders_bindump(const char *src_filename, int max_fshver, ScriptedShadersBinDumpOwner &dest)
{
  shaders_internal::init_stateblocks();
  char fname[DAGOR_MAX_PATH];
  build_shaderdump_filename(fname, src_filename, max_fshver);

  // FIXME
  // vromfs_get_file_data cause error for miniui shaders, this is workaround to avoid it
  const size_t inplace_load_min_size_threshold = 1 << 20;

  VromReadHandle dump_data = ::vromfs_get_file_data(fname);
  if (dump_data.data() && data_size(dump_data) > inplace_load_min_size_threshold)
  {
    debug("[SH] Loading precompiled shaders from VROMFS::'%s'...[%p]", fname, dump_data.data());
    if (dest.loadData((uint8_t *)dump_data.data(), data_size(dump_data)))
    {
      return true;
    }
  }
  if (!shaders_internal::shader_reload_allowed && dest.getDump() && dest->classes.size())
  {
    fatal("shader reload is disabled by project");
    return false;
  }

  IGenLoad *crd = NULL;
  int dump_file_sz = 0;
  FullFileLoadCB crd_file(fname, DF_IGNORE_MISSING | DF_READ);
  bool full_file_load = true; // To consider: make something like FullMMappedFileLoadCB

#if _TARGET_ANDROID
  Tab<char> assetData;
  if (!android_get_asset_data(fname, assetData))
    clear_and_shrink(assetData);
  InPlaceMemLoadCB crd_mem(assetData.data(), data_size(assetData));
  if (assetData.data())
  {
    crd = &crd_mem;
    dump_file_sz = data_size(assetData);
    full_file_load = false;
  }
#endif

  if (!crd && crd_file.fileHandle)
  {
    crd = &crd_file;
    dump_file_sz = df_length(crd_file.fileHandle);
  }

  if (!crd)
  {
    // rebuild shaders
    debug("[SH] Precompiled shader file '%s' not found", fname);
    return false;
  }

  ScriptedShadersBinDumpOwner prev_sh = eastl::move(dest);

  // load precached file
  DAGOR_TRY
  {
    debug("[SH] Loading precompiled shaders from '%s'...", fname);
    if (dest.load(*crd, dump_file_sz, full_file_load))
    {
      debug("[SH] Precompiled shaders from '%s' loaded OK regs = %d", fname, dest->maxRegSize);
      if (dest->maxRegSize > MAX_TEMP_REGS)
        fatal("too much shader temp registers %d, current limit is %d", dest->maxRegSize, MAX_TEMP_REGS);

      if (!prev_sh.getDump() || shaders_internal::reload_shaders_materials(prev_sh))
      {
        prev_sh.clear();
        if (dest.getDump() == &shBinDump())
          shglobvars::init_varids_loaded();
        return true;
      }
    }
  }
  DAGOR_CATCH(IGenLoad::LoadException e) { debug("[SH] exception while loading %s!", fname); }

  dest.clear();
  dest = eastl::move(prev_sh);
  if (dest.getDump() && dest->classes.size())
  {
    dest.initAfterLoad();
    return false;
  }

  debug("[SH] Invalid '%s' version", fname);
  fatal("Invalid '%s' version - recompile shaders!", fname);
  return false;
}
static String last_loaded_dump[2];

const char *last_loaded_shaders_bindump(bool sec) { return last_loaded_dump[sec ? 1 : 0].c_str(); }

bool load_shaders_bindump(const char *src_filename, int max_fshver, bool sec_dump_for_exp)
{
  String &dump = last_loaded_dump[sec_dump_for_exp ? 1 : 0];
  if (src_filename)
    dump = src_filename;
  return load_shaders_bindump(dump.c_str(), max_fshver, shBinDumpOwner(!sec_dump_for_exp));
}

bool load_shaders_bindump_with_fence(const char *src_filename, int max_fshver)
{
  G_ASSERT_RETURN(src_filename, false);
  G_ASSERT_RETURN(shaders_bindump_reload_fence::check_current_thread(), false);

  shaders_bindump_reload_fence::JobManagersFence fence;
  return load_shaders_bindump(src_filename, max_fshver);
}

static void unload_shaders_bindump(ScriptedShadersBinDumpOwner &dest)
{
  dest.clear();
#if DAGOR_DBGLEVEL > 0
  shaderbindump::resetInvalidVariantMarks();
#endif
}
void unload_shaders_bindump(bool sec_dump_for_exp) { unload_shaders_bindump(shBinDumpOwner(!sec_dump_for_exp)); }

static int forceFSH = FSH_AS_HARDWARE;

// return maximum permitted FSH version
int getMaxFSHVersion() { return forceFSH; }
void limitMaxFSHVersion(int f) { forceFSH = f; }

class ShadersRestartProc : public SRestartProc
{
public:
  class ShadersResetOnDriverReset : public SRestartProc
  {
  public:
    virtual const char *procname() { return "ShadersReset"; }
    ShadersResetOnDriverReset() : SRestartProc(RESTART_VIDEODRV) { inited = 1; }

    void startup() { shaders_internal::reload_shaders_materials_on_driver_reset(); }
    void shutdown() { ShaderGlobal::reset_textures(true); }
  };
  SimpleString fileName;
  int maxFshVer;
  ShadersResetOnDriverReset drvResetRp;

  virtual const char *procname() { return "shaders"; }

  ShadersRestartProc() : SRestartProc(RESTART_GAME | RESTART_VIDEO) {}

  void startup()
  {
    shaders_internal::init_stateblocks();
    if (maxFshVer == FSH_AS_HARDWARE)
      maxFshVer = getMaxFSHVersion();

    if (maxFshVer == FSH_AS_HARDWARE)
    {
      G_ASSERT(d3d::is_inited());
      const Driver3dDesc &dsc = d3d::get_driver_desc();

      if (dsc.fshver & DDFSH_6_6)
        maxFshVer = FSHVER_66;
      else if (dsc.fshver & DDFSH_6_0)
        maxFshVer = FSHVER_60;
      else if (dsc.fshver & DDFSH_5_0)
        maxFshVer = FSHVER_50;
      else if (dsc.fshver & DDFSH_4_1)
        maxFshVer = FSHVER_41;
      else if (dsc.fshver & DDFSH_4_0)
        maxFshVer = FSHVER_40;
      else if (dsc.fshver & DDFSH_3_0)
        maxFshVer = FSHVER_30;
      else if (dsc.fshver & DDFSH_2_A)
        maxFshVer = FSHVER_20A;
      else if (dsc.fshver & DDFSH_2_B)
        maxFshVer = FSHVER_20B;
      else if (dsc.fshver & DDFSH_2_0)
        maxFshVer = FSHVER_R300;
      else if (dsc.fshver & DDFSH_1_4)
        maxFshVer = FSHVER_R200;
      else if (dsc.fshver & DDFSH_1_3)
        maxFshVer = FSHVER_NV25;
      else if (dsc.fshver & DDFSH_1_1)
        maxFshVer = FSHVER_NV20;
      else /*G_ASSERT(0)*/
        maxFshVer = FSHVER_FFP;

      limitMaxFSHVersion(maxFshVer);
    }

    if (!fileName.empty())
    {
      static int fsh_caps[] = {0, DDFSH_1_1, DDFSH_1_3, DDFSH_1_4, DDFSH_2_0, DDFSH_2_B, DDFSH_2_A, DDFSH_3_0, DDFSH_4_0, DDFSH_4_1,
        DDFSH_5_0, DDFSH_6_0, DDFSH_6_6};

      G_ASSERT(maxFshVer >= 0 && maxFshVer < sizeof(fsh_caps) / sizeof(fsh_caps[0]));

      while (!load_shaders_bindump(fileName, maxFshVer))
      {
#if _TARGET_APPLE | _TARGET_ANDROID
        if (maxFshVer <= 7)
          maxFshVer = -1;
#endif
        maxFshVer--;
        while (maxFshVer >= 0 && !(d3d::get_driver_desc().fshver & fsh_caps[maxFshVer]))
          maxFshVer--;
        if (maxFshVer < 0)
        {
          fatal("Cannot find precompiled shaders for this videocard\n"
                "(while looking for files %s.psXX.shdump.bin)\n"
                "This can imply that videocard doesn't meet game's requirements\n\n",
            (char *)fileName);
          break;
        }
      }
      limitMaxFSHVersion(maxFshVer);
    }

    add_restart_proc(&drvResetRp);
  }

  void shutdown()
  {
    del_restart_proc(&drvResetRp);
    close_vdecl();

    // reset public varIds
    shaders_internal::close_global_constbuffers();
    shaders_internal::close_stateblocks();
    shaders_internal::shader_mats.clear();
    shaders_internal::shader_mat_elems.clear();
    ShaderStateBlock::clear();
    auto &dump = shBinDump();
    if (dump.varMap.size())
    {
      unsigned vars_resolved = 0, glob_vars_resolved = 0;
      for (int i = 0, ie = VariableMap::getVariablesCount(); i < ie; i++)
        if (dump.varIdx[i] < dump.VARIDX_ABSENT)
        {
          vars_resolved++;
          if (dump.globvarIdx[i] < dump.VARIDX_ABSENT)
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
#if DAGOR_DBGLEVEL > 0
    if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("dumpShaderStatistics", false))
    {
      const char *dump_name = last_loaded_shaders_bindump(false);
      char fname[DAGOR_MAX_PATH];
      build_shaderdump_filename(fname, dump_name, maxFshVer);
      strcat(fname, ".stat");
      dump_shader_statistics(fname);
      debug("Shader statistics has been dumped to %s", fname);
    }
#endif
    unload_shaders_bindump();
  }
};

static InitOnDemand<ShadersRestartProc> shaders_rproc;

void startup_shaders(const char *src_filename, int max_fshver)
{
  shaders_rproc.demandInit();
  shaders_rproc->fileName = src_filename;
  shaders_rproc->maxFshVer = max_fshver;

  add_restart_proc(shaders_rproc);
}
