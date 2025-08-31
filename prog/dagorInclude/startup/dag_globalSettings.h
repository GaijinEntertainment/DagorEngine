//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
class DataBlock;


#include <osApiWrappers/dag_atomic.h>
#include <supp/dag_define_KRNLIMP.h>

extern KRNLIMP bool dgs_execute_quiet;

extern KRNLIMP void (*dgs_post_shutdown_handler)();
extern KRNLIMP void (*dgs_pre_shutdown_handler)();

extern KRNLIMP void (*dgs_on_promoted_log_tag)(int tag, const char *fmt, const void *valist_or_dsa, int dsa_num);

//! delegate to be called when fatal error occurs; shall return true to halt execution, or false for continue
extern KRNLIMP bool (*dgs_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line);

extern KRNLIMP void (*dgs_shutdown)();
extern KRNLIMP void (*dgs_fatal_report)(const char *msg, const char *call_stack);
#if _TARGET_STATIC_LIB
extern thread_local int (*dgs_fill_fatal_context)(char *buff, int sz, bool terse);
#else
extern KRNLIMP int (*dgs_fill_fatal_context)(char *buff, int sz, bool terse);
#endif
extern KRNLIMP void (*dgs_report_fatal_error)(const char *title, const char *msg, const char *call_stack);
extern KRNLIMP void (*dgs_on_swap_callback)();
extern KRNLIMP void (*dgs_on_dagor_cycle_start)();

//! helper to get fatal context (to reduce copy-paste on usage)
static inline const char *dgs_get_fatal_context(char *buf, int sz, bool terse = true)
{
  if (!dgs_fill_fatal_context)
    return "";
  dgs_fill_fatal_context(buf, sz, terse);
  return buf;
}

//! last resort to recover after out of memory; should return true to try allocate once more
extern KRNLIMP bool (*dgs_on_out_of_memory)(size_t sz);

//! this function should return pointer to global settings in DataBlock form
//! default implementation returns NULL until overriden by another implemenation
//! at startup
extern KRNLIMP const DataBlock *(*dgs_get_settings)();
extern KRNLIMP const DataBlock *(*dgs_get_game_params)();

void dgs_init_argv(int argc, char **argv);
const char *dgs_get_argv(const char *name, int &it, const char *default_value = nullptr);
inline const char *dgs_get_argv(const char *name, const char *default_value = nullptr)
{
  int dummy = 1;
  return dgs_get_argv(name, dummy, default_value);
}

//! returns true if argument was requested and found by dgs_get_argv()
bool dgs_is_arg_used(int arg_index);
void dgs_set_arg_used(int arg_index, bool used = true);

#if _TARGET_PC
void dgs_setproctitle(const char *title);
#else
inline void dgs_setproctitle(const char * /*title*/) {}
#endif

extern KRNLIMP int dgs_argc;
extern KRNLIMP char **dgs_argv;
extern KRNLIMP bool dgs_sse_present;
extern KRNLIMP char dgs_cpu_name[128];

#if _TARGET_ANDROID
extern const char *dagor_android_internal_path;
extern const char *dagor_android_external_path;
#endif

// Counter of rendered frames
extern KRNLIMP int dagor_frame_no_int; // should not be accessed directly!
inline unsigned int dagor_frame_no() { return interlocked_relaxed_load(dagor_frame_no_int); }
inline void dagor_frame_no_increment() { interlocked_increment(dagor_frame_no_int); }
inline void dagor_frame_no_add(int x) { interlocked_add(dagor_frame_no_int, x); }

//! returns current global frame id
//! This is the frame id that is used for rendering and simulation.
extern KRNLIMP uint32_t dagor_global_frame_id;
inline uint32_t dagor_get_global_frame_id() { return interlocked_relaxed_load(dagor_global_frame_id); }

/*      mode
_______/__   \
FULLSCREEN    \
               \
           in window
    ______/_   |   _\_________________
    WINDOWED   |   WINDOWED_FULLSCREEN
      _________|________
      WINDOWED_NO_BORDER  */
enum class WindowMode : int
{
  FULLSCREEN_EXCLUSIVE,
  WINDOWED,
  WINDOWED_NO_BORDER,
  WINDOWED_FULLSCREEN,
  WINDOWED_RESIZABLE,
};

extern KRNLIMP WindowMode dgs_window_mode; // should not be accessed directly!
inline WindowMode dgs_get_window_mode()
{
  return static_cast<WindowMode>(interlocked_relaxed_load(*reinterpret_cast<int *>(&dgs_window_mode)));
}
inline void dgs_set_window_mode(WindowMode mode)
{
  interlocked_relaxed_store(*reinterpret_cast<int *>(&dgs_window_mode), static_cast<int>(mode));
}

//! read-only value contains current state of application activity; false= app is in background
extern KRNLIMP bool dgs_app_active;

//! Last time the system suspended to and resumed from sleeping state. Milliseconds from system start.
extern KRNLIMP unsigned int dgs_last_suspend_at;
extern KRNLIMP unsigned int dgs_last_resume_at;

extern KRNLIMP const char *dagor_get_build_stamp_str_ex(char *buf, size_t bufsz, const char *suffix, const char *dagor_exe_build_date,
  const char *dagor_exe_build_time);

#include <supp/dag_undef_KRNLIMP.h>
