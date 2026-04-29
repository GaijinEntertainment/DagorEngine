// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shCompiler.h"
#include "shCompContext.h"
#include "sh_stat.h"
#include "globalConfig.h"
#include "codeBlocks.h"
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include "shMacro.h"
#include "shLog.h"
#include <shaders/shUtils.h>
#include <shaders/shader_ver.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_io.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include "shsem.h"
#include "linkShaders.h"
#include "loadShaders.h"
#include "nameMap.h"
#include "cppStcode.h"
#include "cppStcodeAssembly.h"
#include "cppStcodeUtils.h"
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/random/dag_random.h>
#include <util/dag_threadPool.h>

#include <atomic>
#include <EASTL/deque.h>

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#endif

extern void parse_shader_script(const char *fn, Tab<SimpleString> *out_filenames, shc::TargetContext &ctx);

#ifdef PROFILE_OPCODE_USAGE
int opcode_usage[2][256];
#endif

namespace shc
{

static bool defTreatInvalidAsNull = false;
static String updbPath;
bool relinkOnly = false;
static SCFastNameMap explicitGlobVarRef;

static unsigned worker_cnt = 0;
static unsigned max_proc_count_for_worker_count = 0;

static constexpr size_t WORKER_STACK_SIZE = 1 << 20;

// Cpujobs bakcend data
static dag::AtomicInteger<uint32_t> jobs_in_flight_count = 0;
static unsigned cpujobs_job_mgr_base = 0;

// Threadpool backend data
static eastl::deque<cpujobs::IJob *> jobs_in_flight;
static uint32_t queue_pos = 0;

static constexpr unsigned MAX_WORKERS_FOR_AFFINITY_MASK = 64;
static constexpr int THREADPOOL_QUEUE_SIZE = 1024;

enum class ShutdownState : uint32_t
{
  RUNNING,
  SHUTTING_DOWN,
  SHUTDOWN_LOCKED
};

static dag::AtomicPod<ShutdownState> shutdown_state = ShutdownState::RUNNING;

bool try_enter_shutdown()
{
  // If succesfully entered, release-sync with anybody trying to enter later in MO and failing,
  //   acquire-sync with possible calls to unlock_shutdown.
  // If failer to enter, acquire-sync with the previous successful try_enter or try_lock

  ShutdownState expected = ShutdownState::RUNNING;
  return shutdown_state.compare_exchange_strong(expected, ShutdownState::SHUTTING_DOWN, dag::memory_order_acq_rel,
    dag::memory_order_acquire);
}

bool try_lock_shutdown()
{
  // Same MO reasoning as in try_enter_shutdown

  ShutdownState expected = ShutdownState::RUNNING;
  return shutdown_state.compare_exchange_strong(expected, ShutdownState::SHUTDOWN_LOCKED, dag::memory_order_acq_rel,
    dag::memory_order_acquire);
}

void unlock_shutdown()
{
  // Release-sync with later enters/locks
  shutdown_state.store(ShutdownState::RUNNING, dag::memory_order_release);
}

static void init_job_execution()
{
  if (!is_multithreaded())
    return;

  thread_local int workerWasInitialized = false;
  if (!workerWasInitialized)
  {
    // Dsc launches a lot of workers across processes, elevated prio only increases contention
    DaThread::applyThisThreadPriority(cpujobs::DEFAULT_THREAD_PRIORITY);
    workerWasInitialized = true;
  }
}

Job::Job() { G_ASSERT(is_main_thread()); }

void Job::doJob()
{
  if (is_in_worker())
    init_job_execution();

  doJobBody();
}

void Job::releaseJob() { releaseJobBody(); }

static eastl::pair<unsigned, unsigned> calculate_jobs_x_processes_caps(unsigned requested_num_workers)
{
  // @NOTE: having this much stuff is bizarre. However, in the current model we would get worse parallelism if we drop either job count
  // on proc count.

  // very small benefit from 24+ processes, but noticeable memory consumption
  constexpr unsigned PROC_CAP = 24;

#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2


#else
  constexpr unsigned PROC_X_JOBS_CAP = 1024;
#endif

  const unsigned reqWorkerCnt = requested_num_workers ? requested_num_workers : cpujobs::get_physical_core_count();

  const unsigned cappedWorkerCnt = min<unsigned>(reqWorkerCnt, MAX_WORKERS_FOR_AFFINITY_MASK);
  const unsigned procCapForJobCount = min<unsigned>(PROC_CAP, PROC_X_JOBS_CAP / cappedWorkerCnt);

  const unsigned procCap = min<int>(cpujobs::get_physical_core_count(), procCapForJobCount);

  sh_debug(SHLOG_INFO, "Processes x threads caps: max procs = %u, threads = %u", procCap, cappedWorkerCnt);

  return {cappedWorkerCnt, procCap};
}

void init_jobs(unsigned num_workers)
{
  G_ASSERT(is_main_thread());

  cpujobs::init(-1, false);

  auto [workers, processesCap] = calculate_jobs_x_processes_caps(num_workers);

  worker_cnt = workers;
  max_proc_count_for_worker_count = processesCap;

  if (!worker_cnt)
    worker_cnt = cpujobs::get_physical_core_count();
  else if (num_workers > MAX_WORKERS_FOR_AFFINITY_MASK)
    worker_cnt = MAX_WORKERS_FOR_AFFINITY_MASK;

  if (worker_cnt == 1)
  {
    debug("started in job-less mode");
    return;
  }

  threadpool::init(worker_cnt, THREADPOOL_QUEUE_SIZE, WORKER_STACK_SIZE);
  debug("started threadpool");
}

void deinit_jobs()
{
  G_ASSERT(!is_in_worker());

  if (!is_multithreaded())
    return;

  // @TODO: specific drop & shutdown? If so, do up threadpool or spoof jobs w/ flag?
  threadpool::shutdown();
  cpujobs::term(false);
  debug("terminated threadpool");
}

unsigned worker_count() { return worker_cnt; }
unsigned max_allowed_process_count() { return max_proc_count_for_worker_count; }
bool is_multithreaded() { return worker_cnt > 1; }
int current_worker() { return threadpool::get_current_worker_id(); }
bool is_in_worker() { return current_worker() != -1; }

void await_all_jobs(void (*on_released_cb)())
{
  G_ASSERT(!is_in_worker());

  if (jobs_in_flight.empty())
    return;

  threadpool::barrier_active_wait_for_job(jobs_in_flight.back(), threadpool::PRIO_HIGH, queue_pos);
  for (auto &j : jobs_in_flight)
  {
    threadpool::wait(j);
    j->releaseJob();
    if (on_released_cb)
      (*on_released_cb)();
  }

  jobs_in_flight.clear();
}

void add_job(Job *job)
{
  G_ASSERT(is_main_thread());

  if (!is_multithreaded())
  {
    job->doJob();
    job->releaseJob();
    return;
  }

  jobs_in_flight.push_back(job);
  threadpool::add(jobs_in_flight.back(), threadpool::PRIO_DEFAULT, queue_pos, threadpool::AddFlags::IgnoreNotDone);
  threadpool::wake_up_all();
}

void startup()
{
  resetCompiler();
  enable_sh_debug_con(true);
}

// close compiler
void shutdown() { enable_sh_debug_con(false); }

// reset shader compiler internal structures (before next compilation)
void resetCompiler() { reset_source_file(); }

void reset_source_file()
{
  close_shader_class();

  ErrorCounter::curShader().reset();
  ErrorCounter::allShaders().reset();
}

String get_obj_file_name_from_source(const String &source_file_name, const ShCompilationInfo &comp)
{
  char intermediateDir[260];
  strcpy(intermediateDir, comp.intermDir().c_str());
  dd_append_slash_c(intermediateDir);

  String objFileName = String(intermediateDir) + dd_get_fname(source_file_name);
  comp.hwopts().appendOptsTo(objFileName);
  objFileName += ".obj";

  return objFileName;
}

// check shader file cache & return true, if cache needs recompilation
bool should_recompile_sh(const CompilationContext &ctx, const String &sourceFileName)
{
  String objFileName = get_obj_file_name_from_source(sourceFileName, ctx.compInfo());
  return check_scripted_shader(objFileName, {}, ctx, shc::config().compileCppStcode()) != CompilerAction::NOTHING;
}
CompilerAction should_recompile(const CompilationContext &ctx)
{
  const auto &comp = ctx.compInfo();
  CompilerAction dumpCheckResult = check_scripted_shader(comp.dest().c_str(), comp.sources(), ctx, false);
  if (dumpCheckResult == CompilerAction::COMPILE_AND_LINK)
    return dumpCheckResult;

  int64_t destFileTime;
  if (!get_file_time64(comp.dest().c_str(), destFileTime))
    return CompilerAction::COMPILE_AND_LINK;

  for (unsigned int sourceFileNo = 0; sourceFileNo < comp.sources().size(); sourceFileNo++)
  {
    const String &sourceFileName = comp.sources()[sourceFileNo];
    String objFileName = get_obj_file_name_from_source(sourceFileName, comp);

    int64_t objFileTime;
    if (!get_file_time64(objFileName, objFileTime))
      return CompilerAction::COMPILE_AND_LINK;

    if (objFileTime > destFileTime)
      return CompilerAction::COMPILE_AND_LINK;

    if (should_recompile_sh(ctx, sourceFileName))
      return CompilerAction::COMPILE_AND_LINK;
  }
  return dumpCheckResult;
}

// compile shader files & generate variants to disk. return false, if error occurs
void compileShader(CompilerAction compiler_action, bool no_save, bool should_rebuild, const shc::CompilationContext &ctx)
{
  // Sanity check, args should be validated before calling the function
  G_ASSERT(!should_rebuild || (compiler_action != CompilerAction::NOTHING && compiler_action != CompilerAction::LINK_ONLY));

  if (compiler_action == CompilerAction::NOTHING)
    return;

  sh_debug(SHLOG_INFO, "Compile shaders to '%s'", ctx.compInfo().dest().str());

  const ShCompilationInfo &compInfo = ctx.compInfo();

  compInfo.hwopts().dumpInfo();

  // Compile.
#ifdef PROFILE_OPCODE_USAGE
  memset(opcode_usage, 0, sizeof(opcode_usage));
#endif

  if (compiler_action != CompilerAction::LINK_ONLY)
  {
    for (unsigned int sourceFileNo = 0; sourceFileNo < compInfo.sources().size(); sourceFileNo++)
    {
      const String &sourceFileName = compInfo.sources()[sourceFileNo];
      String objFileName = get_obj_file_name_from_source(sourceFileName, compInfo);

      bool need_recompile = should_rebuild || should_recompile_sh(ctx, sourceFileName);
      if (!need_recompile)
      {
        sh_debug(SHLOG_INFO, "No changes in '%s', skipping", sourceFileName.str());
      }
      else
      {
        if (relinkOnly)
        {
          sh_debug(SHLOG_FATAL, "need to recompile %s but compilation is denied by -relinkOnly", sourceFileName.str());
          return;
        }
        sh_debug(SHLOG_NORMAL, "[INFO] compiling '%s'...", sourceFileName.str());
        fflush(stdout);
        Tab<SimpleString> dependenciesList(tmpmem_ptr());

        TargetContext targetCtx = ctx.makeTargetContext(sourceFileName.c_str());

        reset_source_file();

        CodeSourceBlocks::incFiles.reset();
        parse_shader_script(sourceFileName, &dependenciesList, targetCtx);

#if _CROSS_TARGET_DX12
        if (use_two_phase_compilation(shc::config().targetPlatform))
          recompile_shaders(targetCtx);
#endif

        dependenciesList.reserve(dependenciesList.size() + CodeSourceBlocks::incFiles.nameCount());
        for (int i = 0, e = CodeSourceBlocks::incFiles.nameCount(); i < e; i++)
          dependenciesList.push_back() = CodeSourceBlocks::incFiles.getName(i);

        if (shc::config().dependencyDumpMode)
        {
          for (const auto &dep : dependenciesList)
            ctx.reportDepFile(dep);
          continue;
        }

        update_shaders_timestamps(dependenciesList, targetCtx);
        if (!no_save)
          save_scripted_shaders(objFileName, dependenciesList, targetCtx);

        ShaderCompilerStat::collectTargetStats(targetCtx);
      }
    }
  }
  if (compiler_action == CompilerAction::COMPILE_ONLY)
    return;

#ifdef PROFILE_OPCODE_USAGE
  debug("opcode usage (stateblocks):");
  for (int i = 0; i < 256; i++)
    if (opcode_usage[0][i])
      debug("  %05d: %s[%d]", opcode_usage[0][i], ShUtils::shcod_tokname(i), i);
  debug("opcode usage (dynamic):");
  for (int i = 0; i < 256; i++)
    if (opcode_usage[1][i])
      debug("  %05d: %s[%d]", opcode_usage[1][i], ShUtils::shcod_tokname(i), i);
  debug("-----------------------");
#endif

  // Link.

  sh_debug(SHLOG_NORMAL, "[INFO] Linking...");

  resetCompiler();
  String stats;
  stats.resize(2048);
  get_memory_stats(stats, 2048);
  debug("%s", stats.str());

  if (!no_save)
  {
    int64_t reft = ref_time_ticks();

    Tab<SimpleString> dependenciesList(tmpmem_ptr());
    for (const String &source : compInfo.sources())
      dependenciesList.push_back(SimpleString(source.c_str()));

    if (shc::config().dependencyDumpMode)
    {
      for (const auto &dep : dependenciesList)
        ctx.reportDepFile(dep);
      return;
    }

    TargetContext targetCtx = ctx.makeTargetContext(compInfo.dest().c_str());

    for (unsigned int sourceFileNo = 0; sourceFileNo < compInfo.sources().size(); sourceFileNo++)
    {
      const String &sourceFileName = compInfo.sources()[sourceFileNo];
      String objFileName = get_obj_file_name_from_source(sourceFileName, compInfo);

      file_ptr_t objFile = df_open(objFileName, DF_READ);
      G_ASSERT(objFile);

      int mmaped_len = 0;
      const void *mmaped = df_mmap(objFile, &mmaped_len);
      bool res = link_scripted_shaders((const uint8_t *)mmaped, mmaped_len, objFileName, sourceFileName, targetCtx);
      df_unmap(mmaped, mmaped_len);
      G_ASSERT(res);
#if SHOW_MEM_STAT
      get_memory_stats(stats, 2048);
      debug("%s", stats.str());
#endif
      df_close(objFile);
    }

    ShaderGlobal::validate_linked_gvar_collection(targetCtx);

    sh_debug(SHLOG_NORMAL, "[INFO] linked in %gms", get_time_usec(reft) / 1000.);

    reft = ref_time_ticks();
    sh_debug(SHLOG_NORMAL, "[INFO] Saving...");

    // Don't save cppstcode for the lib, it would be gibberish
    save_scripted_shaders(compInfo.dest().c_str(), dependenciesList, targetCtx, false);

    sh_debug(SHLOG_NORMAL, "[INFO] saved in %gms", get_time_usec(reft) / 1000.);

    ShaderCompilerStat::collectTargetStats(targetCtx);
  }
}

bool buildShaderBinDump(const char *bindump_fn, const char *sh_fn, bool forceRebuild, bool minidump, BindumpPackingFlags packing_flags,
  const shc::CompilationContext &comp)
{
  if (!forceRebuild)
  {
    int64_t sh_time, dump_time;
    if (!get_file_time64(sh_fn, sh_time))
    {
      dd_erase(bindump_fn);
      sh_debug(SHLOG_ERROR, "Time stat syscall for bindump %s erred, aborting build...", bindump_fn);
      return false;
    }

    if (get_file_time64(bindump_fn, dump_time))
    {
      if (shc::config().compileCppStcode() && !minidump)
      {
        auto statMaybe = get_main_cpp_files_stat(comp.compInfo());
        if (!statMaybe || statMaybe->savedBlkHash != comp.compInfo().targetBlkHash())
          dump_time = -1; // @HACK to force recompilation
        else
          dump_time = min(dump_time, statMaybe->mtime);
      }

      if (dump_time >= sh_time)
      {
        FullFileLoadCB crd(bindump_fn);
        shader_layout::ScriptedShadersBinDumpCompressedHeader hdr{};
        if (crd.tryRead(&hdr, sizeof(hdr)) == sizeof(hdr))
        {
          if (hdr.magicPart1 == _MAKE4C('VSPS') && hdr.magicPart2 == _MAKE4C('dump') && hdr.version == SHADER_BINDUMP_VER)
          {
            if (memcmp(hdr.buildBlkHash, comp.compInfo().targetBlkHash().data(), sizeof(hdr.buildBlkHash)) == 0)
            {
              sh_debug(SHLOG_NORMAL, "[INFO] Skipping up-to-date binary %sdump '%s'", minidump ? "mini" : "", bindump_fn);
              return true;
            }
            else
            {
              sh_debug(SHLOG_NORMAL, "[INFO] Outdated blk hash '%s' in header of %sdump '%s'",
                blk_hash_string(hdr.buildBlkHash).c_str(), minidump ? "mini" : "", bindump_fn);
            }
          }
        }
      }
    }
  }
  String tempbindump_fn(bindump_fn);
  tempbindump_fn += ".tmp.bin";

  if (!make_scripted_shaders_dump(tempbindump_fn, sh_fn, minidump, packing_flags, comp))
  {
    sh_debug(SHLOG_ERROR, "Can't build binary %sdump from: '%s'\n", minidump ? "mini" : "", sh_fn);
    dd_erase(bindump_fn);
    return false;
  }
  else
  {
    if (!dd_rename(tempbindump_fn.c_str(), bindump_fn))
    {
      sh_debug(SHLOG_ERROR, "Can't rename binary '%s'->'%s'\n", tempbindump_fn.c_str(), bindump_fn);
      return false;
    }
  }

  sh_debug(SHLOG_INFO, "+++ Built shaders binary %sdump: '%s'\n", minidump ? "mini" : "", bindump_fn);
  return true;
}

void clearFlobVarRefList() { explicitGlobVarRef.reset(); }
void addExplicitGlobVarRef(const char *vname) { explicitGlobVarRef.addNameId(vname); }
bool isGlobVarRequired(const char *vname) { return explicitGlobVarRef.getNameId(vname) != -1; }

struct ShaderValidVariantList
{
  struct VarVal
  {
    int nameId;
    float val;
  };
  struct Ref
  {
    short startIdx, count;
  };
  SCFastNameMap vars;
  Tab<VarVal> vvData;
  Tab<Ref> vv;
  bool invalidAsNull;

