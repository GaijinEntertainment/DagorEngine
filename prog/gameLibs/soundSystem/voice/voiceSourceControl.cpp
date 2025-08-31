// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceSourceControl.h"
#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/voiceCommunicator.h>
#include <util/dag_finally.h>


namespace voicechat
{

bool VoiceSourceControl::init(IVoiceSource &source_, float volume_)
{
  G_ASSERT_RETURN(!source, false);
  source = &source_;
  chanSampleRate = source->getSampleRate();
  chanFrameSamples = source->getFrameSamples();
  chanFrameBytes = chanFrameSamples * sample_size;
  volume = volume_;

  if (!sndsys::is_inited())
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();

  FMOD_CREATESOUNDEXINFO exinfo;
  memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
  exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  exinfo.numchannels = 1;
  exinfo.defaultfrequency = chanSampleRate;
  exinfo.format = process_sample_format;
  exinfo.length = chanFrameBytes * playback_frames_count;

  FMOD::Sound *sound = nullptr;
  FMOD_RESULT fmodRes = fmodSys->createSound(NULL, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &sound);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to create playback sound", false);
  G_ASSERT_RETURN(sound, false);
  playbackSound.reset(sound);
  debug("[VC] Create playback sound: sr %u, ns %d", chanSampleRate, exinfo.length / sample_size);

  return true;
}


void VoiceSourceControl::updatePlayback(FMOD::System *fmod_sys)
{
  G_ASSERT_RETURN(source, );
  bool isPlaying = false;
  if (playbackChannel)
    playbackChannel->isPlaying(&isPlaying);
  int nextFillFrame = 0;

  if (isPlaying)
  {
    unsigned int curPlayPos = 0;
    playbackChannel->getPosition(&curPlayPos, FMOD_TIMEUNIT_PCM); //-V1004
    int curPlayFrame = curPlayPos / chanFrameSamples;
    nextFillFrame = (curPlayFrame + 1) % playback_frames_count;
  }
  else
  {
    nextFillFrame = 0;
  }

  if (nextFillFrame == playbackLastFilledFrame)
    return;

  playbackLastFilledFrame = nextFillFrame;
  int16_t *ptr1;
  int16_t *ptr2;
  unsigned int len1;
  unsigned int len2;
  FMOD_RESULT fmodRes =
    playbackSound->lock(nextFillFrame * chanFrameBytes, chanFrameBytes, (void **)&ptr1, (void **)&ptr2, &len1, &len2);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to lock playback buffer", );

  {
    FINALLY([&]() { playbackSound->unlock(ptr1, ptr2, len1, len2); });
    G_ASSERT_RETURN(len1 == chanFrameBytes, );
    G_ASSERT_RETURN(ptr2 == nullptr, );
    source->receiveAudioFrame(ptr1, chanFrameSamples);
  }

  if (!isPlaying)
  {
    fmod_sys->playSound(playbackSound.get(), voiceChannelGroup, 0, &playbackChannel);
    playbackChannel->setVolume(volume);
  }
}

void VoiceSourceControl::setGlobalVoiceVolume(float volume_)
{
  volume = volume_;
  if (playbackChannel)
    playbackChannel->setVolume(volume);
}

} // namespace voicechat
