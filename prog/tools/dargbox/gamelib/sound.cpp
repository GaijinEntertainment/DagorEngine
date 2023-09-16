#include "sound.h"
#include <array>
#include <EASTL/array.h>
#include <EASTL/unordered_set.h>
#include <EASTL/unordered_map.h>
#include <EASTL/string.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_critSec.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqStackChecker.h>
#include <sqstdblob.h>
#include <sqext.h>
#include <memory/dag_framemem.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"


using namespace eastl;

#define OUTPUT_SAMPLE_RATE 48000
#define OUTPUT_CHANNELS    2

#define MAX_PLAYING_SOUNDS  128 // power of 2
#define PLAYING_SOUNDS_MASK (MAX_PLAYING_SOUNDS - 1)
#define ONE_DIV_256         (1.f / 256)
#define ONE_DIV_512         (1.f / 512)

#define PRINT_ERROR logerr
#define PRINT_NOTE  debug


namespace gamelib
{

namespace sound
{

static int playing_sound_count = 0;
static int64_t total_samples_played = 0;
static volatile double total_time_played = 0.0;

static WinCritSec sound_cs;
static volatile bool sound_cs_manual_entered = false;

static float master_volume = 1.0f;

static uint64_t memory_used = 0;


struct PcmSound
{
private:
  float *data;

public:
  int frequency;
  int samples;
  int channels;
  unsigned memoryUsed;

  float *getData() const { return data; }

  void newData(size_t size)
  {
    memoryUsed = size;
    data = new float[(size + 3) / sizeof(float)];
    memory_used += memoryUsed;
  }

  void deleteData()
  {
    memory_used -= memoryUsed;

    delete[] data;
    data = nullptr;
  }

  bool isValid() const { return !!data; }

  inline int getDataMemorySize() const { return channels * (samples + 4) * sizeof(float); }

  float getDuration() const { return samples / float(frequency); }

  int getFrequency() const { return frequency; }

  int getSamples() const { return samples; }

  int getChannels() const { return channels; }

  PcmSound()
  {
    memoryUsed = 0;
    frequency = 44100;
    samples = 0;
    channels = 1;
    data = nullptr;
    ++instance_count;
  }

  PcmSound(const PcmSound &b)
  {
    memoryUsed = 0;
    frequency = b.frequency;
    samples = b.samples;
    channels = b.channels;
    newData(getDataMemorySize());
    memcpy(data, b.data, getDataMemorySize());
    ++instance_count;
  }

  PcmSound &operator=(const PcmSound &b);
  PcmSound(PcmSound &&b);
  PcmSound &operator=(PcmSound &&b);
  ~PcmSound();

  static int instance_count;
};

int PcmSound::instance_count = 0;


struct PlayingSound
{
  const PcmSound *sound;
  double pos;      // in samples
  double startPos; // in samples
  double stopPos;  // in samples
  float pitch;
  float volume;
  float pan;
  float volumeL;
  float volumeR;
  float volumeTrendL;
  float volumeTrendR;
  double timeToStart; // in seconds
  int channels;
  int version;
  bool loop;
  bool stopMode;
  bool waitingStart;

  PlayingSound() { memset(this, 0, sizeof(*this)); }

  bool isEmpty() { return !sound && !stopMode && !waitingStart; }

  void setStopMode()
  {
    if (!sound)
    {
      waitingStart = false;
      return;
    }

    if (stopMode)
    {
      waitingStart = false;
      return;
    }

    version += MAX_PLAYING_SOUNDS;

    if (waitingStart)
    {
      waitingStart = false;
      sound = nullptr;
      return;
    }

    if (channels == 1)
    {
      float val = sound->getData()[unsigned(pos)];
      volumeL *= val;
      volumeR *= val;
    }
    else
    {
      volumeL *= sound->getData()[unsigned(pos) * 2];
      volumeR *= sound->getData()[unsigned(pos) * 2 + 1];
    }
    volumeTrendL = sign(volumeL) * -(1.f / 10000);
    volumeTrendR = sign(volumeR) * -(1.f / 10000);
    stopMode = true;
    sound = nullptr;
  }