  ShaderValidVariantList() : vvData(midmem), vv(midmem), invalidAsNull(defTreatInvalidAsNull) {}
  void fill(const DataBlock &blk)
  {
    int nid = blk.getNameId("invalid");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        const DataBlock &b = *blk.getBlock(i);
        Ref &r = vv.push_back();
        r.startIdx = vvData.size();
        for (int j = 0; j < b.paramCount(); j++)
          if (b.getParamType(j) == DataBlock::TYPE_INT)
          {
            VarVal &v = vvData.push_back();
            v.nameId = vars.addNameId(b.getParamName(j));
            v.val = (real)b.getInt(j);
          }
          else if (b.getParamType(j) == DataBlock::TYPE_REAL)
          {
            VarVal &v = vvData.push_back();
            v.nameId = vars.addNameId(b.getParamName(j));
            v.val = b.getReal(j);
          }
        r.count = vvData.size() - r.startIdx;
        if (!r.count)
          vv.pop_back();
      }
    vvData.shrink_to_fit();
    vv.shrink_to_fit();
    invalidAsNull = blk.getBool("treatInvalidAsNull", invalidAsNull);
  }
  void makeTypeMap(Tab<int> &map, const ShaderVariant::TypeTable &v)
  {
    map.resize(vars.nameCount());
    mem_set_ff(map);
    for (int i = 0; i < v.getCount(); i++)
    {
      int id = vars.getNameId(v.getIntervalName(i));
      if (id != -1)
        map[id] = i;
    }
  }
  void gatherRelevantIdx(FastIntList &idx, dag::ConstSpan<int> map)
  {
    idx.reset();
    for (int i = 0; i < vv.size(); i++)
    {
      bool all_found = true;
      for (int j = vv[i].startIdx, end = j + vv[i].count; j < end; j++)
        if (map[vvData[j].nameId] == -1)
        {
          all_found = false;
          break;
        }
      if (all_found)
        idx.addInt(i);
    }
  }
  void gatherRelevantIdx2(FastIntList &idx, dag::ConstSpan<int> map, const FastIntList &idx_base, dag::ConstSpan<int> base_map)
  {
    idx.reset();
    for (int i = 0; i < vv.size(); i++)
      if (!idx_base.hasInt(i))
      {
        bool all_found = true;
        for (int j = vv[i].startIdx, end = j + vv[i].count; j < end; j++)
          if (map[vvData[j].nameId] == -1 && base_map[vvData[j].nameId] == -1)
          {
            all_found = false;
            break;
          }
        if (all_found)
          idx.addInt(i);
      }
  }

