// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <fmod_studio.hpp>

#include <EASTL/vector.h>
#include <EASTL/sort.h>

#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <soundSystem/vars.h>
#include <soundSystem/varId.h>

#include <soundOcclusionGPU/soundOcclusionGPU.h>

#include "internal/fmodCompatibility.h"
#include "internal/occlusionGPU_internal.h"
#include "internal/events_internal.h"
#include "internal/pool.h"
#include "internal/debug_internal.h"
#include "internal/framememString.h"

#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

namespace sndsys::occlusion_gpu
{
static WinCritSec g_occlusion_gpu_cs;
#define SNDSYS_OCCLUSION_GPU_BLOCK WinAutoLock occlusionGpuLock(g_occlusion_gpu_cs);

struct Blob
{
  Point3 pos = {};
  Point3 externalFactor = {};
  float externalFactorAt = -100.f;
  float attachRadius = 0.f;
  float occlusionRadius = 0.f;
  float gpuOcclusion = 0.f;
  float finalOcclusion = 0.f;
  float activeRadius = 0.f;

  int gpuIdx = -1;

  bool occlusionValid = false;
  float tryDeleteAt = -1;

  Blob(const Point3 &pos, float attach_radius, float occlusion_radius) :
    pos(pos), attachRadius(attach_radius), occlusionRadius(occlusion_radius)
  {}

  ~Blob() { gpuDeactivate(); }

  Blob(const Blob &) = delete;
  Blob(Blob &&) = delete;
  Blob &operator=(const Blob &) = delete;
  Blob &operator=(Blob &&) = delete;

  void gpuActivate()
  {
    if (gpuIdx == -1)
      gpuIdx = snd_occlusion_gpu::create_source(pos, occlusionRadius);
  }

  void gpuDeactivate()
  {
    if (gpuIdx != -1)
      snd_occlusion_gpu::delete_source(gpuIdx);
    gpuIdx = -1;
  }

  bool gpuActive() const { return gpuIdx != -1; }

  void gpuActivate(bool active)
  {
    if (active != gpuActive())
    {
      if (active)
        gpuActivate();
      else
        gpuDeactivate();
    }
  }
};

struct Instance
{
  fmod_instance_t *fmodInstance = nullptr;
  Point3 pos = {};
  VarId varId = {};
  sound_handle_t blobHandle = INVALID_SOUND_HANDLE;
  float occlusion = 0.f;
  float resumeAt = -1.f;

  bool moveBlob = false;
  bool detachWhenOutside = false;
  bool dropOnOcclusionValid = false;
  bool trySuspendOnAttachToBlob = false;
  bool suspended = false;
  bool occlusionValid = false;
};

struct Listener
{
  Point3 pos = {};
};

using BlobsPoolType = Pool<Blob, sound_handle_t, 512, POOLTYPE_OCCLUSION_BLOB, POOLTYPE_TYPES_COUNT>;
G_STATIC_ASSERT(INVALID_SOUND_HANDLE == BlobsPoolType::invalid_value);

static eastl::vector<Instance> g_instances;
static bool g_sort_instances = false;
static BlobsPoolType g_blobs;
static int g_total_blobs = 0;
static Listener g_listener;
static external_factor_t g_external_factor = nullptr;

static dag::AtomicInteger<bool> g_inited = false;

static const float g_def_active_radius = 150.f;
static constexpr float g_start_dist_attenuation_mul = 0.75f;
static constexpr int g_max_active_blobs = 512;
static constexpr int g_max_blobs = 2048;

static constexpr float g_def_attach_radius = 0.5f;
static constexpr float g_def_occlusion_radius = 0.13f;
static constexpr float g_def_hardness_k = 3.f;

static Blob *get_blob(sound_handle_t blob_handle) { return g_blobs.get(blob_handle); }

static bool is_inside_blob(const Point3 &instance_pos, const Blob &blob)
{
  return lengthSq(instance_pos - blob.pos) < blob.attachRadius * blob.attachRadius;
}

static bool is_inside_blob(const Point3 &instance_pos, sound_handle_t blob_handle)
{
  const Blob *blob = get_blob(blob_handle);
  return blob && is_inside_blob(instance_pos, *blob);
}

static eastl::pair<sound_handle_t, Blob *> find_blob_for_instance(const Point3 &pos)
{
  sound_handle_t bestHandle = INVALID_SOUND_HANDLE;
  Blob *bestBlob = nullptr;
  float bestDistSq = 0.f;
  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    if (blob.attachRadius <= 0.f)
      return;
    const float distSq = lengthSq(pos - blob.pos);
    if (distSq < blob.attachRadius * blob.attachRadius)
      if (bestHandle == INVALID_SOUND_HANDLE || distSq < bestDistSq)
      {
        bestHandle = handle;
        bestBlob = &blob;
        bestDistSq = distSq;
      }
  });
  return eastl::make_pair(bestHandle, bestBlob);
}

