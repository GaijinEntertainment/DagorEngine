// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/shaderCacheWarmup/shaderCacheWarmup.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_perfTimer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/mapBinarySearch.h>
#include <shaders/scriptSElem.h>
#include <shaders/scriptSMat.h>
#include <shaders/shStateBlk.h>
#include <shaders/shStateBlock.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_watchdog.h>
#include <util/dag_string.h>
#include <workCycle/dag_delayedAction.h>

#include <EASTL/bitvector.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif
#include <ioSys/dag_dataBlock.h>
#include <statsd/statsd.h>

namespace
{

static String cache_UUID_as_str()
{
  uint32_t cacheUUID[4] = {};
  d3d::driver_command(Drv3dCommand::GET_SHADER_CACHE_UUID, &cacheUUID[0]);
  String cacheUUIDstr("");
  for (uint32_t i : cacheUUID)
    cacheUUIDstr += String(4, "%08lX", i);
  return cacheUUIDstr;
}

static const char *shader_warmup_status_file = "shader_warmup_status.blk";

static bool cache_already_warmed_up()
{
  DataBlock warmupStatus;
  if (dd_file_exists(shader_warmup_status_file))
    warmupStatus.load(shader_warmup_status_file);

  const DataBlock *apiBlock = warmupStatus.getBlockByName(d3d::get_driver_name());
  if (!apiBlock)
    return false;

  if (strcmp(apiBlock->getStr("UUID", ""), cache_UUID_as_str()) != 0)
    return false;

  if (apiBlock->getInt("srcSize", 0) != shBinDumpOwner().getDumpSize())
    return false;

  if (apiBlock->getInt64("srcHash", 0) != shBinDumpOwner().getDumpHash())
    return false;

  return true;
}

static void mark_cache_warmed_up()
{
  DataBlock warmupStatus;
  if (dd_file_exists(shader_warmup_status_file))
    warmupStatus.load(shader_warmup_status_file);

  DataBlock *apiBlock = warmupStatus.addBlock(d3d::get_driver_name());
  apiBlock->setStr("UUID", cache_UUID_as_str());
  apiBlock->setInt("srcSize", shBinDumpOwner().getDumpSize());
  apiBlock->setInt64("srcHash", shBinDumpOwner().getDumpHash());

  warmupStatus.saveToTextFile(shader_warmup_status_file);
}

struct SMProps : public ShaderMaterialProperties
{
  Tab<ShaderMatData::VarValue> valStor;
  SMProps() : valStor(tmpmem) {}

  void init(const shaderbindump::ShaderClass &sc, TEXTUREID t)
  {
    sclass = &sc;
    valStor.resize(sclass->localVars.v.size());
    stvar.init(valStor.data(), valStor.size());

    for (int i = 0; i < MAXMATTEXNUM; ++i)
      textureId[i] = t;

    mem_set_0(stvar);

    for (int i = 0; i < stvar.size(); ++i)
      switch (sclass->localVars.v[i].type)
      {
        case SHVT_COLOR4: stvar[i].c4() = Color4(1, 1, 1, 1); break;

        case SHVT_INT: stvar[i].i = i + 2; break;

        case SHVT_REAL: stvar[i].r = (real)i * 2 + 2; break;

        case SHVT_TEXTURE: stvar[i].texId = t; break;

        case SHVT_SAMPLER: stvar[i].samplerHnd = d3d::INVALID_SAMPLER_HANDLE; break;

        default: G_ASSERT(0);
      }
  }
};

class AutoBlockStateWordScope
{
public:
  AutoBlockStateWordScope(const bool enable) { prev = ShaderGlobal::enableAutoBlockChange(enable); }

