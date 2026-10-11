// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;

namespace voicechat
{

// the vad values act only where the RNNoise speech gate runs
struct CaptureTuning
{
  float vadProbThreshold = 0.6f;
  int vadPrerollFrames = 5;
  int vadHangoverFrames = 50;
  // vadProbThreshold opens the gate and vadCloseThreshold keeps it open, on a probability that keeps vadSmoothing of the
  // earlier frames
  float vadCloseThreshold = 0.6f;
  int vadOnsetFrames = 2;
  float vadSmoothing = 0.5f;
  float agc2InitialGainDb = 6;
  float agc2MaxGainDb = 30;
};

// constraint: each buffered playback ring is sized for these, so a load never outgrows it
static constexpr int MAX_PLAYBACK_START_MARGIN_MS = 200;
static constexpr int MAX_PLAYBACK_LEAD_FRAMES = 6;

// the voice{tuning{}} keys playbackStartMarginMs and playbackLeadFrames: a spurt of a buffered source starts late by the
// margin, and its ring is filled up to the lead past that. A source takes a load at its next push, and the mixer starts
// the next spurt with the new margin
struct PlaybackTuning
{
  int startMarginMs = 96;
  int leadFrames = 2;
  // playbackPanLevel and playbackPanFadeMeters: the FMOD 3D level of a positional voice, the rest is centered, so that a
  // speaker at the side does not take all of the voice to one ear; closer than the fade distance it falls to centered
  float panLevel = 0.7f;
  float panFadeMeters = 1.0f;
};

// the FMOD 3D level for a speaker at this distance from the listener
float pan_level_at(const PlaybackTuning &tuning, float distance);

// constraint: any thread may call every function here

// the first call loads voice{tuning{}} of the game settings; generation gets the generation of the returned values
CaptureTuning capture_tuning(int *generation = nullptr);
PlaybackTuning playback_tuning(int *generation = nullptr);
// tuning_blk is the tuning{} block itself; a key it lacks keeps its default and a value out of range is clamped. A load
// that changes an agc2 value restarts the running AGC2 at agc2InitialGainDb
void load_tuning(const DataBlock &tuning_blk);
void load_tuning_from_settings();
// false, with the tuning unchanged, when the file cannot load or has no voice{tuning{}} block
bool load_tuning_file(const char *path);

} // namespace voicechat