static eastl::pair<sound_handle_t, Blob *> create_blob_impl(const Point3 &pos, float attach_radius, float occlusion_radius)
{
  if (g_total_blobs >= g_max_blobs)
  {
    logerr("[SNDSYS]: max count of occlusion blobs exceeded (%d)", g_total_blobs);
    return {};
  }

  sound_handle_t handle = INVALID_SOUND_HANDLE;
  Blob *blob = g_blobs.emplace(handle, pos, attach_radius, occlusion_radius);
  if (blob)
  {
    blob->activeRadius = g_def_active_radius;
    ++g_total_blobs;
  }

  G_ASSERT(blob != nullptr && handle != INVALID_SOUND_HANDLE);
  return eastl::make_pair(handle, blob);
}

OcclusionBlobHandle create_blob(const Point3 &pos, float attach_radius, float occlusion_radius)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return {};
  return OcclusionBlobHandle(create_blob_impl(pos, attach_radius, occlusion_radius).first);
}

static void delete_blob_impl(sound_handle_t blob_handle)
{
  if (Blob *blob = get_blob(blob_handle))
  {
    g_blobs.reject(blob_handle);
    --g_total_blobs;
    G_ASSERT(g_total_blobs >= 0);
  }
}

void delete_blob(OcclusionBlobHandle &blob_handle)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  delete_blob_impl((sound_handle_t)blob_handle);
  blob_handle.reset();
}

void set_pos(OcclusionBlobHandle blob_handle, const Point3 &pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  if (Blob *blob = get_blob((sound_handle_t)blob_handle))
    blob->pos = pos;
}

void set_blob_active_range(OcclusionBlobHandle blob_handle, float active_radius)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  if (Blob *blob = get_blob((sound_handle_t)blob_handle))
    blob->activeRadius = active_radius;
}

void append_new_instance(fmod_instance_t *fmod_instance, const fmod_description_t *fmod_description, const Point3 &pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  G_ASSERT_RETURN(fmod_instance != nullptr && fmod_description != nullptr, );

  auto pred = [fmod_instance](const Instance &inst) { return inst.fmodInstance == fmod_instance; };
  G_ASSERT_RETURN(eastl::find_if(g_instances.begin(), g_instances.end(), pred) == g_instances.end(), );

  FMOD_STUDIO_PARAMETER_DESCRIPTION desc;
  SOUND_VERIFY(fmod_description->getParameterDescriptionByName("occlusion", &desc));
  const VarId varId = as_var_id(desc.id);
  if (!varId)
  {
    logerr("[SNDSYS] missing var labeled '%s' in event '%s'", "occlusion", get_debug_name(*fmod_description).c_str());
    return;
  }

  Instance &inst = g_instances.push_back();
  inst.fmodInstance = fmod_instance;
  inst.varId = varId;
  inst.pos = pos;
  inst.detachWhenOutside = true;
  inst.trySuspendOnAttachToBlob = true;
}

void attach_to_blob(EventHandle event_handle, OcclusionBlobHandle blob_handle)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  const fmod_instance_t *fmodInstance = fmodapi::get_instance(event_handle);
  if (!fmodInstance)
    return;

  auto pred = [fmodInstance](const Instance &inst) { return inst.fmodInstance == fmodInstance; };
  auto inst = eastl::find_if(g_instances.begin(), g_instances.end(), pred);

  if (inst == g_instances.end())
    return;

  if (inst->blobHandle != INVALID_SOUND_HANDLE)
    return;

  if (!get_blob((sound_handle_t)blob_handle))
    return;

  inst->blobHandle = (sound_handle_t)blob_handle;

  inst->detachWhenOutside = false;
  inst->moveBlob = false;

  g_sort_instances = true;
}