  bool checkValid(dag::ConstSpan<int> idx, dag::ConstSpan<int> map, const ShaderVariant::VariantSrc &v)
  {
    const ShaderVariant::TypeTable &vt = v.getTypes();
    for (int i = 0; i < idx.size(); i++)
    {
      bool all_eq = true;
      for (int j = vv[idx[i]].startIdx, end = j + vv[idx[i]].count; j < end; j++)
      {
        int mid = map[vvData[j].nameId];
        G_ASSERT(mid >= 0);
        if (vt.normalizeValue(mid, vvData[j].val) != v.getNormalizedValue(mid))
        {
          all_eq = false;
          break;
        }
      }
      if (all_eq)
        return true;
    }
    return false;
  }
  bool checkValid2(dag::ConstSpan<int> idx, dag::ConstSpan<int> map, const ShaderVariant::VariantSrc &v, dag::ConstSpan<int> map2,
    const ShaderVariant::VariantSrc &v2)
  {
    const ShaderVariant::TypeTable &vt = v.getTypes();
    const ShaderVariant::TypeTable &vt2 = v2.getTypes();
    for (int i = 0; i < idx.size(); i++)
    {
      bool all_eq = true;
      for (int j = vv[idx[i]].startIdx, end = j + vv[idx[i]].count; j < end; j++)
      {
        int mid = map[vvData[j].nameId];
        int mid2 = map2[vvData[j].nameId];
        G_ASSERT(mid >= 0 || mid2 >= 0);
        if ((mid >= 0 && vt.normalizeValue(mid, vvData[j].val) != v.getNormalizedValue(mid)) ||
            (mid2 >= 0 && vt.normalizeValue(mid2, vvData[j].val) != v2.getNormalizedValue(mid2)))
        {
          all_eq = false;
          break;
        }
      }
      if (all_eq)
        return true;
    }
    return false;
  }

