// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundOcclusionGPU/soundOcclusionGPU.h>
#include <drv/3d/dag_resId.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>

class ComputeShaderElement;
class PostFxRenderer;

namespace snd_occlusion_gpu
{

static constexpr int MAX_RING_BUFFERS = 6;
static constexpr int THREAD_GROUP_SIZE = 64;

struct SourceData
{
  Point3 pos = {0, 0, 0};
  float radius = 1.0f;
  OcclusionMode mode = MODE_SIMPLE;
  bool active = false;
  uint32_t generation = 0;
};

struct PairResult
{
  float values[4] = {0, 0, 0, 0};
  bool valid = false;
  int frameAge = 0;
  uint32_t generation = 0;
};

struct DispatchRecord
{
  dag::Vector<uint32_t> pairMapping;
  dag::Vector<uint32_t> pairGenerations;
  int activePairCount = 0;
};

class GpuSoundOcclusion
{
public:
  void init(const Config &cfg);
  void close();
  bool is_inited() const;

  void set_listener(const Point3 &pos);
  int create_source(const Point3 &pos, float radius, OcclusionMode mode);
  void delete_source(int idx);
  void set_source(int idx, const Point3 &pos, float radius, OcclusionMode mode);
  int get_num_active_sources() const;
  int get_max_active_sources() const;
  int get_active_pair_count() const;

  void dispatch();

  bool get_occlusion(int source_idx, float &out_occlusion) const;
  bool get_occlusion_direct_reverb(int source_idx, float &out_direct, float &out_reverb) const;

  void set_hardness_k(float k);
  void set_cone_half_angle(float deg);
  int get_readback_age() const;

  void debug_render_3d();
  void debug_render_imgui();
  void debug_set_enabled(bool enabled);
  bool debug_is_enabled() const;

  Config cfg;
  bool inited = false;

  eastl::unique_ptr<ComputeShaderElement> occlusionCs;
  UniqueBufHolder inputBuf;
  RingCPUBufferLock resultRingBuffer;

  Point3 listenerPos = {0, 0, 0};
  dag::Vector<SourceData> sources;
  dag::Vector<PairResult> results;
  dag::Vector<int> freeIndices;

  Point3 prevListenerPos = {0, 0, 0};
  dag::Vector<Point3> prevSourcePos;

  DispatchRecord dispatchRecords[MAX_RING_BUFFERS];

  float hardnessK = 8.0f;
  float coneHalfAngleDeg = 15.0f;
  uint32_t frameCounter = 0;
  int lastReadbackFrame = -1;
  bool debugEnabled = false;

private:
  void processReadback();
};

extern GpuSoundOcclusion gpu_sound_occlusion;

void world_sdf_debug_close();

} // namespace snd_occlusion_gpu
