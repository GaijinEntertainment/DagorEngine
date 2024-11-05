// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_atomic.h>
#include <mutex>
#include "daProfilerInternal.h"
#include "stl/daProfilerStl.h"
#include "daProfilePlatform.h"

namespace da_profiler
{

inline int ProfilerData::findThreadIndexUnsafe(uint64_t threadId) const
{
  int at = 0;
  for (auto &thread : threadsData)
  {
    if (thread && thread->storage.threadId == threadId)
      return at;
    at++;
  }
  return -1;
}

int ProfilerData::findThreadIndex(uint64_t threadId) const
{
  std::lock_guard<std::mutex> lock(threadsLock);
  return findThreadIndexUnsafe(threadId);
}

RegisterThreadResult ProfilerData::addThreadData(uint32_t description)
{
  if (interlocked_acquire_load_ptr(tls_storage)) // already created
    return RegisterThreadResult::AlreadyRegistered;
  std::lock_guard<std::mutex> lock(threadsLock);
  if (interlocked_acquire_load_ptr(tls_storage)) // already created
    return RegisterThreadResult::AlreadyRegistered;

  if (!threadsDataStor.capacity())
  {
    threadsDataStor.reserve((get_logical_cores() + 7) & ~7);
    threadsData.reserve(threadsDataStor.capacity());
  }

  const int64_t threadId = get_current_thread_id();
  auto threadEmpty = threadsData.end();
  for (auto threadI = threadsData.begin(), threadE = threadEmpty; threadI != threadE; ++threadI)
  {
    if (!(*threadI) || (*threadI)->storage.threadId == threadId)
    {
      threadEmpty = threadI;
      break;
    }
  }
  PerThreadInfo *newThread = nullptr;
  if (threadEmpty != threadsData.end() && *threadEmpty) // we found thread with same id
  {
    if ((*threadEmpty)->tlsStorage == &tls_storage || (*threadEmpty)->tlsStorage == nullptr)
    {
      // this is "resurrected" thread. Someone called unregister and then register again
      // set global tls_storage to storage to this thread and restore it
    }
#if DAPROFILER_DEBUGLEVEL > 0
    report_logerr("adding different threads with same id shouldn't be happening!");
#endif
    return RegisterThreadResult::Error;
  }
  else if (threadsDataStor.size() < threadsDataStor.capacity())
    newThread = &threadsDataStor.emplace_back();
  else
    newThread = new PerThreadInfo;
  auto &i = newThread ? newThread : (*threadEmpty); // -V668
  interlocked_release_store_ptr(i->tlsStorage, &tls_storage);
  interlocked_release_store_ptr(tls_storage, &i->storage);
  interlocked_increment(threadsGeneration); // so sampling thread knows, that it has to re-arrange threads list to sample
  u64_interlocked_release_store(i->addedTick, cpu_current_ticks());
  // we don't allow threads to change name, to avoid confusion.
  // i->storage.description = description;
  if (newThread)
  {
    i->storage.threadId = threadId;
    i->storage.description = description;
    threadsData.push_back(newThread);
  }
  interlocked_increment(threadsGeneration); // so sampling thread knows, that it has to re-arrange threads list to sample
  return RegisterThreadResult::Registered;
}

RegisterThreadResult ProfilerData::addThreadData(const char *name, const char *file, uint32_t line)
{
  return addThreadData(add_thread_description(file, line, name, 0u));
}

bool ProfilerData::addFrameThread(const char *name, const char *file, uint32_t line)
{
  if (addThreadData(name, file, line) == RegisterThreadResult::Error)
    return false;
  const int64_t threadId = get_current_thread_id();
  if (interlocked_acquire_load_ptr(tls_storage))
  {
    std::lock_guard<std::mutex> lock(threadsLock);
    u64_interlocked_release_store(frameThreadId, threadId);
    frame_thread = true;
  }
  else
    frame_thread = false;
  return true;
}

void ProfilerData::closeRemovedThreads() // should be called from addFrame
{
  // relaxed is fine - we don't care when it will happen, let it happen "eventually"
  if (!interlocked_relaxed_load(removedThreadsCount)) // no removed threads - all threads are active, none were removed
    return;
  std::lock_guard<std::mutex> lock(threadsLock);
  if (!interlocked_acquire_load(removedThreadsCount)) // no removed threads - all threads are active, none were removed
    return;
  const int64_t removeThreadsEarlierThan = safe_first_needed_tick();

  for (auto ti = threadsData.begin(); ti != threadsData.end(); ++ti)
  {
    auto &thread = *ti;
    if (!thread || thread->tlsStorage) // not removed thread!
      continue;

    if ((int64_t)thread->removedTick >= removeThreadsEarlierThan) // not enough time passed, thread data can still be needed in dump
    {
      // this is poor-man data-race fix. Since we have potential race inside startEvent, as we use non-interlocked storage.depth
      // increment than it is _race_. we need to be sure, that there is no "Event in progress" in order to do so, we should use
      // interlocked_increment/interlocked_decrement over storage.depth that would guarantee that if depth == 0, than there is no
      // active events however, that slows down profiling, so we rely on "if some enough time has passed, there is no active events"
      continue;
    }
    // there are some started, but not finished events
    if (!is_safe_to_shutdown())
      continue;
    if (interlocked_acquire_load(thread->storage.depth) != 0)
      continue;
    if (!(thread >= threadsDataStor.begin() && thread < threadsDataStor.end()))
      delete thread;
    thread = nullptr;
    interlocked_decrement(removedThreadsCount);
    threadsData.erase(ti);
    --ti; // it will be incremented back
  }
}

bool ProfilerData::removeCurrentThread()
{
  std::lock_guard<std::mutex> lock(threadsLock); // to avoid race in shutdown, which also sets our tls_storage to 0
  auto storage = interlocked_acquire_load_ptr(tls_storage);
  if (!storage) // not active thread already
    return false;
  if (storage->depth != 0)
  {
    report_logerr("we can't remove thread if we are inside scoped event");
    return false;
  }
  interlocked_release_store_ptr(tls_storage, (ThreadStorage *)nullptr);
  const int idx = findThreadIndexUnsafe(get_current_thread_id());
  if (idx < 0 || !threadsData[idx] || !threadsData[idx]->tlsStorage)
    report_logerr("could not be happening, assert, thread wasn not registered!");
  else
    threadsData[idx]->tlsStorage = nullptr; // tls variable is not valid anymore
  u64_interlocked_release_store(threadsData[idx]->removedTick, cpu_current_ticks());
  interlocked_increment(threadsGeneration); // so sampling thread knows, that it has to arrange new threads
  interlocked_increment(removedThreadsCount);
  // we schedule thread for removing
  return true;
}

extern void add_default_messages();
extern void start_dump_server();

ThreadStorage *ProfilerData::initializeFrameThread()
{
  const uint64_t id = get_current_thread_id();
  {
    std::lock_guard<std::mutex> lock(threadsLock);
    // check under lock
    const uint64_t fid = u64_interlocked_acquire_load(frameThreadId);
    if (fid && fid != id)
    {
      report_logerr("addFrame should be called from only one thread. otherwise gpu frames will become broken (can be fixed)");
      return nullptr;
    }
    else if (fid == id)
    {
      auto ts = interlocked_acquire_load_ptr(tls_storage);
      if (ts) // it was race
        return ts;
      report_logerr("main frame was somehow closed!");
      return nullptr;
    }
  }

  if (addFrameThread("Main", __FILE__, __LINE__))
  {
    initUniqueName();
    // default servers
    start_dump_server();
#if DA_PROFILE_NET_IS_POSSIBLE
    start_network_dump_server(0);
    add_default_messages(); // we only receive messages from network
#endif
#if _TARGET_PC
    start_file_dump_server(get_default_file_dir());
#endif
  }
  return interlocked_acquire_load_ptr(tls_storage);
}


RegisterThreadResult register_thread(desc_id_t description) { return the_profiler.addThreadData(description); }

RegisterThreadResult register_thread(const char *name, const char *file, uint32_t line)
{
  return the_profiler.addThreadData(name, file, line);
}

desc_id_t add_thread_description(const char *file_name, int line, const char *name, uint32_t color)
{
  char buf[64];
  name = name ? name : get_current_thread_name();
  const char *useName = name ? name : buf;
  if (!name)
    snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)get_current_thread_id());
  return add_copy_description(file_name, line, 0, useName, color);
}

void unregister_thread() { the_profiler.removeCurrentThread(); }


}; // namespace da_profiler