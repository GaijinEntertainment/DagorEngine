// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include <soundSystem/quirrel/sqSoundSystem.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <soundSystem/handle.h>
#include <soundSystem/streams.h>
#include <soundSystem/strHash.h>
#include <soundSystem/events.h>
#include <soundSystem/handle.h>
#include <soundSystem/vars.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/banks.h>
#include <soundSystem/debug.h>
#include <sqrat.h>
#include <quirrel/sqrex.h>

namespace sound
{
namespace sqapi
{
Sqrat::Table script_callbacks;

void play_sound(const char *name, const Sqrat::Object &params, float volume, const Point3 *pos)
{
  if (!name || !*name)
    return;

  sndsys::EventHandle eh = pos ? sndsys::init_event(name, *pos) : sndsys::init_event(name);
  if (!eh)
  {
    if (sndsys::is_inited() && sndsys::banks::is_loaded(sndsys::banks::get_master_preset()))
    {
      if (!sndsys::has_event(name))
      {
        logerr("sqapi: there is no sound event '%s'", name);
      }
    }
    return;
  }

  if (!params.IsNull())
  {
    Sqrat::Object::iterator it;
    sndsys::VarDesc varDesc;
    while (params.Next(it))
    {
      if (sndsys::get_var_desc(eh, it.getName(), varDesc))
      {
        HSQOBJECT val = it.getValue();
        sndsys::set_var(eh, varDesc.id, sq_objtofloat(&val));
      }
      else
        G_ASSERTF(0, "Variable %s not found in event %s", it.getName(), name);
    }
  }

  sndsys::set_volume(eh, volume);
  sndsys::start(eh);
  sndsys::abandon(eh);
}

int get_num_event_instances(const char *name)
{
  if (name)
    return sndsys::get_num_event_instances(name);
  return 0;
}

bool is_preset_loaded(const char *preset_name) { return preset_name && sndsys::banks::is_loaded(preset_name); }

void debug_trace(const char *text) { sndsys::debug_trace_info("%s", text); }

void play_one_shot_3d(const char *name, const Point3 &pos)
{
  if (!sndsys::play_one_shot(name, nullptr, &pos) && sndsys::is_inited())
    sndsys::debug_trace_warn("oneshot 3d sound '%s' is not played", name);
}
void play_one_shot(const char *name)
{
  if (!sndsys::play_one_shot(name) && sndsys::is_inited())
    sndsys::debug_trace_warn("oneshot sound '%s' is not played", name);
}

void release_event(sndsys::sound_handle_t event_handle)
{
  sndsys::EventHandle sh(event_handle);
  sndsys::release(sh);
}

void release_all_instances(const char *name) { sndsys::release_all_instances(name); }

void abandon(sndsys::sound_handle_t event_handle, float delay)
{
  sndsys::EventHandle sh(event_handle);
  sndsys::abandon(sh, delay);
}

void release_stream(sndsys::sound_handle_t stream_handle)
{
  sndsys::StreamHandle sh(stream_handle);
  sndsys::release(sh);
}

SQInteger play_sound_sq(HSQUIRRELVM v)
{
  if (sq_gettop(v) < 2)
  {
    G_ASSERT(0);
    return 0;
  }

  const char *name = NULL;
  Sqrat::Object params;
  float volume = 1.0f;

  int top = sq_gettop(v);
  for (int i = 2; i <= top; ++i)
  {
    switch (sq_gettype(v, i))
    {
      case OT_STRING: G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, i, &name))); break;
      case OT_INTEGER:
      case OT_FLOAT: G_VERIFY(SQ_SUCCEEDED(sq_getfloat(v, i, &volume))); break;
      case OT_TABLE:
      case OT_CLASS:
      case OT_INSTANCE:
      {
        HSQOBJECT o;
        G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(v, i, &o)));
        params = Sqrat::Object(o, v);
        break;
      }
      default: break;
    }
  }

  G_ASSERT(name);
  if (name)
    play_sound(name, params, volume, nullptr);

  return 0;
}

void set_volume(const char *snd, float volume)
{
  if (sndsys::is_inited())
    sndsys::set_volume(snd, volume);
}


sndsys::sound_handle_t init_event(const char *name, const char *path)
{
  sndsys::EventHandle handle = sndsys::init_event(name, path);
  if (!handle && sndsys::is_inited())
  {
    logerr("sqapi: sound '%s/%s' failed to play", path, name);
  }
  return (sndsys::sound_handle_t)handle;
}

void set_3d_attr(sndsys::sound_handle_t event_handle, const Point3 &pos)
{
  sndsys::set_3d_attr(sndsys::EventHandle(event_handle), pos);
}

void set_var(sndsys::sound_handle_t event_handle, const char *var_name, float value)
{
  sndsys::set_var(sndsys::EventHandle(event_handle), var_name, value);
}

int get_length(const char *name)
{
  int len = 0;
  sndsys::get_length(name, len);
  return len;
}

void set_timeline_pos(sndsys::sound_handle_t event_handle, int position) { sndsys::set_timeline_position(event_handle, position); }

int get_timeline_pos(sndsys::sound_handle_t event_handle) { return sndsys::get_timeline_position(event_handle); }

void start(sndsys::sound_handle_t event_handle) { sndsys::start(sndsys::EventHandle(event_handle)); }

bool keyoff(sndsys::sound_handle_t event_handle) { return sndsys::keyoff(sndsys::EventHandle(event_handle)); }

bool is_playing(sndsys::sound_handle_t event_handle) { return sndsys::is_playing(sndsys::EventHandle(event_handle)); }

sndsys::sound_handle_t init_stream(const char *url, const Point2 &min_max_distance)
{
  return (sndsys::sound_handle_t)sndsys::init_stream(url, min_max_distance);
}

