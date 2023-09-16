//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

//
// interface for generic Audio system (2D sound/music only)
// (to be used in Engine2 and in sound system-independant projects)
//

class IGenSound;
class IGenMusic;

class IGenAudioSystem
{
public:
  virtual IGenSound *createSound(const char *fname) = 0;
  virtual void playSound(IGenSound *snd, int loop_cnt = 1, float volume = 1.0) = 0;
  virtual void releaseSound(IGenSound *snd) = 0;

  virtual IGenMusic *createMusic(const char *fname, bool loop = true) = 0;
  virtual void setMusicVolume(IGenMusic *music, float volume) = 0;
  virtual void playMusic(IGenMusic *music, int loop_cnt = -1, float volume = 1.0) = 0;
  virtual void stopMusic(IGenMusic *music) = 0;
  virtual void releaseMusic(IGenMusic *music) = 0;
};

extern IGenAudioSystem *dagor_audiosys;

//! install null audio driver (default) to dagor_audiosys
void install_dagor_null_audio_driver();
