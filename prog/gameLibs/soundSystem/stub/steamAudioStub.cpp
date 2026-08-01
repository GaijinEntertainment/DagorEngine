// Copyright (C) Gaijin Games KFT.  All rights reserved.

class DataBlock;
class TMatrix;
class Point3;
struct FMOD_DSP_DESCRIPTION;

namespace FMOD
{
class DSP;
class System;
namespace Studio
{
class EventInstance;
} // namespace Studio
} // namespace FMOD

namespace sndsys::steam_audio
{
bool is_inited() { return false; }
void init(const DataBlock &, FMOD::System *) {}
void shutdown() {}
void update_listener(const TMatrix &) {}
void update() {}
void set_spatialized_direction(FMOD::DSP *, const Point3 &) {}
bool get_spatializer_description(FMOD_DSP_DESCRIPTION &) { return false; }

namespace spatializer
{
void append(FMOD::Studio::EventInstance *, const Point3 &) {}
void set_pos(FMOD::Studio::EventInstance &, const Point3 &) {}
void release(FMOD::Studio::EventInstance *) {}
} // namespace spatializer

} // namespace sndsys::steam_audio