  void dump()
  {
    for (int i = 0; i < vv.size(); i++)
    {
      debug("  invalid %d:", vv[i].count);
      for (int j = 0; j < vv[i].count; j++)
        debug("    %s[%d]=%f:", vars.getName(vvData[vv[i].startIdx + j].nameId), vvData[vv[i].startIdx + j].nameId,
          vvData[vv[i].startIdx + j].val);
    }
  }
};
static eastl::vector<ShaderValidVariantList> svvl;
static SCFastNameMap svvlNames;
static ShaderValidVariantList *curSvvl = NULL;
static const ShaderVariant::TypeTable *curStatVariantTypes = NULL, *curDynVariantTypes = NULL;
static FastIntList curVvStIdx, curVvDynIdx;
static Tab<int> stTypeMap(midmem), dynTypeMap(midmem);

void setInvalidAsNullDef(bool on) { defTreatInvalidAsNull = on; }
void setValidVariantsBlock(DataBlock *block)
{
  svvl.clear();
  svvlNames.reset();
  if (!block)
    return;

  svvl.emplace_back();
  svvl.back().fill(*block);
  int nid = block->getNameId("invalid");
  for (int i = 0; i < block->blockCount(); i++)
  {
    if (block->getBlock(i)->getBlockNameId() == nid)
      continue;
    if (svvlNames.getNameId(block->getBlock(i)->getBlockName()) != -1)
    {
      sh_debug(SHLOG_ERROR, "duplicate block <%s> in valid_variants block", block->getBlock(i)->getBlockName());
      continue;
    }

    svvl.emplace_back();
    ShaderValidVariantList &l = svvl.back();
    l.fill(*block);
    l.fill(*block->getBlock(i));
    if (l.vv.size())
      svvlNames.addNameId(block->getBlock(i)->getBlockName());
    else
      svvl.pop_back();
  }
  if (svvl.size() == 1 && !svvl[0].vv.size())
  {
    svvl.clear();
    svvlNames.reset();
  }

  for (int i = 0; i < svvl.size(); i++)
  {
    debug("shader <%s>, %d", i == 0 ? "ALL" : svvlNames.getName(i - 1), svvl[i].vv.size());
    svvl[i].dump();
  }
}

