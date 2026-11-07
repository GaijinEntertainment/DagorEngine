// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <perfMon/dag_cpuFreq.h>
#include <fmod_studio.hpp>

#include <EASTL/vector.h>
#include <EASTL/sort.h>

#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <soundSystem/vars.h>
#include <soundSystem/varId.h>
#include <soundSystem/strHash.h>

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

using blob_template_id_t = str_hash_t;
static constexpr blob_template_id_t undefined_blob_template_id_value = SND_HASH("");
static blob_template_id_t resolve_blob_template_id(const char *name)
{
  return name != nullptr ? SND_HASH_SLOW(name) : undefined_blob_template_id_value;
}

// Serialises every backend call, so a backend needs no lock of its own. Taken after the blob lock, never before it;
// update() drops the blob lock across the backend update, so blob-only calls never wait for a measurement pass.
static WinCritSec g_occlusion_backend_cs;
#define SNDSYS_OCCLUSION_BACKEND_BLOCK WinAutoLock occlusionBackendLock(g_occlusion_backend_cs);

// Set by set_backend() or by init() to the world SDF one; close() clears it, so a restart registers again.
static Backend g_backend;
static constexpr int g_sdf_max_sources = 512;

static Backend make_sdf_backend()
{
  Backend b;
  b.isInited = &snd_occlusion_gpu::is_inited;
  b.close = &snd_occlusion_gpu::close;
  b.setListener = [](const Point3 &pos, const Point3 &) { snd_occlusion_gpu::set_listener(pos); };
  // wrapped: create_source and set_source have a default mode argument
  b.createSource = [](const Point3 &pos, float radius) { return snd_occlusion_gpu::create_source(pos, radius); };
  b.setSource = [](int idx, const Point3 &pos, float radius) { snd_occlusion_gpu::set_source(idx, pos, radius); };
  b.deleteSource = &snd_occlusion_gpu::delete_source;
  b.getOcclusion = &snd_occlusion_gpu::get_occlusion;
  b.mainThreadUpdate = &snd_occlusion_gpu::dispatch;
  b.numActiveSources = &snd_occlusion_gpu::get_num_active_sources;
  b.maxActiveSources = &snd_occlusion_gpu::get_max_active_sources;
  return b; // an SDF knows no detours, so the layer applies the hint
}

#if DAGOR_DBGLEVEL > 0
// The debug draw traces a line a blob under the locks the sound thread's update waits on, so each blob keeps what it traced
// for a while, and only a few are traced a frame.
static constexpr int g_debug_materials_period_ms = 150;
static constexpr int g_debug_materials_budget = 6;
static constexpr uint8_t g_debug_empty_blob_alpha = 0x50; // a blob with no sound instance is drawn see-through

struct DebugMaterialsCache
{
  int32_t tracedAt = 0;
  bool traced = false;
  bool valid = false; // the backend answered
  DebugMaterials materials;
};
#endif

struct Blob
{
  struct Template
  {
    float attachRadius = 0.f;
    float occlusionRadius = 0.f;
    blob_template_id_t id = undefined_blob_template_id_value;
    Template() = default;
    Template(float attach_radius, float occlusion_radius, blob_template_id_t id) :
      attachRadius(attach_radius), occlusionRadius(occlusion_radius), id(id)
    {}
  };

  uint64_t detourHintSequence = 0;
  uint64_t detourHintCountedSequence = 0;

  const Template tmpl;

  Point3 pos = {};
  Point3 externalFactor = {};
  float externalFactorAt = -100.f;

  float gpuOcclusion = 0.f;
  float finalOcclusion = 0.f;
  float finalOcclusionBands[OCCLUSION_BANDS] = {};
  float activeRadius = 0.f;
  float detourHint = 0.f;
  float detourHintAge = -1.f;
  uint32_t detourHintDropped = 0; // debug draw: calls refused for a sequence below the counted one

  int gpuIdx = -1;

  float tryDeleteAt = -1;
  bool detourHintFed = false; // debug draw: told apart from a caller that never feeds this blob
  bool occlusionValid = false;
#if DAGOR_DBGLEVEL > 0
  DebugMaterialsCache materialsTrace; // debug draw: the last trace of the centre line
#endif

  Blob(const Point3 &pos, Template tmpl) : tmpl(tmpl), pos(pos) {}

  ~Blob() { gpuDeactivate(); }

