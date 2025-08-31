//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_compilerDefs.h>
#include <util/dag_preprocessor.h>
#if DA_PROFILER_ENABLED
#include <perfMon/dag_perfTimer.h> //for profile_ref_ticks (inlined rdtsc) only
#endif
#ifndef DA_API
#define DA_API_DATA KRNLIMP
#define DA_API      KRNLIMP DAGOR_NOINLINE
#endif

#if defined(_MSC_VER)
#define DA_PROFILE_FUNC __FUNCSIG__
#else
#define DA_PROFILE_FUNC __PRETTY_FUNCTION__
#endif

#include <perfMon/dag_daProfilerToken.h>
#include <supp/dag_define_KRNLIMP.h>

namespace da_profiler
{
// thread-safe calls
enum
{
  EVENTS = 1 << 1,
  GPU = 1 << 2,
  TAGS = 1 << 3,            // capture tags
  SAMPLING = 1 << 4,        // sample main thread
  SAVE_SPIKES = 1 << 5,     // save spikes to ring buffer scratch board. works only for ring buffer profiler (not continuos)
  PLATFORM_EVENTS = 1 << 6, // platform specific events (such as Pix).
  UNIQUE_EVENTS = 1 << 7, // not timelined events. For profiling small functions being called many time, or continuos profiling of one
                          // function
  CONTINUOUS = 1 << 21,   // continuous profiling, otherwise ring buffer is used. Spikes would not be saved, during continuos profiling
  // pausing/resuming continuous profiling is semi-possible, as everything besides frame information will be freed
  // so if you stop continuous profiling, request_dump
};
enum class RegisterThreadResult
{
  Error,
  AlreadyRegistered,
  Registered
};
struct EventData;
struct GpuEventData;
struct ThreadStorage;
union TagSingleWordArgument
{
  TagSingleWordArgument() {}
  TagSingleWordArgument(uint32_t a) { i = a; }
  TagSingleWordArgument(int a) { i = a; }
#if _TARGET_64BIT
  TagSingleWordArgument(size_t a) { i = (uint32_t)a; }
#endif
  TagSingleWordArgument(float a) { f = a; }
  TagSingleWordArgument(double a) { f = (float)a; }
  uint32_t i;
  float f;
};
enum class ProfilerPluginStatus
{
  Disabled,
  Enabled,
  Error
};
class ProfilerPlugin
{
public:
  virtual ~ProfilerPlugin() {}
  virtual void unregister() {}        // called on unregistering
  virtual bool setEnabled(bool) = 0;  // return enabled state
  virtual bool isEnabled() const = 0; // return enabled state
};
} // namespace da_profiler

struct TimeIntervalInfo; // legacy
typedef void (*ProfilerDumpFunctionPtr)(void *ctx, uintptr_t cNodeId, uintptr_t parentNodeId, const char *,
  const TimeIntervalInfo &); // legacy

#if !DA_PROFILER_ENABLED

#define DA_PROFILE_ADD_DESCRIPTION(...)       0
#define DA_PROFILE_ADD_LOCAL_DESCRIPTION(...) 0
#define DA_PROFILE_ADD_WAIT_DESC(...)         0
#define DA_PROFILE_LEAF_EVENT(...)
#define DA_PROFILE_EVENT_DESC(desc)
#define DA_PROFILE_GPU_EVENT_DESC(desc)
#define DA_PROFILE_EVENT(...)
#define DA_PROFILE_WAIT(...)
#define DA_PROFILE_AUTO_WAIT()
#define DA_PROFILE_NAMED_EVENT()
#define DA_PROFILE_THREAD(Name)
#define DA_PROFILE_GPU_EVENT(...)
#define DA_PROFILE_NAMED_GPU_EVENT(Name)
#define DA_PROFILE_TAG(TAG_NAME, TAG_STRING, ...)
#define DA_PROFILE_TAG_DESC(TAG_DESC, TAG_STRING, ...)
#define DA_PROFILE_TAG_NAMED(TAG_NAME, TAG_STRING, ...)
#define DA_PROFILE_TAG_LINE(TAG_NAME, TAG_COLOR, TAG_STRING, ...)
#define DA_PROFILE_TICK()
#define DA_PROFILE_SHUTDOWN()
#define DA_PROFILE
#define DA_PROFILE_GPU
#define DA_PROFILE_UNIQUE
#define DA_PROFILE_UNIQUE_EVENT(...)
#define DA_PROFILE_UNIQUE_EVENT_NAMED(Name)
#define DA_PROFILE_UNIQUE_EVENT_DESC(Desc)

