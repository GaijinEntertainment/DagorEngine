//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_KRNLIMP.h>

#ifdef __cplusplus
extern "C"
{
#endif

  //! just wraps OutputDebugString
  KRNLIMP void out_debug_str(const char *str);

  //! prepares formatted string (8 kb static buffer) and sends to OutputDebugString
  KRNLIMP void out_debug_str_fmt(const char *fmt, ...);

  //! effective HANDLE for parallel output to console (set as INVALID_HANDLE by default for console applications)
  KRNLIMP void set_debug_console_handle(intptr_t handle);
  KRNLIMP intptr_t get_debug_console_handle();

  KRNLIMP extern const intptr_t invalid_console_handle;


#if _TARGET_IOS | _TARGET_TVOS
  KRNLIMP void set_debug_console_ios_file_output(bool val);
  KRNLIMP bool is_debug_console_ios_file_output();
  KRNLIMP void set_copy_debug_to_ios_console(bool val);
  KRNLIMP bool is_enabled_copy_debug_to_ios_console();
#endif

#ifdef __cplusplus
}
#endif
#include <supp/dag_undef_KRNLIMP.h>
