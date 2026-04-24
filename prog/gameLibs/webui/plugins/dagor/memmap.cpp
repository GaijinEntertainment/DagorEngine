// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <math/random/dag_random.h>
#include <util/dag_string.h>
#include <util/dag_lookup.h>
#include <util/dag_hashedKeyMap.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_stackHlpEx.h>
#include <perfMon/dag_daProfileMemory.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_sort.h>
#include <util/dag_watchdog.h>
#include <errno.h>

namespace webui
{

struct unprofiled_allocator
{
  unprofiled_allocator(const char * = NULL) {}
  unprofiled_allocator(const unprofiled_allocator &) = default;
  unprofiled_allocator(const unprofiled_allocator &, const char *) {}
  unprofiled_allocator &operator=(const unprofiled_allocator &) = default;
  static void *allocate(size_t n, int = 0) { return da_profiler::unprofiled_mem_alloc(n); }
  static void *allocate(size_t n, size_t, size_t, int = 0) { return da_profiler::unprofiled_mem_alloc(n); }
  static void deallocate(void *b, size_t) { return da_profiler::unprofiled_mem_free(b); }
  const char *get_name() const { return "debug_unprofiled_allocator"; }
  void set_name(const char *) {}
};

static void stringy_for_json(char *c)
{
  for (; *c; c++)
  {
    if (*c == '\\')
      *c = '/';
    else if (*c == '"')
      *c = '\'';
    // Also replace control characters just in case
    else if ((unsigned char)*c < 0x20)
      *c = '?';
  }
}

struct MemSnapshotProfile
{
  uint32_t allocations = 0, allocated = 0;
};

typedef HashedKeyMap<da_profiler::profile_mem_data_t, MemSnapshotProfile, 0, oa_hashmap_util::NoHash<da_profiler::profile_mem_data_t>,
  unprofiled_allocator>
  mem_snapshot_t;

void build_mem_map(YAMemSave *buf, da_profiler::profile_mem_data_t detail_frame, const mem_snapshot_t *snapshot_ref,
  mem_snapshot_t *snapshot_save)
{
  struct PrintContext
  {
    YAMemSave &buf;
    dag::Vector<char, unprofiled_allocator> strings;
    struct ModuleInfo
    {
      uintptr_t base, size;
      uint32_t stringId;
    };
    dag::Vector<ModuleInfo, unprofiled_allocator> modules;
    uintptr_t detailedFrame = 0;

    size_t threshold = 16;
    typedef HashedKeyMap<uintptr_t, uint32_t, 0, oa_hashmap_util::MumStepHash<uintptr_t>, unprofiled_allocator> hashmap_t;
    hashmap_t frameSymbols;

    PrintContext(YAMemSave &buf_) : buf(buf_)
    {
      stackhlp_enum_modules([&](const char *n_, size_t base, size_t size) {
        const uint32_t at = strings.size();
        const size_t nlen = strlen(n_);
        intptr_t i = nlen;
        for (; i >= 0 && (n_[i] != '\\' && n_[i] != '/'); --i) {}
        const char *n = n_ + i + 1;
        modules.emplace_back(ModuleInfo{base, size, at});
        strings.insert(strings.end(), n, n_ + nlen + 1);
        stringy_for_json(strings.begin() + at);
        return true;
      });
      stlsort::sort(modules.begin(), modules.end(), [&](const auto &a, const auto &b) { return a.base < b.base; });
      frameSymbols.reserve(4096);
    }
  };
  struct MemDumpNode
  {
    uintptr_t frame = 0;
    MemDumpNode *parent = nullptr;
    size_t selfAllocated = 0, selfAllocations = 0;
    size_t totalAllocated = 0, totalAllocations = 0;
    dag::VectorMap<uintptr_t, MemDumpNode, eastl::less<uintptr_t>, unprofiled_allocator> children;

    void calcTotal(size_t &max_children, size_t &max_allocated)
    {
      max_children = max<size_t>(max_children, children.size());
      max_allocated = max(max_allocated, selfAllocated);
      totalAllocated += selfAllocated;
      totalAllocations += selfAllocations;
      for (auto &it : children)
      {
        it.second.calcTotal(max_children, max_allocated);
        totalAllocated += it.second.totalAllocated;
        totalAllocations += it.second.totalAllocations;
      }
    }

    mutable size_t printed = 0;
    void print(PrintContext &ctx, size_t threshold, bool first) const
    {
      if ((printed++ & 1023) == 0)
        ::watchdog_kick();

      const char *symbol = nullptr;
      if (frame != 0)
      {
        if (auto cidx = ctx.frameSymbols.findVal(frame))
          symbol = ctx.strings.begin() + *cidx;
        else
        {
          static char stk[8192];
          const char *v = stackhlp_get_call_stack(stk, sizeof(stk), frame);
          if (v && *v)
            stringy_for_json(stk);
          else
          {
            auto it = eastl::lower_bound(ctx.modules.begin(), ctx.modules.end(), frame,
              [](const auto &n, uintptr_t frame) { return n.base + n.size < frame; });
            const char *moduleName = nullptr;
            if (it != ctx.modules.end() && it->base + it->size > frame)
              moduleName = ctx.strings.begin() + it->stringId;
            snprintf(stk, sizeof(stk), "%-8llX%s%s%s", (long long unsigned int)frame, moduleName ? "[" : "",
              moduleName ? moduleName : "", moduleName ? "]" : "");
          }
          uint32_t at = uint32_t(ctx.strings.size());
          ctx.frameSymbols.emplace_new(frame, at);
          ctx.strings.insert(ctx.strings.end(), stk, stk + strlen(stk) + 1);
          symbol = ctx.strings.begin() + at;
        }
      }
      if (frame != 0 && threshold == ctx.threshold && frame == ctx.detailedFrame)
      {
        threshold = size_t(totalAllocated * 0.00005f);
      }

      ctx.buf.printf("%s{\"uid\":\"%p\", \"n\":\"%s\"", first ? "" : ",\n", frame, frame ? symbol : "ROOT");
      size_t sa = selfAllocated, sp = selfAllocations;
      bool childrenWritten = false;
      for (auto &it : children)
      {
        if (it.second.totalAllocated <= threshold)
        {
          sa += it.second.totalAllocated;
          sp += it.second.totalAllocations;
          continue;
        }
        if (!childrenWritten)
          ctx.buf.printf(", \"children\":[");
        it.second.print(ctx, threshold, !childrenWritten);
        childrenWritten = true;
      }
      if (childrenWritten)
        ctx.buf.printf("]");
      if (sa)
        ctx.buf.printf(", \"sa\":%lld", (long long int)sa);
      if (sa)
        ctx.buf.printf(", \"sp\":%lld", (long long int)sp);
      ctx.buf.printf("}");
    }
  } root;

  class LogMemDumpCb final : public da_profiler::MemDumpCb
  {
  public:
    const mem_snapshot_t *snapshotRef = nullptr;
    mem_snapshot_t *snapshotSave = nullptr;
    MemDumpNode *to, *current;
    LogMemDumpCb(MemDumpNode *to_, const mem_snapshot_t *ref, mem_snapshot_t *snap) :
      to(to_), current(to_), snapshotRef(ref), snapshotSave(snap)
    {}
    virtual ~LogMemDumpCb() {}
    void startDump(uintptr_t frame, uint64_t allocated, uint64_t allocations, size_t children_count)
    {
      G_ASSERT_RETURN(current, );
      if (frame != 0)
      {
        auto next = &current->children.emplace(frame, MemDumpNode{frame}).first->second;
        next->parent = current;
        current = next;
      }
      if (int64_t(allocated) > 0)
        current->selfAllocated += allocated;
      if (int64_t(allocations) > 0)
        current->selfAllocations += allocations;
      current->children.reserve(children_count);
    }
    void dump(da_profiler::profile_mem_data_t profile, const uintptr_t *stack, uint32_t stack_size, size_t allocated,
      size_t allocations) override
    {
      if (snapshotRef)
      {
        if (auto v = snapshotRef->findVal(profile))
        {
          if (v->allocated >= allocated)
            return;
          else
          {
            allocated -= v->allocated;
            allocations = eastl::max<size_t>(allocations, v->allocations) - v->allocations;
          }
        }
      }
      if (snapshotSave)
      {
        auto ret = snapshotSave->emplace_if_missing(profile);
        ret.first->allocated += allocated;
        ret.first->allocations += allocations;
      }
      current = to;
      if (stack_size)
      {
        for (int i = 0; i < stack_size - 1; ++i)
          startDump(stack[stack_size - 1 - i], 0, 0, 1);
        startDump(stack[0], allocated, allocations, 0);
      }
      else
        startDump(0, allocated, allocations, 0);
    }
  } dump(&root, snapshot_ref, snapshot_save);
  // int64_t ref = profile_ref_ticks();
  da_profiler::dump_memory_map(dump);
  // int dumpUs = profile_time_usec(ref);
  if (!buf)
    return;
  PrintContext ctx(*buf);

  size_t maxChildrenCount = 0, maxAllocation = 0;
  root.calcTotal(maxChildrenCount, maxAllocation);
  ctx.threshold = size_t(root.totalAllocated * 0.00005f);
  ctx.detailedFrame = detail_frame;

  root.print(ctx, ctx.threshold, true);
}


static uint8_t showMemFlameGraph_html[] = {
#include "showMemFlameGraph.html.inl"
};

static mem_snapshot_t reference_snapshot;

void memory_map(RequestInfo *params)
{
  char **pars = params->params;
  const mem_snapshot_t *useSnap = nullptr;
  if (!pars || !*pars)
  {
    html_response_raw(params->conn, (const char *)showMemFlameGraph_html, sizeof(showMemFlameGraph_html));
    return;
  }

  while (pars && *pars)
  {
    static const char *const cmds[] = {"rc", "snapshot"};
    int cmdi = lup(*pars, cmds, countof(cmds));
    errno = 0;
    switch (cmdi)
    {
      case 0: // rc
      {
        da_profiler::profile_mem_data_t uid = strtoull(*(pars + 1), NULL, 16);
        YAMemSave buf(512 << 10);
        build_mem_map(&buf, uid, useSnap, nullptr);
        return text_response(params->conn, buf.mem, (int)buf.offset);
      }
      case 1: // snapshot
      {
        if (strcmp(*(pars + 1), "make") == 0)
          build_mem_map(nullptr, 0, nullptr, &reference_snapshot);
        else if (strcmp(*(pars + 1), "drop") == 0)
          reference_snapshot.clear();
        else if (strcmp(*(pars + 1), "use") == 0)
          useSnap = &reference_snapshot;
      }
      break;
    }
    pars += 2; // next pair
  }
}

} // namespace webui