void prepareTestVariantShader(const char *name)
{
  curStatVariantTypes = NULL;
  curDynVariantTypes = NULL;
  curSvvl = svvl.size() && name ? &svvl[svvlNames.getNameId(name) + 1] : NULL;
  curVvStIdx.reset();
  curVvDynIdx.reset();
}
void prepareTestVariant(const ShaderVariant::TypeTable *sv, const ShaderVariant::TypeTable *dv)
{
  if (!curSvvl)
    return;
  if (sv)
  {
    if (curStatVariantTypes != sv)
    {
      curSvvl->makeTypeMap(stTypeMap, *sv);
      curSvvl->gatherRelevantIdx(curVvStIdx, stTypeMap);
    }
    curStatVariantTypes = sv;
  }

  if (dv)
  {
    if (curDynVariantTypes != dv)
    {
      curSvvl->makeTypeMap(dynTypeMap, *dv);
      curSvvl->gatherRelevantIdx2(curVvDynIdx, dynTypeMap, curVvStIdx, stTypeMap);
    }
    curDynVariantTypes = dv;
  }
  else
  {
    // reset dynamic variant
    curDynVariantTypes = NULL;
  }
}
bool isValidVariant(const ShaderVariant::VariantSrc *sv, const ShaderVariant::VariantSrc *dv)
{
  if (!curSvvl)
    return true;

  if (!dv)
    return !curSvvl->checkValid(curVvStIdx.getList(), stTypeMap, *sv);
  return !curSvvl->checkValid2(curVvDynIdx.getList(), dynTypeMap, *dv, stTypeMap, *sv);
}
bool shouldMarkInvalidAsNull() { return curSvvl ? curSvvl->invalidAsNull : true; }
void setOutputUpdbPath(const char *path) { updbPath = path; }
const char *getOutputUpdbPath() { return updbPath; }
} // namespace shc
