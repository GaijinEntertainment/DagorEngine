#include "sqpcheader.h"
#include "memtrace.h"

#if MEM_TRACE_ENABLED == 1

#include "vartrace.h"
#include "squtils.h"
#include "sqvm.h"
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>
#include <EASTL/sort.h>

#define MEM_TRACE_MAX_VM 8
#define MEM_TRACE_STACK_SIZE 4

namespace sqmemtrace
{

struct ScriptAllocRecord
{
  const char * fileNames[MEM_TRACE_STACK_SIZE] = { 0 };
  int lines[MEM_TRACE_STACK_SIZE] = { 0 };
  int size = 0;
};

typedef struct SQAllocContextT * SQAllocContext;
typedef eastl::unordered_map<const void *, ScriptAllocRecord> ScriptAllocRecordsMap;


static SQAllocContext trace_ctx_to_index[MEM_TRACE_MAX_VM] = { 0 };
static ScriptAllocRecordsMap mem_trace[MEM_TRACE_MAX_VM];



inline ScriptAllocRecordsMap & ctx_to_alloc_records_map(SQAllocContext ctx)
{
  if (!ctx)
    return mem_trace[MEM_TRACE_MAX_VM - 1];

  for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
    if (ctx == trace_ctx_to_index[i])
      return mem_trace[i];

  G_ASSERTF(0, "sq mem trace map not found");
  return mem_trace[MEM_TRACE_MAX_VM - 1];
}

inline int get_memtrace_index(SQAllocContext ctx)
{
  int idx = MEM_TRACE_MAX_VM - 1;
  if (ctx)
    for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
      if (ctx == trace_ctx_to_index[i])
      {
        idx = i;
        break;
      }

  return idx;
}


void add_ctx(SQAllocContext ctx)
{
  debug("sqmemtrace::add_ctx %p", ctx);
  for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
    if (!trace_ctx_to_index[i])
    {
      trace_ctx_to_index[i] = ctx;
      return;
    }

  G_ASSERTF(0, "memtrace: too many VMs, increase MEM_TRACE_MAX_VM");
}


static void dump_sq_allocations_internal(ScriptAllocRecordsMap & alloc_map, int n_top_records)
{
  struct TraceLocation
  {
    const char * fileNames[MEM_TRACE_STACK_SIZE] = { 0 };
    int lines[MEM_TRACE_STACK_SIZE] = { 0 };
  };

  auto hash = [](const TraceLocation& n)
  {
    size_t res = 0;
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
      res += size_t(n.fileNames[i]) + size_t(n.lines[i]);
    return res;
  };

  auto equal = [](const TraceLocation& a, const TraceLocation& b)
  {
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
      if (a.fileNames[i] != b.fileNames[i] || a.lines[i] != b.lines[i])
        return false;
    return true;
  };

  struct ScriptAllocRecordWithCount
  {
    int size = 0;
    int count = 0;
  };

  eastl::unordered_map<TraceLocation, ScriptAllocRecordWithCount, decltype(hash), decltype(equal)> accum(1024, hash, equal);
  for (auto && rec : alloc_map)
  {
    TraceLocation loc;
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
    {
      loc.fileNames[i] = rec.second.fileNames[i];
      loc.lines[i] = rec.second.lines[i];
    }
    auto it = accum.find(loc);
    if (it != accum.end())
    {
      it->second.size += rec.second.size;
      it->second.count++;
    }
    else
    {
      ScriptAllocRecordWithCount m;
      m.size = rec.second.size;
      m.count = 1;
      accum[loc] = m;
    }
  }

  typedef eastl::pair<TraceLocation, ScriptAllocRecordWithCount> AllocInfo;
  eastl::vector<AllocInfo> sorted(accum.begin(), accum.end());
  eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.second.size > b.second.size; });

  debug_("\n==== Begin of quirrel memory allocations ====\n");
  int total = 0;
  unsigned totalBytes = 0;
  int records = 0;
  for (auto && rec : sorted)
  {
    total += rec.second.count;
    totalBytes += rec.second.size;
    debug_("\n%d bytes, %d allocations\n", rec.second.size, rec.second.count);
    for (int i = 0; i < MEM_TRACE_STACK_SIZE && rec.first.fileNames[i]; i++)
      debug_("%s:%d\n", rec.first.fileNames[i], rec.first.lines[i]);

    records++;
    if (n_top_records > 0 && records > n_top_records)
    {
      debug_("\n...");
      break;
    }
  }
  debug_("\ntotal = %d allocations, %u bytes", total, totalBytes);
  debug_("\n==== End of quirrel memory allocations ====\n");
  debug("");
}


void remove_ctx(SQAllocContext ctx)
{
  debug("sqmemtrace::remove_ctx %p", ctx);
  int idx = get_memtrace_index(ctx);

  if (!mem_trace[idx].empty())
  {
    logerr("Detected memory leaks in quirrel VM, see debug for details");
    dump_sq_allocations_internal(mem_trace[idx], 128);
  }

  mem_trace[idx].clear();
  trace_ctx_to_index[idx] = nullptr;
}


void on_alloc(SQAllocContext ctx, HSQUIRRELVM vm, const void * p, size_t size)
{
  ScriptAllocRecordsMap & m = ctx_to_alloc_records_map(ctx);
  ScriptAllocRecord rec;

  rec.fileNames[0] = "unknown";
  rec.lines[0] = 0;
  if (vm && vm->ci)
  {
    SQStackInfos si;
    SQInteger level = 0;

    int count = 0;
    while (SQ_SUCCEEDED(sq_stackinfos(vm, level, &si)))
    {
      if (!si.source)
        break;

      rec.fileNames[count] = si.source;
      rec.lines[count] = int(si.line);
      if (int(si.line) != -1)
      {
        count++;
        if (count >= MEM_TRACE_STACK_SIZE)
          break;
      }
      level++;
    }
  }

  rec.size = int(size);
  m[p] = rec;
}


void on_free(SQAllocContext ctx, const void * p)
{
  ScriptAllocRecordsMap & m = ctx_to_alloc_records_map(ctx);
  auto it = m.find(p);
  if (it != m.end())
    m.erase(it);
}


void reset_statistics_for_current_vm(HSQUIRRELVM vm)
{
  SQAllocContext ctx = vm->_sharedstate->_alloc_ctx;
  int idx = get_memtrace_index(ctx);
  mem_trace[idx].clear();
}


void reset_all()
{
  for (int i = 0; i < MEM_TRACE_MAX_VM; i++)
    mem_trace[i].clear();
}


void dump_statistics_for_current_vm(HSQUIRRELVM vm, int n_top_records)
{
  SQAllocContext ctx = vm->_sharedstate->_alloc_ctx;
  int idx = get_memtrace_index(ctx);
  dump_sq_allocations_internal(mem_trace[idx], n_top_records);
}


void dump_all(int n_top_records)
{
  for (int i = 0; i < MEM_TRACE_MAX_VM; i++)
    if (!mem_trace[i].empty())
    {
      debug("\nMemTrace VM #%d:", i);
      dump_sq_allocations_internal(mem_trace[i], n_top_records);
    }
}

} // namespace sqmemtrace

#else // MEM_TRACE_ENABLED == 0

namespace sqmemtrace
{

void reset_statistics_for_current_vm(HSQUIRRELVM)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::reset_statistics_for_current_vm");
}

void reset_all()
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::reset_all");
}

void dump_statistics_for_current_vm(HSQUIRRELVM, int)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::dump_statistics_for_current_vm");
}

void dump_all(int)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::dump_all");
}

} // namespace sqmemtrace

#endif // MEM_TRACE_ENABLED
