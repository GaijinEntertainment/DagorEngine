// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <EASTL/fixed_string.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include <util/dag_hash.h>
#include <stdio.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/vars.h>
#include <soundSystem/debug.h>
#include <soundSystem/visualLabels.h>
#include <soundSystem/banks.h>
#include "internal/fmodCompatibility.h"
#include "internal/framememString.h"
#include "internal/attributes.h"
#include "internal/banks.h"
#include "internal/delayed.h"
#include "internal/visualLabels.h"
#include "internal/vars.h"
#include "internal/events.h"
#include "internal/pool.h"
#include "internal/occlusion.h"
#include "internal/debug.h"

namespace sndsys
{
using namespace fmodapi;
using namespace banks;

struct Event
{
  EventAttributes attributes;
  FMOD::Studio::EventDescription &eventDescription;
  FMOD::Studio::EventInstance &eventInstance;

  EventInstanceCallback instanceCb = nullptr;
  void *instanceCbData = nullptr;
  dag::Index16 nodeId;
  uint8_t stealingVolume = 0;

  Event(FMOD::Studio::EventDescription &event_description, FMOD::Studio::EventInstance &event_instance) :
    eventDescription(event_description), eventInstance(event_instance)
  {}

  Event(const Event &) = delete;
  Event(Event &&) = delete;
  Event &operator=(const Event &) = delete;
  Event &operator=(Event &&) = delete;
};

enum
{
  CALLBACKS_TO_HANDLE = FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND | FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND,
};

using EventsPoolType = Pool<Event, sound_handle_t, 256, POOLTYPE_EVENT, POOLTYPE_TYPES_COUNT>;
G_STATIC_ASSERT(INVALID_SOUND_HANDLE == EventsPoolType::invalid_value);
static EventsPoolType all_events;

static WinCritSec g_pool_cs;
#define SNDSYS_POOL_BLOCK WinAutoLock poolLock(g_pool_cs);

static uint32_t g_max_event_instances = 0;
static uint32_t g_max_oneshot_event_instances = 0;
static bool g_reject_far_oneshots = false;
static bool g_block_programmer_sounds = false;
static WinCritSec g_programmer_sounds_generation_cs;
static int g_programmer_sounds_generation = 0;

int get_programmer_sounds_generation()
{
  WinAutoLock lock(g_programmer_sounds_generation_cs);
  return g_programmer_sounds_generation;
}

void increment_programmer_sounds_generation()
{
  WinAutoLock lock(g_programmer_sounds_generation_cs);
  g_programmer_sounds_generation = (g_programmer_sounds_generation + 1) % 8;
}

void block_programmer_sounds(bool do_block) { g_block_programmer_sounds = do_block; }

static FrameStr guid_to_str(const FMODGUID &id)
{
  char hex[2 * sizeof(id) + 1] = {0};
  id.hex(hex, sizeof(hex));

  FrameStr str;
  str.sprintf("{%s} [%ull,%ull]", hex, id.bits[0], id.bits[1]);
  return str;
}

static inline sound_handle_t get_sound_handle(const FMOD::Studio::EventInstance &event_instance)
{
  uintptr_t event_handle = 0;
  if (FMOD_OK != event_instance.getUserData((void **)&event_handle) || event_handle == INVALID_SOUND_HANDLE)
    return INVALID_SOUND_HANDLE;
  return (sound_handle_t)event_handle;
}

static inline eastl::pair<EventInstanceCallback, void *> get_instance_cb_with_data(const FMOD::Studio::EventInstance &event_instance)
{
  const sound_handle_t handle = get_sound_handle(event_instance);
  SNDSYS_POOL_BLOCK;
  const Event *event = all_events.get(handle);
  return eastl::make_pair(event ? event->instanceCb : nullptr, event ? event->instanceCbData : nullptr);
}

static FMOD_RESULT F_CALLBACK event_instance_callback(FMOD_STUDIO_EVENT_CALLBACK_TYPE callback_type,
  FMOD_STUDIO_EVENTINSTANCE *event_instance, void *parameters)
{
  if (callback_type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
  {
    FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props = (FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *)parameters;
    FMOD_STUDIO_SOUND_INFO soundInfo = {};
#if DAGOR_DBGLEVEL > 0
    FMOD_RESULT result = get_studio_system()->getSoundInfo(props->name, &soundInfo);
    if (FMOD_OK != result)
    {
      debug_trace_warn("getSoundInfo(%s) failed(): '%s'", props->name, FMOD_ErrorString(result));
      return result;
    }
#else
    SOUND_VERIFY_AND_RETURN(get_studio_system()->getSoundInfo(props->name, &soundInfo));
#endif
    float event_generation = -1.f;
    bool wrongGeneration = false;
    FMOD_RESULT res = ((FMOD::Studio::EventInstance *)event_instance)->getParameterByName("generation", &event_generation, nullptr);
    if (res == FMOD_OK)
      wrongGeneration = ((int)event_generation != g_programmer_sounds_generation);
    if (wrongGeneration)
      return FMOD_OK;

    FMOD::Studio::EventDescription *eventDescription = nullptr;
    if (g_block_programmer_sounds && FMOD_OK == ((FMOD::Studio::EventInstance *)event_instance)->getDescription(&eventDescription))
    {
      FMOD_STUDIO_USER_PROPERTY userProperty = {};
      FMOD_RESULT res = eventDescription->getUserPropertyByIndex(0, &userProperty);
      if (res != FMOD_OK || !is_equal_float(userProperty.floatvalue, 1.f))
        return FMOD_OK;
    }
    FMOD::Sound *sound = nullptr;
    SOUND_VERIFY_AND_RETURN(
      get_system()->createSound(soundInfo.name_or_data, soundInfo.mode | FMOD_NONBLOCKING, &soundInfo.exinfo, &sound));
    props->sound = (FMOD_SOUND *)sound;
    props->subsoundIndex = soundInfo.subsoundindex;
    return FMOD_OK;
  }
  else if (callback_type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
  {
    FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props = (FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *)parameters;
    FMOD_RESULT result = FMOD_OK;
    if (props->sound)
      result = ((FMOD::Sound *)props->sound)->release();
    props->sound = nullptr;
    return result;
  }
  else if (FMOD::Studio::EventInstance *eventInstance = (FMOD::Studio::EventInstance *)event_instance)
  {
    auto cbd = get_instance_cb_with_data(*eventInstance);
    if (cbd.first && cbd.first(callback_type, eventInstance, parameters, cbd.second))
      return FMOD_OK;
  }

  return FMOD_OK;
}

static inline FMOD::Studio::EventInstance *get_valid_event_instance_impl(Event &event)
{
  return event.eventInstance.isValid() ? &event.eventInstance : nullptr;
}

static inline FMOD::Studio::EventInstance *get_valid_event_instance_impl(EventHandle event_handle)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  return event ? get_valid_event_instance_impl(*event) : nullptr;
}

static inline FMOD::Studio::EventDescription *get_event_description_impl(EventHandle event_handle)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  return event ? &event->eventDescription : nullptr;
}

static inline FMOD::Studio::EventDescription *get_event_description_impl(const FMODGUID &event_id)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_err_once("get event %s failed: '%s'", guid_to_str(event_id).c_str(), FMOD_ErrorString(fmodResult));
    return nullptr;
  }
  G_ASSERT(eventDescription != nullptr);
  return eventDescription;
}