  Blob(const Blob &) = delete;
  Blob(Blob &&) = delete;
  Blob &operator=(const Blob &) = delete;
  Blob &operator=(Blob &&) = delete;

  void gpuActivate()
  {
    if (gpuIdx == -1)
    {
      gpuIdx = g_backend.createSource(pos, tmpl.occlusionRadius);
      if (gpuIdx < 0)
        LOGERR_ONCE("[SNDSYS] the occlusion backend is full, a blob plays unoccluded");
    }
  }

  void gpuDeactivate()
  {
    if (gpuIdx != -1)
      g_backend.deleteSource(gpuIdx);
    gpuIdx = -1;
    // the caller resends the hint each update, and a new backend source counts its sequence from 0
    detourHint = 0.f;
    detourHintSequence = 0;
    detourHintAge = -1.f;
#if DAGOR_DBGLEVEL > 0
    materialsTrace = DebugMaterialsCache(); // the line of the source it held is no line of a new one
#endif
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
  VarId varId = {};
  VarId bandVarIds[OCCLUSION_BANDS] = {};
  Point3 pos = {};
  sound_handle_t blobHandle = INVALID_SOUND_HANDLE;
  blob_template_id_t blobTemplateId = undefined_blob_template_id_value;
  float occlusion = 0.f;
  float occlusionBands[OCCLUSION_BANDS] = {};
  float resumeAt = -1.f;

  bool bands = false; // the per-band parameters are written, varId is zero; else varId is
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
static eastl::vector<Blob::Template> g_blob_templates;
static bool g_sort_instances = false;
static BlobsPoolType g_blobs;
static int g_total_blobs = 0;
// delete_blob parks a slot here so it never waits for a backend update; update() and close() give it back.
static eastl::vector<int> g_released_slots;
static Listener g_listener;
static external_factor_t g_external_factor = nullptr;
static detour_hint_gain_t g_detour_hint_gain = nullptr;

static dag::AtomicInteger<bool> g_inited = false;
static BackendSuspends g_backend_suspends;

static const float g_def_active_radius = 150.f;
static bool g_detour_hint_enabled = true;
static constexpr float g_detour_hint_occluded_threshold = 0.5f;
static bool has_detour_hint(const Blob &blob) { return g_detour_hint_enabled && blob.detourHint > 0.f; }

// a backend that takes the hint applies it itself, so the layer must not apply it again
static float deepen_by_detour_hint(const Blob &blob, float occlusion)
{
  if (!g_backend.setSourceDetourHint && g_detour_hint_gain && has_detour_hint(blob) && occlusion > g_detour_hint_occluded_threshold)
    return 1.f - (1.f - occlusion) * g_detour_hint_gain(blob.detourHint);
  return occlusion;
}

static constexpr float g_start_dist_attenuation_mul = 0.75f;
static constexpr int g_max_blobs = 2048;

static constexpr float g_def_attach_radius = 0.5f;
static constexpr float g_def_occlusion_radius = 0.2f;
static constexpr float g_def_hardness_k = 3.f;

static const Blob::Template *get_blob_template(blob_template_id_t template_id)
{
  for (const Blob::Template &blobTemplate : g_blob_templates)
    if (blobTemplate.id == template_id)
      return &blobTemplate;
  return nullptr;
}

static Blob::Template resolve_blob_template(blob_template_id_t template_id)
{
  if (const Blob::Template *blobTemplate = get_blob_template(template_id))
    return *blobTemplate;
  return Blob::Template(g_def_attach_radius, g_def_occlusion_radius, template_id);
}

static Blob *get_blob(sound_handle_t blob_handle) { return g_blobs.get(blob_handle); }

static bool is_inside_blob(const Point3 &instance_pos, const Blob &blob)
{
  return lengthSq(instance_pos - blob.pos) < blob.tmpl.attachRadius * blob.tmpl.attachRadius;
}

static bool is_inside_blob(const Point3 &instance_pos, sound_handle_t blob_handle)
{
  const Blob *blob = get_blob(blob_handle);
  return blob && is_inside_blob(instance_pos, *blob);
}

static eastl::pair<sound_handle_t, Blob *> find_blob_for_instance(const Instance &inst)
{
  sound_handle_t bestHandle = INVALID_SOUND_HANDLE;
  Blob *bestBlob = nullptr;
  float bestDistSq = 0.f;
  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    if (blob.tmpl.attachRadius <= 0.f || blob.tmpl.id != inst.blobTemplateId)
      return;
    const float distSq = lengthSq(inst.pos - blob.pos);
    if (distSq < blob.tmpl.attachRadius * blob.tmpl.attachRadius)
      if (bestHandle == INVALID_SOUND_HANDLE || distSq < bestDistSq)
      {
        bestHandle = handle;
        bestBlob = &blob;
        bestDistSq = distSq;
      }
  });
  return eastl::make_pair(bestHandle, bestBlob);
}

