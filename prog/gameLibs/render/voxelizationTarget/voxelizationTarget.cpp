// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_initOnDemand.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <util/dag_convar.h>
#include <shaders/dag_overrideStates.h>

class VoxelizationTarget
{
  TexPtr sampledTarget;
  shaders::UniqueOverrideStateId voxelizeOverride;
  uint32_t used = 0;

public:
  void init();
  void close();
  ~VoxelizationTarget()
  {
    if (used)
      logerr("VoxelizationTarget used counter is not zero (%d) on exit", used);
  }
  bool ensureSampledTarget(int w, int h, uint32_t fmt);
  bool setSubsampledTargetAndOverride(int w, int h);
  void resetOverride() { shaders::overrides::reset(); }
};

void VoxelizationTarget::close()
{
  if (used == 0)
  {
    logerr("VoxelizationTarget closed more than opened");
    return;
  }
  if (--used != 0)
    return;
  shaders::overrides::destroy(voxelizeOverride);
  sampledTarget.close();
}

void VoxelizationTarget::init()
{
  if (used++ != 0)
    return;
  shaders::OverrideState state;
  state.set(shaders::OverrideState::CULL_NONE);
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::Z_TEST_DISABLE);
  if (d3d::get_driver_desc().caps.hasForcedSamplerCount || d3d::get_driver_desc().caps.hasUAVOnlyForcedSampleCount)
  {
    state.set(shaders::OverrideState::FORCED_SAMPLE_COUNT);
    state.forcedSampleCount = 8; // 8 is always supported
    debug("voxelization uses Forced Sample Count: %d", state.forcedSampleCount);
  }
  voxelizeOverride = shaders::overrides::create(state);
}

bool VoxelizationTarget::ensureSampledTarget(int w, int h, uint32_t fmt)
{
  TextureInfo tinfo;
  if (sampledTarget)
    sampledTarget->getinfo(tinfo);
  if (((tinfo.cflg & TEXFMT_MASK) != (fmt & TEXFMT_MASK)) || tinfo.w < w || tinfo.h < h)
  {
    TexPtr tex = dag::create_tex(NULL, w, h, fmt | TEXCF_RTARGET, 1, "voxelization_msaa_target");
    debug("MSAA voxelization target %s %dx%d fmt 0x%X", tex ? "created" : "can't be created", w, h, fmt);
    if (!tex)
      return false;
    sampledTarget = eastl::move(tex);
  }
  d3d::set_render_target(sampledTarget.get(), 0, 0);
  return true;
}

static int find_max_msaa_format()
{
  using namespace eastl;
  int maxMsaaFormats[] = {
    TEXFMT_R8, // sort by bit count
    TEXFMT_L16,
    TEXFMT_R8G8,
    TEXFMT_R8G8B8A8,
  };
  int maxMsaaSamples[size(maxMsaaFormats)] = {};
  transform(begin(maxMsaaFormats), end(maxMsaaFormats), begin(maxMsaaSamples),
    [](auto fmt) { return d3d::get_max_sample_count(fmt); });
  auto maxIdx = distance(begin(maxMsaaSamples), max_element(begin(maxMsaaSamples), end(maxMsaaSamples)));
  return make_sample_count_flag(maxMsaaSamples[maxIdx]) | maxMsaaFormats[maxIdx];
}

ConVarT<int, true> gi_use_forced_sample_count("render.gi_use_forced_sample_count", 1, 0, 2, "0 - OFF, 1 - ON, 2 - UAV only");

bool VoxelizationTarget::setSubsampledTargetAndOverride(int w, int h)
{
  G_ASSERT(used);
  if (gi_use_forced_sample_count.get() == 1 && d3d::get_driver_desc().caps.hasForcedSamplerCount)
  {
    BaseTexture *backBuf = d3d::get_backbuffer_tex();
    TextureInfo tinfo;
    if (!backBuf->getinfo(tinfo) || (tinfo.cflg & TEXCF_SAMPLECOUNT_MASK) || tinfo.w < w || tinfo.h < h)
    {
      if (!ensureSampledTarget(w, h, TEXFMT_R8))
        return false;
    }
    else
      d3d::set_render_target(backBuf, 0, 0);
  }
  else if (gi_use_forced_sample_count.get() && d3d::get_driver_desc().caps.hasUAVOnlyForcedSampleCount) // vulkan codepath
  {
    d3d::set_render_target(nullptr, 0, 0);
  }
  // do not use pure MSAA path on vulkan, as we can't work there with pass breaks (caller code does not care about that)
  else if (d3d::get_driver_code().is(d3d::vulkan) || !ensureSampledTarget(w, h, find_max_msaa_format()))
  {
    //? w *= 2; h *= 2;//supersampling should be better, but probably videocard is really slow
    if (!ensureSampledTarget(w, h, TEXFMT_R8))
      return false;
  }
  if (gi_use_forced_sample_count.get()) // fixme: use different override, without forced sample count
    shaders::overrides::set(voxelizeOverride);
  d3d::set_depth(nullptr, DepthAccess::SampledRO);
  d3d::setview(0, 0, w, h, 0, 1);
  return true;
}

VoxelizationTarget g_voxelization_target;

void init_voxelization() { g_voxelization_target.init(); }

void close_voxelization() { g_voxelization_target.close(); }

bool set_voxelization_target_and_override(int w, int h) { return g_voxelization_target.setSubsampledTargetAndOverride(w, h); }
void reset_voxelization_override() { g_voxelization_target.resetOverride(); }