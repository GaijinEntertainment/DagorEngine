//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/algorithm.h>
#include <soundSystem/handle.h>
#include <soundSystem/varId.h>
#include <generic/dag_carray.h>

namespace sndsys
{
struct VarDesc
{
  VarId id;
  float minValue = 0.f;
  float maxValue = 0.f;
  float defaultValue = 0.f;
  const char *name = "";
  bool gameControlled = false;
  int fmodParameterType = 0;
};

static constexpr int MAX_EVENT_PARAMS = 12;
bool get_var_desc(EventHandle event_handle, const char *var_name, VarDesc &desc);
bool get_var_desc(const char *event_name, const char *var_name, VarDesc &desc);
bool get_var_desc(const FMODGUID &event_id, const char *var_name, VarDesc &desc);
bool get_var_desc(const FMODGUID &event_id, const VarId &var_id, VarDesc &desc);
VarId get_var_id(const FMODGUID &event_id, const char *var_name, bool can_trace_warn = true);
VarId get_var_id(EventHandle event_handle, const char *var_name);
VarId get_var_id_global(const char *var_name);
bool set_var(EventHandle event_handle, const VarId &var_id, float value);
bool set_var(EventHandle event_handle, const char *var_name, float value);
bool set_var_optional(EventHandle event_handle, const char *var_name, float value);
bool set_var_global(const VarId &var_id, float value);
bool set_var_global(const char *var_name, float value);
void set_vars(EventHandle event_handle, dag::ConstSpan<VarId> ids, dag::ConstSpan<float> values);
void set_vars(EventHandle event_handle, carray<VarId, MAX_EVENT_PARAMS> &ids, carray<float, MAX_EVENT_PARAMS> &values, int count);
void set_vars_global(carray<VarId, sndsys::MAX_EVENT_PARAMS> &ids, carray<float, sndsys::MAX_EVENT_PARAMS> &values, int count);
void init_vars(EventHandle event_handle, carray<VarId, MAX_EVENT_PARAMS> &ids);
bool get_var(EventHandle event_handle, const VarId &var_id, float &val);
bool get_param_count(EventHandle event_handle, int &param_count);
const char *trim_vars_fmt(const char *path);
void set_vars_fmt(const char *format, EventHandle event_handle);
} // namespace sndsys

#include <soundSystem/macroTools.h>

#define SOUND_MAKE_VAR_ID(VAR)      ids[idx++] = sndsys::get_var_id(handle_, #VAR);
#define SOUND_MAKE_VAR_ID_GUID(VAR) ids[idx++] = sndsys::get_var_id(event_id_, #VAR, false);

#define SND_DEF_VARS(NAME, ...)                                \
  struct Vars##NAME final                                      \
  {                                                            \
    static constexpr int capacity = SND_PP_NARG(__VA_ARGS__);  \
    union                                                      \
    {                                                          \
      struct                                                   \
      {                                                        \
        float __VA_ARGS__;                                     \
      };                                                       \
      carray<float, capacity> values = {};                     \
      G_STATIC_ASSERT(eastl::is_pod<decltype(values)>::value); \
    };                                                         \
    inline void init(sndsys::EventHandle handle_)              \
    {                                                          \
      intptr_t idx = 0;                                        \
      SND_FOR_EACH(SOUND_MAKE_VAR_ID, __VA_ARGS__)             \
    }                                                          \
    inline void init(const sndsys::FMODGUID &event_id_)        \
    {                                                          \
      intptr_t idx = 0;                                        \
      SND_FOR_EACH(SOUND_MAKE_VAR_ID_GUID, __VA_ARGS__)        \
    }                                                          \
    inline void apply(sndsys::EventHandle handle_) const       \
    {                                                          \
      sndsys::set_vars(handle_, ids, values);                  \
    }                                                          \
                                                               \
  protected:                                                   \
    carray<sndsys::VarId, capacity> ids = {};                  \
  } NAME

#define SND_DEF_VARS_VALUES(NAME, ...)                                  \
  struct VarsValues##NAME final                                         \
  {                                                                     \
    static constexpr int capacity = SND_PP_NARG(__VA_ARGS__);           \
    union                                                               \
    {                                                                   \
      struct                                                            \
      {                                                                 \
        float __VA_ARGS__;                                              \
      };                                                                \
      carray<float, capacity> values = {};                              \
      G_STATIC_ASSERT(eastl::is_pod<decltype(values)>::value);          \
    };                                                                  \
    template <typename VarsIds>                                         \
    inline void init(sndsys::EventHandle handle_, VarsIds &vars_) const \
    {                                                                   \
      intptr_t idx = 0;                                                 \
      auto &ids = vars_.ids;                                            \
      SND_FOR_EACH(SOUND_MAKE_VAR_ID, __VA_ARGS__)                      \
    }                                                                   \
  } NAME

#define SND_DEF_VARS_IDS(NAME, VALUES)                                      \
  struct VarsIds##NAME final                                                \
  {                                                                         \
    static constexpr int capacity = decltype(VALUES)::capacity;             \
    template <typename Vars>                                                \
    inline void apply(sndsys::EventHandle handle_, const Vars &vars_) const \
    {                                                                       \
      sndsys::set_vars(handle_, ids, vars_.values);                         \
    }                                                                       \
    carray<sndsys::VarId, capacity> ids = {};                               \
  } NAME