static eastl::pair<sound_handle_t, Blob *> create_blob_impl(const Point3 &pos, Blob::Template tmpl)
{
  if (g_total_blobs >= g_max_blobs)
  {
    logerr("[SNDSYS] max count of occlusion blobs exceeded (%d)", g_total_blobs);
    return {};
  }

  sound_handle_t handle = INVALID_SOUND_HANDLE;
  Blob *blob = g_blobs.emplace(handle, pos, tmpl);
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
  SNDSYS_OCCLUSION_GPU_BLOCK; // no backend lock: update() claims the slot on activation
  if (!g_inited.load())
    return {};
  return OcclusionBlobHandle(
    create_blob_impl(pos, Blob::Template(attach_radius, occlusion_radius, undefined_blob_template_id_value)).first);
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

static void give_back_released_slots()
{
  for (int idx : g_released_slots)
    g_backend.deleteSource(idx);
  g_released_slots.clear();
}

void delete_blob(OcclusionBlobHandle &blob_handle)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  if (Blob *blob = get_blob((sound_handle_t)blob_handle); blob && blob->gpuActive())
  {
    g_released_slots.push_back(blob->gpuIdx);
    blob->gpuIdx = -1;
  }
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

void set_blob_detour_hint(OcclusionBlobHandle blob_handle, float delta_m, bool valid, uint64_t measurement_sequence,
  float measurement_age_seconds)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  if (Blob *blob = get_blob((sound_handle_t)blob_handle))
  {
    blob->detourHintFed = true;
    if (measurement_sequence && measurement_sequence < blob->detourHintCountedSequence)
    {
      ++blob->detourHintDropped;
      return;
    }
    blob->detourHint = valid && delta_m > 0.f ? delta_m : 0.f;
    blob->detourHintSequence = measurement_sequence;
    blob->detourHintCountedSequence = max(blob->detourHintCountedSequence, measurement_sequence);
    // the caller resends the age each update, so the layer does not age it
    blob->detourHintAge = measurement_age_seconds;
  }
}

int count_band_params(const fmod_description_t &fmod_description, FMOD_STUDIO_PARAMETER_DESCRIPTION *out_descs)
{
  int count = 0;
  for (int b = 0; b < OCCLUSION_BANDS; ++b)
  {
    FMOD_STUDIO_PARAMETER_DESCRIPTION desc = {};
    if (FMOD_OK == fmod_description.getParameterDescriptionByName(OCCLUSION_BAND_PARAMS[b], &desc) && as_var_id(desc.id))
    {
      if (out_descs)
        out_descs[b] = desc;
      ++count;
    }
  }
  return count;
}

