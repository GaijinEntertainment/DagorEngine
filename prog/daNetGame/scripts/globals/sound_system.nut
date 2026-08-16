from "types" import String, Float, Integer
let sound = require_optional("sound")
if (sound != null)
  return sound
return freeze({
  apply_audio_settings = @(_changedfileds) null
  sound_get_output_devices = @() []
  sound_get_record_devices = @() []
  sound_set_callbacks = @(_cb) null
  sound_set_output_device = @(_dev) null
  sound_release_event = @(_evh) null
  sound_is_playing = @(_evh) null
  sound_start = @(_evh) null
  sound_init_event = @(event, path) assert(event instanceof String) && assert(path instanceof String)
  sound_keyoff = @(_evh) null
  sound_set_volume = @(_eh, volume) assert(volume instanceof Float)
  sound_play_oneshot = @(_evt) null
  sound_set_fixed_time_speed = @(_time) null
  sound_get_length = @(path) assert(path instanceof String)
  sound_set_timeline_pos = @(_evh, pos) assert(pos instanceof Integer)
  sound_get_timeline_pos = @(_evh) null

  sound_play = @(soundEventName, _volume_or_params = null) assert(soundEventName instanceof String)

  INVALID_SOUND_HANDLE = 0

  SOUND_STREAM_ERROR = 1
  SOUND_STREAM_CLOSED = 2
  SOUND_STREAM_OPENED = 3
  SOUND_STREAM_CONNECTING = 4
  SOUND_STREAM_BUFFERING = 5
  SOUND_STREAM_STOPPED = 6
  SOUND_STREAM_PAUSED = 7
  SOUND_STREAM_PLAYING = 8
})
