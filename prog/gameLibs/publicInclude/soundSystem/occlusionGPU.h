//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/handle.h>

class Point3;

namespace sndsys::occlusion_gpu
{
// An event may carry these three parameters, band1, band2 and band3 for 250, 1000 and 4000 Hz, besides "occlusion" or in place of
// it. Each is read like "occlusion", the falloff by distance and the external factor included: the backend's 0 open .. 1 occluded,
// and up to the external factor's target when that is active, 2 for the mute value the banks use, so author the parameter's range
// to reach it. The layer writes the value unclamped. Under a backend that has getOcclusionBands an event that has all three gets
// them and not "occlusion"; under one that has not, the world SDF one, it gets "occlusion" and not the bands, and only an event
// with no "occlusion" gets the bands, each the scalar. An event with none of them gets "occlusion"; one with only some of them
// is an authoring error, and the layer takes it for an event with no occlusion unless it has "occlusion" too. The bands are read only
// after getOcclusion returns true, and getOcclusion stays required because its validity decides the final value in every band.
static constexpr int OCCLUSION_BANDS = 3;
inline constexpr const char *OCCLUSION_BAND_PARAMS[OCCLUSION_BANDS] = {"band1", "band2", "band3"};

OcclusionBlobHandle create_blob(const Point3 &pos, float attach_radius, float occlusion_radius);
void delete_blob(OcclusionBlobHandle &blob_handle);
void set_pos(OcclusionBlobHandle blob_handle, const Point3 &pos);
void set_blob_active_range(OcclusionBlobHandle blob_handle, float active_radius);
// delta_m: how much longer the path around the geometry is than the straight line. With occlusionDetourHint on, the layer
// deepens only an occlusion above 0.5, by the gain of set_detour_hint_gain(), and not at all while none is set;
// a backend that takes it applies it itself. A measurement_sequence below the highest counted one drops the call, 0 skips that test.
// measurement_age_seconds: its age now, -1 unknown; only a backend that takes the hint reads it, so a caller clears an expired one.
// valid false or delta_m <= 0 clears the hint but counts its sequence; leaving the active range clears it too: resend each update.
void set_blob_detour_hint(OcclusionBlobHandle blob_handle, float delta_m, bool valid, uint64_t measurement_sequence,
  float measurement_age_seconds);
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

// the layer scales the open part (1 - occlusion) by gain(delta_m), in the scalar and in each band; null, the default, does not deepen.
// Kept across init and close.
typedef float (*detour_hint_gain_t)(float delta_m);
void set_detour_hint_gain(detour_hint_gain_t gain);

// Which provider the backend's published detour came from.
enum class HintRoute : uint8_t
{
  Visible, // the source is open enough that no detour is read
  Graph,
  Hint,
  None // no provider has answered yet
};

// What a backend holds of one source's detour hint, for the debug draw.
struct DebugHintState
{
  bool valid = false;     // the backend holds a hint for the source
  bool demoted = false;   // the backend stopped trusting it: the pose or the geometry moved on, or the backend was switched off
  bool saturated = false; // an open-ended bucket, never wins over a graph route
  HintRoute route = HintRoute::None;
  float delta = 0.f;      // metres of detour the published answer was read at
  float visibility = 1.f; // the source's clear-ray share, 0..1, 1 open, as snd_propagation::Result::visibility defines it
};

// One stretch of the centre line from the listener to a source that a backend charges, for the debug draw.
struct DebugMaterialSpan
{
  const char *material = "";         // named by the span's own index; valid until the material table loads again
  float start = 0.f;                 // metres from the listener
  float thickness = 0.f;             // metres; 0 for a sheet or a crossing with no pair
  float lossDb[3] = {0.f, 0.f, 0.f}; // 250, 1000, 4000 Hz
  bool thin = false;                 // charged its thin dB, no depth
};

// The materials on that line in distance order, the line of the source's last transmission measurement, from the poses it was taken
// at. The occlusion is read from a bundle of rays, so this is its middle, not all of it.
struct DebugMaterials
{
  static constexpr int MAX_SPANS = 6;
  DebugMaterialSpan spans[MAX_SPANS];
  int count = 0;                      // spans written
  int more = 0;                       // spans charged past MAX_SPANS
  float totalDb[3] = {0.f, 0.f, 0.f}; // the ray's loss after the backend's cap, so a long span can read above it
  int passed = 0;                     // crossings of a material that lets the sound through, charged nothing
  const char *passedMaterial = ""; // the nearest of them, named by its effective material; valid until the material table loads again
  bool measured = false;           // false: the line was truncated or the world could not answer
};

// The source of a blob's occlusion; the layer keeps the blobs, the falloff, the external factor and the FMOD write.
// Members from setSourceDetourHint down are optional. The layer serialises all calls: a backend needs no lock, calls no layer API.
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
  // set: the backend applies the hint each update, to the scalar and to every band it answers, invalid with no hint or
  // occlusionDetourHint off; null: the layer applies it, see set_detour_hint_gain.
  // It gets the stored sequence, 0 uncounted: a repeat is the same measurement, and an invalid repeat withdraws it.
  void (*setSourceDetourHint)(int idx, float delta_m, bool valid, uint64_t measurement_sequence,
    float measurement_age_seconds) = nullptr;
  // the same per band in OCCLUSION_BAND_PARAMS order; null or false: every band reads getOcclusion's value.
  bool (*getOcclusionBands)(int idx, float (&out_occlusion)[OCCLUSION_BANDS]) = nullptr;
  // before update, once per suspend_backend change; a backend starts enabled and gets no first call
  void (*setEnabled)(bool on) = nullptr;
  void (*update)(float dt) = nullptr;   // sound thread, once per update
  void (*mainThreadUpdate)() = nullptr; // main thread, late in the frame
  void (*levelLoaded)() = nullptr;      // main thread; the backend takes the world it traces
  void (*levelUnloaded)() = nullptr;
  int (*numActiveSources)() = nullptr;
  int (*maxActiveSources)() = nullptr;
  void (*debugRender3d)() = nullptr;                               // main thread, from debug_render_3d, dev builds only
  void (*debugRenderImgui)() = nullptr;                            // the same, from debug_render_imgui
  bool (*debugDetourHint)(int idx, DebugHintState &out) = nullptr; // from debug_render_3d too; false for a slot with no source
  // the same: the materials on the centre line of the source's last measurement, out default-constructed;
  // false: no such source, no measurement of it yet, or no world to trace
  bool (*debugMaterials)(int idx, DebugMaterials &out) = nullptr;
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