void append_new_instance(fmod_instance_t *fmod_instance, const fmod_description_t *fmod_description, const Point3 &pos)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;
  G_ASSERT_RETURN(fmod_instance != nullptr && fmod_description != nullptr, );

  auto pred = [fmod_instance](const Instance &inst) { return inst.fmodInstance == fmod_instance; };
  G_ASSERT_RETURN(eastl::find_if(g_instances.begin(), g_instances.end(), pred) == g_instances.end(), );

  // a backend with bands gets them written to an event that has them, else "occlusion"; one with none, the SDF, gets "occlusion"
  // alone, and the bands only for an event that has nothing else
  FMOD_STUDIO_PARAMETER_DESCRIPTION bandDescs[OCCLUSION_BANDS] = {};
  const bool hasBands = count_band_params(*fmod_description, bandDescs) == OCCLUSION_BANDS;

  FMOD_STUDIO_PARAMETER_DESCRIPTION desc = {};
  const VarId scalarVarId =
    FMOD_OK == fmod_description->getParameterDescriptionByName("occlusion", &desc) ? as_var_id(desc.id) : VarId{};
  if (!scalarVarId && !hasBands)
  {
    logerr("[SNDSYS] missing var labeled '%s' in event '%s'", "occlusion", get_debug_name(*fmod_description).c_str());
    return;
  }
  const bool useBands = hasBands && (g_backend.getOcclusionBands || !scalarVarId);
  const VarId varId = useBands ? VarId{} : scalarVarId;

  Instance &inst = g_instances.push_back();
  inst.fmodInstance = fmod_instance;
  inst.varId = varId;
  inst.bands = useBands;
  for (int b = 0; b < OCCLUSION_BANDS; ++b)
    inst.bandVarIds[b] = as_var_id(bandDescs[b].id);
  inst.pos = pos;
  inst.detachWhenOutside = true;
  inst.trySuspendOnAttachToBlob = !g_backend.immediateStart;
  if (g_backend.immediateStart)
  {
    const Blob *warm = find_blob_for_instance(inst).second;
    const bool warmValid = warm && warm->occlusionValid;
    if (varId)
    {
      inst.occlusion = warmValid ? warm->finalOcclusion : desc.defaultvalue;
      SOUND_VERIFY(fmod_instance->setParameterByID(desc.id, inst.occlusion, true));
    }
    if (useBands)
      for (int b = 0; b < OCCLUSION_BANDS; ++b)
      {
        inst.occlusionBands[b] = warmValid ? warm->finalOcclusionBands[b] : bandDescs[b].defaultvalue;
        SOUND_VERIFY(fmod_instance->setParameterByID(bandDescs[b].id, inst.occlusionBands[b], true));
      }
  }
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

// may call this any number of times, only the first creation matters
void create_blob_template(float attach_radius, float occlusion_radius, const char *name)
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  const blob_template_id_t id = resolve_blob_template_id(name);
  if (id == undefined_blob_template_id_value)
  {
    logerr("[SNDSYS] invalid name passed to create_blob_template: '%s'", name ? name : "");
    return;
  }

  if (const Blob::Template *blobTemplate = get_blob_template(id))
  {
    if (blobTemplate->attachRadius != attach_radius || blobTemplate->occlusionRadius != occlusion_radius)
    {
      logerr("[SNDSYS] attempt to (re)create blob template '%s' over existing but with different parameters", name ? name : "");
    }
    return;
  }

  g_blob_templates.emplace_back(attach_radius, occlusion_radius, id);
}

// expected to apply this right after Instance creation but before the first update
void set_preferred_blob_template(EventHandle event_handle, const char *name)
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
  {
    logerr("[SNDSYS] attempt to assign specific blob template '%s' to Instance but it already has a blob", name ? name : "");
    return;
  }

  const blob_template_id_t id = resolve_blob_template_id(name);
  if (get_blob_template(id) == nullptr)
  {
    logerr("[SNDSYS] attempt to set preferred blob template '%s' but no such template found", name ? name : "");
    return;
  }

  inst->blobTemplateId = id;
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

static bool is_inside_active_radius(const Blob &blob)
{
  return lengthSq(blob.pos - g_listener.pos) < blob.activeRadius * blob.activeRadius;
}