  void mixTo(float *__restrict mix, int count, int frequency, double inv_frequency, double buffer_time)
  {
    float wishVolumeL = master_volume * volume * ::min(1.0f + pan, 1.0f);
    float wishVolumeR = master_volume * volume * ::min(1.0f - pan, 1.0f);
    float *__restrict sndData = sound ? sound->getData() : nullptr;

    double advance = sound ? double(sound->frequency) * inv_frequency * pitch : 1.0;

    if (!stopMode && !waitingStart && sound && volumeL > 0.0f && volumeR > 0.0f && wishVolumeL == volumeL && wishVolumeR == volumeR &&
        pos + advance * count < stopPos && sndData != nullptr)
    {
      if (channels == 1)
      {
        for (int i = 0; i < count; i++, mix += 2)
        {
          unsigned ip = unsigned(pos);
          float t = float(pos - ip);
          float v = lerp(sndData[ip], sndData[ip + 1], t);
          mix[0] += v * volumeL;
          mix[1] += v * volumeR;
          pos += advance;
        }
      }
      else // channels == 2
      {
        for (int i = 0; i < count; i++, mix += 2)
        {
          unsigned ip = unsigned(pos);
          float t = float(pos - ip);
          float vl = lerp(sndData[ip * 2], sndData[ip * 2 + 2], t);
          float vr = lerp(sndData[ip * 2 + 1], sndData[ip * 2 + 2 + 1], t);
          mix[0] += vl * volumeL;
          mix[1] += vr * volumeR;
          pos += advance;
        }
      }

      return;
    }

    if (waitingStart && timeToStart > buffer_time)
    {
      timeToStart -= buffer_time;
      return;
    }

    if (!sndData && !stopMode)
      stopMode = true;

    if (channels == 1)
    {
      for (int i = 0; i < count; i++, mix += 2)
      {
        if (waitingStart)
        {
          timeToStart -= inv_frequency;
          if (timeToStart <= 0.0)
          {
            waitingStart = false;
            pos = startPos;
          }
        }
        else if (!stopMode)
        {
          unsigned ip = unsigned(pos);
          float t = float(pos - ip);
          float v = lerp(sndData[ip], sndData[ip + 1], t);

          mix[0] += v * volumeL;
          mix[1] += v * volumeR;

          if (volumeL != wishVolumeL)
          {
            if (fabsf(volumeL - wishVolumeL) <= ONE_DIV_512)
              volumeL = wishVolumeL;
            else if (volumeL < wishVolumeL)
              volumeL += ONE_DIV_512;
            else
              volumeL -= ONE_DIV_512;
          }

          if (volumeR != wishVolumeR)
          {
            if (fabsf(volumeR - wishVolumeR) <= ONE_DIV_512)
              volumeR = wishVolumeR;
            else if (volumeR < wishVolumeR)
              volumeR += ONE_DIV_512;
            else
              volumeR -= ONE_DIV_512;
          }

          pos += advance;
          if (pos >= stopPos)
          {
            if (loop)
              pos = startPos;
            else
            {
              pos = stopPos;
              setStopMode();
            }
          }
        }
        else // stopMode
        {
          if (fabsf(volumeL) <= ONE_DIV_512)
            volumeL = 0.0f;
          else
          {
            volumeL += volumeTrendL;
            volumeL *= 0.997f;
          }

          if (fabsf(volumeR) <= ONE_DIV_512)
            volumeR = 0.0f;
          else
          {
            volumeR += volumeTrendR;
            volumeR *= 0.997f;
          }

          if (volumeR == 0.f && volumeL == 0.f)
          {
            stopMode = false;
            break;
          }

          mix[0] += volumeL;
          mix[1] += volumeR;
        }
      }
    }
    else // channels == 2
    {
      for (int i = 0; i < count; i++, mix += 2)
      {
        if (waitingStart)
        {
          timeToStart -= inv_frequency;
          if (timeToStart <= 0.0)
          {
            waitingStart = false;
            pos = startPos;
          }
        }
        else if (!stopMode)
        {
          unsigned ip = unsigned(pos);
          float t = float(pos - ip);
          float vl = lerp(sndData[ip * 2], sndData[ip * 2 + 2], t);
          float vr = lerp(sndData[ip * 2 + 1], sndData[ip * 2 + 2 + 1], t);

          mix[0] += vl * volumeL;
          mix[1] += vr * volumeR;

          if (volumeL != wishVolumeL)
          {
            if (fabsf(volumeL - wishVolumeL) <= ONE_DIV_512)
              volumeL = wishVolumeL;
            else if (volumeL < wishVolumeL)
              volumeL += ONE_DIV_512;
            else
              volumeL -= ONE_DIV_512;
          }

          if (volumeR != wishVolumeR)
          {
            if (fabsf(volumeR - wishVolumeR) <= ONE_DIV_512)
              volumeR = wishVolumeR;
            else if (volumeR < wishVolumeR)
              volumeR += ONE_DIV_512;
            else
              volumeR -= ONE_DIV_512;
          }

          pos += advance;
          if (pos >= stopPos)
          {
            if (loop)
              pos = startPos;
            else
            {
              pos = stopPos;
              setStopMode();
            }
          }
        }
        else // stopMode
        {
          if (fabsf(volumeL) <= ONE_DIV_512)
            volumeL = 0.0f;
          else
          {
            volumeL += volumeTrendL;
            volumeL *= 0.997f;
          }

          if (fabsf(volumeR) <= ONE_DIV_512)
            volumeR = 0.0f;
          else
          {
            volumeR += volumeTrendR;
            volumeR *= 0.997f;
          }

          if (volumeR == 0.f && volumeL == 0.f)
          {
            stopMode = false;
            break;
          }

          mix[0] += volumeL;
          mix[1] += volumeR;
        }
      }
    }
  }
};

static array<PlayingSound, MAX_PLAYING_SOUNDS> playing_sounds;

int get_total_sound_count() { return PcmSound::instance_count; }

int get_playing_sound_count() { return int(playing_sound_count); }

static int allocate_playing_sound()
{
  for (int i = 1; i < MAX_PLAYING_SOUNDS; i++)
    if (playing_sounds[i].isEmpty())
    {
      playing_sounds[i].version += MAX_PLAYING_SOUNDS;
      return i;
    }
  return -1;
}

static bool is_handle_valid(PlayingSoundHandle ps)
{
  unsigned idx = ps & PLAYING_SOUNDS_MASK;
  return (playing_sounds[idx].version == (ps & (~PLAYING_SOUNDS_MASK))) && idx > 0;
}

static int handle_to_index(PlayingSoundHandle ps)
{
  if (!is_handle_valid(ps))
    return -1;
  return ps & PLAYING_SOUNDS_MASK;
}


static float g_limiter_mult = 1.0f;

static void apply_limiter(float *__restrict buf, int count)
{
  float limiter_mult = g_limiter_mult;
  for (int i = 0; i < count; i++, buf++)
  {
    float v = *buf * limiter_mult;
    *buf = v;
    if (fabsf(v) > 1.0f)
      limiter_mult *= 0.96f;
    if (limiter_mult < 1.0f)
      limiter_mult = ::min(limiter_mult + (0.5f / 65536), 1.0f);
  }

  g_limiter_mult = limiter_mult;
}


static void fill_buffer_cb(float *__restrict out_buf, int frequency, int channels, int samples)
{
  int cnt = 0;
  double total_time_played_ = total_time_played;
  {
    WinAutoLock lock(sound_cs);
    float *mixCursor = out_buf;
    int samplesLeft = samples;
    memset(out_buf, 0, samples * channels * sizeof(float));

    double invFrequency = 1.0 / frequency;
    int step = 256;

    while (samplesLeft > 0)
    {
      cnt = 0;
      for (auto &&s : playing_sounds)
        if (!s.isEmpty())
        {
          cnt++;
          s.mixTo(mixCursor, ::min(samplesLeft, step), frequency, invFrequency, ::min(samplesLeft, step) * invFrequency);
        }

      samplesLeft -= step;
      mixCursor += step * channels;
      total_samples_played += ::min(samplesLeft, step);
      total_time_played_ += ::min(samplesLeft, step) * invFrequency;
    }
  }
  apply_limiter(out_buf, samples * channels);
  playing_sound_count = cnt;

  total_time_played = total_time_played_;
}


static ma_device miniaudio_device;
static volatile bool device_initialized = false;
static ma_log ma_log_struct = {0};
static ma_context context = {0};


static void on_error_log(void *user_data, ma_uint32 level, const char *message)
{
  G_UNUSED(user_data);
  if (level <= 1)
    PRINT_ERROR("%s", message);
  else
    PRINT_NOTE("%s", message);
}

static void miniaudio_data_callback(ma_device *p_device, void *p_output, const void *p_input, ma_uint32 frame_count)
{
  G_UNUSED(p_input);

  if (!device_initialized)
  {
    memset(p_output, 0, frame_count * ma_get_bytes_per_frame(p_device->playback.format, p_device->playback.channels));
    return;
  }

  fill_buffer_cb((float *)p_output, OUTPUT_SAMPLE_RATE, p_device->playback.channels, frame_count);
}

void init_sound_lib_internal()
{
  if (device_initialized)
    return;

  ma_log_init(nullptr, &ma_log_struct);
  ma_log_register_callback(&ma_log_struct, {on_error_log, nullptr});

  ma_context_init(NULL, 0, NULL, &context);
  context.pLog = &ma_log_struct;

  ma_device_config deviceConfig;

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = OUTPUT_CHANNELS;
  deviceConfig.sampleRate = OUTPUT_SAMPLE_RATE;
  deviceConfig.dataCallback = miniaudio_data_callback;
  deviceConfig.pUserData = nullptr;

  if (ma_device_init(&context, &deviceConfig, &miniaudio_device) != MA_SUCCESS)
  {
    PRINT_ERROR("SOUND: Failed to open playback device");
    return;
  }

  PRINT_NOTE("Sound device name: %s", miniaudio_device.playback.name);

  if (ma_device_start(&miniaudio_device) != MA_SUCCESS)
  {
    PRINT_ERROR("SOUND: Failed to start playback device");
    ma_device_uninit(&miniaudio_device);
    return;
  }

  device_initialized = true;
}

void initialize() { memset(&playing_sounds[0], 0, sizeof(playing_sounds[0]) * playing_sounds.size()); }

void finalize()
{
  device_initialized = false;
  WinAutoLock lock(sound_cs);
  ma_device_uninit(&miniaudio_device);
}


static PcmSound *create_sound_from_data(int frequency, int channels, dag::ConstSpan<float> data, string &err_msg)
{
  if (!device_initialized)
    init_sound_lib_internal();

  if (frequency < 1)
  {
    err_msg = eastl::string(eastl::string::CtorSprintf{}, "Invalid frequency %d", frequency);
    return nullptr;
  }

  if (channels != 1 && channels != 2)
  {
    err_msg =
      eastl::string(eastl::string::CtorSprintf{}, "Only mono and stereo sounds are supported, requested %d channels", channels);
    return nullptr;
  }

  if (!data.size())
  {
    err_msg = "Empty data";
    return nullptr;
  }

  PcmSound *s = new PcmSound();
  s->frequency = frequency;
  s->channels = channels;
  s->samples = data.size() / channels;
  s->newData(s->getDataMemorySize());
  memcpy(s->getData(), data.data(), s->getDataMemorySize());

  for (int chn = 0; chn < channels; ++chn)
    s->getData()[s->samples * channels + chn] = s->getData()[chn];

  return s;
}


static PcmSound *create_sound_from_file(const char *file_name, string &err_msg)
{
  if (!device_initialized)
    init_sound_lib_internal();

  if (!file_name || !file_name[0])
  {
    err_msg = "Cannot create sound. File name is empty";
    return nullptr;
  }

  // if (!fs::is_path_string_valid(file_name))
  // {
  //   PRINT_ERROR("Cannot open sound '%s'. Absolute paths or access to the parent directory is prohibited.", file_name);
  //   return PcmSound();
  // }

  unsigned int channels;
  unsigned int sampleRate;
  ma_uint64 totalPCMFrameCount;
  float *pSampleData = nullptr;

  const char *p = strrchr(file_name, '.');
  if (p && !stricmp(p, ".wav") && !p[4])
    pSampleData = ma_dr_wav_open_file_and_read_pcm_frames_f32(file_name, &channels, &sampleRate, &totalPCMFrameCount, nullptr);
  else if (p && !stricmp(p, ".mp3") && !p[4])
  {
    ma_dr_mp3_config config = {0};
    pSampleData = ma_dr_mp3_open_file_and_read_pcm_frames_f32(file_name, &config, &totalPCMFrameCount, nullptr);
    channels = config.channels;
    sampleRate = config.sampleRate;
  }
  else if (p && !stricmp(p, ".flac") && !p[4])
    pSampleData = ma_dr_wav_open_file_and_read_pcm_frames_f32(file_name, &channels, &sampleRate, &totalPCMFrameCount, nullptr);
  else
  {
    err_msg = eastl::string(eastl::string::CtorSprintf{},
      "Cannot create sound from '%s', unrecognized file format. Expected .wav, .flac or .mp3", file_name);
    return nullptr;
  }

  if (!pSampleData)
  {
    err_msg = eastl::string(eastl::string::CtorSprintf{}, "Cannot create sound from '%s'", file_name);
    return nullptr;
  }

  if (channels != 1 && channels != 2)
  {
    err_msg = eastl::string(eastl::string::CtorSprintf{}, "Cannot create sound from '%s', invalid channels count = %d", int(channels));
    return nullptr;
  }

  PcmSound *s = new PcmSound();
  s->channels = int(channels);
  s->frequency = int(sampleRate);
  s->samples = int(totalPCMFrameCount);
  s->newData(s->getDataMemorySize());
  memcpy(s->getData(), pSampleData, channels * s->samples * sizeof(float));
  if (s->channels == 2)
  {
    s->getData()[s->samples * 2] = s->getData()[0];
    s->getData()[s->samples * 2 + 1] = s->getData()[1];
  }
  else
  {
    s->getData()[s->samples] = s->getData()[0];
  }

  ma_dr_wav_free(pSampleData, NULL);

  return s;
}


static SQInteger get_sound_data(HSQUIRRELVM vm)
{
  if (!Sqrat::vargs::check_var_types<const PcmSound *>(vm, 1))
    return sq_throwerror(vm, "PcmSound instance expected");

  const PcmSound *pcm = Sqrat::Var<const PcmSound *>(vm, 1).value;

  SQUserPointer blobData = nullptr;
  if (SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blobData)))
  {
    SQInteger blobSize = sqstd_getblobsize(vm, 2);
    int nSamples = ::min<int>(pcm->samples, blobSize / (pcm->channels * sizeof(float)));
    int outSize = nSamples * pcm->channels * sizeof(float);
    memcpy(blobData, pcm->getData(), outSize);
    memset((char *)blobData + outSize, 0, blobSize - outSize);
  }
  else if (sq_gettype(vm, 2) == OT_ARRAY)
  {
    sq_arrayresize(vm, 2, pcm->channels * pcm->samples);
    float *data = pcm->getData();
    for (SQInteger i = 0, n = pcm->channels * pcm->samples; i < n; ++i)
    {
      sq_pushinteger(vm, i);
      sq_pushfloat(vm, data[i]);
      sq_rawset(vm, 2);
    }
  }
  else
    return sq_throwerror(vm, "Data destination must be a blob or an array");