  ~AutoBlockStateWordScope() { ShaderGlobal::enableAutoBlockChange(prev); }

private:
  bool prev;
};

eastl::vector<const shaderbindump::ShaderClass *> gather_shader_classes(const Tab<const char *> &shader_names)
{
  const ScriptedShadersBinDump *sbd = &shBinDump();
  eastl::vector<const shaderbindump::ShaderClass *> shaderClasses;
  shaderClasses.reserve(shader_names.size());

  for (const char *shaderName : shader_names)
  {
    const shaderbindump::ShaderClass *sc = sbd->findShaderClass(shaderName);
    if (sc)
      shaderClasses.push_back(sc);
    else
      debug("shaders warmup: failed to find %s", shaderName);
  }

  return shaderClasses;
}

#define MS(us) (int)((us)*1000)
static const int COMPILE_TIME_LIMIT_DEFAULT = 100;
static const int TAIL_WAIT_TIME = 100;
static bool is_loading_thread = false;
static int compileTimeLimit = 0;
static int flushEveryNPipelines = 0;
static int maxFlushPeriodMs = 0;
enum
{
  NOT_PRESENTED = 0,
  PRESENTED_ENOUGH = 1
};
static volatile int was_present = NOT_PRESENTED;
static void (*prev_on_swap_cb)() = nullptr;
static int lastPresentAt = 0;

static void on_present()
{
  if (prev_on_swap_cb)
    prev_on_swap_cb();

  interlocked_increment(was_present);
}

static void replace_on_swap_cb()
{
  d3d::GpuAutoLock gpuLock;
  prev_on_swap_cb = ::dgs_on_swap_callback;
  ::dgs_on_swap_callback = on_present;
}

static void restore_on_swap_cb()
{
  d3d::GpuAutoLock gpuLock;
  ::dgs_on_swap_callback = prev_on_swap_cb;
}

class DynamicD3DFlusher
{
public:
  ~DynamicD3DFlusher() { releaseGPU(); }

  bool isGPUAcquired() { return gpuLocked; }

  bool afterPipelineCreation()
  {
    G_ASSERT(gpuLocked);

    compiledPipelinesCount += 1;
    size_t queued = d3d::driver_command(Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH);
    perPipeFlush = compiledPipelinesCount == flushEveryNPipelines;
    perQueueFlush = queued >= (flushEveryNPipelines * 2);
    return perPipeFlush || perQueueFlush;
  }

  void afterPipelineCreationFlush()
  {
    int64_t timeus = 0;
// detailed profiling
#if 0
    if (perPipeFlush)
    {
      TIME_PROFILE_NAME(pp_flush, String(32, "per_pipe-%u-%u", compiledPipelinesCount, flushEveryNPipelines));
      timeus = flushCommands();
    }
    else if (perQueueFlush)
    {
      TIME_PROFILE_NAME(pq_flush, String(32, "per_queued-%u-%u", queued, flushEveryNPipelines));
      timeus = flushCommands();
    }
#else
    timeus = flushCommands();
#endif

    if (perPipeFlush)
    {
      if (timeus < compileTimeLimit)
        flushEveryNPipelines += 1;
      else
        flushEveryNPipelines = (flushEveryNPipelines > 2) ? flushEveryNPipelines - 2 : 1;
    }

    int timems = timeus / 1000;
    if (timems > maxFlushPeriodMs)
      maxFlushPeriodMs = timems;
    compiledPipelinesCount = 0;
  }

  bool acquireGpu()
  {
    if (!gpuLocked)
    {
      d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
      d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN);

      gpuLocked = true;

      return true;
    }
    return false;
  }

  bool releaseGPU()
  {
    if (gpuLocked)
    {
      if (compiledPipelinesCount > 0)
        flushCommands();
      else
        unlockGpu();

      return true;
    }
    return false;
  }

private:
  int64_t flushCommands()
  {
    TIME_PROFILE(warmup_shaders_flush_cmds);
    if (is_loading_thread)
    {
      int64_t timeus = profile_ref_ticks();
      interlocked_release_store(was_present, NOT_PRESENTED);
      unlockGpu();
      if (perQueueFlush)
        spin_wait([&] { return d3d::driver_command(Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH) > flushEveryNPipelines; });
      spin_wait([&] { return interlocked_relaxed_load(was_present) <= PRESENTED_ENOUGH; });
      timeus = profile_time_usec(timeus);
      return timeus;
    }
    else
    {
      int64_t timeus = profile_ref_ticks();
      d3d::driver_command(Drv3dCommand::D3D_FLUSH, (void *)"shaders warmup");
      unlockGpu();
      watchdog_kick();
      timeus = profile_time_usec(timeus);

      return timeus;
    }
  }

private:
  void unlockGpu()
  {
    if (gpuLocked)
    {
      gpuLocked = false;
      d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END);
      d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
    }
  }

private:
  bool gpuLocked = false;
  bool perPipeFlush = false;
  bool perQueueFlush = false;
  size_t compiledPipelinesCount = 0;
};

class ShadersWarmup
{
public:
  using InvalidIntervalsSet = ska::flat_hash_map<uint16_t, uint64_t>; // [intervalId, normalizedValuesMask]
  ShadersWarmup(DynamicD3DFlusher &flusher, InvalidIntervalsSet &intervals) : d3dFlusher(flusher), invalidIntervals(intervals) {}

  ~ShadersWarmup()
  {
    // ScriptedShaderElement cleanups state on destruction, must be done under GPU lock
    d3d::GpuAutoLock gpuLock;
    elements.clear();
  }

