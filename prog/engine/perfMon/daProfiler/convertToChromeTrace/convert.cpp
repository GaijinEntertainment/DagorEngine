#include <string.h>
#include <stdio.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_basePath.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include "../daProfilerDumpUtils.h"

using namespace da_profiler;

static int show_usage();

template <typename ResponseProcessor>
inline void process_file_dump(IGenLoad &load_cb_no_header, ResponseProcessor &proc)
{
  if (read_responses(load_cb_no_header, [&](IGenLoad &cb, const DataResponse &resp) {
        ResponseProcessorResult ret = proc(cb, resp);
        if (ret == ResponseProcessorResult::Stop)
          return ReadResponseCbStatus::Stop;
        if (ret == ResponseProcessorResult::Copy)
          return ReadResponseCbStatus::Skip;
        return ReadResponseCbStatus::Continue;
      }) != ReadResponseStatus::Ok)
    printf("can't process dump\n");
}

template <typename Cb>
static bool read_dump(IGenLoad &load_cb, Cb proc)
{
  DumpHeader header;
  if (load_cb.tryRead(&header, sizeof(header)) != sizeof(header) || header.magic != DumpHeader::MAGIC)
    return false;
  if (header.flags & DumpHeader::Zlib)
  {
    printf("compressed\n");
    ZlibLoadCB zlcb(load_cb, INT_MAX);
    process_file_dump(zlcb, proc);
  }
  else if (header.flags & DumpHeader::Reserved0)
  {
    printf("unsupported zip stream. Re-save with a new daProfiler.exe\n");
    return false;
  }
  else
    process_file_dump(load_cb, proc);
  return true;
};

inline eastl::string replace_slashes(eastl::string &s)
{
  for (auto &c : s)
    if (c == '\\')
      c = '/';
  return s;
}

inline eastl::string read_short_string_str(IGenLoad &stream)
{
  uint32_t length = read_vlq_uint(stream);
  eastl::string r;
  r.resize(length);
  if (length)
    stream.read(&r[0], length);
  r[length] = 0;
  return r;
};