  return 0;
}


static SQInteger set_sound_data(HSQUIRRELVM vm)
{
  if (!Sqrat::vargs::check_var_types<const PcmSound *>(vm, 1))
    return sq_throwerror(vm, "PcmSound instance expected");

  const PcmSound *pcm = Sqrat::Var<const PcmSound *>(vm, 1).value;

  SQUserPointer blobData = nullptr;
  if (SQ_SUCCEEDED(sqstd_getblob(vm, 2, &blobData)))
  {
    SQInteger blobSize = sqstd_getblobsize(vm, 2);
    int pcmSize = pcm->getDataMemorySize();
    int usedSize = ::min<int>(pcmSize, blobSize);
    memcpy(pcm->getData(), blobData, usedSize);
    memset((char *)(pcm->getData()) + pcmSize, 0, pcmSize - usedSize);
  }
  else if (sq_gettype(vm, 2) == OT_ARRAY)
  {
    SQInteger nSamples = ::min<int>(sq_getsize(vm, 2), pcm->channels * pcm->samples);
    HSQOBJECT hArray;
    sq_getstackobj(vm, 2, &hArray);
    sq_ext_get_array_floats(hArray, 0, nSamples, (float *)pcm->getData());
    memset((char *)(pcm->getData()) + nSamples * sizeof(float), 0, pcm->getDataMemorySize() - nSamples * sizeof(float));
  }
  else
    return sq_throwerror(vm, "Data destination must be a blob or an array");

  if (pcm->channels == 2)
  {
    pcm->getData()[pcm->samples * 2] = pcm->getData()[0];
    pcm->getData()[pcm->samples * 2 + 1] = pcm->getData()[1];
  }
  else
  {
    pcm->getData()[pcm->samples] = pcm->getData()[0];
  }
  return 0;
}


