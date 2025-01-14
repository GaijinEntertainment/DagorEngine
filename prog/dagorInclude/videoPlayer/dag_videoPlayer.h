//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>

class IPoint2;


class IGenVideoPlayer
{
public:
  //! explicit destructor
  virtual void destroy() = 0;


  //! reopens another file in player
  //! returns "this" when video format is the same and player implementation is smart enough
  //! when required, player is allowed to call this->destroy() and return nwely created player instance
  //! if file cannot be open or format is incorrect, player will call this->destroy() and return NULL
  virtual IGenVideoPlayer *reopenFile(const char *fname) = 0;

  //! reopens another memory data in player
  //! behaves similarly to reopenFile()
  virtual IGenVideoPlayer *reopenMem(dag::ConstSpan<char> data) = 0;


  //! start playing with scheduling first frame render time at curTime+prebuf_delay_ms;
  //! may take some time (for Ogg Theora player - at least 5ms) to prebuffer first frame
  virtual void start(int prebuf_delay_ms = 100) = 0;

  //! pauses playback, optionally resetting queue of decoded frames
  virtual void stop(bool reset = false) = 0;

  //! rewind to beginning
  virtual void rewind() = 0;

  //! set/clear autorewind flag
  virtual void setAutoRewind(bool auto_rewind) = 0;


  //! returns frame size of loaded video
  virtual IPoint2 getFrameSize() = 0;

  //! returns movie duration in ms or -1 when container doesn't provide duration
  virtual int getDuration() = 0;

  //! returns current playback position in ms
  virtual int getPosition() = 0;

  //! returns true when movie is fully played back and stopped
  virtual bool isFinished() = 0;


  //! reads/decodes ahead frames; tries to spend not more that max_usec microseconds
  virtual void advance(int max_usec) = 0;

  //! returns current frame in YUV format (supported by: Ogg Theora player, PAM player)
  virtual bool getFrame(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV, d3d::SamplerHandle &smp) = 0;

  //! should be called by client after frame textures are dispatched to d3d (to prevent texture overwrite-whe-n-used)
  virtual void onCurrentFrameDispatched() = 0;

  //! render current frame to screen rectangle (supported by: WMW player)
  virtual bool renderFrame(const IPoint2 *lt = NULL, const IPoint2 *rb = NULL) = 0;

  //! callbacks for 3d device reset - recreate RTs, queries etc.
  virtual void beforeResetDevice() = 0;
  virtual void afterResetDevice() = 0;

public:
  //! creates instance of Ogg Theora video player to playback from file
  static IGenVideoPlayer *create_ogg_video_player(const char *fname, int buf_frames = 4);
  //! creates instance of Ogg Theora video player to playback from memory
  static IGenVideoPlayer *create_ogg_video_player_mem(dag::ConstSpan<char> data, int buf_frames = 4);

  //! creates instance of Ogg Theora video player to playback from file
  static IGenVideoPlayer *create_av1_video_player(const char *fname, int buf_frames = 4);

  //! calls advance for all currently created Ogg Theora players
  //! (shares execution time proportionally by frame size between all active players)
  static void advanceActivePlayers(int max_usec);
};
