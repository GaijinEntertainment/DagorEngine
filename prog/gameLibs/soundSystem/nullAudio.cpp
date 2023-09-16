#include <sound/dag_genAudio.h>
#include <stdint.h>

class DagorNullAudio : public IGenAudioSystem
{
public:
  virtual IGenSound *createSound(const char *) { return (IGenSound *)(uintptr_t)0x0BAD0000; }
  virtual void playSound(IGenSound *, int, float) {}
  virtual void releaseSound(IGenSound *) {}

  virtual IGenMusic *createMusic(const char *, bool) { return (IGenMusic *)(uintptr_t)0x0BAD0000; }
  virtual void setMusicVolume(IGenMusic *, float) {}
  virtual void playMusic(IGenMusic *, int, float) {}
  virtual void stopMusic(IGenMusic *) {}
  virtual void releaseMusic(IGenMusic *) {}
};

static DagorNullAudio null_audio_default;

IGenAudioSystem *dagor_audiosys = &null_audio_default;