#ifndef DA_STUB // allows to compile stubs
#define DA_STUB inline
#endif

namespace da_profiler
{
DA_STUB uint64_t get_current_cpu_ticks() { return 0; }
DA_STUB bool register_plugin(const char *, ProfilerPlugin *) { return false; }
DA_STUB bool unregister_plugin(const char *) { return true; }
DA_STUB ProfilerPluginStatus set_plugin_enabled(const char *, bool)
{
  return ProfilerPluginStatus::Error;
} // return current status, if plugin missing - Error
DA_STUB void create_event(uint32_t, uint64_t) {}
DA_STUB void add_mode(uint32_t) {}
DA_STUB void remove_mode(uint32_t) {}
DA_STUB uint32_t get_current_mode() { return 0; }
DA_STUB void set_mode(uint32_t) {}
DA_STUB void set_spike_parameters(uint32_t, uint32_t, uint32_t) {}
DA_STUB void set_spike_save_parameters(uint32_t, uint32_t) {}
DA_STUB void get_spike_parameters(uint32_t &, uint32_t &, uint32_t &) {}
DA_STUB void get_spike_save_parameters(uint32_t &, uint32_t &) {}

DA_STUB void set_sampling_parameters(uint32_t, uint32_t, uint32_t) {}
DA_STUB void get_sampling_parameters(uint32_t &, uint32_t &, uint32_t &) {}
DA_STUB void set_continuous_limits(uint32_t, uint32_t) {}
DA_STUB void pause_sampling() {}
DA_STUB void resume_sampling() {}
DA_STUB void sync_stop_sampling() {}
DA_STUB desc_id_t add_description(const char *, int, uint32_t, const char *, uint32_t = 0u) { return 0; }
DA_STUB desc_id_t add_copy_description(const char *, int, uint32_t, const char *, uint32_t = 0u) { return 0; }
DA_STUB desc_id_t get_tls_description(const char *, int, uint32_t, const char *, uint32_t = 0u) { return 0; }
DA_STUB const char *get_description(desc_id_t, uint32_t &) { return nullptr; }

// if name is null, will use current thread name
DA_STUB desc_id_t add_thread_description(const char *, int, const char *, uint32_t = 0u) { return 0; }
DA_STUB void add_short_string_tag(desc_id_t, const char *) {}
DA_STUB void add_short_string_tag(desc_id_t, const char *, const TagSingleWordArgument *, uint32_t) {}
template <typename... Args>
inline void add_short_string_tag_args(desc_id_t, const char *, Args...)
{}
DA_STUB RegisterThreadResult register_thread(const char *, const char *, uint32_t) { return RegisterThreadResult::Registered; }
DA_STUB RegisterThreadResult register_thread(desc_id_t) { return RegisterThreadResult::Registered; }
DA_STUB void unregister_thread() {}
DA_STUB void request_dump() {}
DA_STUB void dump_frames(ProfilerDumpFunctionPtr, void *) {} // legacy, remove me

DA_STUB bool start_network_dump_server(int) { return false; }
DA_STUB bool stop_network_dump_server() { return false; }

DA_STUB bool start_file_dump_server(const char *) { return false; }
DA_STUB bool stop_file_dump_server(const char *) { return false; }
DA_STUB bool start_log_dump_server(const char *) { return false; }
DA_STUB bool stop_log_dump_server(const char *) { return false; }
DA_STUB void shutdown() {}
DA_STUB void tick_frame() {}
DA_STUB void create_leaf_event_raw(desc_id_t, uint64_t, uint64_t) {}
DA_STUB EventData *start_event(desc_id_t, ThreadStorage *&) { return nullptr; }
DA_STUB void end_event(EventData &, ThreadStorage &) {}
DA_STUB GpuEventData *start_gpu_event(desc_id_t, const char *) { return nullptr; }
DA_STUB void end_gpu_event(GpuEventData &) {}
struct UniqueEventData
{};
DA_STUB void end_unique_event(UniqueEventData &, uint64_t) {}
DA_STUB void add_unique_event(UniqueEventData &) {}
DA_STUB const UniqueEventData *get_unique_event(uint32_t, uint32_t &) { return nullptr; }
inline int get_active_mode() { return 0; }
inline void create_leaf_event(desc_id_t, uint64_t, uint64_t) {}
} // namespace da_profiler