static void update_blob(Blob &blob)
{
  blob.gpuActivate(is_inside_active_radius(blob));

  if (!blob.gpuActive())
    return;
  g_backend.setSource(blob.gpuIdx, blob.pos, blob.tmpl.occlusionRadius);
  if (g_backend.setSourceDetourHint)
    g_backend.setSourceDetourHint(blob.gpuIdx, blob.detourHint, has_detour_hint(blob), blob.detourHintSequence, blob.detourHintAge);
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

// what is written for a backend's occlusion, scalar or one band of it: the falloff by distance, the external factor, the rounding
static float finalize_occlusion(const Blob &blob, float occlusion)
{
  float result = 0.f;
  if (blob.occlusionValid)
  {
    const float dist = length(blob.pos - g_listener.pos);
    result = cvt(dist, blob.activeRadius * g_start_dist_attenuation_mul, blob.activeRadius, occlusion, 0.f);
  }
  if (g_external_factor)
    result = lerp(result, blob.externalFactor.y, blob.externalFactor.x);
  if (!g_backend.continuousParameters)
    result = floor(result * 10.f + 0.5f) * 0.1f;
  return result;
}

void update(float cur_time, float dt, const TMatrix &listener_tm)
{
  // named, not the macro: it is released before the backend update
  WinAutoLock occlusionGpuLock(g_occlusion_gpu_cs);
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  if (!g_inited.load())
    return;

  give_back_released_slots();
  g_listener.pos = listener_tm.getcol(3);

  g_backend.setListener(g_listener.pos, listener_tm.getcol(2));

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
      auto handleAndBlob = find_blob_for_instance(inst);
      inst.blobHandle = handleAndBlob.first;
      if (inst.blobHandle == INVALID_SOUND_HANDLE)
      {
        // create new blob
        handleAndBlob = create_blob_impl(inst.pos, resolve_blob_template(inst.blobTemplateId));
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

    float gpuOcclusionBands[OCCLUSION_BANDS] = {};
    bool backendBands = false;
    if (blob.gpuActive())
    {
      blob.occlusionValid = g_backend.getOcclusion(blob.gpuIdx, blob.gpuOcclusion);
      backendBands = blob.occlusionValid && g_backend.getOcclusionBands && g_backend.getOcclusionBands(blob.gpuIdx, gpuOcclusionBands);
    }
    else
    {
      blob.gpuOcclusion = 0.f;
      blob.occlusionValid = false;
    }

    if (g_external_factor && cur_time >= blob.externalFactorAt)
    { // about game-specific cases e.g when is_underground(listener) != is_underground(source) need to mute sounds(set param to "2")
      const float distSq = lengthSq(blob.pos - g_listener.pos);
      g_external_factor(g_listener.pos, blob.pos, distSq, blob.gpuOcclusion, blob.occlusionValid, blob.externalFactor);
      blob.externalFactorAt = cur_time + blob.externalFactor.z;
    }

    blob.finalOcclusion = finalize_occlusion(blob, deepen_by_detour_hint(blob, blob.gpuOcclusion));
    // a backend with no bands repeats the scalar, hint and all, in every band
    for (int b = 0; b < OCCLUSION_BANDS; ++b)
      blob.finalOcclusionBands[b] =
        backendBands ? finalize_occlusion(blob, deepen_by_detour_hint(blob, gpuOcclusionBands[b])) : blob.finalOcclusion;
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
      // FMOD may hold the authored default or a formatted var, so a first valid value is written even if equal
      const bool first = !inst.occlusionValid && blob->occlusionValid;
      inst.occlusionValid = blob->occlusionValid;
      const bool mayWrite = !g_backend.immediateStart || blob->occlusionValid || !blob->gpuActive();
      if (inst.varId && (first || (inst.occlusion != blob->finalOcclusion && mayWrite)))
      {
        inst.occlusion = blob->finalOcclusion;
        inst.fmodInstance->setParameterByID(as_fmod_param_id(inst.varId), inst.occlusion, first && g_backend.immediateStart);
      }
      if (inst.bands)
        for (int b = 0; b < OCCLUSION_BANDS; ++b)
          if (first || (mayWrite && inst.occlusionBands[b] != blob->finalOcclusionBands[b]))
          {
            inst.occlusionBands[b] = blob->finalOcclusionBands[b];
            inst.fmodInstance->setParameterByID(as_fmod_param_id(inst.bandVarIds[b]), inst.occlusionBands[b],
              first && g_backend.immediateStart);
          }

      if (inst.suspended && (blob->occlusionValid || !blob->gpuActive() || cur_time >= inst.resumeAt))
        resume_instance(inst);
    }

  const bool switchBackend = g_backend_suspends.takeChange();
  const bool backendEnabled = g_backend_suspends.enabled;

  // a whole measurement pass: main thread blob calls must not wait for it
  occlusionGpuLock.unlockFinal();
  if (switchBackend && g_backend.setEnabled)
    g_backend.setEnabled(backendEnabled); // off waits for jobs; blob lock released
  // last, so it reads the positions this update set
  if (g_backend.update)
    g_backend.update(dt);
}

void gpu_update()
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  if (!g_inited.load())
    return;

  if (g_backend.mainThreadUpdate)
    g_backend.mainThreadUpdate();
}

void set_backend(const Backend &backend)
{
  G_ASSERT(is_main_thread());
  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  G_ASSERT_RETURN(!g_inited.load(), ); // blobs hold slot indices of the current backend
  // a null isInited withdraws; the others are called unchecked
  G_ASSERT_RETURN(!backend.isInited || (backend.close && backend.setListener && backend.createSource && backend.setSource &&
                                         backend.deleteSource && backend.getOcclusion), );
  g_backend = backend;
}

