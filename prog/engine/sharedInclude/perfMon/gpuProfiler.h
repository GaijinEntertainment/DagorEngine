// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
struct DrawStatSingle;
namespace gpu_profiler
{
bool init();
void shutdown();
uint32_t insert_time_stamp();
uint32_t insert_time_stamp(uint64_t *result);
uint32_t insert_time_stamp_unsafe(uint64_t *result);
uint64_t get_time_stamp(uint32_t id);
uint64_t ticks_per_second();
bool is_on();
void finish_queries();
int get_cpu_ticks_reference(int64_t *out_cpu, int64_t *out_gpu); // cpu timestamp doesn't return frequency. but assumed to be same as
                                                                 // ref_ticks_frequency (unless old AMDs)
int get_profile_cpu_ticks_reference(int64_t &out_cpu, int64_t &out_gpu); // cpu timestamp is in
                                                                         // profile_ref_ticks/profile_ticks_frequency domain
uint32_t flip();
uint32_t flip(uint64_t *result);
void begin_event(const char *name);
void end_event();
void stop_ds(DrawStatSingle &ds);
void start_ds(DrawStatSingle &ds);

struct FuncTable
{
  bool (*init)();
  void (*shutdown)();
  uint32_t (*insert_time_stamp)(uint64_t *result);
  uint32_t (*insert_time_stamp_unsafe)(uint64_t *result);
  uint64_t (*ticks_per_second)();
  void (*finish_queries)();
  int (*get_profile_cpu_ticks_reference)(int64_t &out_cpu, int64_t &out_gpu); // cpu timestamp is in
                                                                              // profile_ref_ticks/profile_ticks_frequency domain
  uint32_t (*flip)(uint64_t *result);

  void (*begin_event)(const char *name);
  void (*end_event)();
  void (*stop_ds)(DrawStatSingle &ds);
  void (*start_ds)(DrawStatSingle &ds);

  inline void fill()
  {
#define SET_ENTRY(NM) this->NM = &gpu_profiler::NM
    SET_ENTRY(init);
    SET_ENTRY(shutdown);
    SET_ENTRY(insert_time_stamp);
    SET_ENTRY(insert_time_stamp_unsafe);
    SET_ENTRY(ticks_per_second);
    SET_ENTRY(finish_queries);
    SET_ENTRY(get_profile_cpu_ticks_reference);
    SET_ENTRY(flip);
    SET_ENTRY(begin_event);
    SET_ENTRY(end_event);
    SET_ENTRY(stop_ds);
    SET_ENTRY(start_ds);
#undef SET_ENTRY
  }
};
} // namespace gpu_profiler

#include <supp/dag_define_KRNLIMP.h>
KRNLIMP void install_gpu_profile_func_table(const gpu_profiler::FuncTable &tbl);
inline void install_gpu_profile_func_table()
{
  gpu_profiler::FuncTable tbl;
  tbl.fill();
  install_gpu_profile_func_table(tbl);
}
#include <supp/dag_undef_KRNLIMP.h>