static inline FMOD::Studio::EventDescription *get_event_description_impl(const char *full_path)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  const FMOD_RESULT fmodResult = get_studio_system()->getEvent(full_path, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_err_once("get event '%s' failed: '%s'", full_path, FMOD_ErrorString(fmodResult));
    return nullptr;
  }
  G_ASSERT(eventDescription != nullptr);
  return eventDescription;
}

static inline EventAttributes get_event_attributes_impl(EventHandle event_handle)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  return event ? event->attributes : EventAttributes();
}

static inline bool get_valid_event_impl(EventHandle event_handle, FMOD::Studio::EventDescription *&event_description,
  FMOD::Studio::EventInstance *&event_instance, EventAttributes &attributes)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  if (event && event->eventInstance.isValid())
  {
    event_description = &event->eventDescription;
    event_instance = &event->eventInstance;
    attributes = event->attributes;
    return true;
  }
  return false;
}

static inline bool get_valid_event_attributes_impl(EventHandle event_handle, EventAttributes &attributes)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  if (event && event->eventInstance.isValid())
  {
    attributes = event->attributes;
    return true;
  }
  return false;
}

static inline void stop_impl(FMOD::Studio::EventInstance &event_instance, bool allow_fadeout = false)
{
  SOUND_VERIFY(event_instance.stop(allow_fadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE));
}

static inline void release_event_instance(const EventAttributes *attributes, FMOD::Studio::EventInstance &event_instance, bool is_stop)
{
  if (event_instance.isValid())
  {
    if (is_stop)
      stop_impl(event_instance);

    if (attributes && attributes->hasOcclusion())
    {
      if (occlusion::release(&event_instance))
        return;
    }

    event_instance.release();
  }
}

void release_all_instances(const char *path)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  FrameStr eventPath;
  eventPath.sprintf("event:/%s", path);
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(eventPath.c_str()))
    eventDescription->releaseAllInstances();
}

static FMOD::Studio::EventInstance *create_event_instance(FMOD::Studio::EventDescription *event_description,
  const EventAttributes &attributes, const char *debug_full_path)
{
  TIME_PROFILE(create_event_instance);

  int numInstances = 0;
  SOUND_VERIFY_AND_RETURN_(event_description->getInstanceCount(&numInstances), nullptr);

  if (!numInstances)
    append_visual_label(*event_description, attributes);

  if (attributes.isOneshot())
  {
    // problem is that many instances will affect performance very terrible.
    // but there is no instrument in FMOD to hard limit max count of instances.
    // FMOD will create as many instances as you tell it, and "max instances" property is not respected here.
    // it is a "intended behavior" as they say: instances exceeding limit will be created but will not play.
    // what can do with that in general: almost nothing.
    // will not treat exceeded count for ONESHOT instances as an error because it is normal by design for oneshot to do not start.

    if (g_max_oneshot_event_instances)
    {
      if (numInstances >= g_max_oneshot_event_instances)
      {
        debug_trace_warn("max instances limit (%d) exceeded for oneshot event \"%s\"", g_max_oneshot_event_instances, debug_full_path);
        return nullptr;
      }
    }
  }
  else if (g_max_event_instances)
  {
    if (numInstances >= g_max_event_instances)
    {
      debug_trace_err_once("create instance of \"%s\" failed: max instances limit (%d) exceeded", debug_full_path,
        g_max_event_instances);
      return nullptr;
    }
  }

  FMOD::Studio::EventInstance *eventInstance = nullptr;
  FMOD_RESULT fmodResult = event_description->createInstance(&eventInstance);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_err("create instance of \"%s\" failed: '%s'", debug_full_path, FMOD_ErrorString(fmodResult));
    return nullptr;
  }

  if (!eventInstance || !eventInstance->isValid())
  {
    debug_trace_err("new instance of \"%s\" is not valid", debug_full_path);
    return nullptr;
  }
  return eventInstance;
}

static inline EventHandle create_event_handle_impl(const DescAndInstance &desc_and_instance, const EventAttributes &attributes)
{
  SNDSYS_POOL_BLOCK;
  G_ASSERT_RETURN(desc_and_instance.first && desc_and_instance.second, {});
  sound_handle_t handle = INVALID_SOUND_HANDLE;
  Event *event = all_events.emplace(handle, *desc_and_instance.first, *desc_and_instance.second);
  if (!event)
    return {};

  event->attributes = attributes;
  return EventHandle(handle);
}

static EventHandle create_event_handle(DescAndInstance &desc_and_instance, const EventAttributes &attributes)
{
  if (!desc_and_instance.first || !desc_and_instance.second)
    return {};

  EventHandle eventHandle = create_event_handle_impl(desc_and_instance, attributes);
  if (!eventHandle)
  {
    desc_and_instance.second->release();
    return eventHandle;
  }

  SOUND_VERIFY(desc_and_instance.second->setUserData((void *)(uintptr_t)(sound_handle_t)eventHandle));
  SOUND_VERIFY(desc_and_instance.second->setCallback(event_instance_callback, CALLBACKS_TO_HANDLE));

  return eventHandle;
}

static inline FrameStr make_path(const char *name, const char *path)
{
  name = trim_vars_fmt(name);
  path = trim_vars_fmt(path);

  FrameStr fullPath;
  if (*name && *path)
  {
    if (*name == '/')
      fullPath.sprintf("event:/%s%s", path, name);
    else
      fullPath.sprintf("event:/%s/%s", path, name);
  }
  else
    fullPath.sprintf("event:/%s", *name ? name : path);

  return fullPath;
}

static __forceinline void set_3d_attr_impl(FMOD::Studio::EventInstance &event_instance, const Attributes3D &attributes_3d)
{
#if DAGOR_DBGLEVEL > 0
  const Point3 &pos = as_point3(attributes_3d.position);
  const Point3 &vel = as_point3(attributes_3d.velocity);
  const Point3 &dir = as_point3(attributes_3d.forward);
  const Point3 &up = as_point3(attributes_3d.up);

  G_ASSERTF_RETURN(!check_nan(pos), , "Vector is not valid, event \"%s\"", get_debug_name(event_instance).c_str());
  G_ASSERTF_RETURN(!check_nan(vel), , "Vector is not valid, event \"%s\"", get_debug_name(event_instance).c_str());
  G_ASSERTF_RETURN(!check_nan(dir), , "Vector is not valid, event \"%s\"", get_debug_name(event_instance).c_str());
  G_ASSERTF_RETURN(!check_nan(up), , "Vector is not valid, event \"%s\"", get_debug_name(event_instance).c_str());
#endif
  SOUND_VERIFY(event_instance.set3DAttributes(&attributes_3d));
}

static __forceinline void set_3d_attr_internal(EventHandle event_handle, const Attributes3D &attributes_3d)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return;
  if (attributes.is3d())
  {
    set_3d_attr_impl(*eventInstance, attributes_3d);
    if (attributes.hasOcclusion())
      occlusion::set_pos(*eventInstance, as_point3(attributes_3d.position));
  }
}