void open_stream(sndsys::sound_handle_t stream_handle) { sndsys::open(sndsys::StreamHandle(stream_handle)); }

void close_stream(sndsys::sound_handle_t stream_handle) { sndsys::close(sndsys::StreamHandle(stream_handle)); }

void set_stream_pos(sndsys::sound_handle_t stream_handle, const Point3 &pos)
{
  sndsys::set_pos(sndsys::StreamHandle(stream_handle), pos);
}

sndsys::StreamState get_stream_state(sndsys::sound_handle_t stream_handle)
{
  return sndsys::get_state(sndsys::StreamHandle(stream_handle));
}

const char *get_stream_state_name(sndsys::sound_handle_t stream_handle)
{
  return sndsys::get_state_name(sndsys::StreamHandle(stream_handle));
}

SQInteger get_output_devices(HSQUIRRELVM sqvm)
{
  Sqrat::Array result(sqvm);
  for (const sndsys::DeviceInfo &dev : sndsys::get_output_devices())
  {
    Sqrat::Table info(sqvm);
    info.SetValue("name", dev.name);
    info.SetValue("id", dev.deviceId);
    info.SetValue("rate", dev.rate);
    result.Append(info);
  }
  Sqrat::PushVar(sqvm, result);
  return 1;
}

SQInteger get_record_devices(HSQUIRRELVM sqvm)
{
  Sqrat::Array result(sqvm);
  for (const sndsys::DeviceInfo &dev : sndsys::get_record_devices())
  {
    Sqrat::Table info(sqvm);
    info.SetValue("name", dev.name);
    info.SetValue("id", dev.deviceId);
    info.SetValue("rate", dev.rate);
    result.Append(info);
  }
  Sqrat::PushVar(sqvm, result);
  return 1;
}

void set_output_device(int dev_id) { sndsys::set_output_device(dev_id); }


void release_vm(HSQUIRRELVM vm)
{
  if (!script_callbacks.IsNull() && script_callbacks.GetVM() == vm)
    script_callbacks.Release();
}

void set_callbacks(Sqrat::Object callbacks_) { script_callbacks = callbacks_; }

void on_record_devices_list_changed()
{
  if (!script_callbacks.IsNull())
    sqrex::within(script_callbacks).exec("on_record_devices_list_changed");
}

void on_output_devices_list_changed()
{
  if (!script_callbacks.IsNull())
    sqrex::within(script_callbacks).exec("on_output_devices_list_changed");
}
} // namespace sqapi
} // namespace sound

///@module sound
SQ_DEF_AUTO_BINDING_MODULE(bind_sound, "sound") // To consider: split client & server scripts and remove (most of) this binding from
                                                // server
{
  using namespace sound::sqapi;

  Sqrat::Table soundTbl(vm);
  soundTbl.SetValue("INVALID_SOUND_HANDLE", (int)sndsys::INVALID_SOUND_HANDLE);

  soundTbl.SetValue("SOUND_STREAM_ERROR", (int)sndsys::StreamState::ERROR);
  soundTbl.SetValue("SOUND_STREAM_CLOSED", (int)sndsys::StreamState::CLOSED);
  soundTbl.SetValue("SOUND_STREAM_OPENED", (int)sndsys::StreamState::OPENED);
  soundTbl.SetValue("SOUND_STREAM_CONNECTING", (int)sndsys::StreamState::CONNECTING);
  soundTbl.SetValue("SOUND_STREAM_BUFFERING", (int)sndsys::StreamState::BUFFERING);
  soundTbl.SetValue("SOUND_STREAM_STOPPED", (int)sndsys::StreamState::STOPPED);
  soundTbl.SetValue("SOUND_STREAM_PAUSED", (int)sndsys::StreamState::PAUSED);
  soundTbl.SetValue("SOUND_STREAM_PLAYING", (int)sndsys::StreamState::PLAYING);

  soundTbl //
    .Func("sound_debug_trace", debug_trace)
    .Func("sound_play_one_shot_3d", play_one_shot_3d)
    .Func("sound_get_num_event_instances", get_num_event_instances)
    .Func("sound_is_preset_loaded", is_preset_loaded)
    .Func("sound_play_one_shot", play_one_shot)
    .Func("sound_release_all_instances", release_all_instances)
    .SquirrelFunc("sound_play", play_sound_sq, -2, ".s")
    .Func("sound_set_volume", set_volume)
    .Func("sound_init_event", init_event)
    .Func("sound_release_event", release_event)
    .Func("sound_set_3d_attr", set_3d_attr)
    .Func("sound_set_var", set_var)
    .Func("sound_get_length", get_length)
    .Func("sound_set_timeline_pos", set_timeline_pos)
    .Func("sound_get_timeline_pos", get_timeline_pos)
    .Func("sound_start", start)
    .Func("sound_keyoff", keyoff)
    .Func("sound_abandon", abandon)
    .Func("sound_is_playing", is_playing)
    .Func("sound_init_stream", init_stream)
    .Func("sound_open_stream", open_stream)
    .Func("sound_close_stream", close_stream)
    .Func("sound_set_stream_pos", set_stream_pos)
    .Func("sound_release_stream", release_stream)
    .Func("sound_get_stream_state", get_stream_state)
    .Func("sound_get_stream_state_name", get_stream_state_name)
    .SquirrelFunc("sound_get_output_devices", get_output_devices, 1, ".")
    .SquirrelFunc("sound_get_record_devices", get_record_devices, 1, ".")
    .Func("sound_set_output_device", set_output_device)
    .Func("sound_set_callbacks", set_callbacks)
    /**/;

  return soundTbl;
}
