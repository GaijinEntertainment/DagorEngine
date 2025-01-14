// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>

// @TODO: trunc w/ defines
enum class StcodeTargetArch : uint8_t
{
  DEFAULT,
  X86,
  X86_64,
  ARM64,
  ARM64_E,
  ARM_V7,
  ARM_V7S,
  ARMEABI_V7A,
  ARM64_V8A,
  I386,
  E2K,
};

enum class StcodeTargetPlatform : uint8_t
{
  DEFAULT,
#if _CROSS_TARGET_SPIRV
  PC,
  ANDROID,
  NSWITCH,
#elif _CROSS_TARGET_METAL
  MACOSX,
  IOS,
#endif
};

inline constexpr StcodeTargetArch ALL_STCODE_ARCHS[] = {
  StcodeTargetArch::DEFAULT,
  StcodeTargetArch::X86,
  StcodeTargetArch::X86_64,
  StcodeTargetArch::ARM64,
  StcodeTargetArch::ARM64_E,
  StcodeTargetArch::ARM_V7,
  StcodeTargetArch::ARM_V7S,
  StcodeTargetArch::ARMEABI_V7A,
  StcodeTargetArch::ARM64_V8A,
  StcodeTargetArch::I386,
  StcodeTargetArch::E2K,
};
inline constexpr StcodeTargetPlatform ALL_STCODE_PLATFORMS[] = {
  StcodeTargetPlatform::DEFAULT,
#if _CROSS_TARGET_SPIRV
  StcodeTargetPlatform::PC,
  StcodeTargetPlatform::ANDROID,
  StcodeTargetPlatform::NSWITCH,
#elif _CROSS_TARGET_METAL
  StcodeTargetPlatform::MACOSX,
  StcodeTargetPlatform::IOS,
#endif
};