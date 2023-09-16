#include <daRg/dag_uiSound.h>
#include <sound/dag_genAudio.h>
namespace darg
{

class DagorNullAudio : public IGenAudioSystem
{
public:
  virtual IGenSound *createSound(const char *) { return (IGenSound *)0x0BAD0000; }
  virtual void playSound(IGenSound *, int, float) {}
  virtual void releaseSound(IGenSound *) {}

  virtual IGenMusic *createMusic(const char *, bool) { return (IGenMusic *)0x0BAD0000; }
  virtual void setMusicVolume(IGenMusic *, float) {}
  virtual void playMusic(IGenMusic *, int, float) {}
  virtual void stopMusic(IGenMusic *) {}
  virtual void releaseMusic(IGenMusic *) {}
};

static DagorNullAudio null_audio_default;
struct DummySoundPlayer : IUiSoundPlayer
{
  virtual void play(const char *, const Sqrat::Object &, float, const Point2 *) {}
  virtual IGenAudioSystem *getAudioSystem() { return &null_audio_default; }
} dummy;


IUiSoundPlayer *ui_sound_player = &dummy;

} // namespace darg
