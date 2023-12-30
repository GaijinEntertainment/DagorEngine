//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <util/dag_generationRefId.h>
#include <3d/dag_resId.h>

class D3dResource;
struct Frustum;
class Occlusion;
class BaseTexture;

//
// proper dafx call order:
//
// dafx::start_update
// ...
// dafx::wait_for_update
// dafx::update_culling_state0
// dafx::update_culling_state1
// dafx::update_culling_state2
// dafx::before_render
// dafx::partition_workers_if_ cull0 proxy_cull0
// dafx::partition_workers_if_ cull1 proxy_cull1
// dafx::render cull0
// dafx::render cull2
// dafx::render proxy_cull1
// dafx::render cull1
// dafx::render proxy_cull0

namespace dafx
{
struct SystemIdDummy
{};
struct CullingIdDummy
{};
struct ContextIdDummy
{};
struct InstanceIdDummy
{};
using SystemId = GenerationRefId<8, SystemIdDummy>;
using CullingId = GenerationRefId<8, CullingIdDummy>;
using ContextId = GenerationRefId<8, ContextIdDummy>;
using InstanceId = GenerationRefId<8, InstanceIdDummy>;

const uint32_t DEBUG_DISABLE_CULLING = 1 << 1;
const uint32_t DEBUG_DISABLE_SORTING = 1 << 2;
const uint32_t DEBUG_DISABLE_RENDER = 1 << 3;
const uint32_t DEBUG_DISABLE_SIMULATION = 1 << 4;
const uint32_t DEBUG_DISABLE_OCCLUSION = 1 << 5;
const uint32_t DEBUG_SHOW_BBOXES = 1 << 6;

struct CpuExecData
{
  void *globalValues;
  void *culling;
  void *buffer;

  const void *instances;
  int instancesCount;

  const void *dispatches;
  int dispatchesCount;
};
using cpu_exec_cb_t = void (*)(CpuExecData &);

enum class EmitterType : int
{
  UNDEFINED = 0,
  FIXED = 1,          // emmit X at once and keep them alive
  BURST = 2,          // emmit X every Y period, Z times
  LINEAR = 3,         // emmit with constant spawn rate (X parts / Y lifetime)
  DISTANCE_BASED = 4, // emmit 1 part for every X emitter traveled
};

struct EmitterData
{
  EmitterType type = EmitterType::UNDEFINED;

  struct FixedData
  {
    unsigned int count = 0; // total elems count
  };

  struct BurstData
  {
    unsigned int countMin = 0;  // min per burst
    unsigned int countMax = 0;  // min per burst
    unsigned int cycles = 0;    // max burst cycles (0 means no limit)
    float period = 0;           // burst period
    float lifeLimit = 0;        // negative = immortal
    unsigned int elemLimit = 0; // if elems reach this value, emission will be suspended. Can be 0, which means it calculate itself.
  };

  struct LinearData
  {
    unsigned int countMin = 0; // min elems count
    unsigned int countMax = 0; // max elems count
    float lifeLimit = 0;       // must be > 0
    bool instant = false;      // if true, first particle will be created immediately, without waiting for the spawn tick
  };

  struct DistanceBasedData
  {
    unsigned int elemLimit = 0;
    float distance = 0;
    float lifeLimit = 0;
    float idlePeriod = 0;
  };

  float delay = 0;              // delay for the first emission
  float globalLifeLimitMin = 0; // after this time spawning new parts will be cancelled. 0 for infinity
  float globalLifeLimitMax = 0; // per instance life limit (rnd min..max)

  float minEmissionFactor = 0.f;
  FixedData fixedData;
  BurstData burstData;
  LinearData linearData;
  DistanceBasedData distanceBasedData;
};

enum class EmissionType : int
{
  UNDEFINED = 0,
  NONE = 1,
  SHADER = 2,
  REF_DATA = 3,
};

struct EmissionData
{
  EmissionType type = EmissionType::UNDEFINED;

  eastl::string shader;
  eastl::vector<unsigned char> renderRefData;
  eastl::vector<unsigned char> simulationRefData;
};

enum class SimulationType : int
{
  UNDEFINED = 0,
  NONE = 1,
  SHADER = 2,
};

struct SimulationData
{
  SimulationType type = SimulationType::UNDEFINED;
  eastl::string shader;
};

struct RenderDesc
{
  eastl::string tag;
  eastl::string shader;
};

struct ValueBindDesc
{
  ValueBindDesc(const eastl::string &n, int o, int s) : name(n), offset(o), size(s) {}

  eastl::string name;
  int offset;
  int size;
};

enum class SortingType : int
{
  NONE = 0,
  BACK_TO_FRONT = 1,
  FRONT_TO_BACK = 2,
  BY_SHADER = 3,
};

struct CullingDesc
{
  CullingDesc(const eastl::string &t, SortingType s, int vrs_rate = 0) : tag(t), sortingType(s), vrsRate(vrs_rate) {}