static bool allow_event_init(ieff_t flags, const Point3 *position, const EventAttributes &attributes)
{
  const bool isOneshot = attributes.isOneshot() && !attributes.hasSustainPoint();
  if (!isOneshot && (flags & IEF_EXCLUDE_NON_ONESHOT))
    return false;
  else if (isOneshot && (IEF_EXCLUDE_FAR_ONESHOT & flags) && g_reject_far_oneshots)
  {
    if (attributes.is3d() && attributes.getMaxDistance() > 0.f)
    {
      if (position != nullptr)
      {
        if (lengthSq(get_3d_listener_pos() - *position) >= sqr(attributes.getMaxDistance()))
          return false;
      }
    }
  }
  return true;
}

static DescAndInstance init_event_impl(const FrameStr &full_path, ieff_t flags, const Point3 *position, EventAttributes &attributes)
{
  TIME_PROFILE_DEV(init_event_impl);
  SNDSYS_IF_NOT_INITED_RETURN_({});
  FMOD::Studio::EventDescription *eventDescription = nullptr;

  attributes = find_event_attributes(full_path.c_str(), full_path.length());
  if (!attributes)
  {
    if (!banks::is_valid_event(full_path.c_str()))
    {
      debug_trace_err_once("init event '%s' failed: event is not valid", full_path.c_str());
      return {};
    }
    eventDescription = get_event_description_impl(full_path.c_str());
    if (!eventDescription)
      return {};
    make_and_add_event_attributes(full_path.c_str(), full_path.length(), *eventDescription, attributes);
  }
  if (!allow_event_init(flags, position, attributes))
    return {};
#if DAGOR_DBGLEVEL > 0
  if (position == nullptr && attributes.isOneshot() && attributes.is3d())
    debug_trace_warn("null position for oneshot event \"%s\"", full_path.c_str());
#endif
  if (!eventDescription)
  {
    eventDescription = get_event_description_impl(full_path.c_str());
    if (!eventDescription)
      return {};
  }
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  EventAttributes fmodAttributes = make_event_attributes(*eventDescription);
  if (!(attributes == fmodAttributes))
  {
    if (attributes.getMaxDistance() != fmodAttributes.getMaxDistance())
      logerr("Cached attributes.getMaxDistance() is not equal to current FMOD attributes!");
    else if (attributes.getVisualLabelIdx() != fmodAttributes.getVisualLabelIdx())
      logerr("Cached attributes.getVisualLabelIdx() is not equal to current FMOD attributes!");
    else if (attributes.isOneshot() != fmodAttributes.isOneshot())
      // may happen after var(parameter) set -> fmod snapshot attached -> event.isOneshot=false (fmod bug)
      logerr("Cached attributes.isOneshot() is not equal to current FMOD attributes!");
    else if (attributes.is3d() != fmodAttributes.is3d())
      logerr("Cached attributes.is3d() is not equal to current FMOD attributes!");
    else if (attributes.hasSustainPoint() != fmodAttributes.hasSustainPoint())
      logerr("Cached attributes.hasSustainPoint() is not equal to current FMOD attributes!");
    else
      logerr("Cached attributes are not equal to current FMOD attributes!");
  }
#endif

  G_ASSERT(eventDescription);
  FMOD::Studio::EventInstance *eventInstance = create_event_instance(eventDescription, attributes, full_path.c_str());
  if (eventInstance == nullptr)
    return {};

  if (position != nullptr && attributes.is3d())
    set_3d_attr_impl(*eventInstance, *position);

  return {eventDescription, eventInstance};
}

EventHandle init_event(const char *name, const char *path, ieff_t flags, const Point3 *position /* = nullptr*/)
{
  FrameStr fullPath = make_path(name, path);
  EventAttributes attributes;
  DescAndInstance descAndInstance = init_event_impl(fullPath, flags, position, attributes);
  if (!descAndInstance.second)
    return {};

  set_vars_fmt(name, {EventHandle(), fullPath.c_str()}, *descAndInstance.second);
  set_vars_fmt(path, {EventHandle(), fullPath.c_str()}, *descAndInstance.second);

  EventHandle handle = create_event_handle(descAndInstance, attributes);

  if (!handle)
  {
    if (descAndInstance.second)
      release_event_instance(nullptr, *descAndInstance.second, true);
    return EventHandle(INVALID_SOUND_HANDLE);
  }

  if (attributes.hasOcclusion() && position)
    occlusion::append(descAndInstance.second, descAndInstance.first, *position);

  return handle;
}

bool has_event(const char *name, const char *path)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FrameStr fullPath = make_path(name, path);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  return FMOD_OK == get_studio_system()->getEvent(fullPath.c_str(), &eventDescription);
}

int get_num_event_instances(const char *name, const char *path)
{
  SNDSYS_IF_NOT_INITED_RETURN_(0);
  int numInstances = 0;
  FrameStr fullPath = make_path(name, path);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  if (FMOD_OK == get_studio_system()->getEvent(fullPath.c_str(), &eventDescription))
  {
    SOUND_VERIFY(eventDescription->getInstanceCount(&numInstances));
  }
  return numInstances;
}

int get_num_event_instances(EventHandle event_handle)
{
  if (const FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle))
  {
    int numInstances = 0;
    if (FMOD_OK == eventDescription->getInstanceCount(&numInstances))
      return numInstances;
  }
  return 0;
}

EventHandle init_event(const FMODGUID &event_id, const Point3 *position /* = nullptr*/)
{
  SNDSYS_IF_NOT_INITED_RETURN_({});
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  EventAttributes attributes = find_event_attributes(event_id);
  if (!attributes)
  {
    if (!banks::is_valid_event(event_id))
      return {};
    eventDescription = get_event_description_impl(event_id);
    if (!eventDescription)
      return {};
    make_and_add_event_attributes(event_id, *eventDescription, attributes);
  }

  if (!allow_event_init(IEF_EXCLUDE_FAR_ONESHOT, position, attributes))
    return {};

  if (!eventDescription)
  {
    eventDescription = get_event_description_impl(event_id);
    if (!eventDescription)
      return {};
  }

#if DAGOR_DBGLEVEL > 0
  if (position == nullptr && attributes.isOneshot() && attributes.is3d())
    debug_trace_warn("null position for oneshot event \"%s\"", get_debug_name(*eventDescription).c_str());

  DescAndInstance descAndInstance = {
    eventDescription, create_event_instance(eventDescription, attributes, get_debug_name(*eventDescription).c_str())};
#else
  DescAndInstance descAndInstance = {eventDescription, create_event_instance(eventDescription, attributes, "")};
#endif

  EventHandle handle = create_event_handle(descAndInstance, attributes);

  if (!handle)
  {
    if (descAndInstance.second)
      release_event_instance(nullptr, *descAndInstance.second, true);
    return EventHandle(INVALID_SOUND_HANDLE);
  }

  if (position != nullptr)
  {
    set_3d_attr(handle, *position);

    if (attributes.hasOcclusion())
      occlusion::append(descAndInstance.second, descAndInstance.first, *position);
  }

  return handle;
}

