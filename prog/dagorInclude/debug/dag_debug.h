//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifdef __cplusplus
#include <util/dag_globDef.h>
#include <stdarg.h>
#include <debug/dag_log.h>
#endif

#include <supp/dag_define_KRNLIMP.h>

#ifndef __cplusplus // C only interface

KRNLIMP void cdebug(const char *, ...);
KRNLIMP void cdebug_(const char *, ...);
KRNLIMP void debug_dump_stack(const char *text, int skip_frames);

#else // C++ interface start

template <typename... Args>
inline void debug(const Args &...args) //< outputs formatted string and adds \n
{
  logdbg(args...);
}
template <typename... Args>
inline void debug_(const Args &...args) //< outputs only formatted string (no \n added)
{
  logdbg_(args...);
}
#define DEBUG_CTX(...) LOGDBG_CTX(__VA_ARGS__)

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

extern "C"
{
  KRNLIMP void cdebug(const char *, ...) PRINTF_LIKE;
  KRNLIMP void cdebug_(const char *, ...) PRINTF_LIKE;
  KRNLIMP void debug_dump_stack(const char *text = 0, int skip_frames = 0);
}

//! flushed current output buffer and sets further flushing policy
KRNLIMP void debug_flush(bool);

//! flushed current output buffer and sets further flushing policy, so debug_flush(false) will be ignored
KRNLIMP void force_debug_flush(bool);

//! write fatalerr, logerr, etc., true by default
KRNLIMP void debug_allow_level_files(bool en);

//! prints checkpoint in format: cp: file,line
#define DEBUG_CP() __debug_cp(__FILE__, __LINE__)
KRNLIMP void __debug_cp(const char *fn, int ln);

#define DEBUG_DUMP_VAR(X) debug(#X "=%@", X)

KRNLIMP int tail_debug_file(char *dst, int max_bytes);

#else
extern "C"
{
  inline void cdebug(const char *, ...) {}
  inline void cdebug_(const char *, ...) {}
  inline void debug_dump_stack(const char * /*text*/ = 0, int /*skip_frames*/ = 0) {}
}

inline void debug_flush(bool) {}
inline void force_debug_flush(bool) {}
inline void debug_allow_level_files(bool) {}
#define DEBUG_CP()
inline void __debug_cp(const char *, int) {}
inline int tail_debug_file(char *, int) { return 0; }

#define DEBUG_DUMP_VAR(X) (void)(X)

#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#endif // end of C++

#include <supp/dag_undef_KRNLIMP.h>