  int vrsRate;
  eastl::string tag;
  SortingType sortingType;
};

struct SystemDesc
{
  EmitterData emitterData;
  EmissionData emissionData;
  SimulationData simulationData;
  eastl::vector<RenderDesc> renderDescs; // optional if subsystems are present

  unsigned int renderElemSize = 0;     // 1 element size in bytes
  unsigned int simulationElemSize = 0; // 1 element size in bytes
  unsigned int serviceDataSize = 0;    // optional shader accessible data at the end of the buffer chunk

  bool customDepth = false;
  uint32_t qualityFlags = 0xffffffff; // for exclusion for custom config params, i.e. - quality settings, see dafx::Config::qualityMask
  unsigned int renderPrimPerPart = 2;

  eastl::vector<unsigned char> serviceData;

  eastl::vector<TEXTUREID> texturesVs; // optional
  eastl::vector<TEXTUREID> texturesPs; // optional
  eastl::vector<TEXTUREID> texturesCs; // optional

  eastl::vector<ValueBindDesc> localValuesDesc; // optional
  eastl::vector<ValueBindDesc> reqGlobalValues; // optional

  int renderSortDepth = 0; // for manual composite sorting
  eastl::vector<SystemDesc> subsystems;
  int gameResId = -1;
};

struct Config;
struct Stats;

ContextId create_context(const Config &cfg);
void release_context(ContextId cid);
void before_reset_device(ContextId cid);
void after_reset_device(ContextId cid);

void register_cpu_override_shader(ContextId cid, const eastl::string &name, cpu_exec_cb_t cb);

SystemId register_system(ContextId cid, const SystemDesc &desc, const eastl::string &name);
void release_system(ContextId cid, SystemId sid);
void release_all_systems(ContextId cid);
bool get_system_name(ContextId cid, SystemId sid, eastl::string &out_name);
SystemId get_system_by_name(ContextId cid, const eastl::string &name);
int get_system_value_offset(ContextId cid, SystemId sid, eastl::string &name, int ref_size);
bool prefetch_system_textures(ContextId cid, SystemId sys_id);

InstanceId create_instance(ContextId cid, SystemId sid);
InstanceId create_instance(ContextId cid, const eastl::string &name);
void destroy_instance(ContextId cid, InstanceId &iid);
void reset_instance(ContextId cid, InstanceId iid);
void warmup_instance(ContextId cid, InstanceId iid, float time);
void clear_instances(ContextId cid, int keep_allocated_count); // pass keep_allocated_count = 0 for complete memory shrink

CullingId create_culling_state(ContextId cid, const eastl::vector<CullingDesc> &descs);
CullingId create_proxy_culling_state(ContextId cid);
void destroy_culling_state(ContextId cid, CullingId cull_id);
void update_culling_state(ContextId cid, CullingId cullid, const Frustum &frustum, const Point3 &view_pos,
  Occlusion *(*occlusion_sync_wait_f)() = nullptr);
void clear_culling_state(ContextId cid, CullingId cullid);
void remap_culling_state_tag(ContextId cid, CullingId cullid, const eastl::string &from, const eastl::string &to); // pass same values
                                                                                                                   // to reset remap
uint32_t inverse_remap_tags_mask(ContextId cid, uint32_t tags_mask);
int fetch_culling_state_visible_count(ContextId cid, CullingId cullid, const eastl::string &tag);
void partition_workers_if_outside_sphere(ContextId cid, CullingId cull_id_from, CullingId cull_id_to,
  const eastl::vector<eastl::string> &tags, Point4 sphere);
void partition_workers_if_inside_sphere(ContextId cid, CullingId cull_id_from, CullingId cull_id_to,
  const eastl::vector<eastl::string> &tags, Point4 sphere);

void start_update(ContextId cid, float dt, bool update_gpu = true, bool tp_wake_up = true);
void finish_update(ContextId cid, bool update_gpu_fetch = true, bool beforeRenderFrameFrameBoundary = false);
void finish_update_cpu_only(ContextId cid); // must be called before finish_update_gpu_only
void finish_update_gpu_only(ContextId cid, bool update_gpu_fetch,
  bool beforeRenderFrameFrameBoundary = false); // must be called after finish_update_cpu_only

bool update_finished(ContextId cid);
void flush_command_queue(ContextId cid);

void updateDafxFrameBounds(ContextId cid);

void before_render(ContextId cid, uint32_t tags_mask = 0xFFFFFFFF);
void before_render(ContextId cid, const eastl::vector<eastl::string> &tags_name);
bool render(ContextId ctx_id, CullingId cull_id, const eastl::string &tag);

void flush_global_values(ContextId cid);

void set_custom_depth(ContextId cid, BaseTexture *tex);
void set_global_value(ContextId cid, const char *name, size_t name_len, const void *data, int size);
inline void set_global_value(ContextId cid, eastl::string_view name, const void *data, int size)
{
  set_global_value(cid, name.data(), name.size(), data, size);
}
bool get_global_value(ContextId cid, const eastl::string &name, void *data, int size);

void set_instance_value(ContextId cid, InstanceId iid, const eastl::string &name, const void *data, int size);
void set_instance_value_opt(ContextId cid, InstanceId iid, const eastl::string &name, const void *data, int size);
void set_instance_value(ContextId cid, InstanceId iid, int offset, const void *data, int size);
void set_subinstance_value(ContextId cid, InstanceId iid, int sub_idx, int offset, const void *data, int size);
void set_instance_pos(ContextId cid, InstanceId iid, const Point4 &pos); // for sorting and distance based emitters
void set_instance_emission_rate(ContextId cid, InstanceId iid, float v);
void set_instance_status(ContextId cid, InstanceId iid, bool enabled);
bool get_instance_status(ContextId cid, InstanceId iid);
void set_instance_visibility(ContextId cid, InstanceId iid, bool visible);
bool is_instance_renderable_active(ContextId cid, InstanceId iid);
bool is_instance_renderable_visible(ContextId cid, InstanceId iid);
bool get_instance_value(ContextId cid, InstanceId iid, int offset, void *out_data, int size);
bool get_subinstances(ContextId cid, InstanceId iid, eastl::vector<InstanceId> &out);
eastl::string get_instance_info(ContextId cid, InstanceId iid);

void render_debug_opt(ContextId cid);
void set_debug_flags(ContextId cid, uint32_t flags);
const Config &get_config(ContextId cid);
bool set_config(ContextId cid, const Config &cfg);
uint32_t get_debug_flags(ContextId cid);
void clear_stats(ContextId cid);
void get_stats(ContextId cid, Stats &out_s);
void get_stats_as_string(ContextId cid, eastl::string &out_s);

unsigned int get_emitter_limit(const EmitterData &data, bool skip_error);

struct Config
{
  bool use_render_sbuffer = true;
  uint32_t qualityMask = 0xffffffff;

