#include <soundSystem/userCreatedSound.h>
#include <fmod_studio.hpp>
#include <osApiWrappers/dag_critSec.h>
#include <soundSystem/soundSystem.h>
#include "internal/debug.h"

namespace sndsys
{

static CustomStereoSoundCb fill_buffer_cb = nullptr;
static int frequency = 44100;
static int channels = 2;
uint64_t custom_sound_play_pos = 0;

namespace fmodapi
{
FMOD::System *get_system();
}

static FMOD::Sound *sound = nullptr;
static FMOD::Channel *channel = nullptr;


static WinCritSec crit_sec;


FMOD_RESULT F_CALLBACK pcmreadcallback(FMOD_SOUND *, void *data, unsigned int datalen)
{
  WinAutoLock lock(crit_sec);
  int samples = datalen / (sizeof(float) * channels);
  if (fill_buffer_cb)
    fill_buffer_cb((float *)data, frequency, channels, samples);
  custom_sound_play_pos += samples;
  return FMOD_OK;
}

FMOD_RESULT F_CALLBACK pcmsetposcallback(FMOD_SOUND *, int, unsigned int, FMOD_TIMEUNIT) { return FMOD_OK; }


bool start_custom_sound(int frequency_, int channels_, CustomStereoSoundCb fill_buffer_cb_)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  stop_custom_sound();

  if (frequency_ < 1000 || frequency_ > 300000 || channels_ < 1 || channels_ > 2 || !fill_buffer_cb_)
    return false;

  WinAutoLock lock(crit_sec);
  frequency = frequency_;
  channels = channels_;
  custom_sound_play_pos = 0;
  fill_buffer_cb = fill_buffer_cb_;

  FMOD_CREATESOUNDEXINFO exinfo;
  memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
  exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  exinfo.numchannels = channels;
  exinfo.defaultfrequency = frequency;
  exinfo.decodebuffersize = 1024; // ??? WTF ??? cannot be less than 1024
  exinfo.length = frequency * sizeof(float) * channels * 2;
  exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
  exinfo.pcmreadcallback = pcmreadcallback;
  exinfo.pcmsetposcallback = pcmsetposcallback;

  SOUND_VERIFY_AND_DO(fmodapi::get_system()->update(), return false);
  SOUND_VERIFY_AND_DO(fmodapi::get_system()->createSound(0, FMOD_OPENUSER | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, &exinfo, &sound),
    return false);
  SOUND_VERIFY_AND_DO(fmodapi::get_system()->playSound(sound, 0, 0, &channel), return false);
  // SOUND_VERIFY_AND_DO(channel->setVolume(1.0f), return false);
  SOUND_VERIFY_AND_DO(channel->setPitch(1.0f), return false);
  // SOUND_VERIFY_AND_DO(channel->setPaused(false), return false);
  SOUND_VERIFY_AND_DO(fmodapi::get_system()->update(), return false);
  return true;
}

void stop_custom_sound()
{
  SNDSYS_IF_NOT_INITED_RETURN;
  WinAutoLock lock(crit_sec);

  if (sound)
  {
    sound->release();
    sound = nullptr;
  }

  fill_buffer_cb = nullptr;
}

uint64_t get_custom_sound_play_pos() { return custom_sound_play_pos; }


} // namespace sndsys