static SQInteger play_sound(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<PcmSound *>(vm, 2))
    return sq_throwerror(vm, "Valid PcmSound expected");

  Sqrat::Var<const PcmSound *> varSnd(vm, 2);
  if (varSnd.value == nullptr)
    return sq_throwerror(vm, "PcmSound is null");

  const PcmSound &sound = *varSnd.value;

  Sqrat::Table params;
  if (sq_gettop(vm) > 2)
    params = Sqrat::Var<Sqrat::Table>(vm, 3).value;
  else
    params = Sqrat::Object(vm);

  if (!device_initialized)
    init_sound_lib_internal();

  float volume = params.GetSlotValue("volume", 1.0f);
  float pitch = params.GetSlotValue("pitch", 1.0);
  float pan = params.GetSlotValue("pan", 0.0f);
  float start_time = params.GetSlotValue("start_time", 0.0f);
  float end_time = params.GetSlotValue("end_time", VERY_BIG_NUMBER);
  bool loop = params.GetSlotValue("loop", false);
  float defer_time_sec = params.GetSlotValue("defer", 0.0f);

  WinAutoLock lock(sound_cs);

  int idx = allocate_playing_sound();
  if (idx < 0 || sound.samples <= 2)
    return PlayingSoundHandle();

  PlayingSound &s = playing_sounds[idx];

  pitch = ::clamp(pitch, 0.00001f, 1000.0f);
  pan = ::clamp(pan, -1.0f, 1.0f);
  volume = ::clamp(volume, 0.0f, 100000.0f);

  double start = ::clamp(double(int64_t(start_time * sound.frequency)), 0.0, double(sound.samples - 1));
  double stop = ::clamp(double(int64_t(end_time * sound.frequency)), start, double(sound.samples - 1));
  double pos = start;
  if (defer_time_sec < 0.0f)
    pos = ::min(double(int(-defer_time_sec * sound.frequency)), stop);

  s.channels = sound.channels;
  s.sound = &sound;
  s.volume = volume;
  s.pitch = pitch;
  s.pan = pan;
  s.volumeL = master_volume * volume * ::min(1.0f + pan, 1.0f);
  s.volumeR = master_volume * volume * ::min(1.0f - pan, 1.0f);

  s.pos = pos;
  s.startPos = start;
  s.stopPos = stop;
  s.loop = loop;
  s.stopMode = false;
  s.timeToStart = ::max(defer_time_sec, 0.0f);
  s.waitingStart = (s.timeToStart != 0.0);

  PlayingSoundHandle res = idx | s.version;
  sq_pushinteger(vm, res);
  return 1;
}