  bool vrs_enabled = false;
  bool use_async_thread = true;
  bool low_prio_jobs = false;
  unsigned int max_async_threads = 0; // if 0, threadpool::get_num_workers() will be used

  int render_buffer_gc_tail = 100;
  int render_buffer_gc_after = 5 * 1024 * 1024;
  int render_buffer_start_size = 512 * 1024;

  float render_buffer_shrink_ratio = 0.2f;
  float emission_factor = 1.f;
  float min_emission_factor = 0.2f;
  unsigned int emission_limit = 4096;
  unsigned int data_buffer_size = 524288;
  unsigned int staging_buffer_size = 65536;
  unsigned int warmup_sims_budged = 50; // parent/root instances
  unsigned int max_warmup_steps_per_instance = 10;

  unsigned int texture_start_slot = 10;

#if DAGOR_DBGLEVEL > 0
  eastl::vector<eastl::string> unsupportedShaders;
#endif

  static const int emission_max_limit = 65536;

  static const int max_render_tags = 16;
  static const int max_system_depth = 8;
  static const int max_culling_states = 32;
  static const int max_staging_frames = 5;

  static const int max_res_slots = 16;
  static const int max_culling_feedbacks = 10;
  static const unsigned int max_inactive_frames = 10; // amount of frames before it become 'inactive'

  static inline const char culling_discard_shader[] = "dafx_culling_discard";
  static inline const char dt_global_id[] = "dt";
};

struct Stats
{
  int updateEmitters;

  int cpuEmissionWorkers;
  int gpuEmissionWorkers;
  int cpuSimulationWorkers;
  int gpuSimulationWorkers;

  int cpuDispatchCalls;
  int gpuDispatchCalls;

  int totalRenderBuffersAllocSize;
  int totalRenderBuffersUsageSize;

  int cpuBufferAllocSize;
  int cpuBufferUsageSize;
  int cpuBufferGarbageSize;

  int gpuBufferAllocSize;
  int gpuBufferUsageSize;
  int gpuBufferGarbageSize;

  int stagingSize;

  int cpuBufferPageCount;
  int gpuBufferPageCount;

  int allRenderWorkers;
  int cpuRenderWorkers;

  int cpuElemProcessed;
  int gpuElemProcessed;
  int visibleTriangles;
  int renderedTriangles;

  int renderDrawCalls;
  int renderSwitchBuffers;
  int renderSwitchShaders;
  int renderSwitchTextures;
  int renderSwitchDispatches;

  int cpuCullElems;
  int gpuCullElems;

  int totalInstances;
  int activeInstances;

  int gpuTransferCount;
  int gpuTransferSize;

  struct Queue
  {
    int createInstance;
    int destroyInstance;
    int resetInstance;
    int setInstancePos;
    int setInstanceStatus;
    int setInstanceEmissionRate;
    int setInstanceValue;
    int setInstanceValueTotalSize;
  };
  Queue queue[2]; // 2 last frames
};
} // namespace dafx