void suspend_backend(bool suspend)
{
  G_ASSERT(is_main_thread());
  SNDSYS_OCCLUSION_GPU_BLOCK;
  g_backend_suspends.suspend(suspend);
}

void level_loaded()
{
  G_ASSERT(is_main_thread());
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  if (g_inited.load() && g_backend.levelLoaded)
    g_backend.levelLoaded();
}

void level_unloaded()
{
  G_ASSERT(is_main_thread());
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  if (g_inited.load() && g_backend.levelUnloaded)
    g_backend.levelUnloaded();
}

void init(const DataBlock &blk)
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK;

  G_ASSERT_RETURN(!g_inited.load(), );

  g_detour_hint_enabled = blk.getBool("occlusionDetourHint", true);

  // no host backend: the layer owns the world SDF one
  if (!g_backend.isInited)
  {
    g_backend = make_sdf_backend();
    snd_occlusion_gpu::Config sndOccCfg;
    sndOccCfg.maxSources = g_sdf_max_sources;
    snd_occlusion_gpu::init(sndOccCfg);
    snd_occlusion_gpu::set_hardness_k(g_def_hardness_k); // only stores a constant, safe before the check
  }

  if (!g_backend.isInited())
  {
    logerr("[SNDSYS] sound occlusion backend was not properly initialized");
    g_backend = Backend(); // close() is not reached, and the next init retries
    return;
  }

  g_inited.store(true);
}

void close()
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK; // blobs free their slots before the backend closes
  if (!g_inited.load())
    return;

#if DAGOR_DBGLEVEL > 0
  g_blobs.enumerate(
    [&](Blob & /*blob*/, sound_handle_t handle) { debug_trace_warn("Blob handle %lld was not deleted", int64_t(handle)); });
#endif
  g_blobs.close();
  give_back_released_slots();

  g_backend.close();
  g_backend = Backend();
  g_backend_suspends = BackendSuspends();

  g_total_blobs = 0;
  g_inited.store(false);
}

bool is_inited() { return g_inited.load(); }

bool has_backend()
{
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
  return g_backend.isInited != nullptr;
}

void set_external_factor(external_factor_t external_factor)
{
  G_ASSERT(is_main_thread());

  SNDSYS_OCCLUSION_GPU_BLOCK;
  if (!g_inited.load())
    return;

  g_external_factor = external_factor;
}

void set_detour_hint_gain(detour_hint_gain_t gain)
{
  G_ASSERT(is_main_thread());
  SNDSYS_OCCLUSION_GPU_BLOCK;
  g_detour_hint_gain = gain;
}

#if DAGOR_DBGLEVEL > 0
static E3DCOLOR occlusion_to_color(float occ) { return E3DCOLOR(0xff, (1.f - saturate(occ)) * 0xff, 0, 0xff); }

#include "./occlusionGPU_debug.inl"

struct HintColors
{
  E3DCOLOR frame; // the plate's border
  E3DCOLOR ring;  // around the blob's sphere; no alpha: none
};

// the hint stops before the backend, or the backend cannot say what it did with it
static const HintColors g_hint_neutral = {E3DCOLOR(0, 0, 0, 160), E3DCOLOR(0, 0, 0, 0)};
static const HintColors g_hint_used = {E3DCOLOR(110, 60, 210, 210), E3DCOLOR(180, 140, 255, 255)};
// delivered, the backend demoted it, and the published answer still rests on it
static const HintColors g_hint_held = {E3DCOLOR(50, 80, 170, 210), E3DCOLOR(140, 170, 255, 255)};
// delivered, the answer does not rest on it
static const HintColors g_hint_unused = {E3DCOLOR(140, 100, 0, 210), E3DCOLOR(255, 200, 60, 255)};
// the backend has no source for the blob, or does not hold a hint delivered to it
static const HintColors g_hint_bad = {E3DCOLOR(180, 20, 20, 230), E3DCOLOR(255, 60, 60, 255)};