  void warmupShaders(const eastl::vector<const shaderbindump::ShaderClass *> &shader_classes)
  {
    for (const auto &shaderClass : shader_classes)
      warmup(*shaderClass);

    if (d3dFlusher.isGPUAcquired())
      onGpuReleased();
    d3dFlusher.releaseGPU();
  }

private:
  PtrTab<ScriptedShaderMaterial> materials;
  PtrTab<ScriptedShaderElement> elements;
  eastl::bitvector<> usedStVariants;
  eastl::bitvector<> usedDynVariants;
  DynamicD3DFlusher &d3dFlusher;
  const InvalidIntervalsSet &invalidIntervals;

  virtual ScriptedShaderMaterial *initMaterial(const shaderbindump::ShaderClass &sc) = 0;
  virtual bool isValidShader(const int vpr, const int fsh) const = 0;
  virtual void onStaticVariant(ScriptedShaderElement &el) = 0;
  virtual void onDynamicVariant(const shaderbindump::ShaderCode::Pass &variant, const size_t variant_id,
    const ScriptedShaderElement &el) = 0;

  virtual void onGpuAcquired() = 0;
  virtual void onGpuReleased() = 0;

  void enumerateVariants(const shaderbindump::VariantTable &vt, int code_num, eastl::bitvector<> &usedVariants)
  {
    usedVariants.resize(code_num);
    eastl::fill(usedVariants.begin(), usedVariants.end(), false);

    int totalVariants = code_num ? 1 : 0;
    if (vt.codePieces.size())
    {
      const auto &ib = vt.codePieces.back();
      totalVariants = shBinDump().intervals[ib.intervalId].getValCount() * ib.totalMul;
    }

    for (int variantCode = 0; variantCode < totalVariants;)
    {
      bool variantValid = true;
      for (const auto &cp : vt.codePieces)
      {
        const auto &interval = shBinDump().intervals[cp.intervalId];
        unsigned ival = (variantCode / cp.totalMul) % interval.getValCount();
        uint64_t ivalBit = uint64_t(1) << ival;

        auto invalidInterval = invalidIntervals.find(interval.nameId);
        if (invalidInterval != invalidIntervals.end() && (invalidInterval->second & ivalBit))
        {
          variantCode += cp.totalMul;
          variantValid = false;
          break;
        }
      }

      if (variantValid)
      {
        int variantId = vt.findVariant(variantCode);
        if (variantId != shaderbindump::VariantTable::FIND_NOTFOUND && variantId != shaderbindump::VariantTable::FIND_NULL)
          usedVariants[variantId] = true;
        ++variantCode;
      }
    }
  }

  void warmup(const shaderbindump::ShaderClass &shaderClass)
  {
    TIME_PROFILE_NAME(warmup_iter, shaderClass.name.data());
    debug("shaders warmup: [shader class: %s, %d static variants]", shaderClass.name.data(), shaderClass.code.size());

    ScriptedShaderMaterial *ssm = initMaterial(shaderClass);
    if (!ssm)
    {
      logerr("shaders warmup: ERROR: creating ScriptedShaderMaterial");
      return;
    }
    materials.push_back(ssm);

    if (invalidIntervals.empty())
    {
      for (size_t stVariantId = 0; stVariantId < shaderClass.code.size(); ++stVariantId)
      {
        const shaderbindump::ShaderCode &code = shaderClass.code[stVariantId];
        ScriptedShaderElement *el = elements.push_back(ScriptedShaderElement::create(code, *ssm, ""));

        onStaticVariant(*el);

        uint32_t dynWarmedup = 0u;
        for (size_t dynVariantId = 0; dynVariantId < code.passes.size(); ++dynVariantId)
        {
          if (warmupDynamicVariant(code.passes[dynVariantId], dynVariantId, *el))
            ++dynWarmedup;
        }
        debug("shaders warmup: static variant #%d:  %d/%d dynamic variants", stVariantId, dynWarmedup, code.passes.size());
      }
    }
    else
    {
      enumerateVariants(shaderClass.stVariants, shaderClass.code.size(), usedStVariants);
      for (int stVariantId = 0; stVariantId < shaderClass.code.size(); ++stVariantId)
      {
        if (!usedStVariants[stVariantId])
          continue;

        const shaderbindump::ShaderCode &code = shaderClass.code[stVariantId];
        ScriptedShaderElement *el = elements.push_back(ScriptedShaderElement::create(code, *ssm, ""));

        onStaticVariant(*el);
        enumerateVariants(code.dynVariants, code.passes.size(), usedDynVariants);

        uint32_t dynWarmedup = 0u;
        for (int dynVariantId = 0; dynVariantId < code.passes.size(); ++dynVariantId)
        {
          if (!usedDynVariants[dynVariantId])
            continue;

          if (warmupDynamicVariant(code.passes[dynVariantId], dynVariantId, *el))
            ++dynWarmedup;
        }

        debug("shaders warmup: static variant #%d:  %d/%d dynamic variants", stVariantId, dynWarmedup, code.passes.size());
      }
    }
  }