bool has_event(const FMODGUID &event_id)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  return fmodResult == FMOD_OK && eventDescription != nullptr;
}

bool get_event_id(const char *name, const char *path, bool is_snapshot, FMODGUID &event_id, bool debug_trace)
{
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  char completeEventPath[DAGOR_MAX_PATH];
  const char *type = is_snapshot ? "snapshot" : "event";
  if (path && *path)
    SNPRINTF(completeEventPath, DAGOR_MAX_PATH, "%s:/%s/%s", type, path, name);
  else
    SNPRINTF(completeEventPath, DAGOR_MAX_PATH, "%s:/%s", type, name);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD_RESULT fmodResult = get_studio_system()->getEvent(completeEventPath, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    if (debug_trace)
      debug_trace_warn("get event \"%s\" failed: '%s'", completeEventPath, FMOD_ErrorString(fmodResult));
    return false;
  }
  SOUND_VERIFY_AND_DO(eventDescription->getID((FMOD_GUID *)&event_id), return false);
  return true;
}

static inline FMOD::Studio::EventInstance *release_event_impl(EventHandle event_handle, EventAttributes &attributes)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  if (!event)
    return nullptr;
  attributes = event->attributes;
  FMOD::Studio::EventInstance *eventInstance = &event->eventInstance;
  all_events.reject((sound_handle_t)event_handle);
  return eventInstance;
}

static bool get_user_property_impl(const FMOD::Studio::EventDescription &event_description, const char *name,
  FMOD_STUDIO_USER_PROPERTY_TYPE type, FMOD_STUDIO_USER_PROPERTY &user_property)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  const FMOD_RESULT res = event_description.getUserProperty(name, &user_property);
  return res == FMOD_OK && user_property.type == type;
}

static float get_property_impl(const FMOD::Studio::EventDescription &event_description, const char *name, float def_val)
{
  FMOD_STUDIO_USER_PROPERTY userProperty;
  if (get_user_property_impl(event_description, name, FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT, userProperty))
    return userProperty.floatvalue;
  return def_val;
}

static const char *get_property_impl(const FMOD::Studio::EventDescription &event_description, const char *name, const char *def_val)
{
  FMOD_STUDIO_USER_PROPERTY userProperty;
  if (get_user_property_impl(event_description, name, FMOD_STUDIO_USER_PROPERTY_TYPE_STRING, userProperty))
    return userProperty.stringvalue;
  return def_val;
}

static inline bool should_delay(EventHandle event_handle, float delay)
{
  if (delay > 0.f || delayed::is_delayed(event_handle))
    return true;
  if (!delayed::is_enable_distant_delay())
    return false;

  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return false;
  if (!attributes.isDelayable())
    return false;
  Attributes3D attributes3d;
  SOUND_VERIFY_AND_DO(eventInstance->get3DAttributes(&attributes3d), return false);
  const Point3 pos = as_point3(attributes3d.position);
  return delayed::is_far_enough_for_distant_delay(pos);
}

static inline bool should_delay(EventHandle event_handle, const Point3 &pos, float delay)
{
  if (delay > 0.f || delayed::is_delayed(event_handle))
    return true;
  if (!delayed::is_enable_distant_delay())
    return false;
  if (!delayed::is_far_enough_for_distant_delay(pos))
    return false;
  EventAttributes attributes;
  if (!get_valid_event_attributes_impl(event_handle, attributes))
    return false;
  return attributes.isDelayable();
}
static inline bool should_delay_oneshot(const EventAttributes &attributes, const Point3 &pos)
{
  if (!delayed::is_enable_distant_delay())
    return false;
  if (!attributes.isDelayable())
    return false;
  return delayed::is_far_enough_for_distant_delay(pos);
}

void release_immediate(EventHandle &event_handle, bool is_stop /*= true*/)
{
  EventAttributes attributes;
  FMOD::Studio::EventInstance *eventInstance = release_event_impl(event_handle, attributes);
  event_handle.reset();
  if (eventInstance)
    release_event_instance(&attributes, *eventInstance, is_stop);
}
void release_delayed(EventHandle &event_handle, float delay /* = 0.f*/)
{
  delayed::release(event_handle, delay);
  event_handle.reset();
}
void release(EventHandle &event_handle, float delay /* = 0.f*/, bool is_stop /*= true*/)
{
  if (should_delay(event_handle, delay))
    release_delayed(event_handle, delay);
  else
    release_immediate(event_handle, is_stop);
}

static inline bool is_playing(FMOD::Studio::EventInstance &event_instance)
{
  FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
  SOUND_VERIFY_AND_DO(event_instance.getPlaybackState(&playbackState), return false);
  return playbackState != FMOD_STUDIO_PLAYBACK_STOPPED;
}

bool is_playing(EventHandle event_handle)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    return is_playing(*eventInstance) || delayed::is_starting(event_handle);
  return false;
}

const char *debug_event_state(EventHandle event_handle)
{
  if (!is_valid_handle(event_handle))
    return "invalid event handle";
  const FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle);
  if (!eventInstance)
    return "null event instance";

  if (delayed::is_starting(event_handle))
    return "delayed starting";

  if (is_virtual(event_handle))
    return "virtual";

  FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
  FMOD_RESULT res = eventInstance->getPlaybackState(&playbackState);
  if (FMOD_OK != res)
    return FMOD_ErrorString(res);
  if (playbackState == FMOD_STUDIO_PLAYBACK_PLAYING)
    return "playbackState=FMOD_STUDIO_PLAYBACK_PLAYING";
  else if (playbackState == FMOD_STUDIO_PLAYBACK_SUSTAINING)
    return "playbackState=FMOD_STUDIO_PLAYBACK_SUSTAINING";
  else if (playbackState == FMOD_STUDIO_PLAYBACK_STOPPED)
    return "playbackState=FMOD_STUDIO_PLAYBACK_STOPPED";
  else if (playbackState == FMOD_STUDIO_PLAYBACK_STARTING)
    return "playbackState=FMOD_STUDIO_PLAYBACK_STARTING";
  else if (playbackState == FMOD_STUDIO_PLAYBACK_STOPPING)
    return "playbackState=FMOD_STUDIO_PLAYBACK_STOPPING";
  return "playbackState=unexpected value";
}

bool is_stopping(EventHandle event_handle)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
  {
    FMOD_STUDIO_PLAYBACK_STATE playbackState = {};
    SOUND_VERIFY_AND_DO(eventInstance->getPlaybackState(&playbackState), return false);
    return playbackState == FMOD_STUDIO_PLAYBACK_STOPPING;
  }
  return false;
}

static inline bool is_virtual(FMOD::Studio::EventInstance *event_instance)
{
  bool isVirtual = false;
  SOUND_VERIFY_AND_DO(event_instance->isVirtual(&isVirtual), return false);
  return isVirtual;
}
bool is_virtual(EventHandle event_handle)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    return is_virtual(eventInstance);
  return false;
}