// How far a blob's detour hint got, so the draw says where it stops: the caller, the layer or the backend. Returns its colors.
static HintColors debug_hint_lines(const Blob &blob, FrameStr &str)
{
  if (!blob.gpuActive() && is_inside_active_radius(blob) && g_backend.numActiveSources && g_backend.maxActiveSources &&
      g_backend.numActiveSources() == g_backend.maxActiveSources())
  {
    str.append_sprintf("\nNO BACKEND SOURCE: the pool is full (%d), the blob plays unoccluded", g_backend.maxActiveSources());
    return g_hint_bad;
  }
  if (!blob.detourHintFed)
  {
    str.append("\nhint: never fed, nothing sets it for this blob");
    return g_hint_neutral;
  }
  if (blob.detourHintDropped)
    str.append_sprintf("\nhint: %u calls dropped, sequence below the counted one", blob.detourHintDropped);
  if (!blob.gpuActive())
  {
    str.append(is_inside_active_radius(blob) ? "\nhint: blob not activated yet" : "\nhint: blob outside the active range");
    return g_hint_neutral;
  }
  if (blob.detourHint <= 0.f)
  {
    str.append("\nhint: cleared by the caller: no row, an expired table, valid false or delta <= 0");
    return g_hint_neutral;
  }

  str.append_sprintf("\nhint: %.1fm seq %llu", blob.detourHint, (unsigned long long)blob.detourHintSequence);
  if (blob.detourHintAge >= 0.f)
    str.append_sprintf(" age %.2fs", blob.detourHintAge);
  if (!g_detour_hint_enabled)
  {
    str.append("\nOFF: occlusionDetourHint is no");
    return g_hint_neutral;
  }

  if (!g_backend.setSourceDetourHint)
  {
    if (!g_detour_hint_gain)
    {
      str.append("\nlayer: skipped, no detour hint gain is set");
      return g_hint_unused;
    }
    if (blob.gpuOcclusion > g_detour_hint_occluded_threshold)
    {
      str.append_sprintf("\nlayer: APPLIED, gain %.2f", g_detour_hint_gain(blob.detourHint));
      return g_hint_used;
    }
    str.append_sprintf("\nlayer: skipped, occlusion %.2f is not above %.1f", blob.gpuOcclusion, g_detour_hint_occluded_threshold);
    return g_hint_unused;
  }

  DebugHintState lib;
  if (!g_backend.debugDetourHint || !g_backend.debugDetourHint(blob.gpuIdx, lib))
  {
    str.append("\nlib: no state to read");
    return g_hint_neutral;
  }
  if (lib.demoted)
  {
    // a demoted hint that is still valid holds the answer it measured, which the route says
    if (lib.valid && lib.route == HintRoute::Hint)
    {
      str.append_sprintf("\nlib: HELD, demoted, the answer still rests on it, detour %.1fm", lib.delta);
      return g_hint_held;
    }
    str.append("\nlib: DEMOTED, the pose or the geometry moved on, or the library was off");
    return g_hint_unused;
  }
  if (!lib.valid)
  {
    str.append("\nlib: NOT HELD, the backend got the call and refused or dropped it, see seq and age");
    return g_hint_bad;
  }
  switch (lib.route)
  {
    case HintRoute::Hint:
      str.append_sprintf("\nlib: USED, detour %.1fm%s", lib.delta, lib.saturated ? " (saturated)" : "");
      return g_hint_used;
    case HintRoute::Visible: str.append_sprintf("\nlib: ignored, source visible (%.2f)", lib.visibility); return g_hint_unused;
    case HintRoute::Graph:
      str.append_sprintf("\nlib: graph route won, detour %.1fm%s", lib.delta, lib.saturated ? " (hint saturated)" : "");
      return g_hint_unused;
    case HintRoute::None: break;
  }
  str.append("\nlib: held, no answer has read it yet");
  return g_hint_unused;
}

static void debug_materials_lines(const DebugMaterials &m, FrameStr &str)
{
  if (!m.measured)
    str.append("materials: the line is unmeasured");
  else if (m.count == 0)
    str.append("materials: clear line");
  else
    str.append("materials on the line:");
  for (int i = 0; i < m.count; ++i)
  {
    const DebugMaterialSpan &s = m.spans[i];
    if (s.thin)
      str.append_sprintf("\n%s at %.1fm: thin %.0f dB", s.material, s.start, s.lossDb[1]);
    else
      str.append_sprintf("\n%s at %.1fm: %.2fm, %.0f/%.0f/%.0f dB", s.material, s.start, s.thickness, s.lossDb[0], s.lossDb[1],
        s.lossDb[2]);
  }
  if (m.more)
    str.append_sprintf("\n+%d more", m.more);
  if (m.count)
    str.append_sprintf("\ntotal, capped by maxLoss: %.0f/%.0f/%.0f dB", m.totalDb[0], m.totalDb[1], m.totalDb[2]);
  if (m.passed)
    str.append_sprintf("\npasses %s x%d at no loss", m.passedMaterial, m.passed);
}

