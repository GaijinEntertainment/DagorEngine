// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "soundOcclusionGPU_internal.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <math/dag_Point4.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_log.h>
#include <memory/dag_framemem.h>

#define SHADER_VARS_LIST \
  VAR(snd_occ_params)    \
  VAR(snd_occ_tuning)    \
  VAR(snd_occ_listener_pos)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SHADER_VARS_LIST
#undef VAR

namespace snd_occlusion_gpu
{

GpuSoundOcclusion gpu_sound_occlusion;


void GpuSoundOcclusion::init(const Config &in_cfg)
{
  if (inited)
    close();

  cfg = in_cfg;

  occlusionCs.reset(new_compute_shader("sound_occlusion_gpu_cs", true));
  if (!occlusionCs)
  {
    logwarn("soundOcclusionGPU: shader 'sound_occlusion_gpu_cs' not found, GPU occlusion disabled");
    return;
  }

  resultRingBuffer.init(sizeof(Point4), cfg.maxSources, cfg.ringBufferFrames, "snd_occ_results", SBCF_UA_STRUCTURED_READBACK, 0,
    false);

  inputBuf = UniqueBufHolder(dag::create_sbuffer(sizeof(Point4), cfg.maxSources,
                               SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED | SBCF_DYNAMIC, 0, "snd_occ_input"),
    "snd_occ_input");

  sources.resize(cfg.maxSources);
  results.resize(cfg.maxSources);
  prevSourcePos.resize(cfg.maxSources, Point3(0, 0, 0));

  for (auto &r : results)
  {
    memset(r.values, 0, sizeof(r.values));
    r.valid = false;
    r.frameAge = 0;
  }

  freeIndices.resize(cfg.maxSources);
  for (int i = 0; i < cfg.maxSources; i++)
    freeIndices[i] = cfg.maxSources - 1 - i; // pop_back() will yield 0, 1, 2, ...

  frameCounter = 0;
  lastReadbackFrame = -1;
  inited = true;

  debug("soundOcclusionGPU: initialized with %d sources, %d ring frames", cfg.maxSources, cfg.ringBufferFrames);
}


void GpuSoundOcclusion::close()
{
  if (!inited)
    return;

  occlusionCs.reset();
  inputBuf.close();
  resultRingBuffer.close();
  sources.clear();
  results.clear();
  prevSourcePos.clear();
  freeIndices.clear();
  inited = false;
}


bool GpuSoundOcclusion::is_inited() const { return inited && occlusionCs; }


void GpuSoundOcclusion::set_listener(const Point3 &pos) { listenerPos = pos; }


void GpuSoundOcclusion::set_source(int idx, const Point3 &pos, float radius, OcclusionMode mode)
{
  if (idx >= 0 && idx < cfg.maxSources)
  {
    sources[idx].pos = pos;
    sources[idx].radius = radius;
    sources[idx].mode = mode;
    sources[idx].active = true;
  }
}


int GpuSoundOcclusion::create_source(const Point3 &pos, float radius, OcclusionMode mode)
{
  if (!inited || freeIndices.empty())
    return -1;

  const int idx = freeIndices.back();
  freeIndices.pop_back();
  G_ASSERT(!sources[idx].active && freeIndices.size() < cfg.maxSources);

  sources[idx].pos = pos;
  sources[idx].radius = radius;
  sources[idx].mode = mode;
  sources[idx].active = true;
  return idx;
}

void GpuSoundOcclusion::delete_source(int idx)
{
  G_ASSERT_RETURN(idx >= 0 && idx < cfg.maxSources, );
  G_ASSERT_RETURN(sources[idx].active, );

  sources[idx].active = false;
  sources[idx].generation++;
  results[idx].valid = false;
  freeIndices.push_back(idx);
}

int GpuSoundOcclusion::get_num_active_sources() const
{
  int num = 0;
  if (inited)
    for (int si = 0; si < cfg.maxSources; si++)
      if (sources[si].active)
        ++num;
  return num;
}

int GpuSoundOcclusion::get_max_active_sources() const { return inited ? sources.size() : 0; }

int GpuSoundOcclusion::get_active_pair_count() const
{
  int count = 0;
  for (int si = 0; si < cfg.maxSources; si++)
    if (sources[si].active)
      count++;
  return count;
}


void GpuSoundOcclusion::processReadback()
{
  int stride = 0;
  uint32_t readFrame = 0;
  while (uint8_t *data = resultRingBuffer.lock(stride, readFrame, false))
  {
    int recordIdx = readFrame % MAX_RING_BUFFERS;
    DispatchRecord &rec = dispatchRecords[recordIdx];
    const Point4 *readbackResults = (const Point4 *)data;

    for (int i = 0; i < rec.activePairCount; i++)
    {
      uint32_t si = rec.pairMapping[i];
      if (si >= (uint32_t)cfg.maxSources)
        continue;

      PairResult &pr = results[si];

      if (rec.pairGenerations[i] != sources[si].generation)
        continue;

      float newVals[4] = {readbackResults[i].x, readbackResults[i].y, readbackResults[i].z, readbackResults[i].w};

      if (pr.valid)
      {
        float posDelta = length(sources[si].pos - prevSourcePos[si]) + length(listenerPos - prevListenerPos);
        float alpha = (posDelta > 0.5f) ? 0.3f : 0.1f;

        for (int c = 0; c < 4; c++)
          pr.values[c] = pr.values[c] * (1.0f - alpha) + newVals[c] * alpha;
      }
      else
      {
        for (int c = 0; c < 4; c++)
          pr.values[c] = newVals[c];
      }

      pr.valid = true;
      pr.frameAge = 0;
    }

    resultRingBuffer.unlock();
    lastReadbackFrame = (int)readFrame;
  }

  for (auto &r : results)
    if (r.valid)
      r.frameAge++;
}


void GpuSoundOcclusion::dispatch()
{
  if (!inited || !occlusionCs)
    return;

  if (d3d::device_lost(nullptr))
    return;

  processReadback();

  // Build per-source input: 1 float4 per source = (pos.xyz, packed(radius|mode))
  dag::Vector<Point4, framemem_allocator> inputUpload;
  dag::Vector<uint32_t, framemem_allocator> sourceMapping;
  dag::Vector<uint32_t, framemem_allocator> generationMapping;

  for (int si = 0; si < cfg.maxSources; si++)
  {
    if (!sources[si].active)
      continue;

    // Pack mode into 2 LSBs of radius float
    uint32_t radiusBits;
    memcpy(&radiusBits, &sources[si].radius, sizeof(float));
    radiusBits = (radiusBits & ~3u) | ((uint32_t)sources[si].mode & 3u);
    float packedW;
    memcpy(&packedW, &radiusBits, sizeof(float));

    inputUpload.push_back(Point4(sources[si].pos.x, sources[si].pos.y, sources[si].pos.z, packedW));
    sourceMapping.push_back(si);
    generationMapping.push_back(sources[si].generation);
  }

  int activeSourceCount = (int)sourceMapping.size();
  if (activeSourceCount == 0)
    return;

  if (!inputBuf.getBuf())
    return;

  inputBuf.getBuf()->updateData(0, sizeof(Point4) * activeSourceCount, inputUpload.data(), VBLOCK_DISCARD | VBLOCK_WRITEONLY);
  inputBuf.setVar();

  // Get readback target from ring buffer
  int frame = 0;
  Sbuffer *resultBuffer = (Sbuffer *)resultRingBuffer.getNewTarget(frame);
  if (!resultBuffer)
    return;

  // Store dispatch record for readback mapping
  int recordIdx = frame % MAX_RING_BUFFERS;
  dispatchRecords[recordIdx].pairMapping.assign(sourceMapping.begin(), sourceMapping.end());
  dispatchRecords[recordIdx].activePairCount = activeSourceCount;
  dispatchRecords[recordIdx].pairGenerations.assign(generationMapping.begin(), generationMapping.end());

  // Zero result buffer, bind UAV, barrier, set constants, dispatch
  d3d::zero_rwbufi(resultBuffer);
  STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), resultBuffer);
  d3d::resource_barrier({resultBuffer, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  snd_occ_paramsVarId.set_int4(activeSourceCount, frameCounter, 0, 0);
  snd_occ_tuningVarId.set_float4(hardnessK, coneHalfAngleDeg * (3.14159265f / 180.0f), 0, 0);
  snd_occ_listener_posVarId.set_float4(listenerPos.x, listenerPos.y, listenerPos.z, 0);

  occlusionCs->dispatch((activeSourceCount + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE, 1, 1);

  resultRingBuffer.startCPUCopy();

  // Save positions for temporal blending next frame
  for (int si = 0; si < cfg.maxSources; si++)
    prevSourcePos[si] = sources[si].pos;
  prevListenerPos = listenerPos;

  frameCounter++;
}


bool GpuSoundOcclusion::get_occlusion(int source_idx, float &out_occlusion) const
{
  out_occlusion = 0;
  if (!inited)
    return false;
  if (source_idx < 0 || source_idx >= cfg.maxSources)
    return false;

  const PairResult &r = results[source_idx];
  if (!r.valid)
    return false;

  out_occlusion = r.values[0];
  return true;
}


bool GpuSoundOcclusion::get_occlusion_direct_reverb(int source_idx, float &out_direct, float &out_reverb) const
{
  out_direct = 0;
  out_reverb = 0;
  if (!inited)
    return false;
  if (source_idx < 0 || source_idx >= cfg.maxSources)
    return false;

  const PairResult &r = results[source_idx];
  if (!r.valid)
    return false;

  out_direct = r.values[0];
  out_reverb = r.values[1];
  return true;
}


void GpuSoundOcclusion::set_hardness_k(float k) { hardnessK = k; }
void GpuSoundOcclusion::set_cone_half_angle(float deg) { coneHalfAngleDeg = deg; }

int GpuSoundOcclusion::get_readback_age() const
{
  if (lastReadbackFrame < 0)
    return -1;
  return (int)frameCounter - lastReadbackFrame;
}

static void after_reset(bool) { gpu_sound_occlusion.resultRingBuffer.reset(); }

// --- Free functions delegating to global instance ---

void init(const Config &cfg) { gpu_sound_occlusion.init(cfg); }
void close()
{
  world_sdf_debug_close();
  gpu_sound_occlusion.close();
}
bool is_inited() { return gpu_sound_occlusion.is_inited(); }

void set_listener(const Point3 &pos) { gpu_sound_occlusion.set_listener(pos); }
void set_source(int idx, const Point3 &pos, float radius, OcclusionMode mode)
{
  gpu_sound_occlusion.set_source(idx, pos, radius, mode);
}
int create_source(const Point3 &pos, float radius, OcclusionMode mode) { return gpu_sound_occlusion.create_source(pos, radius, mode); }
void delete_source(int idx) { gpu_sound_occlusion.delete_source(idx); }
int get_num_active_sources() { return gpu_sound_occlusion.get_num_active_sources(); }
int get_max_active_sources() { return gpu_sound_occlusion.get_max_active_sources(); }

int get_active_pair_count() { return gpu_sound_occlusion.get_active_pair_count(); }

void dispatch() { gpu_sound_occlusion.dispatch(); }

bool get_occlusion(int source_idx, float &out_occlusion) { return gpu_sound_occlusion.get_occlusion(source_idx, out_occlusion); }
bool get_occlusion_direct_reverb(int source_idx, float &out_direct, float &out_reverb)
{
  return gpu_sound_occlusion.get_occlusion_direct_reverb(source_idx, out_direct, out_reverb);
}

void set_hardness_k(float k) { gpu_sound_occlusion.set_hardness_k(k); }
void set_cone_half_angle(float deg) { gpu_sound_occlusion.set_cone_half_angle(deg); }
int get_readback_age() { return gpu_sound_occlusion.get_readback_age(); }

// =============================================================================
} // namespace snd_occlusion_gpu

REGISTER_D3D_AFTER_RESET_FUNC(snd_occlusion_gpu::after_reset);
