// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/occlusion.h>

class DataBlock;

namespace FMOD
{
class System;
namespace Studio
{
class EventInstance;
class EventDescription;
} // namespace Studio
} // namespace FMOD

namespace sndsys::occlusion
{
void oneshot(FMOD::Studio::EventInstance &instance, const Point3 &pos);

void set_pos(FMOD::Studio::EventInstance &instance, const Point3 &pos);

void append(FMOD::Studio::EventInstance *instance, const FMOD::Studio::EventDescription *description, const Point3 &pos);

bool release(FMOD::Studio::EventInstance *instance);

void update(const Point3 &listener);

typedef void (*debug_enum_sources_t)(FMOD::Studio::EventInstance *, group_id_t, const Point3 &, float, bool, bool, bool,
  const Point3 &);

void get_debug_info(int &total_traces, int &max_traces);

void debug_enum_sources(debug_enum_sources_t);

void init(const DataBlock &blk, FMOD::System *low_level_system);

void close();

const char *get_occlusion_param_name();
} // namespace sndsys::occlusion
