// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_log.h>
#include "internal/steamAudio.h"
#include <phonon.h>

#define STEAMAUDIO_MSG(MSG) "[SteamAudio] "##MSG

static IPLContext g_context = nullptr;
static IPLAudioSettings g_audio_settings = {};
static bool g_inited = false;

static void IPLCALL steam_audio_log_cb(IPLLogLevel level, const char *message)
{
  logmessage(level == IPL_LOGLEVEL_ERROR ? LOGLEVEL_ERR : LOGLEVEL_DEBUG, "[SteamAudio] %s", message);
}

namespace sndsys::steam_audio
{

bool init(const DataBlock &blk)
{
  if (g_inited)
    return true;

  const DataBlock &steamAudioBlk = *blk.getBlockByNameEx("steamAudio");
  if (!steamAudioBlk.getBool("init", false))
  {
    debug(STEAMAUDIO_MSG("disabled"));
    return false;
  }

  IPLContextSettings ctx_settings = {};
  ctx_settings.version = STEAMAUDIO_VERSION;
  ctx_settings.logCallback = steam_audio_log_cb;

  IPLerror err = iplContextCreate(&ctx_settings, &g_context);
  if (err != IPL_STATUS_SUCCESS)
  {
    logerr(STEAMAUDIO_MSG("iplContextCreate failed: %d"), (int)err);
    return false;
  }

  g_audio_settings.samplingRate = 48000;
  g_audio_settings.frameSize = 1024;

  g_inited = true;
  debug(STEAMAUDIO_MSG("initialized"));
  return true;
}

void shutdown()
{
  if (!g_inited)
    return;
  if (g_context)
    iplContextRelease(&g_context);
  g_audio_settings = {};
  g_inited = false;
}

void update_listener(const TMatrix & /*tm*/)
{
  if (!g_inited)
    return;
}

} // namespace sndsys::steam_audio
