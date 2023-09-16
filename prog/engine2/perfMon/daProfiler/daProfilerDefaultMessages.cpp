#include "daProfilerMessageServer.h"
#include "daProfilerMessageTypes.h"
#include "daProfilerInternal.h"
#include <ioSys/dag_genIo.h>

// this is default receivers (from web/live to game)
namespace da_profiler
{
static void skip(IGenLoad &cb, uint32_t len)
{
  char buf[64];
  for (; len;)
  {
    const size_t sz = min(sizeof(buf), (size_t)len);
    cb.read(buf, sz);
    len -= sz;
  }
}

static void start_inf_capture(uint16_t, uint32_t len, IGenLoad &load_cb, IGenSave &)
{
  add_mode(CONTINUOUS);
  skip(load_cb, len);
}
static void stop_inf_capture(uint16_t, uint32_t, IGenLoad &, IGenSave &save_cb)
{
  if (interlocked_acquire_load(active_mode) & CONTINUOUS)
  {
    // request dump and stop
    request_dump();
  }
  remove_mode(CONTINUOUS);
  response_finish(save_cb);
}
static void cancel_inf_capture(uint16_t, uint32_t, IGenLoad &, IGenSave &save_cb)
{
  remove_mode(CONTINUOUS);
  response_finish(save_cb);
}

static void cancel_profiling(uint16_t, uint32_t, IGenLoad &, IGenSave &) { set_mode(0); }

static void capture(uint16_t, uint32_t, IGenLoad &, IGenSave &) { request_dump(); }
static void turn_sampling(uint16_t, uint32_t, IGenLoad &load_cb, IGenSave &)
{
  bool b;
  load_cb.read(&b, sizeof(b));
  if (b)
    resume_sampling();
  else
    pause_sampling();
}

static void set_settings(uint16_t, uint32_t len, IGenLoad &load_cb, IGenSave &)
{
  UserSettings settings;
  if (read_settings(load_cb, settings, len))
  {
    settings.mode |= get_current_mode() & CONTINUOUS; // do not change current infinite_profiling state
    set_settings(settings);
  }
}

static void plugin_command(uint16_t, uint32_t len, IGenLoad &load_cb, IGenSave &)
{
  int plugins = load_cb.readInt();
  int read = sizeof(int);
  for (int i = 0; i < plugins; i++)
  {
    int pluginLen = load_cb.readInt();
    read += sizeof(int);
    char buf[512];
    if (pluginLen >= sizeof(buf))
    {
      skip(load_cb, len - read);
      return;
    }
    load_cb.read(buf, pluginLen);
    set_plugin_enabled(buf, load_cb.readIntP<1>() != 0);
    read += pluginLen + 1;
  }
}

static void disconnect(uint16_t, uint32_t, IGenLoad &, IGenSave &save_cb) { response_finish(save_cb); }

static void connected(uint16_t, uint32_t, IGenLoad &, IGenSave &) {}

void add_default_messages()
{
  add_message(Connected, &connected);
  add_message(Disconnect, &disconnect);
  add_message(SetSettings, &set_settings);
  add_message(StartInfiniteCapture, &start_inf_capture);
  add_message(StopInfiniteCapture, &stop_inf_capture);
  add_message(CancelInfiniteCapture, &cancel_inf_capture);
  add_message(Capture, &capture);
  add_message(TurnSampling, &turn_sampling);
  add_message(CancelProfiling, &cancel_profiling);
  add_message(PluginCommand, &plugin_command);
}
} // namespace da_profiler
