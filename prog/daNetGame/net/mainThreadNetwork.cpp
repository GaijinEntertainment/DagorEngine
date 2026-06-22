// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net.h"
#include <atomic>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_assert.h>

static std::atomic<int> main_thread_network_cache{-1};

bool is_main_thread_network()
{
  int v = main_thread_network_cache.load(std::memory_order_acquire);
  if (v >= 0)
    return v != 0;
  bool userThreadOwned = dgs_get_argv("userThreadNet") != nullptr;
  if (!userThreadOwned && dgs_get_settings())
    userThreadOwned = dgs_get_settings()->getBlockByNameEx("net")->getBool("userThreadOwned", false);
  int desired = userThreadOwned ? 0 : 1;
  int expected = -1;
  main_thread_network_cache.compare_exchange_strong(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire);
  return main_thread_network_cache.load(std::memory_order_acquire) == 1;
}

void set_main_thread_network(bool value)
{
  G_ASSERT(is_main_thread());
  main_thread_network_cache.store(value ? 1 : 0, std::memory_order_release);
}