void set_sound_pitch(PlayingSoundHandle handle, float pitch)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return;
  playing_sounds[idx].pitch = pitch;
}

void set_sound_volume(PlayingSoundHandle handle, float volume)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return;
  playing_sounds[idx].volume = volume;
}

void set_sound_pan(PlayingSoundHandle handle, float pan)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return;
  playing_sounds[idx].pan = pan;
}

bool is_playing(PlayingSoundHandle handle)
{
  int idx = handle_to_index(handle);
  if (idx < 0 || playing_sounds[idx].stopMode)
    return false;

  return true;
}

float get_sound_play_pos(PlayingSoundHandle handle)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return 0.0f;

  if (!playing_sounds[idx].sound || playing_sounds[idx].stopMode || playing_sounds[idx].waitingStart)
    return 0.0f;

  return float(playing_sounds[idx].pos / playing_sounds[idx].sound->frequency);
}

void set_sound_play_pos(PlayingSoundHandle handle, float pos_seconds)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return;

  if (!playing_sounds[idx].sound || playing_sounds[idx].stopMode)
    return;

  double p = floor(playing_sounds[idx].sound->frequency * pos_seconds);
  playing_sounds[idx].pos = ::clamp(p, playing_sounds[idx].startPos, playing_sounds[idx].stopPos);
}