int _cdecl main(int argc, char **argv)
{
  if (argc < 2)
    return show_usage();
  eastl::string dest;
  if (argc > 2)
    dest = argv[2];
  else
    dest = eastl::string(argv[1]) + ".json";
  dd_add_base_path("");
  FullFileLoadCB lcb(argv[1]);
  if (!lcb.fileHandle)
  {
    printf("can't open file <%s>\n", argv[1]);
    return 1;
  }
  FILE *destF = fopen(dest.c_str(), "w");
  if (!destF)
  {
    printf("can't open file <%s>\n", dest.c_str());
    return 1;
  }

  fprintf(destF, "{ \"traceEvents\": [\n");
  double tickToNs = 1e6;
  int64_t first = 0;
  SymbolsCache symbols;
  struct Desc
  {
    eastl::string name, file;
    int lineNo = 0;
  };
  Desc empty;
  eastl::vector<Desc> descs;
  struct Thread
  {
    eastl::string name;
    int64_t tid;
    int mask;
  };
  eastl::vector<Thread> threads;
  int pid = 1, mainThreadIndex = 0;
  for (;;)
  {
    bool hasDescBoard = false;
    if (
      !read_dump(lcb, [&](IGenLoad &cb, const DataResponse &resp) {
        if (resp.type == DataResponse::FrameDescriptionBoard)
        {
          const int board = cb.readInt();
          int64_t freq = cb.readInt64();
          tickToNs = freq / 1e6;
          cb.readInt64(); // orig
          cb.readInt();   // prec
          cb.readInt64(); // start
          cb.readInt64(); // end
          auto readThreads = [&]() {
            int threadCount = cb.readInt();
            threads.clear();
            for (int i = 0; i < threadCount; ++i)
            {
              int64_t tid = cb.readInt64();
              int pid = cb.readInt();
              eastl::string name = read_short_string_str(cb);
              int maxD = cb.readInt();
              int prio = cb.readInt();
              int mask = cb.readInt();
              ;
              threads.emplace_back(Thread{move(name), tid, mask});
            }
          };
          readThreads();
          int fibers = cb.readInt();
          mainThreadIndex = cb.readInt();
          descs.clear();
          for (;;)
          {
            int flags = cb.readIntP<1>();
            if (flags == 0xFF)
              break;
            eastl::string name = replace_slashes(read_short_string_str(cb));
            eastl::string file = replace_slashes(read_short_string_str(cb));
            int line = cb.readInt();
            uint32_t color = read_vlq_uint(cb);
            descs.push_back(Desc{move(name), move(file), line});
          }

          int mode = cb.readInt(); // mode, not used
          int proc = cb.readInt();
          // thread descs
          readThreads(); // for whatever reasons, save twice

          pid = cb.readInt();
          int cores = cb.readInt();
          return ResponseProcessorResult::Skip;
        }
        else if (resp.type == DataResponse::EventFrame)
        {
          int board = read_vlq_uint(cb);
          int thread_index = read_vlq_int(cb);
          int fiber_index = read_vlq_int(cb);
          int type = read_vlq_int(cb);
          int64_t tid = threads[thread_index].tid;
          while (true)
          {
            int depth = read_vlq_int(cb);
            if (depth < 0)
              break;
            int desc = read_vlq_uint(cb);
            auto &d = desc < descs.size() ? descs[desc] : empty;
            uint64_t start = cb.readInt64();
            uint64_t end = cb.readInt64();
            double duration = (end - start) / tickToNs;
            fprintf(destF,
              "{\"name\":\"%s\", \"ph\":\"X\", \"ts\":%f, \"dur\":%f, \"pid\":%d, \"tid\":%lld, \"args\":{\"src_file\":\"%s,%d\"}},\n",
              d.name.c_str(), start / tickToNs, duration, pid, (long long)tid, d.file.c_str(), d.lineNo);
          }
          cb.readInt64(); // start
          cb.readInt64(); // end
          return ResponseProcessorResult::Skip;
        }
        else if (resp.type == DataResponse::CallstackPack)
        {
          return ResponseProcessorResult::Copy; // I can save callstacks, but I can't see them in chrome trace
          const int board = cb.readInt();
          for (;;)
          {
            uint64_t count = cb.readInt64();
            if (count == ~0ULL)
              break;
            int64_t tick = cb.readInt64();
            int64_t tid = cb.readInt64();
            if (tid == 15468)
              tid = 13;
            fprintf(destF, "{\"name\": \"sample\", \"ph\": \"i\", \"ts\": %f, \"pid\": %d, \"tid\": %lld, \"s\": \"t\", \"stack\": [",
              tick / tickToNs, pid, (long long)tid);
            uint64_t stacks[256];
            G_ASSERT(count < 256);
            cb.read(stacks, sizeof(int64_t) * count);
            for (uint32_t j = 0; j < count; ++j) // store addresses. that should not be nesessary, we better establish communications
                                                 // with server or save minidump if not live
            {
              uint64_t addr = stacks[count - 1 - j];
              fprintf(destF, "%s\"0x%llx\"", j == 0 ? "" : ",", (long long)addr);
            }
            fprintf(destF, "]},\n");
          }
          return ResponseProcessorResult::Skip; // already saved
        }
        else if (resp.type == DataResponse::CallstackDescriptionBoard)
        {
          int board = cb.readInt();
          read_modules(cb, symbols);
          read_symbols(cb, symbols);
          return ResponseProcessorResult::Skip;
        }
        else
          return ResponseProcessorResult::Copy;
      }))
      break;
  }
  for (auto &t : threads)
  {
    fprintf(destF,
      "{\"name\": \"thread_name\", \"cat\":\"__metadata\", \"ph\": \"M\", \"ts\":0, "
      "\"pid\": %d, \"tid\": %lld, \"args\":{\"name\":\"%s\"}}%s\n",
      pid, (long long)t.tid, t.name.c_str(), &t == &threads.back() ? "" : ",");
  }
  fprintf(destF, "],\n"); // traceEvents
  fprintf(destF, "\"displayTimeUnit\": \"ns\"\n");
  fprintf(destF, "}\n");
  fclose(destF);

  return 0;
}

static int show_usage()
{
  printf("usage: source-path.dap [dest-path.json]\n");
  return 1;
}
