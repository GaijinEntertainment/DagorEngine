// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerDumpServer.h"
#include <osApiWrappers/dag_ttas_spinlock.h>

namespace da_profiler
{

struct ScopedTTASSpinLock
{
  volatile int &lock;
  ScopedTTASSpinLock(volatile int &l) : lock(l) { ttas_spinlock_lock(lock); }
  ~ScopedTTASSpinLock() { ttas_spinlock_unlock(lock); }
  ScopedTTASSpinLock(const ScopedTTASSpinLock &) = delete;
  ScopedTTASSpinLock &operator=(const ScopedTTASSpinLock &) = delete;
};

bool ProfilerData::registerPlugin(const char *name, ProfilerPlugin *p)
{
  report_debug("registering plugin %s", name);
  ScopedTTASSpinLock guard(pluginSpinlock);
  if (plugins.find_as(name) != plugins.end())
  {
    report_debug("registering plugin %s failed", name);
    return false;
  }
  interlocked_increment(pluginGeneration);
  plugins.emplace(name, p);
  return true;
}

bool ProfilerData::unregisterPlugin(const char *name)
{
  report_debug("unregistering plugin %s", name);
  ScopedTTASSpinLock guard(pluginSpinlock);
  auto it = plugins.find_as(name);
  if (it == plugins.end())
    return false;
  interlocked_increment(pluginGeneration);
  it->second->unregister();
  plugins.erase(it);
  return true;
}

void ProfilerData::unregisterPlugins()
{
  ScopedTTASSpinLock guard(pluginSpinlock);
  for (auto &i : plugins)
    i.second->unregister();
  interlocked_increment(pluginGeneration);
  plugins.clear();
}

ProfilerPluginStatus ProfilerData::setPluginEnabled(const char *name, bool enabled)
{
  ScopedTTASSpinLock guard(pluginSpinlock);
  auto it = plugins.find_as(name);
  if (it == plugins.end())
    return ProfilerPluginStatus::Error;
  interlocked_increment(pluginGeneration);
  const bool ret = it->second->setEnabled(enabled);
  return ret ? ProfilerPluginStatus::Enabled : ProfilerPluginStatus::Disabled;
}

void ProfilerData::sendPlugins()
{
  hash_map<string, bool> plugs;
  plugs.reserve(4);
  {
    ScopedTTASSpinLock guard(pluginSpinlock);
    plugs.reserve(plugins.size());
    for (auto &i : plugins)
      plugs.emplace(i.first, i.second->isEnabled());
  }
  send_plugins(move(plugs));
}

ProfilerPluginStatus set_plugin_enabled(const char *name, bool enabled) { return the_profiler.setPluginEnabled(name, enabled); }

bool register_plugin(const char *name, ProfilerPlugin *p) { return the_profiler.registerPlugin(name, p); }

bool unregister_plugin(const char *name) { return the_profiler.unregisterPlugin(name); }

} // namespace da_profiler