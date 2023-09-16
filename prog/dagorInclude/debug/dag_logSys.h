//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_log.h>
#include <generic/dag_span.h>

#include <supp/dag_define_COREIMP.h>

typedef int (*debug_override_timestamp_cb_t)(char *dest, int dest_sz, int64_t t_msec);
typedef int (*debug_log_callback_t)(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line);

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
KRNLIMP const char *get_log_directory();
KRNLIMP const char *get_log_filename();
KRNLIMP void close_debug_files();
KRNLIMP void flush_debug_file();
KRNLIMP void start_debug_system(const char *exe_fname, const char *prefix = ".log/", bool datetime_name = true);
KRNLIMP void start_classic_debug_system(const char *debug_fname, bool en_time = true, bool hhmmss_fmt = false, int hhmmss_subsec = 0,
  bool append_file = false);
KRNLIMP void debug_enable_timestamps(bool en_time, bool hhmmss_fmt = false, int hhmmss_subsec = 0);
KRNLIMP void debug_setup_tags(const dag::ConstSpan<int> *allowed_tags, const dag::ConstSpan<int> *ignored_tags,
  const dag::ConstSpan<int> *promoted_tags);
#if DAGOR_FORCE_LOGS
KRNLIMP const unsigned char *get_default_log_crypt_key();
KRNLIMP void crypt_debug_setup(const unsigned char *nkey, unsigned max_size = 0);
KRNLIMP int get_nearest_crypted_len(int maxlen, int lev = LOGLEVEL_DEBUG);
#endif
KRNLIMP void debug_enable_thread_ids(bool en_tid);
KRNLIMP void debug_set_thread_name(const char *persistent_thread_name_ptr);
KRNLIMP void debug_override_log_timestamp_format(debug_override_timestamp_cb_t);
KRNLIMP debug_log_callback_t debug_set_log_callback(debug_log_callback_t cb);
#else
inline const char *get_log_directory() { return ""; }
inline const char *get_log_filename() { return ""; }
inline void close_debug_files() {}
inline void flush_debug_file() {}
inline void start_debug_system(const char *, const char *prefix = 0, bool = true) { (void)prefix; }
inline void start_classic_debug_system(const char *, bool = true, bool = false, int = 0, bool = false) {}
inline void debug_enable_timestamps(bool, bool = false, int = 0) {}
inline void debug_setup_tags(const dag::ConstSpan<int> *, const dag::ConstSpan<int> *, const dag::ConstSpan<int> *) {}
inline void debug_enable_thread_ids(bool) {}
inline void debug_set_thread_name(const char *) {}
inline void debug_override_log_timestamp_format(debug_override_timestamp_cb_t) {}
inline debug_log_callback_t debug_set_log_callback(debug_log_callback_t) { return NULL; }
#endif

#include <supp/dag_undef_COREIMP.h>