static inline bool keyoff_impl(FMOD::Studio::EventInstance &event_instance, const EventAttributes &attributes)
{
  if (attributes.hasSustainPoint())
  {
    if (is_playing(event_instance))
    {
#if FMOD_VERSION >= 0x00020200
      SOUND_VERIFY_AND_DO(event_instance.keyOff(), return false);
      event_instance.keyOff(); // workaround for https://youtrack.gaijin.team/issue/EEX-749,
                               // https://youtrack.gaijin.team/issue/MCT-10202
#else
      SOUND_VERIFY_AND_DO(event_instance.triggerCue(), return false);
#endif
      // expecting that playback will pass all sustaining pts and will stop&release automatically, so don't need to call
      // event_instance.stop()
      return true;
    }
  }
  return false;
}

static inline FMOD::Studio::EventInstance *abandon_event_impl(EventHandle event_handle, EventAttributes &attributes)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  if (!event)
  {
    attributes = {};
    return nullptr;
  }

  FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(*event);
  attributes = event->attributes;

  all_events.reject((sound_handle_t)event_handle);

  return eventInstance;
}

static void abandon_impl(FMOD::Studio::EventInstance &event_instance, const EventAttributes &attributes)
{
  if (!keyoff_impl(event_instance, attributes))
  {
    if (!attributes.isOneshot())
    {
      // Oneshot events will stop naturally, while looping events need to be stopped manually (e.g. by a call to
      // Studio::EventInstance::stop). If the event is oneshot then it will naturally terminate and can be used as a fire - and-forget
      // style oneshot sound by calling Studio::EventInstance::start followed by Studio::EventInstance::release.
      stop_impl(event_instance, true);
    }
  }

  release_event_instance(&attributes, event_instance, false);
}

void abandon_immediate(EventHandle &event_handle)
{
  EventAttributes attributes;
  FMOD::Studio::EventInstance *eventInstance = abandon_event_impl(event_handle, attributes);
  event_handle.reset();
  if (eventInstance)
    abandon_impl(*eventInstance, attributes);
}

void abandon_delayed(EventHandle &event_handle, float delay /* = 0.f*/)
{
  delayed::abandon(event_handle, delay);
  event_handle.reset();
}

void abandon(EventHandle &event_handle, float delay /* = 0.f*/)
{
  if (should_delay(event_handle, delay))
    abandon_delayed(event_handle, delay);
  else
    abandon_immediate(event_handle);
}

bool is_valid_handle(EventHandle event_handle)
{
  SNDSYS_POOL_BLOCK;
  return all_events.get((sound_handle_t)event_handle) != nullptr;
}
bool is_valid_event_instance(EventHandle event_handle) { return get_valid_event_instance_impl(event_handle) != nullptr; }

static __forceinline void start_impl(FMOD::Studio::EventInstance &event_instance)
{
  if (is_playing(event_instance))
    stop_impl(event_instance);
  SOUND_VERIFY_AND_DO(event_instance.start(), return);
}

void start_immediate(EventHandle event_handle)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    sndsys::start_impl(*eventInstance);
}
void start_delayed(EventHandle event_handle, float delay /* = 0.f*/) { delayed::start(event_handle, delay); }
void start(EventHandle event_handle, float delay /* = 0.f*/)
{
  if (should_delay(event_handle, delay))
    start_delayed(event_handle, delay);
  else
    start_immediate(event_handle);
}

void start(EventHandle event_handle, const Point3 &pos, float delay /*= 0.f*/)
{
  if (should_delay(event_handle, pos, delay))
    start_delayed(event_handle, delay);
  else
    start_immediate(event_handle);
}

void stop_immediate(EventHandle event_handle, bool allow_fadeout, bool try_keyoff)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return;
  if (allow_fadeout && try_keyoff && keyoff_impl(*eventInstance, attributes))
    return;
  stop_impl(*eventInstance, allow_fadeout);
}
void stop_delayed(EventHandle event_handle, bool allow_fadeout, bool try_keyoff, float delay)
{
  delayed::stop(event_handle, allow_fadeout, try_keyoff, delay);
}
void stop(EventHandle event_handle, bool allow_fadeout, bool try_keyoff, float delay)
{
  if (should_delay(event_handle, delay))
    stop_delayed(event_handle, allow_fadeout, try_keyoff, delay);
  else
    stop_immediate(event_handle, allow_fadeout, try_keyoff);
}

bool keyoff(EventHandle event_handle)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return false;
  return keyoff_impl(*eventInstance, attributes);
}

bool play_one_shot(const char *name, const char *path, const Point3 *position, ieff_t flags, float delay /* = 0.f*/)
{
  FrameStr fullPath = make_path(name, path);
  EventAttributes attributes;
  DescAndInstance descAndInstance = init_event_impl(fullPath, flags, position, attributes);
  if (!descAndInstance.second)
    return false;

  set_vars_fmt(name, {EventHandle(), fullPath.c_str()}, *descAndInstance.second);
  set_vars_fmt(path, {EventHandle(), fullPath.c_str()}, *descAndInstance.second);

  if (delay > 0.f || (position != nullptr && should_delay_oneshot(attributes, *position)))
  {
    EventHandle handle = create_event_handle(descAndInstance, attributes);

    if (!handle)
    {
      if (descAndInstance.second)
        release_event_instance(nullptr, *descAndInstance.second, true);
      return false;
    }

    delayed::start(handle, delay);
    delayed::abandon(handle, 0.f);
    return true;
  }

  start_impl(*descAndInstance.second);

  if (attributes.hasOcclusion() && position)
    occlusion::append(descAndInstance.second, descAndInstance.first, *position);

  abandon_impl(*descAndInstance.second, attributes);

  if (!attributes.isOneshot() || attributes.hasSustainPoint())
    debug_trace_warn("non oneshot event \"%s\" played with play_one_shot", fullPath.c_str());

  return true;
}

bool is_one_shot(EventHandle event_handle)
{
  const EventAttributes attributes = get_event_attributes_impl(event_handle);
#if DAGOR_DBGLEVEL > 0
  // result should match value from attributes https://cvs1.gaijin.lan/c/dagor4/+/535697
  if ((attributes.isOneshot() && !attributes.hasSustainPoint()) != attributes.isOneshot())
    logerr("events.cpp::is_one_shot(): event treated as oneshot has sustain pt '%s'",
      get_description(event_handle) ? get_debug_name(*get_description(event_handle)).c_str() : "");
#endif
  return attributes.isOneshot();
}

bool is_delayable(EventHandle event_handle)
{
  const EventAttributes attributes = get_event_attributes_impl(event_handle);
  return attributes.isDelayable();
}

bool has_occlusion(EventHandle event_handle)
{
  const EventAttributes attributes = get_event_attributes_impl(event_handle);
  return attributes.hasOcclusion();
}

bool is_one_shot(const FMODGUID &event_id)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  return eventDescription ? is_event_oneshot_impl(*eventDescription) && !has_sustain_point_impl(*eventDescription) : false;
}

bool is_snapshot(const FMODGUID &event_id)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_warn("get event \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  bool isSnapshot = false;
  if (eventDescription)
    SOUND_VERIFY(eventDescription->isSnapshot(&isSnapshot));
  return isSnapshot;
}

float get_audibility(const FMOD::Studio::EventInstance &eventInstance)
{
  FMOD::ChannelGroup *channelGroup = nullptr;
  FMOD_RESULT result = eventInstance.getChannelGroup(&channelGroup);
  float audibility = 0.f;
  if (result == FMOD_OK)
  {
    result = channelGroup->getAudibility(&audibility);
    if (result == FMOD_OK)
      return audibility;
  }
  return 0.f;
}

