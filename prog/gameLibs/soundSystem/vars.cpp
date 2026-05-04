// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_hash.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <soundSystem/vars.h>
#include "internal/fmodCompatibility.h"
#include "internal/events_internal.h"
#include "internal/vars_internal.h"
#include "internal/delayed_internal.h"
#include "internal/debug_internal.h"
#include <generic/dag_carray.h>
#include <math/dag_check_nan.h>

namespace sndsys
{
using namespace fmodapi;
static inline void make_var_desc(FMOD_STUDIO_PARAMETER_DESCRIPTION &parameter_desc, VarDesc &desc)
{
  desc.id = as_var_id(parameter_desc.id);
  desc.minValue = parameter_desc.minimum;
  desc.maxValue = parameter_desc.maximum;
  desc.defaultValue = parameter_desc.defaultvalue;
  desc.name = parameter_desc.name;
  desc.gameControlled = FMOD_STUDIO_PARAMETER_GAME_CONTROLLED == parameter_desc.type;
  desc.fmodParameterType = int(parameter_desc.type);
}

bool get_var_desc(const FMOD::Studio::EventDescription &event_description, const char *var_name, VarDesc &desc)
{
  desc = {};
  FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDesc;
  if (FMOD_OK != event_description.getParameterDescriptionByName(var_name, &parameterDesc))
    return false;
  make_var_desc(parameterDesc, desc);
  return true;
}

bool get_var_desc(const FMOD::Studio::EventDescription &event_description, const VarId &var_id, VarDesc &desc)
{
  desc = {};
  FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDesc;
  if (FMOD_OK != event_description.getParameterDescriptionByID(as_fmod_param_id(var_id), &parameterDesc))
    return false;
  make_var_desc(parameterDesc, desc);
  return true;
}

// event:t="[name=value;...;name=value]*"
void set_vars_fmt(const char *format, eastl::pair<EventHandle, const char *> debug_name_or_handle,
  FMOD::Studio::EventInstance &event_instance)
{
  if (!format || *format != '[')
    return;

  const char *start = format + 1;
  const char *end = strchr(start, ']');

  if (!end)
    debug_trace_warn("Syntax error in event path '%s', ending ']' is missing", format);
  else
  {
    eastl::basic_string<char, framemem_allocator> varName;
    for (; start < end;)
    {
      const char *next = strchr(start, ';');
      if (!next)
        next = end;
      const char *separator = strchr(start, '=');
      if (!separator || separator >= next)
      {
        debug_trace_warn("Syntax error in event path '%s', '=' is expected at '%s'", format, start);
        return;
      }

      const float value = strtof(separator + 1, nullptr);
      if (!check_finite(value) || value < -100000.f || value > 100000.f)
      {
        debug_trace_warn("Syntax error in event path '%s', 'real' is expected after '=' at '%s'", format, start);
        return;
      }

      varName.assign(start, separator);
      start = next + 1;

#if DAGOR_DBGLEVEL > 0
      if (varName == "dbgvol")
      {
        logwarn("Do not merge paths with 'dbgvol' var to any branch, use it locally '%s'", format);
        event_instance.setVolume(value);
        continue;
      }
#endif
      const FMOD_RESULT result = !varName.empty() ? event_instance.setParameterByName(varName.c_str(), value) : FMOD_ERR_INVALID_PARAM;
#if DAGOR_DBGLEVEL > 0
      if (result != FMOD_OK)
        debug_trace_warn("set_var \"%s\" failed (\"%s\"): \"%s\"", varName.c_str(),
          debug_name_or_handle.second ? debug_name_or_handle.second : get_debug_name(debug_name_or_handle.first).c_str(),
          FMOD_ErrorString(result));
#else
      G_UNREFERENCED(debug_name_or_handle);
      G_UNREFERENCED(result);
#endif
    }
  }
}

bool get_var_desc(EventHandle event_handle, const char *var_name, VarDesc &desc)
{
  desc = {};
  G_ASSERT(var_name);
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  const FMOD::Studio::EventDescription *eventDescription = get_description(event_handle);
  return eventDescription && get_var_desc(*eventDescription, var_name, desc);
}

bool get_var_desc(const char *event_name, const char *var_name, VarDesc &desc, bool can_trace_warn /* = true*/)
{
  desc = {};
  G_ASSERT(event_name && var_name);
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  char eventPath[DAGOR_MAX_PATH];
  SNPRINTF(eventPath, sizeof(eventPath), "event:/%s", event_name);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  const FMOD_RESULT fmodResult = get_studio_system()->getEvent(eventPath, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    if (can_trace_warn)
      debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  return eventDescription && get_var_desc(*eventDescription, var_name, desc);
}

bool get_var_desc(const FMODGUID &event_id, const char *var_name, VarDesc &desc, bool can_trace_warn /* = true*/)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  const FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    if (can_trace_warn)
      debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  return eventDescription && get_var_desc(*eventDescription, var_name, desc);
}

bool get_var_desc(const FMODGUID &event_id, const VarId &var_id, VarDesc &desc, bool can_trace_warn /* = true*/)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  const FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    if (can_trace_warn)
      debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  return eventDescription && get_var_desc(*eventDescription, var_id, desc);
}

