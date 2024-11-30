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
void apply_oneshot(FMOD::Studio::EventInstance &instance, const Point3 &pos, FMOD::System &low_level_system);

void set_pos(FMOD::Studio::EventInstance &instance, const Point3 &pos);

void set_pos(group_id_t group_id, const Point3 &pos);

void set_group(FMOD::Studio::EventInstance *instance, group_id_t group_id);

void append(FMOD::Studio::EventInstance *instance, const FMOD::Studio::EventDescription *description, const Point3 &pos);

void erase(FMOD::Studio::EventInstance *instance, bool apply_occlusion);

void update(const Point3 &listener, FMOD::System &low_level_system);

typedef void (*debug_enum_sources_t)(FMOD::Studio::EventInstance *, group_id_t, const Point3 &, float, bool, bool);

void debug_enum_sources(debug_enum_sources_t);

void init(const DataBlock &blk);
} // namespace sndsys::occlusion
