#pragma once

#include <stddef.h>
#include <sqconfig.h>

#define MEM_TRACE_ENABLED  SQ_VAR_TRACE_ENABLED

typedef struct SQAllocContextT * SQAllocContext;
typedef struct SQVM* HSQUIRRELVM;

namespace sqmemtrace
{
  typedef void (*HugeAllocHookCB)(unsigned int /*size*/, unsigned /*cur_threshold*/, HSQUIRRELVM /*vm*/);

  void set_huge_alloc_hook(HugeAllocHookCB hook, unsigned int size_threshold);
  int set_huge_alloc_threshold(int size_threshold); // return previous value

  // get_quirrel_object_size returns table:
  // {
  //   size = <total size in bytes>
  //   size_histogram = {
  //     allocated_size = count
  //     ...
  //   }
  //   type_count_histogram = {
  //     type_name = count
  //     ...
  //   }
  //   type_size_histogram = {
  //     type_name = total_allocated_size
  //     ...
  //   }
  // }
  SQInteger get_quirrel_object_size(HSQUIRRELVM vm);
  SQInteger is_quirrel_object_larger_than(HSQUIRRELVM vm);


  // get_quirrel_object_size_as_string() calls get_quirrel_object_size() and converts result table to string
  SQInteger get_quirrel_object_size_as_string(HSQUIRRELVM vm);

#if MEM_TRACE_ENABLED == 1

  void add_ctx(SQAllocContext ctx);
  void remove_ctx(SQAllocContext ctx);
  void on_alloc(SQAllocContext ctx, HSQUIRRELVM vm, const void * p, size_t size);
  void on_free(SQAllocContext ctx, const void * p);
  void reset_statistics_for_current_vm(HSQUIRRELVM vm);
  void reset_all();
  void dump_statistics_for_current_vm(HSQUIRRELVM vm, int n_top_records = -1);
  void dump_all(int n_top_records = -1);

#else

  void reset_statistics_for_current_vm(HSQUIRRELVM vm);
  void reset_all();
  void dump_statistics_for_current_vm(HSQUIRRELVM vm, int n_top_records);
  void dump_all(int n_top_records);

#endif //MEM_TRACE_ENABLED == 1
}
