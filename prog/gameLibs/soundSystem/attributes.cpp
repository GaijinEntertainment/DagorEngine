// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_localConv.h>
#include <EASTL/string.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/debug.h>
#include <soundSystem/banks.h>
#include <util/dag_hashedKeyMap.h>
#include "internal/banks.h"
#include "internal/attributes.h"
#include "internal/visualLabels.h"
#include "internal/pathHash.h"
#include "internal/occlusion.h"
#include "internal/debug.h"

static WinCritSec g_attributes_crit_set;
#define SNDSYS_ATTRIBUTES_BLOCK WinAutoLock attributesLock(g_attributes_crit_set);

using namespace sndsys::banks;

namespace sndsys
{
static HashedKeyMap<path_hash_t, EventAttributes> g_path_attributes;
static HashedKeyMap<guid_hash_t, EventAttributes> g_guid_attributes;

bool is_event_oneshot_impl(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(is_event_oneshot_impl);
  bool isOneshot = false;
  SOUND_VERIFY(event_description.isOneshot(&isOneshot));
  return isOneshot;
}

bool is_event_3d_impl(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(is_event_3d_impl);
  bool is3d = false;
  SOUND_VERIFY(event_description.is3D(&is3d));
  return is3d;
}

bool has_sustain_point_impl(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(has_cue_impl);
  bool hasSustainPoint = false;
#if FMOD_VERSION >= 0x00020200
  SOUND_VERIFY(event_description.hasSustainPoint(&hasSustainPoint));
#else
  SOUND_VERIFY(event_description.hasCue(&hasSustainPoint));
#endif
  return hasSustainPoint;
}

bool has_occlusion_impl(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(has_occlusion_impl);
  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  const char *occlusion_param_name = occlusion::get_occlusion_param_name();
  return occlusion::is_inited() && FMOD_OK == event_description.getParameterDescriptionByName(occlusion_param_name, &paramDesc) &&
         is_event_3d_impl(event_description);
}

bool is_delayable_impl(const FMOD::Studio::EventDescription &event_description)
{
  if (!is_event_3d_impl(event_description))
    return false;
  FMOD_STUDIO_USER_PROPERTY userProperty;
  const FMOD_RESULT res = event_description.getUserProperty("dstdel", &userProperty);
  if (res == FMOD_OK && userProperty.type == FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT)
    return userProperty.floatvalue == 1.;
  return false;
}

float get_event_max_distance_impl(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(get_event_max_distance);
  float maxDistance = 0.f;
#if FMOD_VERSION >= 0x00020200
  SOUND_VERIFY(event_description.getMinMaxDistance(nullptr, &maxDistance));
#else
  SOUND_VERIFY(event_description.getMaximumDistance(&maxDistance));
#endif
  return maxDistance;
}

static int get_user_label_idx(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(get_user_label_idx);
  FMOD_STUDIO_USER_PROPERTY userProperty;
  static constexpr const char *property_name = "vlbl";
  if (FMOD_OK == event_description.getUserProperty(property_name, &userProperty))
  {
    G_ASSERTF_RETURN(userProperty.type == FMOD_STUDIO_USER_PROPERTY_TYPE_STRING, -1,
      "User property '%s' has wrong value type (string expected)", property_name);
    return get_visual_label_idx(userProperty.stringvalue);
  }
  return -1;
}

EventAttributes make_event_attributes(const FMOD::Studio::EventDescription &event_description)
{
  TIME_PROFILE_DEV(make_event_attributes);
#if DAGOR_DBGLEVEL > 0
  FMOD_GUID id = {};
  SOUND_VERIFY(event_description.getID(&id));
  G_ASSERT(banks::is_valid_event(*reinterpret_cast<FMODGUID *>(&id)));
#endif
  return EventAttributes(get_event_max_distance_impl(event_description), is_event_oneshot_impl(event_description),
    is_event_3d_impl(event_description), has_sustain_point_impl(event_description), has_occlusion_impl(event_description),
    is_delayable_impl(event_description), get_user_label_idx(event_description));
}

EventAttributes find_event_attributes(const char *path, size_t path_len)
{
  TIME_PROFILE_DEV(find_event_attributes);
  SNDSYS_ATTRIBUTES_BLOCK;
  if (const EventAttributes *val = g_path_attributes.findVal(hash_fun(path, path_len)))
    return *val;
  return EventAttributes{};
}

EventAttributes find_event_attributes(const FMODGUID &event_id)
{
  TIME_PROFILE_DEV(find_event_attributes);
  SNDSYS_ATTRIBUTES_BLOCK;
  if (const EventAttributes *val = g_guid_attributes.findVal(hash_fun(event_id)))
    return *val;
  return EventAttributes{};
}

void make_and_add_event_attributes(const char *path, size_t path_len, const FMOD::Studio::EventDescription &event_description,
  EventAttributes &event_attributes)
{
  TIME_PROFILE_DEV(make_and_cache_event_attributes);
  event_attributes = make_event_attributes(event_description);
  SNDSYS_ATTRIBUTES_BLOCK;
  g_path_attributes.emplace(hash_fun(path, path_len), event_attributes);
#if DAGOR_DBGLEVEL > 0
  char buf[DAGOR_MAX_PATH];
  SNPRINTF(buf, sizeof(buf), "%s", path);
  dd_strlwr(buf);
  if (strcmp(path, buf) != 0)
    logerr("Sound path should have _lower_ case: '%s'", path);
#endif
}

void make_and_add_event_attributes(const FMODGUID &event_id, const FMOD::Studio::EventDescription &event_description,
  EventAttributes &event_attributes)
{
  TIME_PROFILE_DEV(make_and_cache_event_attributes);
  event_attributes = make_event_attributes(event_description);
  SNDSYS_ATTRIBUTES_BLOCK;
  g_guid_attributes.emplace(hash_fun(event_id), event_attributes);
}

size_t get_num_cached_path_attributes()
{
  SNDSYS_ATTRIBUTES_BLOCK;
  return g_path_attributes.size();
}

size_t get_num_cached_guid_attributes()
{
  SNDSYS_ATTRIBUTES_BLOCK;
  return g_guid_attributes.size();
}

void invalidate_attributes_cache()
{
  SNDSYS_ATTRIBUTES_BLOCK;
  g_path_attributes.clear();
  g_guid_attributes.clear();
}
} // namespace sndsys