#undef DA_STUB

#else

#if _TARGET_STATIC_LIB
#define DA_PROFILE_THREADS 1
#endif

// should we store filenames in events? in release builds we don't to reduce executable size and leakage of file names
#ifndef DA_PROFILE_FILE_NAMES
#if DAGOR_DBGLEVEL > 0
#define DA_PROFILE_FILE_NAMES 1
#else
#define DA_PROFILE_FILE_NAMES 0
#endif
#endif

#include <stdint.h>
#include <osApiWrappers/dag_atomic.h> //for relaxed interlocked access to active mode only.

namespace da_profiler
{
// thread-safe calls
//
DA_API bool register_plugin(const char *name, ProfilerPlugin *);            // return true, if plugin was registered
DA_API bool unregister_plugin(const char *name);                            // return true, if plugin was unregistered
DA_API ProfilerPluginStatus set_plugin_enabled(const char *, bool enabled); // return current status, if plugin missing - Error
// all set/get functions are thread safe
// combination of mode (EVENTS, GPU, etc). basically interlocked_or with next active mode
DA_API void add_mode(uint32_t mask); // interlocked or, will defer till next frame

// stopping profiling doesn't save capture! call request_dump first
DA_API void remove_mode(uint32_t mask);  // interlocked and, will defer till next frame
DA_API uint32_t get_current_mode();      // interlocked load, will defer till next frame
DA_API void set_mode(uint32_t new_mode); // will defer till next frame. Just remove_mode(get_current_mode()&(~new_mode)) +
                                         // add_mode(new_mode)

// when frame will be considered spiked if it is longer than max(min_msec, average*average_mul + add_msec)
// min_msec = 0 - means no spike profiling
// these settings are atomic, as they are really applied on tick frame
DA_API void set_spike_parameters(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec);

// save N frames abefore and after spike (if SAVE_SPIKES mode)
DA_API void set_spike_save_parameters(uint32_t frames_before, uint32_t frames_after);
// sampling_rate_per_sec == 0 - is invalid, and call would be ignored
// valid range is 10+ (each 100msec) and up to 10000 (each 0.1msec). Beware, on machines with less than 4 cores, sampling is only
// possible with 1msec+ intervals sampling_spike_rate_per_sec is never less than sampling_rate_per_sec if sampling is on, you can set
// sampling_rate_per_sec = 1 and sampling_spike_rate_per_sec = 1000, to basically sample only spikes "spikes" are defined above, if
// spikes profiling is on it won't be basically doing anything until spike threshold from frame start is reached, and than will start
// sampling. that reduces pressure on memory and performance impact during profiling, and yet allows to gather some info during spikes
// sampling_rate_per_sec. Current _SAFE_ maximum is 1000, increasing that number is possible.
// having more than 1000 is possible if you have more than 4 cores, but that could lead to thread stravation
// threads_sampling_rate_mul is a natural multiplier of current sampling rate
// each n-th sample we would be sampling other threads
// if threads_sampling_rate_mul = 0, than other (instrumented) threads are not sampled at all
// if threads_sampling_rate_mul = 1, than other (instrumented) threads are sampled at same rate as main
// if threads_sampling_rate_mul = 2, than other (instrumented) threads are sampled twice rarely than main
// it is preferrable to sample other threads rarely, than main
// if main is waiting for other threads (typical), than sampling other threads can affect total performance,
//  and will be seen in profiler as "waiting for slow other threads", which is not nessesarily true
// these setting are atomic per param, but are not atomic together. In practice it shouldn't matter, unless you set/get same settings
// from different threads thread1 is setting, thread2 is getting to-set-and-then-restore. Then thread2 will receive some combination of
// freq/rate, which was temporal however, realistically speaking - shouldn't be happening profiler tool sets these in thread though

// get* returns what was set in set*, not what it is used.
// It is atomic, but not thread safe if you set and get in different threads you can get inconsistent values (but atomic)
DA_API void get_spike_parameters(uint32_t &min_msec, uint32_t &average_mul, uint32_t &add_msec);
DA_API void get_spike_save_parameters(uint32_t &frames_before, uint32_t &frames_after);

//! these settings are immediate if sampling is already going (so you can increase sampling where needed)!
DA_API void set_sampling_parameters(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq,
  uint32_t threads_sampling_rate_mul);
DA_API void get_sampling_parameters(uint32_t &sampling_freq, uint32_t &sampling_spike_freq, uint32_t &threads_sampling_rate_mul);

// after reaching <frames> frame limit, continuos profiling will dump to file/server (but not stop!)
// after reaching <size_mb> one single profiling dump size (unpacked), continuos profiling will stop, and has to be restarted
// zero - means no limit
DA_API void set_continuous_limits(uint32_t frames, uint32_t size_mb);

// will pause sampling till next frame or resume_sampling called. Doesn't guarantee that samples won't be gathered
DA_API void pause_sampling();

// will resume paused sampling. Does nothing if there is no sampling
DA_API void resume_sampling();

// to be called before dump all threads (in watchdog, etc). Causes sync with profiling/sampling thread, which can be slow (depending on
// number of threads)
//!!call me before !dump_all_thread_callstacks()!
// stops sampling till end of current frame
DA_API void sync_stop_sampling();

// file_name assumed to be non-moveable, something like __FILE__ OR nullptr in all descriptions!
// add to global table (name+line+pointer(file_name)) of descriptions, returns descriptionId
// name is assumed to be non-moveable, something like #Label or DA_PROFILE_FUNC
DA_API desc_id_t add_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color = 0u);

