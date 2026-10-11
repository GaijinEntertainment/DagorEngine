// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceTuning.h"
#include <soundSystem/voiceCommunicator.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>
#include <EASTL/algorithm.h>

namespace voicechat
{

static WinCritSec tuning_mutex;
static CaptureTuning capture_values;   // guarded by tuning_mutex
static OpusTuning opus_values;         // guarded by tuning_mutex
static PlayoutTuning playout_values;   // guarded by tuning_mutex
static PlaybackTuning playback_values; // guarded by tuning_mutex
static volatile int generation = 0;    // 0 until the first load

static const DataBlock &settings_tuning()
{
  const DataBlock *settings = dgs_get_settings ? dgs_get_settings() : nullptr;
  return settings ? *settings->getBlockByNameEx("voice")->getBlockByNameEx("tuning") : DataBlock::emptyBlock;
}

static void load_locked(const DataBlock &blk)
{
  const CaptureTuning defaults;
  CaptureTuning &t = capture_values;
  t.vadProbThreshold = eastl::clamp(blk.getReal("vadProbThreshold", defaults.vadProbThreshold), 0.0f, 1.0f);
  t.vadPrerollFrames = eastl::clamp(blk.getInt("vadPrerollFrames", defaults.vadPrerollFrames), 0, 50);
  t.vadHangoverFrames = eastl::clamp(blk.getInt("vadHangoverFrames", defaults.vadHangoverFrames), 1, 500);
  t.vadCloseThreshold = eastl::clamp(blk.getReal("vadCloseThreshold", t.vadProbThreshold), 0.0f, t.vadProbThreshold);
  // constraint: the preroll holds the onset frames before the one that opens the gate
  t.vadOnsetFrames = eastl::clamp(blk.getInt("vadOnsetFrames", defaults.vadOnsetFrames), 1, t.vadPrerollFrames + 1);
  t.vadSmoothing = eastl::clamp(blk.getReal("vadSmoothing", defaults.vadSmoothing), 0.0f, 0.9f);
  t.agc2InitialGainDb = eastl::clamp(blk.getReal("agc2InitialGainDb", defaults.agc2InitialGainDb), 0.0f, 50.0f);
  // constraint: WebRTC turns AGC2 off for a max gain of 0 dB or less
  t.agc2MaxGainDb = eastl::clamp(blk.getReal("agc2MaxGainDb", defaults.agc2MaxGainDb), eastl::max(t.agc2InitialGainDb, 1.0f), 50.0f);
  const OpusTuning opusDefaults;
  opus_values.bitrateBps = eastl::clamp(blk.getInt("opusBitrateBps", opusDefaults.bitrateBps), 6000, 64000);
  opus_values.complexity = eastl::clamp(blk.getInt("opusComplexity", opusDefaults.complexity), 0, 10);
  opus_values.inbandFec = blk.getBool("opusInbandFec", opusDefaults.inbandFec);
  opus_values.packetLossPercent =
    eastl::clamp(blk.getInt("opusPacketLossPercent", opusDefaults.packetLossPercent), 0, MAX_OPUS_PACKET_LOSS_PERCENT);
  opus_values.widebandLossPercent =
    eastl::clamp(blk.getInt("opusWidebandLossPercent", opusDefaults.widebandLossPercent), 0, MAX_OPUS_PACKET_LOSS_PERCENT + 1);
  const PlayoutTuning playoutDefaults;
  PlayoutTuning &p = playout_values;
  p.minJitterMs = eastl::clamp(blk.getInt("playoutMinJitterMs", playoutDefaults.minJitterMs), 0, 500);
  p.maxJitterMs = eastl::clamp(blk.getInt("playoutMaxJitterMs", playoutDefaults.maxJitterMs), p.minJitterMs, 1000);
  p.jitterMarginMs = eastl::clamp(blk.getInt("playoutJitterMarginMs", playoutDefaults.jitterMarginMs), 0, 200);
  p.initialJitterMs = eastl::clamp(blk.getInt("playoutInitialJitterMs", playoutDefaults.initialJitterMs), 0, 1000);
  const PlaybackTuning playbackDefaults;
  PlaybackTuning &b = playback_values;
  b.startMarginMs = eastl::clamp(blk.getInt("playbackStartMarginMs", playbackDefaults.startMarginMs), 0, MAX_PLAYBACK_START_MARGIN_MS);
  // constraint: a mixer block can take more than one frame, so with no margin a lead of one frame runs dry
  b.leadFrames = eastl::clamp(blk.getInt("playbackLeadFrames", playbackDefaults.leadFrames), 2, MAX_PLAYBACK_LEAD_FRAMES);
  b.panLevel = eastl::clamp(blk.getReal("playbackPanLevel", playbackDefaults.panLevel), 0.0f, 1.0f);
  b.panFadeMeters = eastl::clamp(blk.getReal("playbackPanFadeMeters", playbackDefaults.panFadeMeters), 0.0f, 10.0f);
  interlocked_increment(generation);
  const OpusTuning &o = opus_values;
  debug("[VC] tuning: VAD open %.2f close %.2f after %d frames smoothing %.2f, preroll %d and hangover %d frames, AGC2 initial "
        "%.1f dB and max %.1f dB, Opus %d bps complexity %d FEC %s at %d%% loss wideband from %d%%, playout jitter min %d max %d "
        "margin %d initial %d ms, playback margin %d ms lead %d frames pan %.2f fading in to %.1f m",
    t.vadProbThreshold, t.vadCloseThreshold, t.vadOnsetFrames, t.vadSmoothing, t.vadPrerollFrames, t.vadHangoverFrames,
    t.agc2InitialGainDb, t.agc2MaxGainDb, o.bitrateBps, o.complexity, o.inbandFec ? "on" : "off", o.packetLossPercent,
    o.widebandLossPercent, p.minJitterMs, p.maxJitterMs, p.jitterMarginMs, p.initialJitterMs, b.startMarginMs, b.leadFrames,
    b.panLevel, b.panFadeMeters);
}

static void load_once_locked(int *generation_out)
{
  if (interlocked_acquire_load(generation) == 0)
    load_locked(settings_tuning());
  if (generation_out)
    *generation_out = interlocked_acquire_load(generation);
}

CaptureTuning capture_tuning(int *generation_out)
{
  WinAutoLock lock(tuning_mutex);
  load_once_locked(generation_out);
  return capture_values;
}

PlaybackTuning playback_tuning(int *generation_out)
{
  WinAutoLock lock(tuning_mutex);
  load_once_locked(generation_out);
  return playback_values;
}

OpusTuning opus_tuning(int *generation_out)
{
  WinAutoLock lock(tuning_mutex);
  load_once_locked(generation_out);
  return opus_values;
}

PlayoutTuning playout_tuning(int *generation_out)
{
  WinAutoLock lock(tuning_mutex);
  load_once_locked(generation_out);
  return playout_values;
}

int tuning_generation() { return interlocked_acquire_load(generation); }

float pan_level_at(const PlaybackTuning &tuning, float distance)
{
  if (distance >= tuning.panFadeMeters)
    return tuning.panLevel;
  return tuning.panLevel * eastl::max(distance, 0.0f) / tuning.panFadeMeters;
}

void load_tuning(const DataBlock &tuning_blk)
{
  WinAutoLock lock(tuning_mutex);
  load_locked(tuning_blk);
}

void load_tuning_from_settings() { load_tuning(settings_tuning()); }

bool load_tuning_file(const char *path)
{
  DataBlock file;
  if (!dblk::load(file, path, dblk::ReadFlag::ROBUST))
    return false;
  const DataBlock *voice = file.getBlockByName("voice");
  const DataBlock *tuning = voice ? voice->getBlockByName("tuning") : nullptr;
  if (!tuning)
    return false;
  load_tuning(*tuning);
  return true;
}

} // namespace voicechat

