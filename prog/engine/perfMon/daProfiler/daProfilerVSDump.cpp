// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerDumpServer.h"
#include "daProfilerSamplingUtils.h"
#include "stl/daProfilerStl.h"
#include "dumpLayer.h"
#include <time.h>
#include <hash/wyhash.h>

// fileserver / another dump saving server
// game send dumps to servers, servers process them
//  works on a synchronious file (but called from thread)

namespace da_profiler
{

inline char *u64_to_hex_string(uint64_t val, char *str)
{
  static const char *hex_remap = "0123456789abcdef";
  if (val)
  {
    while (!(val & 0xF000000000000000ULL))
      val <<= 4;
  }
  do
  {
    *(str++) = hex_remap[val >> 60ULL];
    val <<= 4ull;
  } while (val);
  return str;
}
inline char *u64_to_hex_string_0x(uint64_t val, char *str)
{
  str[0] = '0';
  str[1] = 'x';
  return u64_to_hex_string(val, str + 2);
}

class VerySleepyDumpServer final : public ProfilerDumpClient
{
public:
  static VerySleepyDumpServer *instance;
  unique_ptr<char[]> path;
  VerySleepyDumpServer(const char *path_)
  {
    if (path_)
    {
      const size_t len = strlen(path_) + 1;
      path = make_unique<char[]>(len);
      memcpy(path.get(), path_, len);
    }
    instance = this;
  }
  ~VerySleepyDumpServer() { instance = nullptr; }
  const char *getDumpClientName() const override { return "VerySleepyDump"; }
  bool saveCallstacks(const Dump &dump, const char *dir, tm *ctime)
  {
    char buf[1024];
    buf[0] = 0;
    HashedKeySet<uint64_t, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> addressSet;
    double duration = (dump.frames.back().end - dump.frames.front().start) / double(cpu_frequency());
    uint64_t samples = 0;
    {
#define VS_VERSION "0.92"
      snprintf(buf, sizeof(buf), "%s/Version %s required", dir, VS_VERSION);
      FullFileSaveCB cb(buf);
      if (!cb.fileHandle) // cant' open file
        return false;
      cb.write(VS_VERSION "\n", sizeof(VS_VERSION) + 1);
    }
    {
      snprintf(buf, sizeof(buf), "%s/Callstacks.txt", dir);
      FullFileSaveCB cb(buf);
      if (!cb.fileHandle) // cant' open file
        return false;
      vector<uint64_t> callFrames; // size + callstack
      HashedKeyMap<uint64_t, uint64_t> samplesCountMap;
      vector<ProfilerData::LastRevCallStack> threadRevStacks;
      HashedKeyMap<uint64_t, uint16_t> tidIndices;

      for (const uint16_t *b = dump.stacks.begin(), *i = b, *end = dump.stacks.end(); i < end;)
      {
        ++samples;
        const uint32_t newCount = sampling_saved_cs_count(i), sameCount = sampling_same_cs_count(i);
        const uint64_t totalSize = newCount + sameCount;
        if (totalSize > countof(ProfilerData::LastRevCallStack::revStack))
        {
          report_logerr("too big stack");
          break;
        }
        const uint64_t tid = sampling_saved_tid(i);
        const uint16_t *cs_p48 = sampling_saved_cs(i);
        auto emp = tidIndices.emplace_if_missing(tid);
        if (emp.second)
        {
          *emp.first = threadRevStacks.size();
          threadRevStacks.push_back();
        }
        ProfilerData::LastRevCallStack &tidRevStack = threadRevStacks[*emp.first];
        if (sameCount > tidRevStack.currentStackSize)
        {
          report_logerr("invalid stack");
          break;
        }
        for (uint32_t newStackI = 0; newStackI < newCount; ++newStackI, cs_p48 += 3)
          tidRevStack.revStack[sameCount + newStackI] = to_64_bit_pointer(cs_p48);
        tidRevStack.currentStackSize = totalSize;

        if (tidRevStack.currentStackSize)
        {
          const uint64_t hash = wyhash(tidRevStack.revStack, sizeof(uint64_t) * tidRevStack.currentStackSize, tid);
          if (uint64_t *val = samplesCountMap.findVal(hash, [&](uint64_t index_count) {
                const uint64_t *cs = callFrames.data() + uint32_t(index_count);
                return *cs == tidRevStack.currentStackSize &&
                       memcmp(cs + 2, tidRevStack.revStack, sizeof(uint64_t) * tidRevStack.currentStackSize) == 0;
              }))
            *val += (1ULL << 32ULL);
          else
          {
            samplesCountMap.emplace(hash, callFrames.size());
            callFrames.push_back(tidRevStack.currentStackSize);
            callFrames.push_back(tid);
            callFrames.insert(callFrames.end(), tidRevStack.revStack, tidRevStack.revStack + tidRevStack.currentStackSize);
          }
        }
        i += sampling_allocated_words(newCount);
      }

      samplesCountMap.iterate([&](uint64_t, uint64_t index_count) {
        const uint64_t *cs = callFrames.data() + uint32_t(index_count);
        const uint32_t samples_count = (index_count >> 32ull) + 1;
        uint64_t tid = cs[1];
        uint32_t count = (uint32_t)cs[0];
        cs += 2;
        for (uint32_t j = 0; j < count; ++j)
        {
          const uint64_t addr = cs[count - 1 - j]; // reversed callstack
          addressSet.insert(addr);
          char *e = u64_to_hex_string_0x(addr, buf);
          *e = j < count - 1 ? ' ' : '\n';
          cb.write(buf, e + 1 - buf);
        }
        snprintf(buf, sizeof(buf), "%llu %g\n", (unsigned long long)tid, samples_count * duration / samples);
        cb.write(buf, strlen(buf));
      });
    }
    {
      snprintf(buf, sizeof(buf), "%s/Symbols.txt", dir);
      FullFileSaveCB cb(buf);
      if (!cb.fileHandle) // cant' open file
        return false;
      if (can_resolve_symbols())
      {
        auto &symbols = the_profiler.symbolsCache;
        std::lock_guard<std::mutex> lock(symbols.lock);
        symbols.initModules();
        for (auto addr : addressSet)
        {
          const uint32_t index = symbols.addSymbol(addr);
          char *e = u64_to_hex_string_0x(addr, buf);
          *e = ' ';
          cb.write(buf, e + 1 - buf);

          if (index == ~0u) // unresolved
          {
            snprintf(buf, sizeof(buf), "\"\" \"[%llX]\" \"\" 0\n", (unsigned long long)addr);
            cb.write(buf, strlen(buf));
          }
          else
          {
            auto &symb = symbols.symbols[index];
            snprintf(buf, sizeof(buf), "\"my.exe\" \"%s\" \"", symbols.strings.getStringFromId(symb.fun));
            cb.write(buf, strlen(buf));
            const char *filePath = symbols.strings.getStringFromId(symb.file);
            char *to = buf;
            for (char *end = buf + sizeof(buf) - 2; *filePath && to < end; to++, filePath++)
            {
              *to = *filePath;
              if (*filePath == '\\')
                *++to = *filePath;
            }
            cb.write(buf, to - buf);
            snprintf(buf, sizeof(buf), "\" %d\n", symb.line);
            cb.write(buf, strlen(buf));
          }
        }
      }
      else
      {
        for (auto addr : addressSet)
        {
          snprintf(buf, sizeof(buf), " 0x%llx \"\" \"[%llX]\" \"\" 0\n", (unsigned long long)addr, (unsigned long long)addr);
          cb.write(buf, strlen(buf));
        }
      }
    }
    {
      snprintf(buf, sizeof(buf), "%s/Threads.txt", dir);
      FullFileSaveCB cb(buf);
      if (!cb.fileHandle) // cant' open file
        return false;
      for (auto &thread : dump.threads)
      {
        snprintf(buf, sizeof(buf), "%llu\n", (unsigned long long)thread.threadId);
        cb.write(buf, strlen(buf));
        uint32_t cf;
        const char *name = the_profiler.descriptions.get(thread.description, cf);
        snprintf(buf, sizeof(buf), "%s\n", name ? name : "-");
        cb.write(buf, strlen(buf));
      }
    }
    {
      snprintf(buf, sizeof(buf), "%s/Stats.txt", dir);
      FullFileSaveCB cb(buf);
      if (!cb.fileHandle) // cant' open file
        return false;
      snprintf(buf, sizeof(buf), "Filename: %s\n", get_executable_name());
      cb.write(buf, strlen(buf));
      snprintf(buf, sizeof(buf), "Duration: %g\n", duration);
      cb.write(buf, strlen(buf));
      strftime(buf, sizeof(buf), "Date: %a %b %H:%M:%S %Y\n", ctime);
      cb.write(buf, strlen(buf));
      snprintf(buf, sizeof(buf), "Samples: %lld\n", (unsigned long long)samples);
      cb.write(buf, strlen(buf));
    }
    return true;
  }
  DumpProcessing process(const Dump &dump) override
  {
    if (dump.stacks.empty())
      return DumpProcessing::Continue;
    char buf[256];
    buf[0] = 0;
    time_t rawtime = (time_t)global_timestamp();
    tm *t = localtime(&rawtime);
    snprintf(buf, sizeof(buf), "%s%04d.%02d.%02d-%02d.%02d.%02d.sleepy", path.get() ? path.get() : "", 1900 + t->tm_year,
      t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    dd_mkdir(buf);
    if (!saveCallstacks(dump, buf, t))
    {
      logerr("can't save very sleepy stacks&symbols to %s", buf);
      return DumpProcessing::Continue;
    }
    return DumpProcessing::Continue;
  }
  DumpProcessing updateDumpClient(const UserSettings &) override { return DumpProcessing::Continue; }
  int priority() const override { return 0; }
  void removeDumpClient() override { delete this; }
};
VerySleepyDumpServer *VerySleepyDumpServer::instance = nullptr;

bool stop_very_sleepy_dump_server() { return remove_dump_client(VerySleepyDumpServer::instance); }

void start_very_sleepy_dump_server(const char *path)
{
  stop_very_sleepy_dump_server();
  add_dump_client(new VerySleepyDumpServer(path));
}

}; // namespace da_profiler