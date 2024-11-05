// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerDumpServer.h"
#include <osApiWrappers/dag_ttas_spinlock.h>

namespace da_profiler
{

bool ProfilerData::registerPlugin(const char *name, ProfilerPlugin *p)
{
  report_debug("registering plugin %s", name);
  ttas_spinlock_lock(pluginSpinlock);
  if (plugins.find_as(name) != plugins.end())
  {
    report_debug("registering plugin %s failed", name);
    ttas_spinlock_unlock(pluginSpinlock);
    return false;
  }
  interlocked_increment(pluginGeneration);
  plugins.emplace(name, p);
  ttas_spinlock_unlock(pluginSpinlock);
  return true;
}

bool ProfilerData::unregisterPlugin(const char *name)
{
  report_debug("unregistering plugin %s", name);
  ttas_spinlock_lock(pluginSpinlock);
  auto it = plugins.find_as(name);
  if (it == plugins.end())
  {
    ttas_spinlock_unlock(pluginSpinlock);
    return false;
  }
  interlocked_increment(pluginGeneration);
  it->second->unregister();
  plugins.erase(it);
  ttas_spinlock_unlock(pluginSpinlock);
  return true;
}

void ProfilerData::unregisterPlugins()
{
  ttas_spinlock_lock(pluginSpinlock);
  for (auto &i : plugins)
    i.second->unregister();
  interlocked_increment(pluginGeneration);
  plugins.clear();
  ttas_spinlock_unlock(pluginSpinlock);
}

ProfilerPluginStatus ProfilerData::setPluginEnabled(const char *name, bool enabled)
{
  ttas_spinlock_lock(pluginSpinlock);
  auto it = plugins.find_as(name);
  if (it == plugins.end())
  {
    ttas_spinlock_unlock(pluginSpinlock);
    return ProfilerPluginStatus::Error;
  }
  interlocked_increment(pluginGeneration);
  const bool ret = it->second->setEnabled(enabled);
  ttas_spinlock_unlock(pluginSpinlock);
  return ret ? ProfilerPluginStatus::Enabled : ProfilerPluginStatus::Disabled;
}

void ProfilerData::sendPlugins()
{
  hash_map<string, bool> plugs;
  plugs.reserve(4);
  ttas_spinlock_lock(pluginSpinlock);
  plugs.reserve(plugins.size());
  for (auto &i : plugins)
    plugs.emplace(i.first, i.second->isEnabled());
  ttas_spinlock_unlock(pluginSpinlock);
  send_plugins(move(plugs));
}

ProfilerPluginStatus set_plugin_enabled(const char *name, bool enabled) { return the_profiler.setPluginEnabled(name, enabled); }

bool register_plugin(const char *name, ProfilerPlugin *p) { return the_profiler.registerPlugin(name, p); }

bool unregister_plugin(const char *name) { return the_profiler.unregisterPlugin(name); }

} // namespace da_profiler