static bool voice_tuning_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME_EX("voice", "reload_tuning", 1, 2, "reloads voice{tuning{}} from the game settings, or from a blk file",
    "[file.blk]")
  {
    if (argc < 2)
      voicechat::load_tuning_from_settings();
    else if (!voicechat::load_tuning_file(argv[1]))
    {
      console::print_d("voice.reload_tuning: cannot load %s", argv[1]);
      return found;
    }
    const voicechat::CaptureTuning t = voicechat::capture_tuning();
    const voicechat::OpusTuning o = voicechat::opus_tuning();
    const voicechat::PlayoutTuning p = voicechat::playout_tuning();
    const voicechat::PlaybackTuning b = voicechat::playback_tuning();
    console::print_d("voice.reload_tuning: VAD open %.2f close %.2f after %d frames smoothing %.2f, preroll %d, hangover %d, "
                     "AGC2 initial %.1f dB, max %.1f dB, Opus %d bps complexity %d FEC %s at %d%% loss wideband from %d%%, "
                     "playout jitter min %d max %d margin %d initial %d ms, playback margin %d ms lead %d frames pan %.2f fading "
                     "in to %.1f m",
      t.vadProbThreshold, t.vadCloseThreshold, t.vadOnsetFrames, t.vadSmoothing, t.vadPrerollFrames, t.vadHangoverFrames,
      t.agc2InitialGainDb, t.agc2MaxGainDb, o.bitrateBps, o.complexity, o.inbandFec ? "on" : "off", o.packetLossPercent,
      o.widebandLossPercent, p.minJitterMs, p.maxJitterMs, p.jitterMarginMs, p.initialJitterMs, b.startMarginMs, b.leadFrames,
      b.panLevel, b.panFadeMeters);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(voice_tuning_console_handler);
