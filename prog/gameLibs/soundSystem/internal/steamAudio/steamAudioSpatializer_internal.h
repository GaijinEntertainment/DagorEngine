// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>

class DataBlock;

namespace FMOD
{
class System;
namespace Studio
{
class EventInstance;
} // namespace Studio
} // namespace FMOD

namespace sndsys::steam_audio::spatializer
{
void init(const DataBlock &blk, FMOD::System *);
void close();

// Called when a 3d event instance is created.
void append(FMOD::Studio::EventInstance *instance, const Point3 &pos);

// Called whenever the event's 3d position changes.
void set_pos(FMOD::Studio::EventInstance &instance, const Point3 &pos);

// Called when event is being released. Returns true if the spatializer took ownership of destruction.
void release(FMOD::Studio::EventInstance *instance);

// Called every frame from sndsys::end_update.
void update();
} // namespace sndsys::steam_audio::spatializer