// add to global table (name+line+pointer(file_name)) of descriptions, returns descriptionId, but creates copy of name
// name is any string, can be on heap/stack
DA_API desc_id_t add_copy_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color = 0u);

// thread_local hash table (name+line+pointer(file_name))->descriptionId, creates copy of 'name'
// if there is no such description in our calling thread, it will be created. If there is - it will be returned.
DA_API desc_id_t get_tls_description(const char *file_name, int line, uint32_t flags, const char *name, uint32_t color = 0u);

// iterating over descriptions. null means it is last
DA_API const char *get_description(desc_id_t, uint32_t &color_and_flags);

// if name is null, will use current thread name
DA_API desc_id_t add_thread_description(const char *file_name, int line, const char *name, uint32_t color = 0u);

// add some _very short_ string (<=54 length) to current cpu event.
// if needed, other types of tags can be implemented via same tags
// if you need more than 54 - you would have to add more tags (split)
DA_API void add_short_string_tag(desc_id_t description, const char *string);

// fast format string. Only single words arguments (int, float, doubles which are converted to floats) are accepted and only
// %d/%x/%f/%g at least 4 parameters will be saved, total amount of saved parameters depends on length of formatted string formatted
// string should be shorter than 49 symbols with only one parameter)
//  if you pass more 4+ parameters, maximum length of formatted string is 37
//  the format string is not resolved in runtime, rather it is resolved in tool
//  so it is relatively fast (just a bit slower than tag without arguments)
DA_API void add_short_string_tag(desc_id_t description, const char *format, const TagSingleWordArgument *, uint32_t);
template <typename... Args>
inline void add_short_string_tag_args(desc_id_t description, const char *fmt, Args... args)
{
  if (sizeof...(args))
  {
    TagSingleWordArgument a[sizeof...(args) ? sizeof...(args) : 1] = {args...};
    add_short_string_tag(description, fmt, sizeof...(args) ? a : nullptr, (int)sizeof...(args));
  }
  else
    add_short_string_tag(description, fmt);
}


