//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_COREIMP.h>

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
  KRNLIMP void set_debug_console_ios_file_output();
  KRNLIMP bool is_debug_console_ios_file_output();
#endif

#ifdef __cplusplus
}
#endif
#include <supp/dag_undef_COREIMP.h>