void drop_instance(EventHandle event_handle)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  if (const fmod_instance_t *fmodInstance = fmodapi::get_instance(event_handle))
  {
    auto pred = [fmodInstance](const Instance &inst) { return inst.fmodInstance == fmodInstance; };
    auto inst = eastl::find_if(g_instances.begin(), g_instances.end(), pred);

    if (inst == g_instances.end())
      return;

    inst->dropOnOcclusionValid = true;
  }
}

void set_pos(const fmod_instance_t *fmod_instance, const Point3 &pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  auto pred = [fmod_instance](const Instance &inst) { return inst.fmodInstance == fmod_instance; };
  auto it = eastl::find_if(g_instances.begin(), g_instances.end(), pred);

  if (it != g_instances.end())
    it->pos = pos;
}

void on_release(fmod_instance_t *fmod_instance)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  auto pred = [fmod_instance](const Instance &inst) { return inst.fmodInstance == fmod_instance; };
  auto it = eastl::find_if(g_instances.begin(), g_instances.end(), pred);
  if (it != g_instances.end())
    g_instances.erase(it);
}

bool is_inside_active_range(const Point3 &pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  return lengthSq(pos - g_listener.pos) < g_def_active_radius * g_def_active_radius;
}

static int get_num_valid_instances(sound_handle_t blob_handle)
{
  int num = 0;
  for (const Instance &inst : g_instances)
    if (inst.blobHandle == blob_handle && inst.fmodInstance->isValid())
      ++num;
  return num;
}

static void update_blob(Blob &blob)
{
  blob.gpuActivate(lengthSq(blob.pos - g_listener.pos) < blob.activeRadius * blob.activeRadius);

  if (blob.gpuActive())
    snd_occlusion_gpu::set_source(blob.gpuIdx, blob.pos, blob.occlusionRadius);
}

static bool delete_or_update_blob(Blob &blob, sound_handle_t blob_handle, float cur_time) // -> true to delete
{
  if (blob.tryDeleteAt >= 0.f && cur_time >= blob.tryDeleteAt)
  {
    blob.tryDeleteAt = cur_time + 1.f;
    if (get_num_valid_instances(blob_handle) == 0)
      return true;
  }
  update_blob(blob);
  return false;
}

static bool should_drop_instance(const Instance &inst)
{
  return (inst.dropOnOcclusionValid && inst.occlusionValid) || !inst.fmodInstance->isValid();
}

static void resume_instance(Instance &inst)
{
  if (inst.suspended)
  {
    if (inst.fmodInstance->isValid())
      SOUND_VERIFY(inst.fmodInstance->setPaused(false));
    inst.suspended = false;
  }
}

bool is_suspended(const fmod_instance_t *fmod_instance)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return false;

  auto pred = [fmod_instance](const Instance &inst) { return inst.fmodInstance == fmod_instance; };
  auto it = eastl::find_if(g_instances.begin(), g_instances.end(), pred);
  if (it != g_instances.end())
    return it->suspended;
  return false;
}