// all name, file, line can be null
// file is something like __FILE__, i.e. static const nonmoveable string
DA_API RegisterThreadResult register_thread(const char *name, const char *file, uint32_t line);
DA_API RegisterThreadResult register_thread(desc_id_t description);
DA_API void unregister_thread();

// will cause current profiling dump to be saved (if profiling is on)
// if we are doing 'infinite' profiling, it will dump current state
// if we are doing normal 'ring' profiling, it will dump current ring
// it will not do anything if profiling was switched off
// dump will be generated on next addFrame (synced) and saved asynced
DA_API void request_dump();
DA_API void dump_frames(ProfilerDumpFunctionPtr, void *); // legacy, remove me

DA_API bool start_network_dump_server(int port); // profiler dumps will be sent to this port, if something is connected
DA_API bool stop_network_dump_server();          // we have only one network client

DA_API bool start_file_dump_server(const char *path); // profiler dumps will be saved in this path
DA_API bool stop_file_dump_server(const char *path);  // remove one file server
DA_API bool start_log_dump_server(const char *name);  // specified GPU event will be reported to log
DA_API bool stop_log_dump_server(const char *name);   // remove one log server
DA_API uint64_t get_current_cpu_ticks();
#if NATIVE_PROFILE_TICKS
inline uint64_t inline_profiler_ticks() { return profile_ref_ticks(); };
#else
inline uint64_t inline_profiler_ticks() { return get_current_cpu_ticks(); };
#endif
} // namespace da_profiler

// thread-unsafe calls
namespace da_profiler
{
// thread-unsafe calls!

// shutdown is NOT thread safe and can NOT be called simultaneously with tick_frame
// caling anything from profiler after shutdown is UB (though I would expect everything to work:))
DA_API void shutdown();
// tick_frame to be called from only one thread - the one you count your frames from ("main")
// not to be called with shutdown();
DA_API void tick_frame();
} // namespace da_profiler