void stop_sound(PlayingSoundHandle handle)
{
  WinAutoLock lock(sound_cs);

  int idx = handle_to_index(handle);
  if (idx < 0)
    return;

  if (!playing_sounds[idx].sound || playing_sounds[idx].stopMode)
    return;

  playing_sounds[idx].setStopMode();
}

void stop_all_sounds()
{
  WinAutoLock lock(sound_cs);

  for (auto &&s : playing_sounds)
    if (!s.isEmpty())
      s.setStopMode();
}

void enter_sound_critical_section()
{
  if (sound_cs_manual_entered)
    return;

  sound_cs.lock();
  sound_cs_manual_entered = true;
}

void leave_sound_critical_section()
{
  if (sound_cs_manual_entered)
  {
    sound_cs_manual_entered = false;
    sound_cs.unlock();
  }
}

void set_master_volume(float volume)
{
  WinAutoLock lock(sound_cs);
  master_volume = volume;
}

float get_output_sample_rate() { return OUTPUT_SAMPLE_RATE; }

int64_t get_total_samples_played() { return total_samples_played; }

double get_total_time_played() { return total_time_played; }

double get_memory_used() { return double(memory_used); }


PcmSound::PcmSound(PcmSound &&b)
{
  WinAutoLock lock(sound_cs);

  for (auto &&s : playing_sounds)
    if (s.sound == this || s.sound == &b)
      if (!s.isEmpty())
        s.setStopMode();

  frequency = b.frequency;
  samples = b.samples;
  channels = b.channels;
  memoryUsed = b.memoryUsed;
  data = b.data;

  b.memoryUsed = 0;
  b.data = nullptr;

  ++instance_count;
}

