// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include "binds.h"
#include "buffers.h"
#include "shaders.h"
#include "emitters.h"
#include <EASTL/vector.h>
#include <EASTL/bonus/tuple_vector.h>

namespace dafx
{
struct InstanceState
{
  int aliveStart;
  int aliveCount;
  int aliveLimit;
};

struct EmissionState
{
  int start;
  int count;
};

struct SimulationState
{
  int start;
  int count;
};

struct DataTransfer
{
  DataTransfer() : srcOffset(0xffffffff), dstOffset(0xffffffff), size(0) {}
  DataTransfer(unsigned int s, unsigned int d, unsigned int sz) : srcOffset(s), dstOffset(d), size(sz) {}

  unsigned int srcOffset;
  unsigned int dstOffset;
  unsigned int size;
};

constexpr int queue_instance_sid = 0xffffffff;
constexpr int dummy_instance_sid = 0xfffffff0;

enum : int
{
  INST_RID,
  INST_FLAGS,
  INST_REF_FLAGS,
  INST_SYNCED_FLAGS,
  INST_DEPTH,
  INST_LOD,

  INST_PARENT_SID,
  INST_SUBINSTANCES,

  INST_VALUE_BIND_ID,

  INST_GPU_BUFFER,
  INST_CPU_BUFFER,

  INST_GPU_PARENT_OFFSET,
  INST_CPU_PARENT_OFFSET,

  INST_RENDER_ELEM_STRIDE,
  INST_SIMULATION_ELEM_STRIDE,

  INST_PARENT_RENDER_ELEM_STRIDE,
  INST_PARENT_SIMULATION_ELEM_STRIDE,

  INST_RENDER_DATA_SIZE,
  INST_SIMULATION_DATA_SIZE,
  INST_SERVICE_DATA_SIZE,

  INST_PRIM_PER_PART,

  INST_SERVICE_REF_DATA_ID,

  INST_RENDER_REF_DATA_SIZE,
  INST_SIMULATION_REF_DATA_SIZE,
  INST_SERVICE_REF_DATA_SIZE,

  INST_PARENT_RENDER_DATA_SIZE,
  INST_PARENT_SIMULATION_DATA_SIZE,

  INST_HEAD_DATA_TRANSFER,
  INST_RENDER_DATA_TRANSFER,
  INST_SIMULATION_DATA_TRANSFER,
  INST_SERVICE_DATA_TRANSFER,

  INST_TARGET_RENDER_BUF,
  INST_TARGET_RENDER_DATA_OFFSET,
  INST_TARGET_RENDER_PARENT_OFFSET,

  INST_EMISSION_GPU_SHADER,
  INST_EMISSION_CPU_SHADER,

  INST_SIMULATION_GPU_SHADER,
  INST_SIMULATION_CPU_SHADER,

  INST_ACTIVE_STATE,
  INST_EMITTER_STATE,
  INST_EMISSION_STATE,
  INST_SIMULATION_STATE,

  INST_RENDER_TAGS,
  INST_RENDER_SHADERS,

  INST_LOCAL_RES_CS,
  INST_LOCAL_RES_VS,
  INST_LOCAL_RES_PS,

  INST_BBOX,
  INST_POSITION,
  INST_VIEW_DIST,
  INST_RENDER_SORT_DEPTH,
  INST_CULLING_ID,
  INST_LAST_VALID_BBOX_FRAME,

#if DAFX_STAT
  INST_RENDERABLE_TRIS,
#else
  INST_RENDERABLE_TRIS = -1,
#endif

#if DAGOR_DBGLEVEL > 0
  INST_GAMERES_ID,
#endif
};

using InstanceStream = eastl::tuple_vector<InstanceId, // INST_RID
  uint32_t,                                            // INST_FLAGS
  uint32_t,                                            // INST_REF_FLAGS
  uint32_t,                                            // INST_SYNCED_FLAGS
  int,                                                 // INST_DEPTH
  uint8_t,                                             // INST_LOD

  int,                // INST_PARENT_SID
  eastl::vector<int>, // INST_SUBINSTANCES

  ValueBindId, // INST_VALUE_BIND_ID

  GpuBuffer, // INST_GPU_BUFFER
  CpuBuffer, // INST_CPU_BUFFER

  int, // INST_GPU_PARENT_OFFSET
  int, // INST_CPU_PARENT_OFFSET

  int, // INST_RENDER_ELEM_STRIDE
  int, // INST_SIMULATION_ELEM_STRIDE

  int, // INST_PARENT_RENDER_ELEM_STRIDE
  int, // INST_PARENT_SIMULATION_ELEM_STRIDE

  int, // INST_RENDER_DATA_SIZE
  int, // INST_SIMULATION_DATA_SIZE
  int, // INST_SERVICE_DATA_SIZE

  int, // INST_PRIM_PER_PART

  RefDataId, // INST_SERVICE_REF_DATA_ID

  int, // INST_RENDER_REF_DATA_SIZE,
  int, // INST_SIMULATION_REF_DATA_SIZE,
  int, // INST_SERVICE_REF_DATA_SIZE,

  int, // INST_PARENT_RENDER_DATA_SIZE
  int, // INST_PARENT_SIMULATION_DATA_SIZE