// scope calls
namespace da_profiler
{

struct UniqueEventData;

// scope calls
extern DA_API_DATA int active_mode;
inline int get_active_mode() { return interlocked_relaxed_load(active_mode); }
DA_API void create_leaf_event_raw(desc_id_t description, uint64_t start, uint64_t end); // low level, should not be called
inline void create_leaf_event(desc_id_t description, uint64_t start, uint64_t end)
{
  if ((get_active_mode() & EVENTS))
    create_leaf_event_raw(description, start, end);
}
DA_API EventData *start_event(desc_id_t description, ThreadStorage *&);
DA_API void end_event(EventData &, ThreadStorage &);
DA_API GpuEventData *start_gpu_event(desc_id_t description, const char *d3d_event_name);
DA_API void end_gpu_event(GpuEventData &);
DA_API void end_unique_event(UniqueEventData &, uint64_t);
DA_API void add_unique_event(UniqueEventData &);
DA_API const UniqueEventData *get_unique_event(uint32_t i, uint32_t &frames); // allows iterating over. if returns nullptr - no more
                                                                              // events

struct ScopedEvent // 16 bytes
{
  EventData *e;
  ThreadStorage *s;                                                                                   //-V730
  ScopedEvent(desc_id_t descr) : e((get_active_mode() & EVENTS) ? start_event(descr, s) : nullptr) {} //-V730
  ScopedEvent(const ScopedEvent &) = delete;
  ScopedEvent &operator=(const ScopedEvent &) = delete;
  ScopedEvent(ScopedEvent &&a) : e(a.e), s(a.s) { a.e = nullptr; };
  ScopedEvent &operator=(ScopedEvent &&a)
  {
    auto ae = a.e;
    a.e = e;
    e = ae;
    auto as = a.s;
    a.s = s;
    s = as;
    return *this;
  }
  ~ScopedEvent()
  {
    if (e)
      end_event(*e, *s);
  }
};


struct UniqueEventData // can only be used as a static variable!
{
  uint64_t minTicks = ~0ULL, maxTicks = 0, totalTicks = 0, totalOccurencies = 0;
  uint32_t desc = 0;
  uint32_t startFrame = 0;
  UniqueEventData(uint32_t d) : desc(d) { add_unique_event(*this); };
};
struct ScopedUniqueEvent
{
  uint64_t start;
  UniqueEventData &ued;
  ScopedUniqueEvent(UniqueEventData &e) : ued(e), start((get_active_mode() & UNIQUE_EVENTS) ? inline_profiler_ticks() : 0ULL) {}
  ~ScopedUniqueEvent() { start ? end_unique_event(ued, inline_profiler_ticks() - start) : (void)start; }
};

struct ScopedGPUEvent // 24 byte
{
  EventData *e;
  ThreadStorage *s;                                                     //-V730
  GpuEventData *egpu;                                                   //-V730
  ScopedGPUEvent(desc_id_t descr, const char *d3d_event_name = nullptr) //-V730
  {
    const int mode = get_active_mode();
    if (!(mode & EVENTS))
    {
      e = nullptr;
      return;
    }
    egpu = (mode & GPU) ? start_gpu_event(descr, d3d_event_name) : nullptr; // should be before cpu event, for accurate measurements
    e = start_event(descr, s);
  }
  ~ScopedGPUEvent()
  {
    if (e)
    {
      end_event(*e, *s); // should be before gpu event, for accurate measurements of cpu time
      if (egpu)
        end_gpu_event(*egpu);
    }
  }
  ScopedGPUEvent(const ScopedGPUEvent &) = delete;
  ScopedGPUEvent &operator=(const ScopedGPUEvent &) = delete;
  ScopedGPUEvent(ScopedGPUEvent &&a) : e(a.e), s(a.s), egpu(a.egpu) { a.e = nullptr; };
  ScopedGPUEvent &operator=(ScopedGPUEvent &&a)
  {
    auto ae = a.e;
    a.e = e;
    e = ae;
    auto as = a.s;
    a.s = s;
    s = as;
    auto aegpu = a.egpu;
    a.egpu = egpu;
    egpu = aegpu;
    return *this;
  }
};
struct ScopedThread
{
  bool unregister = false;
  ScopedThread(desc_id_t desc) { unregister = (register_thread(desc) == RegisterThreadResult::Registered); }
  ~ScopedThread()
  {
    if (unregister)
      unregister_thread();
  }
  ScopedThread(const ScopedThread &) = delete;
  ScopedThread &operator=(const ScopedThread &) = delete;
  ScopedThread(ScopedThread &&a) : unregister(a.unregister) { a.unregister = false; }
  ScopedThread &operator=(ScopedThread &&a)
  {
    bool u = unregister;
    unregister = a.unregister;
    a.unregister = u;
    return *this;
  }
};
} // namespace da_profiler

#define DA_PROFILE_ADD_LOCAL_DESCRIPTION(...) \
  ::da_profiler::add_copy_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, ##__VA_ARGS__)

#define DA_PROFILE_ADD_DESCRIPTION(FILE, LINE, ...) ::da_profiler::add_copy_description(FILE, LINE, 0, ##__VA_ARGS__)
#define DA_PROFILE_ADD_WAIT_DESC(FILE, LINE, ...)   ::da_profiler::add_copy_description(FILE, LINE, ::da_profiler::IsWait, ##__VA_ARGS__)

#define DA_PROFILE_THREAD_INT(NAME)                                                                                       \
  ::da_profiler::desc_id_t DAG_CONCAT(autogen_thread_description_, __LINE__) =                                            \
    ::da_profiler::add_thread_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, NAME);                    \
  ::da_profiler::ScopedThread DAG_CONCAT(a_profile_thread_, __LINE__)(DAG_CONCAT(autogen_thread_description_, __LINE__)); \
  G_UNUSED(DAG_CONCAT(a_profile_thread_, __LINE__))

#if DA_PROFILE_THREADS
#define DA_PROFILE_THREAD(NAME) DA_PROFILE_THREAD_INT(NAME)
#else
#define DA_PROFILE_THREAD(NAME)
#endif

