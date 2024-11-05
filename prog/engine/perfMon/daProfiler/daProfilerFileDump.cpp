// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "stl/daProfilerString.h"
#include "daProfilerDumpServer.h"
#include "dumpLayer.h"
#include <time.h>

// fileserver / another dump saving server
// game send dumps to servers, servers process them
//  works on a synchronious file (but called from thread)

namespace da_profiler
{

static const char *names[(int)Dump::Type::Unknown + 1] = {"ring", "capture", "spikes", "profile"};

class FileDumpServer final : public ProfilerDumpClient
{
public:
  string lastPaths[countof(names)] = {};
  string path;
  FileDumpServer(const char *path_)
  {
    if (path_)
      path = path_;
  }
  ~FileDumpServer() { close(); }
  void close()
  {
    //? write finishes?
  }
  // ProfilerDumpClient
  const char *getDumpClientName() const override { return path.c_str(); }
  DumpProcessing process(const Dump &dump) override
  {
    char buf[256];
    buf[0] = 0;
    time_t rawtime = global_timestamp();
    tm *t = localtime(&rawtime);
    const int dumpType = min((uint32_t)dump.type, (uint32_t)countof(names) - 1);
    if (!dump.appendToCurrent || lastPaths[dumpType].empty())
      lastPaths[dumpType].sprintf("%s%s-%04d.%02d.%02d-%02d.%02d.%02d.dap", path.c_str(), names[dumpType], 1900 + t->tm_year,
        t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    FullFileSaveCB cb(lastPaths[dumpType].c_str(), DF_APPEND);
    if (!cb.fileHandle) // cant' open file
      return DumpProcessing::Continue;
    cb.seektoend(); // allows appending

    ZlibSaveCB zcb(cb, 1 /*best speed*/);
    DumpHeader header;
    header.flags |= DumpHeader::Zlib; // actually zlib
    cb.write((const char *)&header, sizeof(header));
    the_profiler.saveDump(zcb, dump);
    // the_profiler.finish(zcb);//we dont' write latest null, so we can continue to add
    zcb.finish();

    return DumpProcessing::Continue;
  }
  DumpProcessing updateDumpClient(const UserSettings &) override { return DumpProcessing::Continue; }
  int priority() const override { return 1; }
  void removeDumpClient() override { delete this; }
};

bool stop_file_dump_server(const char *path) { return remove_dump_client_by_name(path ? path : "null file"); }
bool start_file_dump_server(const char *path)
{
  add_dump_client(new FileDumpServer(path));
  return true;
}

}; // namespace da_profiler