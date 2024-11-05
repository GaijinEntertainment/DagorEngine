// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerDumpServer.h"
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "locklessRingBuf.h"
#include "stl/daProfilerString.h"
#include "stl/daProfilerHashmap.h"
#include "stl/daProfilerAlgorithm.h"
#include <osApiWrappers/dag_threads.h> //execute in new thread. we can just use stl threads

// this is is generalized server that works in thread and receives dumps from clients, to be sent to files/net/etc

namespace da_profiler
{

class ProfilerDumpServer
{
  vector<ProfilerDumpClient *> clients;
  mutable std::recursive_mutex clientsLock;

  vector<unique_ptr<Dump>> dumps;
  mutable std::mutex dumpsLock;

  RingBuffer<uint64_t> frameTimes;
  std::atomic<bool> liveReporting;
  std::atomic<uint32_t> avilableFrames;

  UserSettings settings;
  mutable std::mutex settingsLock;
  int stop = 0;
  mutable std::mutex threadLock;

  mutable std::mutex pluginsLock;
  hash_map<string, bool> plugins;
  int pluginsGen = 0;

public:
  ProfilerDumpServer()
  {
    settings.generation = ~0u;
    liveReporting = false;
    avilableFrames = 0;
  }
  ~ProfilerDumpServer() { shutdown(); }
  void addDump(unique_ptr<Dump> &&dump)
  {
    std::lock_guard<std::mutex> lock(dumpsLock);
    dumps.emplace_back(move(dump));
  }
  void sendPlugins(hash_map<string, bool> &&a)
  {
    std::lock_guard<std::mutex> lock(pluginsLock);
    plugins = move(a);
    pluginsGen++;
  }
  void setSettings(const UserSettings &s)
  {
    std::lock_guard<std::mutex> lock(settingsLock);
    settings = s;
  }
  void addClient(ProfilerDumpClient *client)
  {
    if (!client)
      return;
    std::lock_guard<std::recursive_mutex> lock(clientsLock);
    clients.emplace_back(client);
    sort(clients.begin(), clients.end(), [](auto &a, auto &b) { return a->priority() > b->priority(); });
  }
  bool removeClient(ProfilerDumpClient *client)
  {
    if (!client)
      return false;
    std::lock_guard<std::recursive_mutex> lock(clientsLock);
    auto ci = find(clients.begin(), clients.end(), client);
    if (ci == clients.end())
      return false;
    clients.erase(ci);
    client->removeDumpClient();
    return true;
  }
  bool removeClient(const char *name)
  {
    if (!name)
      return false;
    std::lock_guard<std::recursive_mutex> lock(clientsLock);
    auto ci = find_if(clients.begin(), clients.end(), [&](auto &c) { return strcmp(c->getDumpClientName(), name) == 0; });
    if (ci == clients.end())
      return false;
    auto client = *ci;
    clients.erase(ci);
    client->removeDumpClient();
    return true;
  }
  void shutdown()
  {
    interlocked_release_store(stop, 1);
    std::lock_guard<std::mutex> lock(threadLock); // ensure thread has stopped
    {
      std::lock_guard<std::recursive_mutex> lock(clientsLock);
      for (auto &c : clients)
        c->removeDumpClient();
      clients.clear();
    }
    {
      std::lock_guard<std::mutex> lock(dumpsLock);
      dumps.clear();
    }
  }

  void update()
  {
    unique_ptr<Dump> dump;
    {
      std::lock_guard<std::mutex> lock(dumpsLock);
      if (!dumps.empty())
      {
        move_swap(dump, dumps.front());
        dumps.erase(dumps.begin()); // sorted
      }
    }
    UserSettings set;
    {
      std::lock_guard<std::mutex> lock(settingsLock);
      set = settings;
    }
    std::lock_guard<std::recursive_mutex> lock(clientsLock);
    bool nextLiveReporting = false;
    for (auto &c : clients)
    {
      nextLiveReporting |= c->reportsLiveFrame();
      if (c->updateDumpClient(set) == DumpProcessing::Stop)
        break;
    }
    if (liveReporting)
    {
      decltype(plugins) pluginsToSend;
      int currentPluginsGen;
      {
        std::lock_guard<std::mutex> lock(pluginsLock);
        currentPluginsGen = pluginsGen;
        pluginsToSend = plugins;
      }

      uint64_t ticks[256];
      if (size_t cnt = frameTimes.read_front(ticks, sizeof(ticks) / sizeof(ticks[0]))) // we do that only once, instead of while, to
                                                                                       // prevent infinite loop
      {
        uint32_t available = avilableFrames;
        for (auto &c : clients)
          c->liveFrame(ticks, cnt, available);
      }
      for (auto &c : clients)
        c->sendPlugins(pluginsToSend, currentPluginsGen);
    }
    liveReporting = nextLiveReporting;
    if (dump)
      for (auto &c : clients)
      {
        if (c->process(*dump.get()) == DumpProcessing::Stop)
          break;
      }
  }
  static void thread_update(ProfilerDumpServer &server, const function<bool()> &is_terminating)
  {
    std::lock_guard<std::mutex> lock(server.threadLock); // ensure thread has stopped
    interlocked_release_store(server.stop, 0);
    while (!is_terminating() && !interlocked_acquire_load(server.stop))
    {
      server.update();
      const int periodMs = (interlocked_relaxed_load(active_mode) & EVENTS) ? 50 : 100;
      sleep_msec(periodMs);
    }
    // Flush dumps that might've been submitted during our last sleep
    server.update();
  }
  void start()
  {
    interlocked_release_store(stop, 1);
    std::lock_guard<std::mutex> lock(threadLock); // ensure thread has stopped
    execute_in_new_thread([&](function<bool()> &&is_terminating) { thread_update(*this, is_terminating); }, "profiler_dump_thread",
      128 << 10);
  }
  void reportFrameTime(uint64_t ticks, uint32_t available)
  {
    if (liveReporting.load(std::memory_order_relaxed))
    {
      frameTimes.push_back(ticks);
      avilableFrames = available;
    }
  }
};

static ProfilerDumpServer dump_server;

void start_dump_server() { dump_server.start(); }
void add_dump_client(ProfilerDumpClient *c) { dump_server.addClient(c); }
void report_frame_time(uint64_t ticks, uint32_t available) { dump_server.reportFrameTime(ticks, available); }
void add_dump(unique_ptr<Dump> &&d) { dump_server.addDump(move(d)); }
void send_settings(const UserSettings &d) { dump_server.setSettings(d); }
void send_plugins(hash_map<string, bool> &&a) { dump_server.sendPlugins(move(a)); }
void shutdown_dump_server() { dump_server.shutdown(); }
bool remove_dump_client(ProfilerDumpClient *c) { return dump_server.removeClient(c); }
bool remove_dump_client_by_name(const char *c) { return dump_server.removeClient(c); }

} // namespace da_profiler