void update(float cur_time, const Point3 &listener_pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  g_listener.pos = listener_pos;

  snd_occlusion_gpu::set_listener(g_listener.pos);

  // update instances

  for (intptr_t i = intptr_t(g_instances.size()) - 1; i >= 0; --i)
  {
    Instance &inst = g_instances[i];
    if (should_drop_instance(inst))
    {
      g_instances.erase(g_instances.begin() + i);
      continue;
    }

    if (inst.blobHandle != INVALID_SOUND_HANDLE)
    {
      if (inst.moveBlob)
      {
        if (Blob *blob = get_blob(inst.blobHandle))
          blob->pos = inst.pos;
      }
      else if (inst.detachWhenOutside && !is_inside_blob(inst.pos, inst.blobHandle))
        inst.blobHandle = INVALID_SOUND_HANDLE;
    }

    if (inst.blobHandle == INVALID_SOUND_HANDLE)
    {
      // find existing blob
      auto handleAndBlob = find_blob_for_instance(inst.pos);
      inst.blobHandle = handleAndBlob.first;
      if (inst.blobHandle == INVALID_SOUND_HANDLE)
      {
        // create new blob
        handleAndBlob = create_blob_impl(inst.pos, g_def_attach_radius, g_def_occlusion_radius);
        inst.blobHandle = handleAndBlob.first;
        inst.moveBlob = true;
        inst.detachWhenOutside = false;
        if (Blob *blob = handleAndBlob.second)
        {
          blob->tryDeleteAt = cur_time + 2.f;
          update_blob(*blob);
        }
        else
        {
          g_instances.erase(g_instances.begin() + i); // let instance play without any occlusion
          continue;
        }
      }

      g_sort_instances = true;
    }

    if (inst.trySuspendOnAttachToBlob)
    {
      inst.trySuspendOnAttachToBlob = false;
      const Blob *blob = get_blob(inst.blobHandle);
      if (blob && blob->gpuActive() && !blob->occlusionValid)
        if (!inst.suspended && inst.fmodInstance->isValid())
        {
          SOUND_VERIFY(inst.fmodInstance->setPaused(true));
          inst.suspended = true;
          inst.resumeAt = cur_time + 1.f;
        }
    }
  }

  // update blobs

  eastl::vector<sound_handle_t, framemem_allocator> list;
  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    if (delete_or_update_blob(blob, handle, cur_time))
    { // should delete
      list.push_back(handle);
      return;
    }

    if (blob.gpuActive())
      blob.occlusionValid = snd_occlusion_gpu::get_occlusion(blob.gpuIdx, blob.gpuOcclusion);
    else
    {
      blob.gpuOcclusion = 0.f;
      blob.occlusionValid = false;
    }

    blob.finalOcclusion = 0.f;
    if (blob.occlusionValid)
    {
      const float dist = length(blob.pos - g_listener.pos);
      blob.finalOcclusion = cvt(dist, blob.activeRadius * g_start_dist_attenuation_mul, blob.activeRadius, blob.gpuOcclusion, 0.f);
    }

    if (g_external_factor)
    {
      if (cur_time >= blob.externalFactorAt)
      { // about game-specific cases e.g when is_underground(listener) != is_underground(source) need to mute sounds(set param to "2")
        const float distSq = lengthSq(blob.pos - g_listener.pos);
        g_external_factor(g_listener.pos, blob.pos, distSq, blob.gpuOcclusion, blob.occlusionValid, blob.externalFactor);
        blob.externalFactorAt = cur_time + blob.externalFactor.z;
      }
      blob.finalOcclusion = lerp(blob.finalOcclusion, blob.externalFactor.y, blob.externalFactor.x);
    }
    blob.finalOcclusion = floor(blob.finalOcclusion * 10.f + 0.5f) * 0.1f;
  });

  // update instances

  for (sound_handle_t handle : list)
    delete_blob_impl(handle);
  list.clear();

  if (g_sort_instances)
  {
    // make friendly cache lookup for get_blob(inst.blobHandle)
    g_sort_instances = false;
    eastl::sort(g_instances.begin(), g_instances.end(), [](const Instance &a, const Instance &b) {
      return g_blobs.get_node_index(a.blobHandle) < g_blobs.get_node_index(b.blobHandle);
    });
  }

  for (Instance &inst : g_instances)
    if (const Blob *blob = get_blob(inst.blobHandle))
    {
      if (inst.occlusion != blob->finalOcclusion)
      {
        inst.occlusion = blob->finalOcclusion;
        inst.occlusionValid = blob->occlusionValid;
        inst.fmodInstance->setParameterByID(as_fmod_param_id(inst.varId), inst.occlusion);
      }

      if (inst.suspended && (blob->occlusionValid || !blob->gpuActive() || cur_time >= inst.resumeAt))
        resume_instance(inst);
    }
}

void gpu_update()
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  snd_occlusion_gpu::dispatch();
}

