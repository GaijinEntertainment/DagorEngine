//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/handle.h>

class Point3;

namespace sndsys::occlusion_gpu
{
OcclusionBlobHandle create_blob(const Point3 &pos, float attach_radius, float occlusion_radius);
void delete_blob(OcclusionBlobHandle &blob_handle);
void set_pos(OcclusionBlobHandle blob_handle, const Point3 &pos);
void set_blob_active_range(OcclusionBlobHandle blob_handle, float active_radius);
void attach_to_blob(EventHandle event_handle, OcclusionBlobHandle blob_handle);
void create_blob_template(float attach_radius, float occlusion_radius, const char *name);
void set_preferred_blob_template(EventHandle event_handle, const char *name);
void drop_instance(EventHandle event_handle);
bool is_inside_active_range(const Point3 &pos);
void debug_render_3d();
void debug_render_imgui(); // Backend::debugRenderImgui, under the backend lock

typedef void (*external_factor_t)(const Point3 &listener, const Point3 &blob_pos, float distance_sq, float occlusion,
  bool occlusion_valid, Point3 &factor);
void set_external_factor(external_factor_t external_factor);

// The source of a blob's occlusion; the layer keeps the blobs, the falloff, the external factor and the FMOD write.
// Members from setEnabled down are optional. The layer serialises all calls: a backend needs no lock, calls no layer API.
struct Backend
{
  bool (*isInited)() = nullptr; // null withdraws; at the end of init, false drops the registration and close is not called
  void (*close)() = nullptr;
  void (*setListener)(const Point3 &pos, const Point3 &forward) = nullptr;
  int (*createSource)(const Point3 &pos, float radius) = nullptr; // the slot, -1 when full
  void (*setSource)(int idx, const Point3 &pos, float radius) = nullptr;
  void (*deleteSource)(int idx) = nullptr;
  // out_occlusion: 0 open .. 1 occluded; false: no value yet, so the layer writes only the external factor unless immediateStart
  bool (*getOcclusion)(int idx, float &out_occlusion) = nullptr;
  // before update, once per suspend_backend change; a backend starts enabled and gets no first call
  void (*setEnabled)(bool on) = nullptr;
  void (*update)(float dt) = nullptr;   // sound thread, once per update
  void (*mainThreadUpdate)() = nullptr; // main thread, late in the frame
  void (*levelLoaded)() = nullptr;      // main thread; the backend takes the world it traces
  void (*levelUnloaded)() = nullptr;
  int (*numActiveSources)() = nullptr;
  int (*maxActiveSources)() = nullptr;
  void (*debugRender3d)() = nullptr;    // main thread, from debug_render_3d, dev builds only
  void (*debugRenderImgui)() = nullptr; // the same, from debug_render_imgui
  // a voice starts unpaused, on a nearby blob's value or else its authored default; false: paused up to 1 s until its blob has a value
  bool immediateStart = false;
  // written unrounded; else rounded to 0.1, so jitter does not rewrite the FMOD parameter every update
  bool continuousParameters = false;
};
// Call before init, which then starts the layer whatever occlusionInit and occlusionGPUInit say; close() of an inited layer drops it.
void set_backend(const Backend &backend);

// Counted: each suspend needs its own resume.
void suspend_backend(bool suspend);

// main thread, once the level's collision is up and before it goes; serialised with the sound thread update by the backend lock
void level_loaded();
void level_unloaded();
} // namespace sndsys::occlusion_gpu