  DataTransfer, // INST_HEAD_DATA_TRANSFER
  DataTransfer, // INST_RENDER_DATA_TRANSFER
  DataTransfer, // INST_SIMULATION_DATA_TRANSFER
  DataTransfer, // INST_SERVICE_DATA_TRANSFER

  D3DRESID, // INST_TARGET_RENDER_BUF
  int,      // INST_TARGET_RENDER_DATA_OFFSET
  int,      // INST_TARGET_RENDER_PARENT_OFFSET

  GpuComputeShaderId, // INST_EMISSION_GPU_SHADER
  CpuComputeShaderId, // INST_EMISSION_CPU_SHADER

  GpuComputeShaderId, // INST_SIMULATION_GPU_SHADER
  CpuComputeShaderId, // INST_SIMULATION_CPU_SHADER

  InstanceState,   // INST_ACTIVE_STATE
  EmitterState,    // INST_EMITTER_STATE
  EmissionState,   // INST_EMISSION_STATE
  SimulationState, // INST_SIMULATION_STATE

  uint32_t,                      // INST_RENDER_TAGS
  eastl::vector<RenderShaderId>, // INST_RENDER_SHADERS

  eastl::vector<TextureDesc>, // INST_LOCAL_RES_CS
  eastl::vector<TextureDesc>, // INST_LOCAL_RES_VS
  eastl::vector<TextureDesc>, // INST_LOCAL_RES_PS

  bbox3f,      // INST_BBOX (this should be fine, since eastl::tuple_vector have alignment-friendly allocator)
  Point4,      // INST_POSITION
  float,       // INST_VIEW_DIST
  int,         // INST_RENDER_SORT_DEPTH
  int,         // INST_CULLING_ID
  unsigned int // INST_LAST_VALID_BBOX_FRAME

#if DAFX_STAT
  ,
  unsigned int // INST_RENDERABLE_TRIS
#endif

#if DAGOR_DBGLEVEL > 0
  ,
  int // INST_GAMERES_ID
#endif
  >;

struct InstanceGroups
{
  static constexpr int groupSize = 256;

  void push_back()
  {
    if (groups.empty() || groups.back().size() == groupSize)
    {
      groups.push_back();
      groups.back().reserve(groupSize);
    }

    groups.back().push_back();
  }

  template <typename T>
  const T &cget(int idx)
  {
    G_FAST_ASSERT(idx / groupSize < groups.size()); // comment me for comparing performance with pre-tuple slicing
                                                    // (tuple_vector doesnt have OOB checks)
    return groups.data()[idx / groupSize].get<T>()[idx % groupSize];
  }

  template <typename T>
  const T &get(int idx) const
  {
    return cget<T>(idx);
  }

  template <typename T>
  T &get(int idx)
  {
    return const_cast<T &>(cget<T>(idx));
  }

  template <int I>
  const typename eastl::tuple_element<I, InstanceStream::value_tuple>::type &cget(int idx) const
  {
    G_FAST_ASSERT(idx / groupSize < groups.size()); // comment me for comparing performance with pre-tuple slicing
                                                    // (tuple_vector doesnt have OOB checks)
    return groups.data()[idx / groupSize].get<I>()[idx % groupSize];
  }

  template <int I>
  const typename eastl::tuple_element<I, InstanceStream::value_tuple>::type &get(int idx) const
  {
    return cget<I>(idx);
  }

  template <int I>
  typename eastl::tuple_element<I, InstanceStream::value_tuple>::type &get(int idx)
  {
    return const_cast<typename eastl::tuple_element<I, InstanceStream::value_tuple>::type &>(cget<I>(idx));
  }

  template <int I, typename T>
  constexpr eastl::enable_if_t<(I < 0), T *> getOpt(int, T *def = nullptr)
  {
    return def;
  }

  template <int I, typename T>
  eastl::enable_if_t<(I >= 0), T *> getOpt(int idx, T * = nullptr)
  {
    return &get<I>(idx);
  }

  void shrink_to_size(int c)
  {
    // shrink whole groups, and not individual instances, we dont care about exact precision
    int gc = c / groupSize;
    if (groups.size() <= gc)
      return;

    groups.resize(gc);
    groups.shrink_to_fit();
  }


  int size() { return groups.size() > 0 ? (groups.size() - 1) * groupSize + groups.back().size() : 0; }

  dag::Vector<InstanceStream> groups;
};

struct Instances
{
  InstanceGroups groups;
  GenerationReferencedData<InstanceId, int> list;
  eastl::vector<int> freeIndicesPending;
  eastl::vector<int> freeIndicesAvailable;
};

void reset_instances_after_reset(Context &ctx);

void create_instances_from_queue(Context &ctx);
void destroy_instance_from_queue(Context &ctx);
void reset_instance_from_queue(Context &ctx);
void set_instance_pos_from_queue(Context &ctx);
void set_instance_status_from_queue(Context &ctx);
void set_instance_visibility_from_queue(Context &ctx);
void set_instance_emission_rate_from_queue(Context &ctx);
void set_instance_value_from_queue(Context &ctx);
} // namespace dafx
DAG_DECLARE_RELOCATABLE(dafx::InstanceStream);
