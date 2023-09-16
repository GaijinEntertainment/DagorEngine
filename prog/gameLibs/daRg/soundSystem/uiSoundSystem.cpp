#include <daRg/soundSystem/uiSoundSystem.h>
#include <daRg/dag_uiSound.h>

#include <soundSystem/quirrel/sqSoundSystem.h>
#include <soundSystem/soundSystem.h>
#include <math/dag_math3d.h>
#include <sound/dag_genAudio.h>

namespace darg
{

class SoundSystemUiSoundPlayer : public darg::IUiSoundPlayer
{
  static constexpr int max_event_instances = 3;
  virtual void play(const char *name, const Sqrat::Object &params, float volume, const Point2 *pos2d)
  {
    if (sound::sqapi::get_num_event_instances(name) >= max_event_instances)
      return;

    if (pos2d)
    {
      const Point3 pos3d = sndsys::get_3d_listener() * Point3(pos2d->x, -pos2d->y, 3.f);
      sound::sqapi::play_sound(name, params, volume, &pos3d);
    }
    else
      sound::sqapi::play_sound(name, params, volume, nullptr);
  }
  virtual IGenAudioSystem *getAudioSystem() { return dagor_audiosys; }
};

static SoundSystemUiSoundPlayer soundsystem_ui_sound_player;


void init_soundsystem_ui_player() { darg::ui_sound_player = &soundsystem_ui_sound_player; }


} // namespace darg