void pause(EventHandle event_handle, bool pause /* = true*/)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    SOUND_VERIFY(eventInstance->setPaused(pause));
}
bool get_paused(EventHandle event_handle)
{
  bool paused = false;
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    SOUND_VERIFY_AND_DO(eventInstance->getPaused(&paused), return false);
  return paused;
}

bool get_playback_state(EventHandle event_handle, PlayBackState &state)
{
  FMOD_STUDIO_PLAYBACK_STATE fmodState = FMOD_STUDIO_PLAYBACK_STOPPED;
  FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle);
  if (!eventInstance)
    return false;
  SOUND_VERIFY_AND_DO(eventInstance->getPlaybackState(&fmodState), return false);
  switch (fmodState)
  {
    case FMOD_STUDIO_PLAYBACK_PLAYING:
    case FMOD_STUDIO_PLAYBACK_SUSTAINING: state = PlayBackState::STATE_STARTED; break;
    case FMOD_STUDIO_PLAYBACK_STARTING:
    case FMOD_STUDIO_PLAYBACK_STOPPING: state = PlayBackState::STATE_LOADING; break;
    case FMOD_STUDIO_PLAYBACK_STOPPED:
    default: state = PlayBackState::STATE_READY;
  }
  return true;
}

bool get_property(const FMODGUID &event_id, int prop_id, int &value)
{
  value = -1;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    debug_trace_warn("get property \"%s\" failed: '%s'", "", FMOD_ErrorString(fmodResult));
    return false;
  }
  FMOD_STUDIO_USER_PROPERTY userProperty;
  SOUND_VERIFY_AND_DO(eventDescription->getUserPropertyByIndex(prop_id, &userProperty), return false);
  value = userProperty.intvalue;
  return true;
}

float get_property(EventHandle event_handle, const char *name, float def_val)
{
  if (const FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle))
    return get_property_impl(*eventDescription, name, def_val);
  return def_val;
}

void set_volume(EventHandle event_handle, float volume)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    SOUND_VERIFY(eventInstance->setVolume(volume));
}

void set_pitch(EventHandle event_handle, float pitch)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    SOUND_VERIFY(eventInstance->setPitch(pitch));
}

void set_timeline_position(EventHandle event_handle, int position)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    SOUND_VERIFY(eventInstance->setTimelinePosition(position));
}

int get_timeline_position(EventHandle event_handle)
{
  int position = -1;
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
    if (FMOD_OK == eventInstance->getTimelinePosition(&position))
    {
#if DAGOR_DBGLEVEL > 0
      if (position < 0)
      {
        debug_trace_err("eventInstance->getTimelinePosition returned negative number");
      }
#endif
      return max(0, position);
    }
  return -1;
}

void set_3d_attr(EventHandle event_handle, const Point3 &pos) { set_3d_attr_internal(event_handle, Attributes3D(pos)); }
void set_3d_attr(EventHandle event_handle, const Point3 &pos, const Point3 &vel, const Point3 &ori, const Point3 &up)
{
  set_3d_attr_internal(event_handle, Attributes3D(pos, vel, ori, up));
}
void set_3d_attr(EventHandle event_handle, const TMatrix4 &tm, const Point3 &vel)
{
  set_3d_attr_internal(event_handle, Attributes3D(tm, vel));
}
void set_3d_attr(EventHandle event_handle, const TMatrix &tm, const Point3 &vel)
{
  set_3d_attr_internal(event_handle, Attributes3D(tm, vel));
}

bool get_3d_attr(EventHandle event_handle, TMatrix &tm, Point3 &vel)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return false;
  if (!attributes.is3d() || !eventInstance->isValid())
    return false;
  Attributes3D attributes3d;
  SOUND_VERIFY_AND_DO(eventInstance->get3DAttributes(&attributes3d), return false);
  tm.setcol(0, as_point3(attributes3d.forward));
  tm.setcol(1, as_point3(attributes3d.up));
  tm.setcol(3, as_point3(attributes3d.position));
  vel = as_point3(attributes3d.velocity);
  return true;
}

bool get_3d_attr(EventHandle event_handle, Point3 &pos)
{
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  EventAttributes attributes;
  if (!get_valid_event_impl(event_handle, eventDescription, eventInstance, attributes))
    return false;
  if (!attributes.is3d() || !eventInstance->isValid())
    return false;
  Attributes3D attributes3d;
  SOUND_VERIFY_AND_DO(eventInstance->get3DAttributes(&attributes3d), return false);
  pos = as_point3(attributes3d.position);
  return true;
}

void set_occlusion(EventHandle event_handle, float direct_occlusion, float reverb_occlusion)
{
  if (FMOD::Studio::EventInstance *eventInstance = get_valid_event_instance_impl(event_handle))
  {
    FMOD::ChannelGroup *channelGroup = nullptr;
    FMOD_RESULT result = eventInstance->getChannelGroup(&channelGroup);
    if (result == FMOD_OK)
      channelGroup->set3DOcclusion(direct_occlusion, reverb_occlusion);
  }
}

void set_node_id(EventHandle event_handle, dag::Index16 node_id)
{
  SNDSYS_POOL_BLOCK;
  if (Event *event = all_events.get((sound_handle_t)event_handle))
    event->nodeId = node_id;
}

dag::Index16 get_node_id(EventHandle event_handle)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  return event ? event->nodeId : dag::Index16();
}

void get_info(size_t &used_handles, size_t &total_handles)
{
  SNDSYS_POOL_BLOCK;
  used_handles = total_handles = 0;
#if DAGOR_DBGLEVEL > 0
  if (is_inited())
  {
    used_handles = all_events.get_used();
    total_handles = all_events.get_total();
  }
#endif // DAGOR_DBGLEVEL > 0
}

bool has_sustain_point(EventHandle event_handle)
{
  const FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle);
  return eventDescription && has_sustain_point_impl(*eventDescription);
}

bool get_length(EventHandle event_handle, int &length)
{
  length = 0;
  FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle);
  if (!eventDescription)
    return false;
  SOUND_VERIFY_AND_DO(eventDescription->getLength(&length), return false);
  return true;
}

bool get_length(const char *name, int &length)
{
  length = 0;
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FrameStr eventPath;
  eventPath.sprintf("event:/%s", name);
  if (FMOD_OK == get_studio_system()->getEvent(eventPath.c_str(), &eventDescription))
    SOUND_VERIFY_AND_DO(eventDescription->getLength(&length), return false);
  return true;
}

bool get_length(const FMODGUID &event_id, int &length)
{
  length = 0;
  FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_id);
  if (!eventDescription)
    return false;
  SOUND_VERIFY_AND_DO(eventDescription->getLength(&length), return false);
  return true;
}