PcmSound &PcmSound::operator=(const PcmSound &b)
{
  if (this == &b)
    return *this;

  WinAutoLock lock(sound_cs);

  for (auto &&s : playing_sounds)
    if (s.sound == this)
      if (!s.isEmpty())
        s.setStopMode();

  deleteData();
  frequency = b.frequency;
  samples = b.samples;
  channels = b.channels;
  newData(getDataMemorySize());
  memcpy(data, b.data, getDataMemorySize());
  return *this;
}

PcmSound &PcmSound::operator=(PcmSound &&b)
{
  if (this == &b)
    return *this;

  WinAutoLock lock(sound_cs);

  for (auto &&s : playing_sounds)
    if (s.sound == this || s.sound == &b)
      if (!s.isEmpty())
        s.setStopMode();

  frequency = b.frequency;
  samples = b.samples;
  channels = b.channels;
  data = b.data;
  memoryUsed = b.memoryUsed;

  b.data = nullptr;
  b.memoryUsed = 0;

  return *this;
}


PcmSound::~PcmSound()
{
  if (!data)
    return;

  WinAutoLock lock(sound_cs);

  for (auto &&s : playing_sounds)
    if (s.sound == this)
      if (!s.isEmpty())
        s.setStopMode();

  deleteData();
  samples = 0;
  --instance_count;
}