static void debug_materials_append(Blob &blob, int &traces_left, FrameStr &str)
{
  if (!g_backend.debugMaterials || !blob.gpuActive() || !is_inside_active_radius(blob))
    return;
  DebugMaterialsCache &cache = blob.materialsTrace;
  const int32_t now = get_time_msec();
  if (traces_left > 0 && (!cache.traced || now - cache.tracedAt >= g_debug_materials_period_ms))
  {
    --traces_left;
    cache.traced = true;
    cache.tracedAt = now;
    cache.materials = DebugMaterials();
    cache.valid = g_backend.debugMaterials(blob.gpuIdx, cache.materials);
  }
  if (!cache.valid)
    return;
  str.append("\n");
  debug_materials_lines(cache.materials, str);
}

void debug_render_3d()
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK;

  if (!g_inited.load())
    return;

  set_cached_debug_lines_wtm(TMatrix::IDENT);
  begin_draw_cached_debug_lines(false, false, false);

  int materialTraces = g_debug_materials_budget;
  g_blobs.enumerate([&](Blob &blob, sound_handle_t handle) {
    const Point3 &spos = blob.pos;
    const float occlusionRadius = max(0.1f, blob.tmpl.occlusionRadius);
    E3DCOLOR color = blob.occlusionValid ? occlusion_to_color(blob.finalOcclusion) : E3DCOLOR(0xff0000ff);

    FrameStr str;
    const int numValidInstances = get_num_valid_instances(handle);
    if (!numValidInstances)
      color.a = g_debug_empty_blob_alpha;
    str.sprintf("{%.2f} instances %d", blob.finalOcclusion, numValidInstances);
    if (blob.occlusionValid)
      str.append_sprintf(" backend %.2f", blob.gpuOcclusion);
    if (blob.occlusionValid && g_backend.getOcclusionBands)
      str.append_sprintf(" bands %.2f/%.2f/%.2f", blob.finalOcclusionBands[0], blob.finalOcclusionBands[1],
        blob.finalOcclusionBands[2]);
    const HintColors hintColors = debug_hint_lines(blob, str);
    debug_materials_append(blob, materialTraces, str);
    add_debug_text_mark(spos, str.c_str(), -1, 2.5f, hintColors.frame);

    if (lengthSq(spos - g_listener.pos) > occlusionRadius * occlusionRadius)
    {
      TMatrix tm;
      tm.setcol(0, occlusionRadius, 0, 0);
      tm.setcol(1, 0, occlusionRadius, 0);
      tm.setcol(2, 0, 0, occlusionRadius);
      tm.setcol(3, spos.x, spos.y, spos.z);
      set_cached_debug_lines_wtm(tm);
      draw_cached_debug_trilist((const Point3 *)g_sphere_faces, g_num_sphere_faces, color);
      set_cached_debug_lines_wtm(TMatrix::IDENT);
    }

    draw_cached_debug_sphere(spos, occlusionRadius, 0xff000000);

    if (hintColors.ring.a)
      draw_cached_debug_sphere(spos, occlusionRadius * 1.3f + 0.05f, hintColors.ring);
  });
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  end_draw_cached_debug_lines();

  // under the backend lock, so the backend needs no sync of its own
  if (g_backend.debugRender3d)
    g_backend.debugRender3d();
}

void debug_render_imgui()
{
  SNDSYS_OCCLUSION_BACKEND_BLOCK;

  if (g_inited.load() && g_backend.debugRenderImgui)
    g_backend.debugRenderImgui();
}
#else
void debug_render_3d() {}
void debug_render_imgui() {}
#endif

DebugState get_debug_state()
{
  SNDSYS_OCCLUSION_GPU_BLOCK;
  SNDSYS_OCCLUSION_BACKEND_BLOCK;
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
  state.numActiveGPUSources = g_backend.numActiveSources ? g_backend.numActiveSources() : 0;
  state.maxActiveGPUSources = g_backend.maxActiveSources ? g_backend.maxActiveSources() : 0;

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
