// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_perfTimer.h>
#include <debug/dag_debug.h>

// That's explicitly dev build only. Profiler is enabled in release buil cfg (on PC), and it is probably too expensive to collect all
// that info in release
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
#if _TARGET_64BIT
#define SKIP_TIME_THRESHOLD_US 5
#else
#define SKIP_TIME_THRESHOLD_US 10
#endif
#define LOGDBG_DUMP_TIME_TRESHOLD_US  2000 // 2msec is 16% of 60fps, and 8% of 30fps. Unacceptable!
#define LOGWARN_DUMP_TIME_TRESHOLD_US 10000

#define DECL_ENT_CREATE_MARKERS \
  _X(disappear_events)          \
  _X(destructors)               \
  _X(creation_events)           \
  _X(creation_cb)               \
  _X(loading_events)            \
  _X(tracked_constructors)      \
  _X(constructors)

struct EntityCreationProfiler
{
  struct AutoTimer
  {
    AutoTimer(uint64_t &r) : ref(r) { ref = profile_ref_ticks(); }
    ~AutoTimer() { ref = profile_ref_ticks() - ref; }
    uint64_t &ref;
  };
  const ecs::EntityManager &manager;
  uint64_t reft;
  ecs::template_t templ;
  eastl::vector<eastl::pair<ecs::component_index_t, uint64_t>> compTimes;

  EntityCreationProfiler(ecs::template_t t, const ecs::EntityManager &manager) :
    manager(manager), templ(t), reft((da_profiler::get_active_mode() & da_profiler::EVENTS) ? profile_ref_ticks() : 0)
  {}

#define _X(x)        \
  uint64_t x##D = 0; \
  AutoTimer x() { return AutoTimer(x##D); }
  DECL_ENT_CREATE_MARKERS
#undef _X

  void start(ecs::component_index_t c)
  {
    if (reft)
      compTimes.emplace_back(c, profile_ref_ticks());
  }
  void end()
  {
    if (reft)
      compTimes.back().second = profile_ref_ticks() - compTimes.back().second;
  }
  ~EntityCreationProfiler()
  {
    if (reft && profile_usec_passed(reft, LOGDBG_DUMP_TIME_TRESHOLD_US))
      dump();
  }
  void dump()
  {
    DA_PROFILE_TAG(create_entity, manager.getTemplateName(templ)); // so we see tags in events profiler
    TIME_PROFILE_DEV(dumpDetailsLog)
    int usecPassed = profile_time_usec(reft);
    int ll = usecPassed > LOGWARN_DUMP_TIME_TRESHOLD_US ? LOGLEVEL_WARN : LOGLEVEL_DEBUG;
    logmessage(ll, "Entity creation of template <%s> took too much time: %d us", manager.getTemplateName(templ), usecPassed);
    double pctK = 100. / double(usecPassed);
#define _X(x)                                         \
  x##D = (double)profile_usec_from_ticks_delta(x##D); \
  if (x##D > SKIP_TIME_THRESHOLD_US)                  \
    debug("  %24s took %6d us (%2.3f%%)", #x, x##D, double(x##D) * pctK);
    DECL_ENT_CREATE_MARKERS
#undef _X
    if ((constructorsD + tracked_constructorsD) > SKIP_TIME_THRESHOLD_US)
    {
      eastl::sort(compTimes.begin(), compTimes.end(), [](auto &a, auto &b) { return a.second > b.second; });
      uint64_t total = 0, cur = 0;
      for (auto &c : compTimes)
        total += c.second;
      uint64_t sumThreshold = total - total / 20; // top 95%
      int i = 0;
      debug("  %2s %40s %55s %6s %s", "#N", "comp_name", "comp_type_name", "usec", "usec_percent");
      for (auto &c : compTimes)
      {
        auto usec = profile_usec_from_ticks_delta(c.second);
        auto tpIndex = manager.getDataComponents().getComponentById(c.first).componentType;
        const char *cName = manager.getDataComponents().getComponentNameById(c.first);
        const char *tpName = manager.getComponentTypes().getTypeNameById(tpIndex);
        debug("  %2d %40s %55s %6d %2.3f", ++i, cName, tpName, usec, usec * pctK);
        cur += c.second;
        if (usec < SKIP_TIME_THRESHOLD_US || cur > sumThreshold)
          break;
      }
    }
  }

protected:
};
#define PROFILE_CREATE(name) \
  TIME_PROFILE_DEV(name);    \
  auto tm##name = entCreatProf.name();
#else
struct EntityCreationProfiler
{
  EntityCreationProfiler(ecs::template_t, const ecs::EntityManager &) {}
  void start(ecs::component_index_t) {}
  void end() {}
};
#define PROFILE_CREATE(name) ((void)0)
#endif