VarId get_var_id(EventHandle event_handle, const char *var_name, bool can_trace_warn /* = true*/)
{
  VarDesc desc;
  if (get_var_desc(event_handle, var_name, desc))
    return desc.id;
  if (can_trace_warn)
    debug_trace_warn("set_var \"%s\" failed (\"%s\")", var_name, get_debug_name(event_handle).c_str());
  return {};
}

VarId get_var_id(const FMODGUID &event_id, const char *var_name, bool can_trace_warn /* = true*/)
{
  SNDSYS_IF_NOT_INITED_RETURN_(VarId{});
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return VarId{};
  }
  if (!eventDescription)
    return VarId{};
  FMOD_STUDIO_PARAMETER_DESCRIPTION parameter;
  fmodResult = eventDescription->getParameterDescriptionByName(var_name, &parameter);
  if (FMOD_OK != fmodResult)
  {
    if (can_trace_warn)
      debug_trace_warn("get getParameter \"%s\" failed: '%s'", var_name, FMOD_ErrorString(fmodResult));
    return VarId{};
  }
  return parameter.type == FMOD_STUDIO_PARAMETER_GAME_CONTROLLED ? as_var_id(parameter.id) : VarId{};
}

bool get_var(EventHandle event_handle, const VarId &var_id, float &val)
{
  val = 0.f;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  const FMOD::Studio::EventInstance *eventInstance = get_instance(event_handle);
  if (!eventInstance)
    return false;
  SOUND_VERIFY_AND_DO(eventInstance->getParameterByID(as_fmod_param_id(var_id), &val, nullptr), return false);
  return true;
}

VarId get_var_id_global(const char *var_name)
{
  SNDSYS_IF_NOT_INITED_RETURN_({});
  FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDesc;
  const FMOD_RESULT fmodResult = get_studio_system()->getParameterDescriptionByName(var_name, &parameterDesc);
  if (FMOD_OK == fmodResult)
  {
    VarDesc desc;
    make_var_desc(parameterDesc, desc);
    return desc.id;
  }
  return {};
}

bool set_var_immediate(EventHandle event_handle, const VarId &var_id, float value)
{
  if (!var_id)
    return false;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventInstance *eventInstance = get_instance(event_handle);
  if (!eventInstance)
    return false;
#if DAGOR_DBGLEVEL > 0
  {
    FMOD::Studio::EventDescription *eventDescription = get_description(event_handle);
    FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDesc;
    if (eventDescription && FMOD_OK == eventDescription->getParameterDescriptionByID(as_fmod_param_id(var_id), &parameterDesc))
      G_ASSERT(parameterDesc.type == FMOD_STUDIO_PARAMETER_GAME_CONTROLLED);
  }
#endif
  const FMOD_RESULT result = eventInstance->setParameterByID(as_fmod_param_id(var_id), value);
#if DAGOR_DBGLEVEL > 0
  if (FMOD_OK != result)
  {
    const FMOD::Studio::EventDescription *eventDescription = get_description(event_handle);
    debug_trace_warn("set_var %s -> %f failed: '%s'", eventDescription ? get_debug_name(*eventDescription).c_str() : "", value,
      FMOD_ErrorString(result));
  }
#else
  G_UNREFERENCED(result);
#endif
  return FMOD_OK == result;
}

bool set_var(EventHandle event_handle, const VarId &var_id, float value)
{
  if (!var_id)
    return false;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  if (should_delay(event_handle, 0.f))
  {
    delayed::set_var(event_handle, var_id, value);
    return true;
  }
  return set_var_immediate(event_handle, var_id, value);
}

bool set_var(EventHandle event_handle, const char *var_name, float value)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  if (const VarId varId = get_var_id(event_handle, var_name, true /*can_trace_warn*/))
    return set_var(event_handle, varId, value);
  return false;
}
bool set_var_optional(EventHandle event_handle, const char *var_name, float value)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  if (const VarId varId = get_var_id(event_handle, var_name, false /*can_trace_warn*/))
    return set_var(event_handle, varId, value);
  return false;
}

bool set_var_global(const VarId &var_id, float value)
{
  if (!var_id)
    return false;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD_RESULT res = get_studio_system()->setParameterByID(as_fmod_param_id(var_id), value);
  if (FMOD_OK != res)
    debug_trace_warn("set_var_global: FMOD error '%s'", FMOD_ErrorString(res));
  return res == FMOD_OK;
}