const char *get_sample_loading_state(const FMOD::Studio::EventDescription &event_description)
{
  FMOD_STUDIO_LOADING_STATE state = {};
  const FMOD_RESULT result = event_description.getSampleLoadingState(&state);
  if (result == FMOD_OK)
  {
    if (state == FMOD_STUDIO_LOADING_STATE_UNLOADING)
      return "unloading";
    if (state == FMOD_STUDIO_LOADING_STATE_UNLOADED)
      return "unloaded";
    if (state == FMOD_STUDIO_LOADING_STATE_LOADING)
      return "loading";
    if (state == FMOD_STUDIO_LOADING_STATE_LOADED)
      return "loaded";
    if (state == FMOD_STUDIO_LOADING_STATE_ERROR)
      return "error";
  }
  return "";
}

#if DAGOR_DBGLEVEL > 0
static eastl::vector<DbgSampleDataRef> dbg_sample_data_descs;
static void dbg_sample_data_add_ref_count(const FMOD::Studio::EventDescription &event_description)
{
  debug("[FMOD] load_sample_data for '%s'", get_debug_name(event_description).c_str());

  FMODGUID id;
  const FMOD_RESULT result = event_description.getID((FMOD_GUID *)&id);
  if (result != FMOD_OK)
  {
    logerr("[SNDSYS] FMOD error '%s'", FMOD_ErrorString(result));
    return;
  }
  auto pred = [&id](const auto &it) { return it.first == id; };
  auto it = eastl::find_if(dbg_sample_data_descs.begin(), dbg_sample_data_descs.end(), pred);
  if (it != dbg_sample_data_descs.end())
    ++it->second;
  else
    dbg_sample_data_descs.push_back({id, 1});
}
static void dbg_sample_data_sub_ref_count(const FMOD::Studio::EventDescription &event_description)
{
  debug("[FMOD] unload_sample_data for '%s'", get_debug_name(event_description).c_str());

  FMODGUID id;
  const FMOD_RESULT result = event_description.getID((FMOD_GUID *)&id);
  if (result != FMOD_OK)
  {
    logerr("[SNDSYS] FMOD error '%s'", FMOD_ErrorString(result));
    return;
  }
  auto pred = [&id](const auto &it) { return it.first == id; };
  auto it = eastl::find_if(dbg_sample_data_descs.begin(), dbg_sample_data_descs.end(), pred);
  G_ASSERT(it != dbg_sample_data_descs.end());
  if (it != dbg_sample_data_descs.end())
  {
    --it->second;
    if (!it->second)
      dbg_sample_data_descs.erase(it);
  }
  else
    logerr("[SNDSYS] sample_data load/unload ref count mismatch %s", guid_to_str(id).c_str());
}
static void dbg_sample_data_verify_ref_count()
{
  for (const auto &it : dbg_sample_data_descs)
  {
    if (it.second != 0)
      logerr("[SNDSYS] sample_data load/unload ref count(%d) mismatch %s", it.second, guid_to_str(it.first).c_str());
  }
}
const dag::Span<DbgSampleDataRef> dbg_get_sample_data_refs()
{
  return make_span(dbg_sample_data_descs.begin(), dbg_sample_data_descs.size());
}
#else
static void dbg_sample_data_add_ref_count(const FMOD::Studio::EventDescription &) {}
static void dbg_sample_data_sub_ref_count(const FMOD::Studio::EventDescription &) {}
static void dbg_sample_data_verify_ref_count() {}
const dag::Span<DbgSampleDataRef> dbg_get_sample_data_refs() { return {}; }
#endif

static bool load_sample_data_impl(FMOD::Studio::EventDescription &event_description)
{
  const FMOD_RESULT result = event_description.loadSampleData();
  if (result != FMOD_OK)
  {
    logerr("[SNDSYS] FMOD error '%s'", FMOD_ErrorString(result));
    return false;
  }
  dbg_sample_data_add_ref_count(event_description);
  return true;
}

static bool unload_sample_data_impl(FMOD::Studio::EventDescription &event_description)
{
  const FMOD_RESULT result = event_description.unloadSampleData();
  if (result != FMOD_OK)
  {
    logerr("[SNDSYS] FMOD error '%s'", FMOD_ErrorString(result));
    return false;
  }
  dbg_sample_data_sub_ref_count(event_description);
  return true;
}

const char *get_sample_loading_state(const char *path)
{
  FrameStr fullPath;
  fullPath.sprintf("event:/%s", path);
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(fullPath.c_str()))
    return get_sample_loading_state(*eventDescription);
  return "";
}

bool load_sample_data(const char *path)
{
  FrameStr fullPath;
  fullPath.sprintf("event:/%s", path);
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(fullPath.c_str()))
    return load_sample_data_impl(*eventDescription);
  return false;
}

bool unload_sample_data(const char *path)
{
  FrameStr fullPath;
  fullPath.sprintf("event:/%s", path);
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(fullPath.c_str()))
    return unload_sample_data_impl(*eventDescription);
  return false;
}

const char *get_sample_loading_state(EventHandle event_handle)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle))
    return get_sample_loading_state(*eventDescription);
  return "";
}

bool load_sample_data(EventHandle event_handle)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle))
    return load_sample_data_impl(*eventDescription);
  return false;
}

bool unload_sample_data(EventHandle event_handle)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_handle))
    return unload_sample_data_impl(*eventDescription);
  return false;
}

const char *get_sample_loading_state(const FMODGUID &event_id)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_id))
    return get_sample_loading_state(*eventDescription);
  return "";
}

bool load_sample_data(const FMODGUID &event_id)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_id))
    return load_sample_data_impl(*eventDescription);
  return false;
}

bool unload_sample_data(const FMODGUID &event_id)
{
  if (FMOD::Studio::EventDescription *eventDescription = get_event_description_impl(event_id))
    return unload_sample_data_impl(*eventDescription);
  return false;
}

bool is_3d(EventHandle event_handle) { return get_event_attributes_impl(event_handle).is3d(); }

bool are_of_the_same_desc(EventHandle first_handle, EventHandle second_handle)
{
  SNDSYS_POOL_BLOCK;
  if (Event *first = all_events.get((sound_handle_t)first_handle))
    if (Event *second = all_events.get((sound_handle_t)second_handle))
      return &first->eventDescription == &second->eventDescription;
  return false;
}

float get_max_distance(EventHandle event_handle) { return get_event_attributes_impl(event_handle).getMaxDistance(); }
float get_max_distance(const char *name)
{
  SNDSYS_IF_NOT_INITED_RETURN_(0.f);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  FrameStr eventPath;
  eventPath.sprintf("event:/%s", name);
  if (FMOD_OK == get_studio_system()->getEvent(eventPath.c_str(), &eventDescription))
    return eventDescription ? get_event_max_distance_impl(*eventDescription) : 0.f;
  return 0.f;
}

float get_max_distance(const FMODGUID &event_id)
{
  SNDSYS_IF_NOT_INITED_RETURN_(-1.f);
  EventAttributes attributes = find_event_attributes(event_id);
  if (attributes)
    return attributes.getMaxDistance();
  else
  {
    if (!banks::is_valid_event(event_id))
      return -1.f;
    FMOD::Studio::EventDescription *eventDescription = nullptr;
    G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
    FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
    if (FMOD_OK == fmodResult)
    {
      float distance = -1.f;
#if FMOD_VERSION >= 0x00020200
      FMOD_RESULT result = eventDescription->getMinMaxDistance(nullptr, &distance);
#else
      FMOD_RESULT result = eventDescription->getMaximumDistance(&distance);
#endif
      if (FMOD_OK == result)
        return distance;
    }
  }
  return -1.f;
}

