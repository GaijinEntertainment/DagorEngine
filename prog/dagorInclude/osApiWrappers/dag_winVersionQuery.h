//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <compare>
#include <EASTL/optional.h>
#include <inttypes.h>

#include <supp/dag_define_KRNLIMP.h>


typedef struct _OSVERSIONINFOEXW OSVERSIONINFOEXW;
#if _TARGET_PC_WIN
//! replacement of GetVersionEx that is working on all versions from Windows XP to Windows 10 build 1511.
KRNLIMP bool get_version_ex(OSVERSIONINFOEXW *osversioninfo);

struct WindowsVersion
{
  uint32_t major;
  uint32_t minor;
  uint32_t build;

  constexpr auto operator<=>(const WindowsVersion &) const = default;
};

/**
 * @brief On Windows returns the OS' version number.
 *        It can be different from the real OS version if application shimming is active.
 *
 * @deprecated
 *
 * @param zero_initialize In case of error during the query the return value will contain 0 as value, otherwiese a nullopt.
 */
KRNLIMP eastl::optional<WindowsVersion> get_windows_version(bool zero_initialize = false);

#else

bool get_version_ex(OSVERSIONINFOEXW *) = delete;
void get_windows_version(bool) = delete;

#endif

#include <supp/dag_undef_KRNLIMP.h>