void init(const DataBlock & /*blk*/)
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;

  G_ASSERT_RETURN(!g_inited.load(), );

  // Initialize GPU sound occlusion
  {
    snd_occlusion_gpu::Config sndOccCfg;
    sndOccCfg.maxSources = g_max_active_blobs;
    snd_occlusion_gpu::init(sndOccCfg);
  }

  if (!snd_occlusion_gpu::is_inited())
  {
    logerr("[SNDSYS] soundOcclusionGPU was not properly initialized");
    return;
  }

  snd_occlusion_gpu::set_hardness_k(g_def_hardness_k);

  g_inited.store(true);
}

void close()
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

#if DAGOR_DBGLEVEL > 0
  g_blobs.enumerate(
    [&](Blob & /*blob*/, sound_handle_t handle) { debug_trace_warn("Blob handle %lld was not deleted", int64_t(handle)); });
#endif
  g_blobs.close();

  snd_occlusion_gpu::close();

  g_total_blobs = 0;
  g_inited.store(false);
}

bool is_inited() { return g_inited.load(); }

void set_external_factor(external_factor_t external_factor)
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  g_external_factor = external_factor;
}

#if DAGOR_DBGLEVEL > 0
static E3DCOLOR occlusion_to_color(float occ) { return E3DCOLOR(0xff, (1.f - saturate(occ)) * 0xff, 0, 0xff); }

#include "./occlusionGPU_debug.inl"
void debug_render_3d()
{
  SNDSYS_OCCLUSION_GPU_BLOCK;

  if (!g_inited.load())
    return;

  set_cached_debug_lines_wtm(TMatrix::IDENT);
  begin_draw_cached_debug_lines(false, false, false);

  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    const Point3 &spos = blob.pos;
    const float occlusionRadius = max(0.1f, blob.occlusionRadius);
    const E3DCOLOR color = blob.occlusionValid ? occlusion_to_color(blob.finalOcclusion) : E3DCOLOR(0xff0000ff);

    FrameStr str;
    const int numValidInstances = get_num_valid_instances(handle);
    if (numValidInstances)
      str.sprintf("n=[%d] o={%.1f}", numValidInstances, blob.finalOcclusion);
    else
      str.sprintf("{%.1f}", blob.finalOcclusion);
    add_debug_text_mark(spos, str.c_str(), -1, 0.5f);

    TMatrix tm;
    tm.setcol(0, occlusionRadius, 0, 0);
    tm.setcol(1, 0, occlusionRadius, 0);
    tm.setcol(2, 0, 0, occlusionRadius);
    tm.setcol(3, spos.x, spos.y, spos.z);
    set_cached_debug_lines_wtm(tm);
    draw_cached_debug_trilist((const Point3 *)g_sphere_faces, g_num_sphere_faces, color);
    set_cached_debug_lines_wtm(TMatrix::IDENT);

    draw_cached_debug_sphere(spos, blob.attachRadius, 0x800000ff);
    draw_cached_debug_sphere(spos, occlusionRadius, 0xff000000);
  });
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  end_draw_cached_debug_lines();
  // snd_occlusion_gpu::debug_render_3d();
}
#else
void debug_render_3d() {}
#endif

DebugState get_debug_state()
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return {};

  DebugState state;

  g_blobs.enumerate([&](const Blob &blob, sound_handle_t /*handle*/) {
    ++state.numUsedBlobs;
    if (blob.gpuActive())
      ++state.numActiveBlobs;
    if (blob.tryDeleteAt >= 0.f)
      ++state.numAutoDeleteBlobs;
  });

  state.numFreeBlobs = g_blobs.get_free();
  state.numInstances = g_instances.size();
  state.numActiveGPUSources = snd_occlusion_gpu::get_num_active_sources();
  state.maxActiveGPUSources = snd_occlusion_gpu::get_max_active_sources();

  return state;
}

void debug_enum_blobs(debug_enum_blobs_t debug_enum_blobs)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    debug_enum_blobs(blob.pos, get_num_valid_instances(handle), blob.finalOcclusion, blob.tryDeleteAt >= 0.f);
  });
}

} // namespace sndsys::occlusion_gpu
