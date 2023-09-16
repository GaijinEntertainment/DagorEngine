#pragma once
#include <stdint.h>

#include "stl/daProfilerString.h"  //for plugins
#include "stl/daProfilerHashmap.h" //for plugins
#include "stl/daProfilerFwdStl.h"

namespace da_profiler
{

struct Dump;
struct UserSettings;
enum class DumpProcessing
{
  Stop,
  Continue
};

class ProfilerDumpClient
{
public:
  virtual DumpProcessing process(const Dump &dump) = 0;
  virtual DumpProcessing updateDumpClient(const UserSettings &s) = 0;
  virtual const char *getDumpClientName() const = 0;
  virtual int priority() const = 0;
  virtual void removeDumpClient() = 0;
  virtual void liveFrame(const uint64_t * /*frame_times*/, uint32_t /*count*/, uint32_t /*available_for_dump*/) {}
  virtual bool reportsLiveFrame() const
  {
    return false;
  } // if client can do anything useful with runtime updating (i.e. online reporting)
  virtual void sendPlugins(const hash_map<string, bool> &, uint32_t /*generation*/) {}
  virtual ~ProfilerDumpClient() {}
};

void start_dump_server();
void add_dump_client(ProfilerDumpClient *);
bool remove_dump_client(ProfilerDumpClient *);
bool remove_dump_client_by_name(const char *);
void add_dump(unique_ptr<Dump, default_delete<Dump>> &&d);
void shutdown_dump_server();

void report_frame_time(uint64_t ticks, uint32_t available_for_dump);
void send_plugins(hash_map<string, bool> &&a);

} // namespace da_profiler