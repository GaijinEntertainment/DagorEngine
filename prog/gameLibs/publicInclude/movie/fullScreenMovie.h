//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//! init move player globals to allow Ogg Theora (.ogg files) playback (PC/XBOX360/PS3);
void init_player_for_ogg_theora();

//! init move player globals to allow native video (non-.ogg files; .wmv or .pam) playback (XBOX360/PS3)
void init_player_for_native_video();

//! init move player globals to allow playback of dagui scene (.blk) with fadein/out (PC/XBOX360/PS3);
void init_player_for_dagui();

//! plays specified movie in full screen; playback is stopped via action buttons on XBOX360/PS3 or Esc on PC
//!   audio_fname specifies sound track for movie (can be NULL for PS3 and PC and not used for XBOX360)
//!   subtitle_prefix will be used as prefix for subtitles and forcefx track files (+.sub.blk and +.ff.blk)
//!   v_chan/a_chan are used for WMV (XBOX360) to specify video and audio channel to be played
//!   leave_bgm specifies whether background music left on or is turned off during playback
void play_fullscreen_movie(const char *fname, const char *audio_fname, const char *subtitle_prefix, bool loop, int v_chan, int a_chan,
  bool leave_bgm, bool show_subtitles, bool do_force_fx, const char *sublangtiming = NULL, float volume = 1.0,
  int skip_delay_msec = 0);


//! draws subtitle text so that it will be rendered in specified safe area, splitting text into lines if needed
//! max_lines==-1 means text will be split by \n
void draw_subtitle_string(const char *text, float lower_scr_bound = 0.95, int max_lines = 10);

//! forces global fixed inverse aspect ratio (e.g., 0.5625 for 16:9 or 0.75 for 4:3) for movies;
//! default is 0 and means automatic aspect ratio detection (as frame.w/frame.h, requires square pixels)
void force_movie_inv_aspect_ratio(float h_by_w_ratio);

//! switches movie skipping behavior
void set_movie_skippable_by_all_devices(bool en);

//! notifies the VR system to cancel its current frame, as the movie player will have its own frame renderer
void set_vr_frame_cancel_and_resume_callbacks(void (*cancel_callback)(), void (*resume_callback)());
