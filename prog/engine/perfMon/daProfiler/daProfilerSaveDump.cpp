// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>
#include <ioSys/dag_memIo.h>
#include "daGpuProfiler.h"
#include "daProfilerInternal.h"
#include "daProfilerSamplingUtils.h"
#include "daProfilePlatform.h"
#include "dumpLayer.h"
#include "stl/daProfilerAlgorithm.h"

namespace da_profiler
{

Dump::Dump(Type tp, bool append_to_current_if_exist) : type(tp), appendToCurrent(append_to_current_if_exist)
{
  dumpAtMs = time_since_launch_msec();
  dateTime = (uint64_t)global_timestamp();
}

static inline uint32_t tag_len(const char *s, uint32_t max_len)
{
  uint32_t argsCount = 0;
  for (uint32_t i = 0; i < max_len; ++i, ++s)
  {
    if (*s == 0)
      return argsCount == 0 ? i : min(i + 1 + argsCount * 4, max_len);
    if (*s == '%' && i < max_len - 1 && s[1] != '%')
      argsCount++;
  }
  return max_len;
}
static DynamicMemGeneralSaveCB &newStream(DynamicMemGeneralSaveCB &s)
{
  s.setsize(0);
  return s;
}
inline void write_vlq_uint(IGenSave &c, uint32_t i)
{
  do
  {
    const uint32_t nextI = i >> 7;
    const uint8_t b = (i & 0x7F) | (nextI ? 0x80 : 0);
    c.write(&b, 1);
    i = nextI;
  } while (i);
}

inline void write_vlq_int(IGenSave &c, int i_)
{
  uint32_t i = i_ < 0 ? -i_ : i_;
  uint32_t nextI = i >> 6;
  uint8_t b = (i & 0x3F) | (nextI ? 0x80 : 0) | (i_ < 0 ? 0x40 : 0);
  c.write(&b, 1);
  for (i = nextI; i; i = nextI)
  {
    nextI = i >> 7;
    uint8_t b = (i & 0x7F) | (nextI ? 0x80 : 0);
    c.write(&b, 1);
  }
}

static void write_string(IGenSave &stream, const char *val, const uint32_t length)
{
  stream.writeInt(length);
  if (length > 0)
    stream.write(val, length);
}
static void write_string(IGenSave &stream, const char *val) { write_string(stream, val, val == nullptr ? 0 : (uint32_t)strlen(val)); }

static void write_short_string(IGenSave &stream, const char *val, const uint32_t length)
{
  write_vlq_uint(stream, length);
  if (length)
    stream.write(val, length);
}

static void write_short_string(IGenSave &stream, const char *val)
{
  write_short_string(stream, val, val == nullptr ? 0 : (uint32_t)strlen(val));
}

static void write_duration(IGenSave &stream, uint64_t start, uint64_t end)
{
  stream.writeInt64(start);
  stream.writeInt64(end);
}

inline void write(IGenSave &stream, const EventData &d)
{
  write_duration(stream, d.start, d.end);
  write_vlq_uint(stream, d.description);
  write_vlq_uint(stream, d.depth);
}

static void write_stoppable(IGenSave &stream, const EventData &d)
{
  write_vlq_int(stream, d.depth);
  write_vlq_uint(stream, d.description);
  write_duration(stream, d.start, d.end);
}

void response_finish(IGenSave &cb)
{
  // send finish
  send_data(cb, DataResponse::NullFrame, nullptr, 0);
  cb.flush();
}

void response_heartbeat(IGenSave &cb)
{
  // send finish
  send_data(cb, DataResponse::Heartbeat, nullptr, 0);
  cb.flush();
}

void response_handshake(IGenSave &cb, uint16_t compression)
{
  DynamicMemGeneralSaveCB stream(tmpmem_ptr(), 512); // we could avoid mem allocation. but it happens like once per frame
  stream.writeInt(0 | (compression << 16));          // todo: status
  write_string(stream, get_current_platform_name());
  write_string(stream, get_current_host_name());
  send_data(cb, DataResponse::Handshake, stream);
}

void response_live_frames(IGenSave &cb, uint32_t available, const uint64_t *times, uint32_t count)
{
  const size_t total_size = sizeof(available) + sizeof(count) + count * sizeof(float);
  DataResponse response(DataResponse::ReportLiveFrameTime, total_size);

  cb.write(&response, sizeof(response));
  cb.writeInt(available);
  cb.writeInt(count);
  double ticksToMsec = 1000. / cpu_frequency();
  for (uint32_t i = 0; i < count; ++i, ++times)
  {
    float fr = float(double(*times) * ticksToMsec);
    cb.write(&fr, sizeof(fr));
  }
  cb.flush();
}

void response_plugins(IGenSave &cb, const hash_map<string, bool> &plugins)
{
  size_t totalSize = sizeof(int);
  for (auto &p : plugins)
    totalSize += sizeof(int) + p.first.length() + 1;

  DataResponse response(DataResponse::Plugins, totalSize);

  cb.write(&response, sizeof(response));
  cb.writeInt(plugins.size());
  for (auto &p : plugins)
  {
    cb.writeInt(p.first.length());
    cb.write(p.first.data(), p.first.length());
    cb.writeIntP<1>(p.second ? 1 : 0);
  }
  cb.flush();
}

void write_modules(IGenSave &symbolsStream, SymbolsCache &symbols)
{
  for (auto &m : symbols.modules)
  {
    symbolsStream.writeInt64(m.base);
    symbolsStream.writeInt64(m.size);
    write_short_string(symbolsStream, m.name);
  }
  symbolsStream.writeInt64(0); // end marker
}

inline void write_symbol(IGenSave &symbolsStream, uint64_t addr, SymbolsCache &symbols)
{
  const uint32_t index = symbols.addSymbol(addr);
  symbolsStream.writeInt64(addr);
  if (index == ~0u)
  {
    symbolsStream.writeInt(-1);
    return;
  }
  auto &symb = symbols.symbols[index];
  symbolsStream.writeInt(symb.line);
  write_short_string(symbolsStream, symbols.strings.getStringFromId(symb.fun));
  write_short_string(symbolsStream, symbols.strings.getStringFromId(symb.file));
}

void write_symbols(IGenSave &symbolsStream, const SymbolsSet &symbolsSet, SymbolsCache &symbols)
{
  for (auto addr : symbolsSet)
    write_symbol(symbolsStream, addr, symbols);
  symbolsStream.writeInt64(0); // end marker
}

void save_dump(IGenSave &cb, const Dump &dump, const ProfilerDescriptions &descriptions, SymbolsCache &symbols) // can be called from
                                                                                                                // threads!
{
  if (dump.frames.empty())
    return;
  const auto threadsBegin = dump.threads.begin(), threadsEnd = dump.threads.end();

  DynamicMemGeneralSaveCB stream(tmpmem_ptr(), 0, 1 << 20); // expand with 1mb steps
  const uint32_t activeFrames = dump.frames.approximateSize();
  int64_t timeGpuEnd = -1;
  uint64_t timeGpuStart = 0;
  const bool hasGpu = dump.frames.forEachStop([&](const FrameInfo &frame) { return (timeGpuStart = frame.gpuStart) != ~0ULL; }) &&
                      dump.gpuEvents.forEachReverseStop([&](const GpuEventData &event) {
                        if (event.end == ~0ull && event.start == ~0ull)
                          return false;
                        timeGpuEnd = (event.end != ~0ull) ? event.end : event.start;
                        return true;
                      });

  if (dump.uniqueProfileRunName)
  {
    DynamicMemGeneralSaveCB &uniqueName = newStream(stream);
    write_short_string(uniqueName, dump.uniqueProfileRunName);
    send_data(cb, DataResponse::UniqueName, uniqueName);
  }

  {
    DynamicMemGeneralSaveCB &summary = newStream(stream);
    // dumpSummary
    summary.writeInt(dump.board);
    double frequency = (double)cpu_frequency();
    summary.writeInt(activeFrames);
    dump.frames.forEach([&](const FrameInfo &frame) { summary.writeReal((frame.end - frame.start) * 1000. / frequency); });

    {
      const char *exe = get_executable_name();
      const char *platform = get_current_platform_name();
      write_short_string(summary, platform);
      write_short_string(summary, exe ? exe : "unknown executable");
      const char *cpu = get_cpu_name();
      if (cpu)
      {
        write_short_string(summary, "CPU");
        write_short_string(summary, cpu);
      }
      char buf[256];

      write_short_string(summary, "Dump time");
      time_t rawtime = (time_t)dump.dateTime;
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&rawtime));
      write_short_string(summary, buf);

      write_short_string(summary, "Time passed after launch");
      const uint32_t timeMs = dump.dumpAtMs;
      if (timeMs > 3600000)
        snprintf(buf, sizeof(buf), "%02d:%02d:%2.3f", (timeMs / 1000) / 3600, (timeMs / 1000) / 60, timeMs / 1000.);
      else
        snprintf(buf, sizeof(buf), "%02d:%2.3f", (timeMs / 1000) / 60, timeMs / 1000.);
      write_short_string(summary, buf);

      write_short_string(summary, nullptr); // end marker
    }
    summary.writeInt(0); // attachments...
    send_data(cb, DataResponse::SummaryPack, summary);
  }
  const uint64_t timeStart = dump.frames.front().start;
  const uint64_t timeEnd = dump.frames.back().end;
  const int threadsCount = dump.threads.size() + (hasGpu ? 1 : 0);
  const auto pid = get_current_process_id();
  auto frameTi = find_if(threadsBegin, threadsEnd, [&](auto &ti) { return ti.threadId == dump.frameThreadId; });
  if (frameTi == threadsEnd)
    frameTi = threadsBegin;
  {
    // dump board
    DynamicMemGeneralSaveCB &boardStream = newStream(stream);
    boardStream.writeInt(dump.board);
    boardStream.writeInt64(cpu_frequency());
    boardStream.writeInt64(0); // Origin
    boardStream.writeInt(0);   // Precision

    write_duration(boardStream, timeStart, timeEnd);
    auto saveThreads = [&]() {
      boardStream.writeInt(threadsCount);
      for (auto ti = threadsBegin; ti != threadsEnd; ++ti)
      {
        boardStream.writeInt64(ti->threadId);
        boardStream.writeInt(pid);
        write_short_string(boardStream, descriptions.getName(ti->description));
        boardStream.writeInt(1);                                    // maxDepth
        boardStream.writeInt(0);                                    // priority
        boardStream.writeInt(frameTi == ti ? ThreadMask::Main : 0); // mask
      }
      // GPU
      if (hasGpu)
      {
        boardStream.writeInt64(-1);
        boardStream.writeInt(pid);
        write_short_string(boardStream, descriptions.getName(descriptions.gpu()));
        boardStream.writeInt(1);               // maxDepth
        boardStream.writeInt(0);               // priority
        boardStream.writeInt(ThreadMask::GPU); // mask
      }
    };
    saveThreads();
    boardStream.writeInt(0);                      // fibers;
    boardStream.writeInt(frameTi - threadsBegin); // forcedMainThreadIndex

    {
      descriptions.storage.forEach([&](const EventDescription &d) {
        boardStream.writeIntP<1>(d.getFlags()); // flags
        write_short_string(boardStream, d.name);
        write_short_string(boardStream, d.fileName);
        boardStream.writeInt(d.lineNo);
        write_vlq_uint(boardStream, d.getColor());
      });
      boardStream.writeIntP<1>(0xFF); // finish stream
    }
    boardStream.writeInt(0); // mode, not used
    boardStream.writeInt(0); // todo: processDescs
    // thread descs
    saveThreads(); // for whatever reasons, save twice

    boardStream.writeInt(pid);
    boardStream.writeInt(get_logical_cores());
    send_data(cb, DataResponse::FrameDescriptionBoard, boardStream);
  }
  {
    // frames
    DynamicMemGeneralSaveCB &framesStream = newStream(stream);
    framesStream.writeInt(dump.board);
    {
      // cpu
      write_vlq_int(framesStream, TYPE_CPU);
      write_vlq_uint(framesStream, descriptions.frame());
      framesStream.writeInt64(frameTi->threadId);
      dump.frames.forEach([&](const FrameInfo &frame) { write_duration(framesStream, frame.start, frame.end); });
      write_duration(framesStream, ~0ULL, ~0ULL); // end marker
    }

    if (hasGpu)
    {
      // gpu
      write_vlq_int(framesStream, TYPE_GPU);
      write_vlq_uint(framesStream, descriptions.frame());
      framesStream.writeInt64(~0ULL); // thread id

      int64_t prevFrameGpuStartCpuTicks = -1;
      const int64_t lastGpuTimeInCpuTicks = dump.cpuGpuClock.from2to1(timeGpuEnd);
      dump.frames.forEachStop([&](const FrameInfo &frame) {
        if (frame.gpuStart == ~0ull)
        {
          if (prevFrameGpuStartCpuTicks != -1)
          {
            if (prevFrameGpuStartCpuTicks <= lastGpuTimeInCpuTicks)
              write_duration(framesStream, prevFrameGpuStartCpuTicks, lastGpuTimeInCpuTicks);
            // this is last frame
            return true;
          }
          // looking for first gpu frame
          return false;
        }
        const int64_t frameStart = dump.cpuGpuClock.from2to1(frame.gpuStart);
        if (prevFrameGpuStartCpuTicks != -1)
          write_duration(framesStream, prevFrameGpuStartCpuTicks, frameStart);
        prevFrameGpuStartCpuTicks = frameStart;
        return false;
      });
      write_duration(framesStream, ~0ULL, ~0ULL); // end marker
    }
    write_vlq_int(framesStream, -1); // end marker
    send_data(cb, DataResponse::FramesPack, framesStream);
  }
  {
    // thread cpu&gpu events
    uint64_t cStart = timeStart, cEnd = timeEnd;
    DynamicMemGeneralSaveCB &threadStream = newStream(stream);
    auto sendScoped = [&](int &eventsCount) {
      if (!eventsCount)
        return;
      write_vlq_int(threadStream, -1);
      write_duration(threadStream, cStart, cEnd);
      send_data(cb, DataResponse::EventFrame, threadStream);
      eventsCount = 0;
    };
    auto writeEvent = [&](int &eventsCount, const EventData &data, int thread_index, int type) {
      if (eventsCount == 0)
      {
        cStart = data.start;
        cEnd = data.end;
        newStream(threadStream);
        // writeHeader
        write_vlq_uint(threadStream, dump.board);
        write_vlq_int(threadStream, thread_index);
        write_vlq_int(threadStream, -1); // fiber index
        write_vlq_int(threadStream, type);
      }
      else
      {
        cStart = min(cStart, data.start);
        cEnd = max(cEnd, data.end);
      }
      write_stoppable(threadStream, data);
      ++eventsCount;
    };

    // std::lock_guard<std::mutex> lock(threadsLock);
    for (auto ti = threadsBegin; ti != threadsEnd; ++ti)
    {
      int threadEventsCount = 0;
      const int threadIndex = ti - threadsBegin;
      const int type = frameTi == ti ? TYPE_CPU : TYPE_NONE;
      if (ti->events.empty())
      {
        // fake event, so tool has something to show
        writeEvent(threadEventsCount, EventData{max(timeStart, ti->addedTick), min(timeEnd, ti->removedTick), ti->description, 0},
          threadIndex, type);
      }
      ti->events.forEach([&](const EventData &data) {
        if (data.start == ~0ULL) // not started event
          return;
        if ((threadEventsCount == 0) && (data.depth != 0))
          return;
        const auto end = data.end == ~0ULL ? timeEnd : data.end;
        if (end >= data.start && data.start >= timeStart && timeEnd >= end) // should be if (data.end >= data.start && (data.start <=
                                                                            // timeEnd || timeEnd <= data.end))
        {
          // smaller sends, on each root event
          if (data.depth == 0 && (type != TYPE_NONE || threadEventsCount >= 128)) // group some amount of events, to reduce header
                                                                                  // overhead, but do not group main frames
            sendScoped(threadEventsCount);
          EventData data2 = data;
          data2.end = data.end == ~0ULL ? timeEnd : data.end;
          writeEvent(threadEventsCount, data2, threadIndex, type);
        }
      });
      sendScoped(threadEventsCount);

      {
        // stream line can be send directly, without memory copy
        DynamicMemGeneralSaveCB &tagStream = newStream(stream);
        tagStream.writeInt(dump.board);
        tagStream.writeInt(ti - threadsBegin);
        dump.frames.forEach([&](const FrameInfo &frame) {
          tagStream.writeInt64(frame.start);
          write_vlq_uint(tagStream, descriptions.frameNo());
          tagStream.writeInt(frame.frameNo);
          if (frame.addFrameCount)
          {
            tagStream.writeInt64(frame.start);
            write_vlq_uint(tagStream, descriptions.addFramesCount());
            tagStream.writeInt(frame.addFrameCount);
          }
        });
        tagStream.writeInt64(~0ULL); // frames tags

        tagStream.writeInt64(~0ULL); // ds tags
        if (!ti->stringTags.empty())
        {
          ti->stringTags.forEach([&](const StringTag &data) {
            if (data.end >= timeStart && data.end <= timeEnd)
            {
              tagStream.writeInt64(data.end);
              write_vlq_uint(tagStream, data.description);
              write_short_string(tagStream, data.str, tag_len(data.str, sizeof(data.str)));
            }
          });
        }
        tagStream.writeInt64(~0ULL);
        send_data(cb, DataResponse::TagsPack, tagStream);
      }
    }

    if (hasGpu)
    {
      newStream(stream);
      const ClockCalibration &cpuGpuClock = dump.cpuGpuClock;
      const int gpuThreadIndex = dump.threads.size();
      int gpuEventsCount = 0;
      vector<const GpuEventData *> gpuRoots;
      gpuRoots.reserve(activeFrames);
      dump.gpuEvents.forEachChunk([&](const GpuEventData *begin, const GpuEventData *end) {
        auto last = end - 1;
        for (auto rbegin = begin - 1; last != rbegin; --last)
          if (last->start != ~0ULL)
            break;
        if (last < begin)
          return;
        end = last + 1;
        for (auto start = begin; start != end; ++start)
        {
          auto &data = *start;
          if (data.start == ~0ULL)
            continue;
          if ((gpuEventsCount == 0) && (data.depth != 0))
            continue;
          const auto currentEnd = data.end == ~0ULL ? timeGpuEnd : data.end;
          if (currentEnd >= data.start && data.start >= timeGpuStart) // should be if (data.end >= data.start && (data.start <= timeEnd
                                                                      // || timeEnd <= data.end))
          {
            if (!data.depth)
            {
              sendScoped(gpuEventsCount);
              gpuRoots.push_back(start);
            }
            EventData data2 = (const EventData &)data;
            data2.start = max(int64_t(0), cpuGpuClock.from2to1(data.start)) + gpuEventsCount; // we add gpuEventsCount as ticks to
                                                                                              // avoid low precision counter.
            data2.end = max(data2.start, uint64_t(cpuGpuClock.from2to1(currentEnd) + gpuEventsCount)); // we add gpuEventsCount as
                                                                                                       // ticks to avoid low precision
                                                                                                       // counter. On hundreds of
                                                                                                       // events this is very small
                                                                                                       // number of ticks
            writeEvent(gpuEventsCount, data2, gpuThreadIndex, TYPE_GPU);
          }
        }
      });
      sendScoped(gpuEventsCount);

      { // dump tags
        DynamicMemGeneralSaveCB &tagStream = newStream(stream);
        tagStream.writeInt(dump.board);
        tagStream.writeInt(gpuThreadIndex);
        uint32_t firstFrame = 0;

        for (size_t i = 0, e = gpuRoots.size(); i < e; ++i)
        {
          auto &data = *gpuRoots[i];
          for (; firstFrame < activeFrames - 1; ++firstFrame)
          {
            auto &f = dump.frames[firstFrame + 1];
            if (f.gpuStart != ~0ull && f.gpuStart > data.start)
              break;
          }
          if (firstFrame >= activeFrames)
            break;
          // frame is not after gpu root
          auto &frame = dump.frames[firstFrame];
          if (frame.gpuStart > data.start && (frame.gpuStart != ~0ULL || i != e - 1))
            continue;
          tagStream.writeInt64(max(int64_t(0), cpuGpuClock.from2to1(data.start))); // this is to match gpu events
          write_vlq_uint(tagStream, descriptions.frame());
          tagStream.writeInt(frame.frameNo);
        }
        tagStream.writeInt64(~0ULL); // end marker

        gpuEventsCount = 0;
        dump.gpuEvents.forEachChunk([&](const GpuEventData *begin, const GpuEventData *end) {
          for (auto start = begin; start != end; ++start)
          {
            auto &data = *start;
            if (data.start == ~0ULL)
              continue;
            if ((gpuEventsCount == 0) && (data.depth != 0))
              continue;
            const auto currentEnd = data.end == ~0ULL ? timeGpuEnd : data.end;
            if (currentEnd >= data.start && data.start >= timeGpuStart && timeGpuEnd >= currentEnd) // should be if (data.end >=
                                                                                                    // data.start && (data.start <=
                                                                                                    // timeEnd || timeEnd <= data.end))
            {
              if (!data.depth)
                gpuEventsCount = 0;
              const int64_t time = max(int64_t(0), cpuGpuClock.from2to1(data.start)) + gpuEventsCount; // this is to match gpu events
              if (data.ds.tri)
              {
                tagStream.writeInt64(time);
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_LOCKVIB]);
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_DP]);
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_RT]);
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_PS]); // todo: programs
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_INS]);
                write_vlq_uint(tagStream, data.ds.val[DRAWSTAT_RENDERPASS_LOGICAL]);
                tagStream.writeInt64(data.ds.tri);
              }
              ++gpuEventsCount;
            }
          }
        });

        tagStream.writeInt64(~0ULL); // end marker
        tagStream.writeInt64(~0ULL); // string tags
        send_data(cb, DataResponse::TagsPack, tagStream);
      }
    }
  }
  if (!dump.stacks.empty())
  {
    // call stacks
    DynamicMemGeneralSaveCB &callstacksStream = newStream(stream);
    callstacksStream.writeInt(dump.board);

    vector<ProfilerData::LastRevCallStack> threadRevStacks;
    HashedKeyMap<uint64_t, uint16_t> tidIndices;
    // todo: we can decrease dump size by allowing profiler tool to work with new format directly
    // this will decrease amount of code here, dump size, performance of saving dumps
    for (const uint16_t *i = dump.stacks.begin(), *end = dump.stacks.end(); i != end;)
    {
      const uint32_t newCount = sampling_saved_cs_count(i), sameCount = sampling_same_cs_count(i);
      const uint64_t totalSize = newCount + sameCount;
      if (totalSize > countof(ProfilerData::LastRevCallStack::revStack))
      {
        report_logerr("too big stack");
        break;
      }
      const uint64_t tid = sampling_saved_tid(i);
      const uint16_t *cs_p48 = sampling_saved_cs(i);
      const uint64_t ticks = sampling_saved_ticks(i);
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
      const bool saveAllStacks = false;
      if (saveAllStacks || (ticks >= timeStart && ticks < timeEnd))
      {
        callstacksStream.write(&totalSize, sizeof(uint64_t));
        callstacksStream.write(&ticks, sizeof(uint64_t));
        callstacksStream.write(&tid, sizeof(uint64_t));
        for (uint32_t stackI = 0; stackI < totalSize; ++stackI)
          callstacksStream.write(&tidRevStack.revStack[totalSize - 1 - stackI], sizeof(uint64_t)); // reversed stack
      }
      i += sampling_allocated_words(newCount);
    }
    callstacksStream.writeInt64(~0ULL); // end marker
    send_data(cb, DataResponse::CallstackPack, callstacksStream);

    // write symbols
    SymbolsSet symbolsSet;
    // modules and symbols
    if (can_resolve_symbols())
    {
      for (const uint16_t *i = dump.stacks.begin(), *end = dump.stacks.end(); i != end;)
      {
        // whole loop is not needed if we use symbol deferred resolver from profiler via network or / minidump
        // can be arranged with messages/responses
        const uint32_t count = sampling_saved_cs_count(i);
        const uint16_t *cs_48 = sampling_saved_cs(i);
        for (uint32_t j = 0; j < count; ++j, cs_48 += 3) // store addresses. that should not be nesessary, we better establish
                                                         // communications with server or save minidump if not live
          if (uint64_t addr = to_64_bit_pointer(cs_48))
            symbolsSet.insert(addr);
        i += sampling_allocated_words(count);
      }
    }

    DynamicMemGeneralSaveCB &symbolsStream = newStream(stream);
    {
      std::lock_guard<std::mutex> lock(symbols.lock);
      symbols.initModules();
      symbolsStream.writeInt(dump.board);
      write_modules(symbolsStream, symbols);
      write_symbols(symbolsStream, symbolsSet, symbols);
    }

    send_data(cb, DataResponse::CallstackDescriptionBoard, symbolsStream);

    DynamicMemGeneralSaveCB &uniqueEventStream = newStream(stream);
    {
      uniqueEventStream.writeInt(dump.board);
      dump.uniqueEvents.forEach([&](auto i) {
        uniqueEventStream.writeInt64(i->totalOccurencies);
        uniqueEventStream.writeInt64(i->minTicks);
        uniqueEventStream.writeInt64(i->maxTicks);
        uniqueEventStream.writeInt64(i->totalTicks);
        write_vlq_uint(uniqueEventStream, i->desc);
        write_vlq_uint(uniqueEventStream,
          dump.uniqueEventsFrames > i->startFrame ? uint32_t(dump.uniqueEventsFrames - i->startFrame) : 0);
      });
      uniqueEventStream.writeInt64(~uint64_t(0));
    }
    send_data(cb, DataResponse::UniqueEventsBoard, uniqueEventStream);

    // remove lock
  }
  cb.flush();
}

}; // namespace da_profiler