  bool warmupDynamicVariant(const shaderbindump::ShaderCode::Pass &variantPasses, const size_t variantId,
    const ScriptedShaderElement &el)
  {
    if (isValidShader(variantPasses.rpass->vprId, variantPasses.rpass->fshId))
    {
      if (d3dFlusher.acquireGpu())
        onGpuAcquired();

      onDynamicVariant(variantPasses, variantId, el);

      if (d3dFlusher.afterPipelineCreation())
      {
        onGpuReleased();
        d3dFlusher.afterPipelineCreationFlush();
      }

      return true;
    }

    return false;
  }
};

class GraphicsShadersWarmup final : public ShadersWarmup
{
public:
  GraphicsShadersWarmup(DynamicD3DFlusher &flusher, InvalidIntervalsSet &intervals, BaseTexture *color, BaseTexture *depth) :
    ShadersWarmup(flusher, intervals), colorTarget(color), depthTarget(depth)
  {
    initResources();
  }

  ~GraphicsShadersWarmup()
  {
    if (texBlobId != BAD_TEXTUREID)
      release_managed_tex(texBlobId);
  }

private:
  void initResources()
  {
    sbd = &shBinDump();

    BaseTexture *tex = d3d::create_tex(nullptr, 1, 1, TEXCF_RGB, 1, "shaders_warmup_blob");
    if (tex)
      texBlobId = register_managed_tex("shaders_warmup_blob", tex);
  }

  virtual ScriptedShaderMaterial *initMaterial(const shaderbindump::ShaderClass &sc) override
  {
    eastl::unique_ptr<SMProps> smp(new SMProps());
    smp->init(sc, texBlobId);

    return ScriptedShaderMaterial::create(*smp);
  }

  virtual void onStaticVariant(ScriptedShaderElement &el) override { el.preCreateStateBlocks(); }

  virtual void onDynamicVariant(const shaderbindump::ShaderCode::Pass &, const size_t variant_id,
    const ScriptedShaderElement &el) override
  {
    d3d::set_program(el.passes[variant_id].id.pr);

    const ShaderStateBlockId variantStateIndex = el.passes[variant_id].id.v.load();
    if (variantStateIndex != ShaderStateBlockId::Invalid &&
        variantStateIndex != ScriptedShaderElement::PackedPassId::DELETED_STATE_BLOCK_ID)
      shaders::render_states::set(ShaderStateBlock::blocks[variantStateIndex].stateIdx);

    const uintptr_t pipelineType = STAGE_PS;
    const uintptr_t topology = PRIM_TRILIST;
    d3d::driver_command(Drv3dCommand::COMPILE_PIPELINE, (void *)pipelineType, (void *)topology);
  }

  virtual void onGpuAcquired() override
  {
    d3d_get_render_target(prevRT);

    d3d::set_render_target();
    if (colorTarget)
      d3d::set_render_target(colorTarget, 0);
    if (depthTarget)
      d3d::set_depth(depthTarget, DepthAccess::RW);
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0.f, 0);

    // clear vertex stream binds to avoid random incompatibility with pipelines
    d3d::setvsrc(0, nullptr, 0);
    d3d::setvsrc(1, nullptr, 0);
  }

  virtual void onGpuReleased() override { d3d_set_render_target(prevRT); }

  virtual bool isValidShader(const int vpr, const int fsh) const override
  {
    using namespace shaderbindump;
    return (vpr >= 0) && (fsh >= 0) && (vpr != ShaderCode::INVALID_FSH_VPR_ID) && (fsh != ShaderCode::INVALID_FSH_VPR_ID);
  }

private:
  eastl::vector<const shaderbindump::ShaderClass *> shaderClasses;
  const ScriptedShadersBinDump *sbd = nullptr;
  BaseTexture *colorTarget = nullptr;
  BaseTexture *depthTarget = nullptr;
  Driver3dRenderTarget prevRT;

  TEXTUREID texBlobId = BAD_TEXTUREID;
};

