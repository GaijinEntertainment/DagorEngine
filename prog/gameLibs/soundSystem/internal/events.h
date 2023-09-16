#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_EVENTSINTERNAL_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_EVENTSINTERNAL_H_
#pragma once

#include <soundSystem/handle.h>
#include "framememString.h"

class DataBlock;

namespace FMOD
{
namespace Studio
{
class EventInstance;
class EventDescription;
} // namespace Studio
} // namespace FMOD

namespace sndsys
{
void events_init(const DataBlock &blk);
void events_close();
void events_update(float dt);
FrameStr get_debug_name(const FMOD::Studio::EventDescription &event_description);
FrameStr get_debug_name(const FMOD::Studio::EventInstance &event_instance);
FrameStr get_debug_name(EventHandle event_handle);
} // namespace sndsys

#endif
