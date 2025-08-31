// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "const3d.h"

#include <util/dag_globDef.h>
#include <EASTL/type_traits.h>
#include <EASTL/array.h>

#include <cstdint>
#include <memory>

enum HlslCompilationStage : uint8_t
{
  HLSL_PS = 0,
  HLSL_VS,
  HLSL_CS,
  HLSL_DS,
  HLSL_HS,
  HLSL_GS,
  HLSL_MS,
  HLSL_AS,

  HLSL_COUNT,

  HLSL_INVALID = 0xFF
};

enum HlslCompilationStageFlags : uint16_t
{
  HLSL_FLAGS_PS = 1 << HLSL_PS,
  HLSL_FLAGS_VS = 1 << HLSL_VS,
  HLSL_FLAGS_CS = 1 << HLSL_CS,
  HLSL_FLAGS_DS = 1 << HLSL_DS,
  HLSL_FLAGS_HS = 1 << HLSL_HS,
  HLSL_FLAGS_GS = 1 << HLSL_GS,
  HLSL_FLAGS_MS = 1 << HLSL_MS,
  HLSL_FLAGS_AS = 1 << HLSL_AS,
  HLSL_FLAGS_ALL = 0xFFFF
};

using hlsl_mask_t = eastl::underlying_type_t<HlslCompilationStageFlags>;

inline constexpr eastl::array<HlslCompilationStage, HLSL_COUNT> HLSL_ALL_LIST = {

  HLSL_PS,
  HLSL_VS,
  HLSL_CS,
  HLSL_DS,
  HLSL_HS,
  HLSL_GS,
  HLSL_MS,
  HLSL_AS,
};

inline constexpr eastl::array<HlslCompilationStageFlags, HLSL_COUNT> HLSL_ALL_FLAGS_LIST = {

  HLSL_FLAGS_PS,
  HLSL_FLAGS_VS,
  HLSL_FLAGS_CS,
  HLSL_FLAGS_DS,
  HLSL_FLAGS_HS,
  HLSL_FLAGS_GS,
  HLSL_FLAGS_MS,
  HLSL_FLAGS_AS,
};

inline constexpr eastl::array<const char *, HLSL_COUNT> HLSL_ALL_NAMES = {
  "pixel",
  "vertex",
  "compute",
  "domain",
  "hull",
  "geometry",
  "mesh",
  "amplification",
};

inline constexpr eastl::array<const char *, HLSL_COUNT> HLSL_ALL_PROFILES = {
  "ps",
  "vs",
  "cs",
  "ds",
  "hs",
  "gs",
  "ms",
  "as",
};

inline constexpr eastl::array<ShaderStage, HLSL_COUNT> HLSL_STAGE_TO_SHADER_STAGE = {
  STAGE_PS, // ps
  STAGE_VS, // vs
  STAGE_CS, // cs
  STAGE_VS, // ds
  STAGE_VS, // hs
  STAGE_VS, // gs
  STAGE_VS, // ms
  STAGE_VS, // as
};

inline bool is_valid(HlslCompilationStage stage) { return stage < HLSL_COUNT; }

inline char hlsl_stage_to_profile_letter(HlslCompilationStage stage) { return HLSL_ALL_PROFILES[stage][0]; }

inline HlslCompilationStage profile_to_hlsl_stage(const char *profile)
{
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
  for (HlslCompilationStage stage : HLSL_ALL_LIST)
  {
    if (strcmp(HLSL_ALL_PROFILES[stage], profile) == 0)
      return stage;
  }
  return HLSL_INVALID;
}

inline HlslCompilationStage profile_to_hlsl_stage(const char *profile, size_t len)
{
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
  for (HlslCompilationStage stage : HLSL_ALL_LIST)
  {
    if (strncmp(HLSL_ALL_PROFILES[stage], profile, len) == 0)
      return stage;
  }
  return HLSL_INVALID;
}

inline HlslCompilationStage valid_profile_to_hlsl_stage(const char *profile)
{
  switch (profile[0])
  {
    case 'p': return HLSL_PS;
    case 'v': return HLSL_VS;
    case 'c': return HLSL_CS;
    case 'd': return HLSL_DS;
    case 'h': return HLSL_HS;
    case 'g': return HLSL_GS;
    case 'm': return HLSL_MS;
    case 'a': return HLSL_AS;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
    default: return HLSL_INVALID;
  }
}

inline void for_each_hlsl_stage(auto &&cb)
{
  for (HlslCompilationStage stage : HLSL_ALL_LIST)
    cb(stage);
}

template <class T>
struct PerHlslStage
{
  union
  {
    eastl::array<T, HLSL_COUNT> all;
    struct
    {
      T ps;
      T vs;
      T cs;
      T ds;
      T hs;
      T gs;
      T ms;
      T as;
    } fields;
  };

  PerHlslStage() { std::construct_at(&all); }
  ~PerHlslStage() { std::destroy_at(&all); }

  PerHlslStage(const PerHlslStage &other) { std::construct_at(&all, other.all); }
  PerHlslStage &operator=(const PerHlslStage &other)
  {
    std::destroy_at(&all);
    std::construct_at(&all, other.all);
    return *this;
  }

  T *profileSwitch(const char *profile)
  {
    HlslCompilationStage stage = profile_to_hlsl_stage(profile);
    return stage == HLSL_INVALID ? nullptr : &all[stage];
  }
  const T *profileSwitch(const char *profile) const { return const_cast<PerHlslStage<T> *>(this)->forProfile(profile); }

  T *validProfileSwitch(const char *profile)
  {
    HlslCompilationStage stage = valid_profile_to_hlsl_stage(profile);
    return stage == HLSL_INVALID ? nullptr : &all[stage];
  }
  const T *validProfileSwitch(const char *profile) const { return const_cast<PerHlslStage<T> *>(this)->forValidProfile(profile); }

  template <class TCb>
  void forEach(hlsl_mask_t mask, TCb &&cb)
  {
    for (HlslCompilationStage stage : HLSL_ALL_LIST)
    {
      if (HLSL_ALL_FLAGS_LIST[stage] & mask)
        cb(all[stage]);
    }
  }

  template <class TCb>
  void forEach(TCb &&cb)
  {
    for (T &item : all)
      cb(item);
  }

  template <class TCb>
  void forProfile(const char *profile, TCb &&cb)
  {
    cb(profileSwitch(profile));
  }
  template <class TCb>
  void forValidProfile(const char *profile, TCb &&cb)
  {
    cb(validProfileSwitch(profile));
  }
};