// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_log.h>

#include <soundSystem/soundSystem.h>

namespace sndsys::asserts_monitor
{
static WinCritSec g_cs;
#define ASSERTS_MONITOR_BLOCK WinAutoLock assertsMonitorLock(g_cs);

static ska::flat_hash_map<eastl::string, uint32_t> g_assert_counts;

static constexpr size_t TABLE_TOP_N = 10;

void on_fmod_message(const char *func, const char *message)
{
  if (!func || !message)
    return;
  if (strcmp(func, "assert") != 0)
    return;
  if (!is_asserts_monitor_enabled())
    return;

  size_t len = strlen(message);
  while (len > 0 && (message[len - 1] == '\n' || message[len - 1] == '\r'))
    --len;
  if (len == 0)
    return;

  eastl::string key(message, len);

  ASSERTS_MONITOR_BLOCK;
  ++g_assert_counts[eastl::move(key)];
}

bool debug_print_asserts_table()
{
  ASSERTS_MONITOR_BLOCK;
  if (g_assert_counts.empty())
    return false;

  eastl::vector<eastl::pair<const eastl::string *, uint32_t>> sorted;
  sorted.reserve(g_assert_counts.size());
  for (const auto &kv : g_assert_counts)
    sorted.emplace_back(&kv.first, kv.second);

  eastl::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

  const size_t shown = eastl::min<size_t>(TABLE_TOP_N, sorted.size());
  debug("[SNDSYS] Sound asserts monitor: FMOD asserts (top %zu of %zu unique):", shown, sorted.size());
  for (size_t i = 0; i < shown; ++i)
    debug("  %5u  %s", sorted[i].second, sorted[i].first->c_str());
  return true;
}
} // namespace sndsys::asserts_monitor
