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

namespace sndsys::occlusion_gpu
{
using fmod_instance_t = FMOD::Studio::EventInstance;
using fmod_description_t = FMOD::Studio::EventDescription;

void set_pos(const fmod_instance_t *fmod_instance, const Point3 &pos);
void on_release(fmod_instance_t *fmod_instance);

void append_new_instance(fmod_instance_t *fmod_instance, const fmod_description_t *fmod_description, const Point3 &pos);
bool is_suspended(const fmod_instance_t *fmod_instance);
void update(float cur_time, const Point3 &listener_pos);
void gpu_update();
void init(const DataBlock &blk);
void close();
bool is_inited();

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
