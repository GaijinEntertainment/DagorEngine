// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sound/dag_genAudio.h>
#include <soundSystem/fmodApi.h>
#include <fmod_studio_common.h>
#include <fmod_studio.hpp>
#include <fmod_errors.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>

namespace sndsys
{
namespace fmodapi
{
class DagorFmod4Audio : public IGenAudioSystem
{
  struct FmodStudioMusic
  {
    FMOD::Sound *music = nullptr;
    FMOD::Channel *chan = nullptr;
  };

public:
  virtual IGenSound *createSound(const char *fname)
  {
    FMOD::Sound *snd = nullptr;
    FMOD_RESULT result = FMOD_OK;
    result = get_system()->createSound(::df_get_real_name(fname), (FMOD_MODE)(FMOD_DEFAULT | FMOD_2D), 0, &snd);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s %s(%s)", FMOD_ErrorString(result), fname, ::df_get_real_name(fname));
      return NULL;
    }
    return (IGenSound *)snd;
  }

  virtual void playSound(IGenSound *snd, int loop_cnt, float volume)
  {
    if (!snd)
      return;
    FMOD::Channel *chan;
    FMOD_RESULT result = FMOD_OK;
    result = get_system()->playSound((FMOD::Sound *)snd, 0, false, &chan);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
    result = chan->setLoopCount(loop_cnt);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
    result = chan->setVolume(volume);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
  }

  virtual void releaseSound(IGenSound *snd)
  {
    if (!snd)
      return;
    ((FMOD::Sound *)snd)->release();
  }


  virtual IGenMusic *createMusic(const char *fname, bool loop)
  {
    FmodStudioMusic *m = new (midmem) FmodStudioMusic;
    FMOD_RESULT result = FMOD_OK;

    result = get_system()->createStream(::df_get_real_name(fname),
      (FMOD_MODE)(FMOD_DEFAULT | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | FMOD_2D), 0, &m->music);

    if (result != FMOD_OK)
    {
      delete m;
      DEBUG_CTX("FMOD error %s %s:\n%s", fname, ::df_get_real_name(fname), FMOD_ErrorString(result));
      return NULL;
    }
    return (IGenMusic *)m;
  }

  virtual void setMusicVolume(IGenMusic *music, float volume)
  {
    if (!music)
      return;

    FmodStudioMusic *m = (FmodStudioMusic *)music;
    if (!m->chan)
      return;

    FMOD_RESULT result = m->chan->setVolume(volume);
    if (result != FMOD_OK)
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
  }

  virtual void playMusic(IGenMusic *music, int loop_cnt, float volume)
  {
    if (!music)
      return;

    FmodStudioMusic *m = (FmodStudioMusic *)music;
    FMOD_RESULT result = FMOD_OK;

    result = get_system()->playSound(m->music, 0, false, &m->chan);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
    result = m->chan->setLoopCount(loop_cnt);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
    result = m->chan->setVolume(volume);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }

    FMOD_REVERB_PROPERTIES noReverb = FMOD_PRESET_OFF;
    result = get_system()->setReverbProperties(0, &noReverb);
    if (result != FMOD_OK)
    {
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
      return;
    }
  }

  virtual void stopMusic(IGenMusic *music)
  {
    if (!music)
      return;
    FmodStudioMusic *m = (FmodStudioMusic *)music;
    FMOD_RESULT result = FMOD_OK;
    if (!m->chan)
      return;
    result = m->chan->stop();
    if (result != FMOD_OK)
      DEBUG_CTX("FMOD error:\n%s", FMOD_ErrorString(result));
    m->chan = NULL;
  }

  virtual void releaseMusic(IGenMusic *music)
  {
    if (!music)
      return;
    FmodStudioMusic *m = (FmodStudioMusic *)music;
    m->music->release();
    delete m;
  }
};


void setup_dagor_audiosys()
{
  static DagorFmod4Audio fmod_audio;
  dagor_audiosys = &fmod_audio;
}
} // namespace fmodapi
} // namespace sndsys