class SqratPcmSoundClass : public Sqrat::Class<PcmSound, Sqrat::NoCopy<PcmSound>>
{
public:
  SqratPcmSoundClass(HSQUIRRELVM vm) : Sqrat::Class<PcmSound, Sqrat::NoCopy<PcmSound>>(vm, "PcmSound")
  {
    BindConstructor(CustomNew, 1);
  }

  static SQInteger CustomNew(HSQUIRRELVM vm)
  {
    if (sq_gettop(vm) < 2 || (sq_gettype(vm, 2) != OT_STRING && sq_gettype(vm, 2) != OT_TABLE))
      return sq_throwerror(vm, _SC("Invalid PcmSound constructor"));

    PcmSound *s = nullptr;
    string errMsg;

    if (sq_gettype(vm, 2) == OT_STRING)
    {
      const SQChar *fn = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &fn)));
      s = create_sound_from_file(fn, errMsg);
    }
    else
    {
      Sqrat::Table params = Sqrat::Var<Sqrat::Table>(vm, 2).value;
      int freq = params.RawGetSlotValue("freq", 0);
      int channels = params.RawGetSlotValue("channels", 0);
      Sqrat::Object objData = params.RawGetSlot("data");
      if (!freq || !channels || objData.IsNull())
        return sq_throwerror(vm, "Required parameters: freq:int, channels:int, data:blob|array");

      if (objData.GetType() == OT_INSTANCE)
      {
        SqStackChecker chk(vm);
        sq_pushobject(vm, objData);
        SQUserPointer dataPtr;
        if (SQ_FAILED(sqstd_getblob(vm, -1, &dataPtr)))
          return sq_throwerror(vm, "data parameter must be blob or array");
        SQInteger dataSize = sqstd_getblobsize(vm, -1);
        chk.restore();
        s = create_sound_from_data(freq, channels, make_span_const((const float *)dataPtr, dataSize / sizeof(float)), errMsg);
      }
      else if (objData.GetType() == OT_ARRAY)
      {
        SqStackChecker chk(vm);
        sq_pushobject(vm, objData);
        SQInteger len = sq_getsize(vm, -1);
        Tab<float> data(framemem_ptr());
        data.resize(len);
        sq_ext_get_array_floats(objData, 0, len, data.data());
        chk.restore();
        s = create_sound_from_data(freq, channels, data, errMsg);
      }
    }

    if (!s)
      return sq_throwerror(vm, errMsg.c_str());

    Sqrat::ClassType<PcmSound>::SetManagedInstance(vm, 1, s);
    return 0;
  }
};


void on_script_reload(bool hard)
{
  G_UNUSED(hard);
  stop_all_sounds();
}


void bind_script(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  SqratPcmSoundClass sqPcmSound(vm);
  sqPcmSound.SquirrelFunc("getSoundData", get_sound_data, 2, "x x|a")
    .SquirrelFunc("setSoundData", set_sound_data, 2, "x x|a")
    .Prop("dataMemorySize", &PcmSound::getDataMemorySize)
    .Prop("duration", &PcmSound::getDuration)
    .Prop("freq", &PcmSound::getFrequency)
    .Prop("samples", &PcmSound::getSamples)
    .Prop("channels", &PcmSound::getChannels);

  Sqrat::Table exports(vm);
  exports.SquirrelFunc("play_sound", play_sound, -2, ". x t|o")
    .Func("set_sound_pitch", set_sound_pitch)
    .Func("set_sound_volume", set_sound_volume)
    .Func("set_sound_pan", set_sound_pan)
    .Func("is_playing", is_playing)
    .Func("get_sound_play_pos", get_sound_play_pos)
    .Func("set_sound_play_pos", set_sound_play_pos)
    .Func("stop_sound", stop_sound)
    .Func("stop_all_sounds", stop_all_sounds)
    .Func("set_master_volume", set_master_volume)
    .Func("get_output_sample_rate", get_output_sample_rate)
    .Func("get_total_samples_played", get_total_samples_played)
    .Func("get_total_time_played", get_total_time_played);
  exports.Bind("PcmSound", sqPcmSound);
  module_mgr->addNativeModule("gamelib.sound", exports);
}

} // namespace sound

} // namespace gamelib