class ComputeShadersWarmup final : public ShadersWarmup
{
public:
  ComputeShadersWarmup(DynamicD3DFlusher &flusher, InvalidIntervalsSet &intervals) : ShadersWarmup(flusher, intervals) {}

private:
  virtual ScriptedShaderMaterial *initMaterial(const shaderbindump::ShaderClass &sc) override
  {
    MaterialData m;
    eastl::unique_ptr<ShaderMaterialProperties> smp(ShaderMaterialProperties::create(&sc, m));
    return ScriptedShaderMaterial::create(*smp);
  }

  virtual void onStaticVariant(ScriptedShaderElement &) override {}

  virtual void onDynamicVariant(const shaderbindump::ShaderCode::Pass &variant, const size_t, const ScriptedShaderElement &el)
  {
    const PROGRAM program = el.getComputeProgram(&variant.rpass.get());

    d3d::set_program(program);
    const uintptr_t pipelineType = STAGE_CS;
    d3d::driver_command(Drv3dCommand::COMPILE_PIPELINE, (void *)pipelineType);
  }

  virtual void onGpuAcquired() override
  {
    // do nothing
  }
  virtual void onGpuReleased() override
  {
    // do nothing
  }

  virtual bool isValidShader(const int vpr, const int fsh) const override
  {
    using namespace shaderbindump;
    return (vpr >= 0) && (fsh >= 0) && (vpr == ShaderCode::INVALID_FSH_VPR_ID) && (fsh != ShaderCode::INVALID_FSH_VPR_ID);
  }
};
} // namespace

void shadercache::warmup_shaders(const Tab<const char *> &graphics_shader_names, const Tab<const char *> &compute_shader_names,
  const WarmupParams &params, const bool is_loading_thrd)
{
  TIME_PROFILE(warmup_shaders);
#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("shader_warmup");
#endif

  shader_warmup_status_file = ::dgs_get_settings()->getBlockByNameEx("shadersWarmup")->getStr("statusBlk", shader_warmup_status_file);
  debug("shader warmup status file: %s", shader_warmup_status_file);
  if (cache_already_warmed_up())
  {
    debug("shaders cache up to date, skipping warmup");
    return;
  }

  compileTimeLimit =
    MS(::dgs_get_settings()->getBlockByNameEx("shadersWarmup")->getInt("compileTimeLimitMs", COMPILE_TIME_LIMIT_DEFAULT));
  flushEveryNPipelines = ::dgs_get_settings()->getBlockByNameEx("shadersWarmup")->getInt("flushEveryNPipelines", 250);

  is_loading_thread = is_loading_thrd;

  int64_t time = profile_ref_ticks();
  if (is_loading_thread)
    replace_on_swap_cb();

  {
    DynamicD3DFlusher d3dFlusher;

    ShadersWarmup::InvalidIntervalsSet invalidIntervals;
    for (const auto &[name, val] : params.invalidVars)
    {
      G_ASSERT(val < 64);
      if (int found = mapbinarysearch::bfindStrId(::shBinDump().varMap, name); found >= 0)
        invalidIntervals[static_cast<uint16_t>(found)] |= uint64_t(1) << val;
    }

    const auto graphicsShaderClasses = gather_shader_classes(graphics_shader_names);
    if (!graphicsShaderClasses.empty())
    {
      TIME_PROFILE(warmup_shaders_gr);
      GraphicsShadersWarmup graphics(d3dFlusher, invalidIntervals, params.colorTarget, params.depthTarget);
      graphics.warmupShaders(graphicsShaderClasses);
    }

    const auto computeShaderClasses = gather_shader_classes(compute_shader_names);
    if (!computeShaderClasses.empty())
    {
      TIME_PROFILE(warmup_shaders_cs);
      ComputeShadersWarmup compute(d3dFlusher, invalidIntervals);
      compute.warmupShaders(computeShaderClasses);
    }
  }

  {
    TIME_PROFILE(warmup_shaders_async_complete_wait);
    while (d3d::driver_command(Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH) > 0)
      sleep_msec(TAIL_WAIT_TIME);
  }

  if (is_loading_thread)
    restore_on_swap_cb();

  {
    TIME_PROFILE(warmup_shaders_save);
    d3d::driver_command(Drv3dCommand::SAVE_PIPELINE_CACHE);
  }

  mark_cache_warmed_up();

  time = profile_time_usec(time);

  // restrict precision to ms
  int64_t dltMs = time / 1000;
  float dltS = dltMs / 1000.0f;
  float maxFlushS = maxFlushPeriodMs / 1000.f;
  statsd::histogram("render.shader_cache_warmup.time_s", dltS);
  statsd::histogram("render.shader_cache_warmup.max_flush_s", maxFlushS);

  debug("shaders warmup took %f sec, max flush time %f sec", dltS, maxFlushS);
}