#define DA_PROFILE_TAG(TAG_NAME, TAG_STRING, ...)                                                                     \
  if (::da_profiler::get_active_mode() & ::da_profiler::TAGS)                                                         \
  {                                                                                                                   \
    static ::da_profiler::desc_id_t description = 0;                                                                  \
    ::da_profiler::desc_id_t descId = interlocked_relaxed_load(description);                                          \
    if (DAGOR_UNLIKELY(descId == 0))                                                                                  \
      interlocked_relaxed_store(description,                                                                          \
        descId = ::da_profiler::add_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, #TAG_NAME)); \
    ::da_profiler::add_short_string_tag_args(descId, TAG_STRING, ##__VA_ARGS__);                                      \
  }

#define DA_PROFILE_TAG_DESC(TAG_DESC, TAG_STRING, ...)        \
  if (::da_profiler::get_active_mode() & ::da_profiler::TAGS) \
    ::da_profiler::add_short_string_tag_args(TAG_DESC, TAG_STRING, ##__VA_ARGS__);

#define DA_PROFILE_TAG_LINE(TAG_NAME, TAG_COLOR, TAG_STRING, ...)                                                                \
  if (::da_profiler::get_active_mode() & ::da_profiler::TAGS)                                                                    \
  {                                                                                                                              \
    static ::da_profiler::desc_id_t description = 0;                                                                             \
    ::da_profiler::desc_id_t descId = interlocked_relaxed_load(description);                                                     \
    if (DAGOR_UNLIKELY(descId == 0))                                                                                             \
      interlocked_relaxed_store(description,                                                                                     \
        descId = ::da_profiler::add_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, #TAG_NAME, TAG_COLOR)); \
    ::da_profiler::add_short_string_tag_args(descId, TAG_STRING, ##__VA_ARGS__);                                                 \
  }

#define DA_PROFILE_TAG_NAMED(TAG_NAME, TAG_STRING, ...)                                                                   \
  if (::da_profiler::get_active_mode() & ::da_profiler::TAGS)                                                             \
  {                                                                                                                       \
    static ::da_profiler::desc_id_t description = 0;                                                                      \
    ::da_profiler::desc_id_t descId = interlocked_relaxed_load(description);                                              \
    if (DAGOR_UNLIKELY(descId == 0))                                                                                      \
      interlocked_relaxed_store(description,                                                                              \
        descId = ::da_profiler::add_copy_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, TAG_NAME)); \
    ::da_profiler::add_short_string_tag_args(descId, TAG_STRING, ##__VA_ARGS__);                                          \
  }

#define DA_PROFILE_WAIT(...)                                                                               \
  ::da_profiler::desc_id_t DAG_CONCAT(autogen_profile_description_, __LINE__) = 0;                         \
  {                                                                                                        \
    static ::da_profiler::desc_id_t description = 0;                                                       \
    if ((DAG_CONCAT(autogen_profile_description_, __LINE__) = interlocked_relaxed_load(description)) == 0) \
      interlocked_relaxed_store(description,                                                               \
        DAG_CONCAT(autogen_profile_description_, __LINE__) = ::da_profiler::add_description(               \
          DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, ::da_profiler::IsWait, ##__VA_ARGS__));    \
  }                                                                                                        \
  ::da_profiler::ScopedEvent DAG_CONCAT(autogen_profile_event_, __LINE__)((DAG_CONCAT(autogen_profile_description_, __LINE__)));

#define DA_PROFILE_AUTO_WAIT() DA_PROFILE_WAIT(DA_PROFILE_FUNC)

#define DA_PROFILE_LEAF_EVENT(desc, start, end) ::da_profiler::create_leaf_event(desc, start, end)

#define DA_PROFILE_EVENT(...)                                                                                                     \
  ::da_profiler::desc_id_t DAG_CONCAT(autogen_profile_description_, __LINE__) = 0;                                                \
  {                                                                                                                               \
    static ::da_profiler::desc_id_t description = 0;                                                                              \
    if ((DAG_CONCAT(autogen_profile_description_, __LINE__) = interlocked_relaxed_load(description)) == 0)                        \
      interlocked_relaxed_store(description, DAG_CONCAT(autogen_profile_description_, __LINE__) = ::da_profiler::add_description( \
                                               DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, ##__VA_ARGS__));          \
  }                                                                                                                               \
  ::da_profiler::ScopedEvent DAG_CONCAT(autogen_profile_event_, __LINE__)((DAG_CONCAT(autogen_profile_description_, __LINE__)));

#define DA_PROFILE_EVENT_DESC(desc)     ::da_profiler::ScopedEvent DAG_CONCAT(autogen_profile_event_, __LINE__)(desc)
#define DA_PROFILE_GPU_EVENT_DESC(desc) ::da_profiler::ScopedGPUEvent DAG_CONCAT(autogen_profile_event_, __LINE__)(desc)

#define DA_PROFILE_UNIQUE_EVENT_DESC(desc)                                                               \
  static ::da_profiler::UniqueEventData DAG_CONCAT(unique_autogen_profile_description_, __LINE__)(desc); \
  ::da_profiler::ScopedUniqueEvent DAG_CONCAT(autogen_profile_unique_event_, __LINE__)(                  \
    DAG_CONCAT(unique_autogen_profile_description_, __LINE__));

#define DA_PROFILE_UNIQUE_EVENT(...)                                                                         \
  static ::da_profiler::UniqueEventData DAG_CONCAT(unique_autogen_profile_description_, __LINE__)(           \
    ::da_profiler::add_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, ##__VA_ARGS__)); \
  ::da_profiler::ScopedUniqueEvent DAG_CONCAT(autogen_profile_unique_event_, __LINE__)(                      \
    DAG_CONCAT(unique_autogen_profile_description_, __LINE__));

#define DA_PROFILE_UNIQUE_EVENT_NAMED(...)                                                                       \
  static ::da_profiler::UniqueEventData DAG_CONCAT(unique_autogen_profile_description_, __LINE__)(               \
    ::da_profiler::get_tls_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, ##__VA_ARGS__)); \
  ::da_profiler::ScopedUniqueEvent DAG_CONCAT(autogen_profile_unique_event_, __LINE__)(                          \
    DAG_CONCAT(unique_autogen_profile_description_, __LINE__));

#define DA_PROFILE_GPU_EVENT(...)                                                                                                  \
  ::da_profiler::desc_id_t DAG_CONCAT(autogen_profile_description_, __LINE__) = 0;                                                 \
  {                                                                                                                                \
    static ::da_profiler::desc_id_t description = 0;                                                                               \
    if ((DAG_CONCAT(autogen_profile_description_, __LINE__) = interlocked_relaxed_load(description)) == 0)                         \
      interlocked_relaxed_store(description, DAG_CONCAT(autogen_profile_description_, __LINE__) = ::da_profiler::add_description(  \
                                               DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, ##__VA_ARGS__));           \
  }                                                                                                                                \
  ::da_profiler::ScopedGPUEvent DAG_CONCAT(autogen_profile_event_, __LINE__)((DAG_CONCAT(autogen_profile_description_, __LINE__)), \
    ##__VA_ARGS__);

#define DA_PROFILE_NAMED_EVENT(...)                                                  \
  ::da_profiler::ScopedEvent DAG_CONCAT(autogen_profile_named_cpu_event_, __LINE__)( \
    ::da_profiler::get_tls_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, ##__VA_ARGS__));

#define DA_PROFILE_NAMED_GPU_EVENT(NAME, ...)                                           \
  ::da_profiler::ScopedGPUEvent DAG_CONCAT(autogen_profile_named_gpu_event_, __LINE__)( \
    ::da_profiler::get_tls_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, 0, NAME, ##__VA_ARGS__), NAME);

#define DA_PROFILE        DA_PROFILE_EVENT(DA_PROFILE_FUNC)
#define DA_PROFILE_GPU    DA_PROFILE_GPU_EVENT(DA_PROFILE_FUNC)
#define DA_PROFILE_UNIQUE DA_PROFILE_UNIQUE_EVENT(DA_PROFILE_FUNC)

#define DA_PROFILE_TICK()     da_profiler::tick_frame()
#define DA_PROFILE_SHUTDOWN() da_profiler::shutdown();

#endif

#undef DA_API
#undef DA_API_DATA
#undef DA_PROFILE_THREADS
#include <supp/dag_undef_KRNLIMP.h>
