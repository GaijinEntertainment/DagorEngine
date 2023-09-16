#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_VARSINTERNAL_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_VARSINTERNAL_H_
#pragma once

#include <soundSystem/handle.h>

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
struct VarId;
struct VarDesc;

void set_vars_fmt(const char *format, eastl::pair<EventHandle, const char *> debug_name_or_handle,
  FMOD::Studio::EventInstance &event_instance);
bool set_var(EventHandle event_handle, FMOD::Studio::EventInstance *event_instance, const char *var_name, float value);
bool get_var_desc(FMOD::Studio::EventDescription &event_description, const char *var_name, VarDesc &);
bool get_var_desc(FMOD::Studio::EventDescription &event_description, const VarId &var_id, VarDesc &);
} // namespace sndsys

#endif