bool set_var_global(const char *var_name, float value)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD_RESULT res = get_studio_system()->setParameterByName(var_name, value);
  if (FMOD_OK != res)
    debug_trace_warn("set_var_global: '%s' FMOD error '%s'", var_name, FMOD_ErrorString(res));
  return res == FMOD_OK;
}

void set_vars(EventHandle event_handle, dag::ConstSpan<VarId> ids, dag::ConstSpan<float> values)
{
  G_ASSERT_RETURN(ids.size() == values.size(), );
  SNDSYS_IF_NOT_INITED_RETURN;
  FMOD::Studio::EventInstance *eventInstance = get_instance(event_handle);
  if (!eventInstance)
    return;

  const float *valuePtr = values.cbegin();
  for (const VarId &id : ids)
  {
    float value = *valuePtr;
    ++valuePtr;

    if (!id)
      continue;
    FMOD_RESULT fresult = eventInstance->setParameterByID(as_fmod_param_id(id), value);
#if DAGOR_DBGLEVEL > 0
    if (fresult != FMOD_OK)
      debug_trace_warn("set_var failed in '%s': '%s'", get_debug_name(event_handle).c_str(), FMOD_ErrorString(fresult));
#endif
    G_UNREFERENCED(fresult);
  }
  // this works like its should not, see ambientSoundES.cpp.inl
  // SOUND_VERIFY(eventInstance->setParametersByIDs(as_fmod_param_id(ids.cbegin()), values.begin(), values.size()));
}

void set_vars(EventHandle event_handle, carray<VarId, MAX_EVENT_PARAMS> &ids, carray<float, MAX_EVENT_PARAMS> &values, int count)
{
  G_ASSERT_RETURN(uint32_t(count) <= MAX_EVENT_PARAMS, );
  SNDSYS_IF_NOT_INITED_RETURN;
  FMOD::Studio::EventInstance *eventInstance = get_instance(event_handle);
  if (!eventInstance)
    return;
  G_STATIC_ASSERT(eastl::is_trivially_copyable<VarId>::value);
  G_STATIC_ASSERT(sizeof(VarId) == sizeof(FMOD_STUDIO_PARAMETER_ID));
  SOUND_VERIFY_AND_DO(
    eventInstance->setParametersByIDs(as_fmod_param_arr(ids.begin()), values.begin(), min((uint32_t)count, values.size())), return);
}

void set_vars_global(carray<VarId, MAX_EVENT_PARAMS> &ids, carray<float, MAX_EVENT_PARAMS> &values, int count)
{
  G_ASSERT_RETURN(uint32_t(count) <= MAX_EVENT_PARAMS, );
  SNDSYS_IF_NOT_INITED_RETURN;
  G_STATIC_ASSERT(eastl::is_trivially_copyable<VarId>::value);
  G_STATIC_ASSERT(sizeof(VarId) == sizeof(FMOD_STUDIO_PARAMETER_ID));
  SOUND_VERIFY_AND_DO(get_studio_system()->setParametersByIDs(as_fmod_param_arr(ids.begin()), values.begin(), count), return);
}

void init_vars(EventHandle event_handle, carray<VarId, MAX_EVENT_PARAMS> &ids)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  FMOD::Studio::EventDescription *eventDescription = get_description(event_handle);
  if (!eventDescription)
    return;
  int param_count = 0;
  SOUND_VERIFY_AND_DO(eventDescription->getParameterDescriptionCount(&param_count), return);
  G_ASSERT(ids.size() >= param_count);
  for (int i = 0; i < param_count; i++)
  {
    FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDesc;
    if (FMOD_OK != eventDescription->getParameterDescriptionByIndex(i, &parameterDesc))
      return;
    ids[i] = parameterDesc.type == FMOD_STUDIO_PARAMETER_GAME_CONTROLLED ? as_var_id(parameterDesc.id) : VarId{};
  }
}

bool get_param_count(EventHandle event_handle, int &param_count)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = get_description(event_handle);
  if (!eventDescription)
    return false;
  SOUND_VERIFY_AND_DO(eventDescription->getParameterDescriptionCount(&param_count), return false);
  return true;
}

bool get_param_count(const FMODGUID &event_id, int &param_count)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = get_description(event_id);
  if (!eventDescription)
    return false;
  SOUND_VERIFY_AND_DO(eventDescription->getParameterDescriptionCount(&param_count), return false);
  return true;
}

const char *trim_vars_fmt(const char *path)
{
  if (!path)
    return "";

  if (*path != '[')
    return path;

  path = strchr(path, ']');
  return path ? path + 1 : "";
}

void set_vars_fmt(const char *format, EventHandle event_handle)
{
  if (FMOD::Studio::EventInstance *event_instance = get_instance(event_handle))
    set_vars_fmt(format, {event_handle, nullptr}, *event_instance);
}
} // namespace sndsys
