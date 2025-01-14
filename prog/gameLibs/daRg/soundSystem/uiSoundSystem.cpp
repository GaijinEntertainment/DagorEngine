// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/soundSystem/uiSoundSystem.h>
#include <daRg/dag_uiSound.h>

#include <soundSystem/quirrel/sqSoundSystem.h>
#include <soundSystem/soundSystem.h>
#include <math/dag_math3d.h>
#include <sound/dag_genAudio.h>

namespace darg
{
static constexpr int max_event_instances = 3;
static constexpr const char *ui_preset_name = "master";

class SoundSystemUiSoundPlayer : public darg::IUiSoundPlayer
{
  virtual void play(const char *name, const Sqrat::Object &params, float volume, const Point2 *pos2d)
  {
    if (!sound::sqapi::is_preset_loaded(ui_preset_name)) // prevent "init event '...' failed" while loading. this is UI player so it is
                                                         // ok here.
      return;
    if (sound::sqapi::get_num_event_instances(name) >= max_event_instances) // protect from command buffer overflow (event 'max
                                                                            // instances' setting will not help)
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