bool preload(const FMODGUID &event_id)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    char hex[2 * sizeof(event_id) + 1] = {0};
    debug_trace_warn("get event \"%s\" failed: '%s'", event_id.hex(hex, sizeof(hex)), FMOD_ErrorString(fmodResult));
    return false;
  }
  SOUND_VERIFY_AND_DO(eventDescription->loadSampleData(), return false);
  return true;
}

bool unload(const FMODGUID &event_id, bool is_strict)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventDescription *eventDescription = nullptr;
  G_STATIC_ASSERT(sizeof(FMODGUID) == sizeof(FMOD_GUID));
  FMOD_RESULT fmodResult = get_studio_system()->getEventByID((FMOD_GUID *)&event_id, &eventDescription);
  if (FMOD_OK != fmodResult)
  {
    char hex[2 * sizeof(event_id) + 1] = {0};
    debug_trace_warn("get event \"%s\" failed: '%s'", event_id.hex(hex, sizeof(hex)), FMOD_ErrorString(fmodResult));
    return false;
  }
  fmodResult = eventDescription->unloadSampleData();
  if (FMOD_OK != fmodResult && is_strict)
  {
    if (fmodResult == FMOD_ERR_STUDIO_NOT_LOADED)
      debug_trace_warn("unloadSampleData \"%s\" failed: '%s'", get_debug_name(*eventDescription).c_str(),
        FMOD_ErrorString(fmodResult));
    else
      debug_trace_err("unloadSampleData \"%s\" failed: '%s'", get_debug_name(*eventDescription).c_str(), FMOD_ErrorString(fmodResult));
    return false;
  }
  return true;
}

void events_init(const DataBlock &blk)
{
  SNDSYS_IS_MAIN_THREAD;
  g_max_event_instances = max(0, blk.getInt("maxEventInstances", 0));
  g_max_oneshot_event_instances = max(0, blk.getInt("maxOneshotEventInstances", g_max_event_instances));
  g_reject_far_oneshots = blk.getBool("rejectFarOneshots", false);
}

static inline auto events_close_impl()
{
  SNDSYS_POOL_BLOCK;
  eastl::vector<FMOD::Studio::EventInstance *, framemem_allocator> instances;
  all_events.enumerate([&](Event &event) {
#if DAGOR_DBGLEVEL > 0
    debug_trace_warn("Event was not released: \"%s\"", get_debug_name(event.eventDescription).c_str());
#endif
    instances.push_back(&event.eventInstance);
  });
  all_events.close();
  return instances;
}

void events_close()
{
  SNDSYS_IS_MAIN_THREAD;
  dbg_sample_data_verify_ref_count();
  auto instances = events_close_impl();
  for (FMOD::Studio::EventInstance *instance : instances)
    release_event_instance(nullptr, *instance, true);
}

void events_update(float dt) { update_visual_labels(dt); }

FrameStr get_debug_name(const FMOD::Studio::EventDescription &event_description)
{
  char completeEventPath[DAGOR_MAX_PATH] = {0};
#if DAGOR_DBGLEVEL > 0
  if (event_description.isValid())
    event_description.getPath(completeEventPath, countof(completeEventPath), nullptr);
#else
  G_UNREFERENCED(event_description);
#endif
  return FrameStr(completeEventPath);
}

FrameStr get_debug_name(const FMOD::Studio::EventInstance &event_instance)
{
#if DAGOR_DBGLEVEL > 0
  if (event_instance.isValid())
  {
    FMOD::Studio::EventDescription *eventDescription = nullptr;
    if (FMOD_OK == event_instance.getDescription(&eventDescription))
      return get_debug_name(*eventDescription);
  }
#else
  G_UNREFERENCED(event_instance);
#endif
  return "";
}

FrameStr get_debug_name(EventHandle event_handle)
{
#if DAGOR_DBGLEVEL > 0
  SNDSYS_POOL_BLOCK;
  if (const Event *event = all_events.get((sound_handle_t)event_handle))
    return get_debug_name(event->eventDescription);
#else
  G_UNREFERENCED(event_handle);
#endif
  return "";
}

float adjust_stealing_volume(EventHandle event_handle, float delta)
{
  FMOD::Studio::EventInstance *eventInstance = nullptr;
  int volume = 0;
  for (;;)
  {
    SNDSYS_POOL_BLOCK;
    Event *event = all_events.get((sound_handle_t)event_handle);
    if (event && event->eventInstance.isValid())
    {
      G_STATIC_ASSERT((eastl::is_same<decltype(event->stealingVolume), uint8_t>::value));

      volume = delta < 0.f ? max(int(event->stealingVolume) - int(ceil(-delta * 0xff)), 0)
                           : min(int(event->stealingVolume) + int(ceil(delta * 0xff)), 0xff);

      if (event->stealingVolume != volume)
      {
        event->stealingVolume = volume;
        eventInstance = &event->eventInstance;
      }
    }
    break;
  }
  if (eventInstance)
    SOUND_VERIFY(eventInstance->setVolume(float(volume) / 0xff));
  return volume;
}

namespace fmodapi
{
static FMOD::Studio::EventInstance *set_instance_cb_impl(EventHandle event_handle, EventInstanceCallback cb, void *cb_data)
{
  SNDSYS_POOL_BLOCK;
  Event *event = all_events.get((sound_handle_t)event_handle);
  if (!event)
    return nullptr;
  event->instanceCb = cb;
  event->instanceCbData = cb_data;
  return get_valid_event_instance_impl(*event);
}

void set_instance_cb(EventHandle event_handle, EventInstanceCallback cb, void *cb_data, uint32_t cb_mask)
{
  if (FMOD::Studio::EventInstance *eventInstance = set_instance_cb_impl(event_handle, cb, cb_data))
    SOUND_VERIFY(eventInstance->setCallback(event_instance_callback, CALLBACKS_TO_HANDLE | cb_mask));
}

bool get_instance_cb_data(FMOD::Studio::EventInstance *event_instance, void *&cb_data)
{
  cb_data = nullptr;
  if (!event_instance)
    return false;
  const sound_handle_t handle = get_sound_handle(*event_instance);
  SNDSYS_POOL_BLOCK;
  const Event *event = all_events.get(handle);
  if (!event)
    return false;
  cb_data = event->instanceCbData;
  return true;
}

FMOD::Studio::EventInstance *get_instance(EventHandle event_handle) { return get_valid_event_instance_impl(event_handle); }

FMOD::Studio::EventDescription *get_description(EventHandle event_handle) { return get_event_description_impl(event_handle); }

FMOD::Studio::EventDescription *get_description(const FMODGUID &event_id) { return get_event_description_impl(event_id); }

FMOD::Studio::EventDescription *get_description(const char *name, const char *path)
{
  SNDSYS_IF_NOT_INITED_RETURN_(nullptr);
  return get_event_description_impl(make_path(name, path).c_str());
}
} // namespace fmodapi
} // namespace sndsys
