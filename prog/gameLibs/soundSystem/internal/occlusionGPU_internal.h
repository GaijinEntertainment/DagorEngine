// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/occlusionGPU.h>

namespace FMOD
{
class System;
namespace Studio
{
class EventInstance;
class EventDescription;
} // namespace Studio
} // namespace FMOD

class DataBlock;
class TMatrix;

namespace sndsys::occlusion_gpu
{
using fmod_instance_t = FMOD::Studio::EventInstance;
using fmod_description_t = FMOD::Studio::EventDescription;

void set_pos(const fmod_instance_t *fmod_instance, const Point3 &pos);
void on_release(fmod_instance_t *fmod_instance);

void append_new_instance(fmod_instance_t *fmod_instance, const fmod_description_t *fmod_description, const Point3 &pos);
bool is_suspended(const fmod_instance_t *fmod_instance);
void update(float cur_time, float dt, const TMatrix &listener_tm);
void gpu_update();
void init(const DataBlock &blk);
void close();
bool is_inited();
bool has_backend();

struct BackendSuspends
{
  int count = 0;
  bool enabled = true; // the backend's state: on until the first change

  void suspend(bool on) { count = on ? count + 1 : (count > 0 ? count - 1 : 0); }
  bool takeChange()
  {
    const bool changed = enabled != (count == 0);
    enabled = count == 0;
    return changed;
  }
};

struct DebugState
{
  int numUsedBlobs = 0;
  int numFreeBlobs = 0;
  int numActiveBlobs = 0;
  int numAutoDeleteBlobs = 0;
  int numInstances = 0;

  int numActiveGPUSources = 0;
  int maxActiveGPUSources = 0;
};

DebugState get_debug_state();

typedef void (*debug_enum_blobs_t)(const Point3 & /*pos*/, int /*num_valid_instances*/, float /*occlusion*/, bool /*auto_delete*/);
void debug_enum_blobs(debug_enum_blobs_t debug_enum_blobs);

} // namespace sndsys::occlusion_gpu
