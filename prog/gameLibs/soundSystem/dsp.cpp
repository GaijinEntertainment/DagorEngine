// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <fmod_studio.hpp>
#include <fmod_studio_common.h>
#include <fmod_dsp_effects.h>
#include <fmod_errors.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/dsp.h>
#include "internal/debug_internal.h"

static WinCritSec g_dsp_cs;
#define DSP_BLOCK WinAutoLock dspLock(g_dsp_cs);

namespace sndsys
{
namespace dsp
{
static float g_pan_3d_decay = 0.f;
static FMOD::DSP *g_pan_dsp = nullptr;

// FFT spectrum analyser — attached to the specific bus channel group on first use
static FMOD::DSP *g_fft_dsp = nullptr;
static FMOD::Studio::Bus *g_fft_bus = nullptr;
static bool g_fft_owns_lock = false;
static constexpr const char *g_bus_path = "bus:/master/fx_lowpass/music_player";
static const int FFT_WINDOW_SIZE = 1024; // 512 unique frequency bins

static void release_fft_dsp()
{
  if (!g_fft_dsp)
    return;
  if (g_fft_bus)
  {
    FMOD::ChannelGroup *channelGroup = nullptr;
    if (g_fft_bus->getChannelGroup(&channelGroup) == FMOD_OK && channelGroup)
      channelGroup->removeDSP(g_fft_dsp);
    if (g_fft_owns_lock)
      g_fft_bus->unlockChannelGroup();
    g_fft_bus = nullptr;
    g_fft_owns_lock = false;
  }
  g_fft_dsp->release();
  g_fft_dsp = nullptr;
}

static bool ensure_fft_dsp()
{
  if (g_fft_dsp)
    return true;
  FMOD::Studio::System *studioSys = fmodapi::get_studio_system();
  FMOD::System *fmodSys = fmodapi::get_system();
  if (!studioSys || !fmodSys)
    return false;
  FMOD::Studio::Bus *bus = nullptr;
  if (studioSys->getBus(g_bus_path, &bus) != FMOD_OK || !bus)
    return false;
  FMOD_RESULT lockRes = bus->lockChannelGroup();
  if (lockRes != FMOD_OK && lockRes != FMOD_ERR_ALREADY_LOCKED)
    return false;
  if (lockRes == FMOD_OK)
    studioSys->flushCommands();
  FMOD::ChannelGroup *channelGroup = nullptr;
  if (bus->getChannelGroup(&channelGroup) != FMOD_OK || !channelGroup)
  {
    if (lockRes == FMOD_OK)
      bus->unlockChannelGroup();
    return false;
  }
  if (fmodSys->createDSPByType(FMOD_DSP_TYPE_FFT, &g_fft_dsp) != FMOD_OK)
  {
    if (lockRes == FMOD_OK)
      bus->unlockChannelGroup();
    return false;
  }
  g_fft_dsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, FFT_WINDOW_SIZE);
  g_fft_dsp->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HANNING);
  if (channelGroup->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, g_fft_dsp) != FMOD_OK)
  {
    g_fft_dsp->release();
    g_fft_dsp = nullptr;
    if (lockRes == FMOD_OK)
      bus->unlockChannelGroup();
    return false;
  }
  g_fft_bus = bus;
  g_fft_owns_lock = (lockRes == FMOD_OK);
  return true;
}


static void release_pan_dsp()
{
  if (!g_pan_dsp)
    return;
  FMOD::System *fmodSys = fmodapi::get_system();
  G_ASSERT(fmodSys);
  if (!fmodSys)
    return;
  SOUND_VERIFY(fmodSys->lockDSP());
  FMOD::ChannelGroup *mastergroup = nullptr;
  SOUND_VERIFY(fmodSys->getMasterChannelGroup(&mastergroup));
  if (mastergroup)
    SOUND_VERIFY(mastergroup->removeDSP(g_pan_dsp));
  SOUND_VERIFY(g_pan_dsp->release());
  g_pan_dsp = nullptr;
  SOUND_VERIFY(fmodSys->unlockDSP());
}

static void create_pan_dsp()
{
  release_pan_dsp();
  if (g_pan_3d_decay <= 0.f)
    return;
  FMOD::System *fmodSys = fmodapi::get_system();
  G_ASSERT(fmodSys);
  if (!fmodSys)
    return;
  FMOD::ChannelGroup *mastergroup = nullptr;
  SOUND_VERIFY_AND_DO(fmodSys->getMasterChannelGroup(&mastergroup), return);
  SOUND_VERIFY_AND_DO(fmodSys->createDSPByType(FMOD_DSP_TYPE_PAN, &g_pan_dsp), return);
  SOUND_VERIFY_AND_DO(g_pan_dsp->setBypass(false), return);
  SOUND_VERIFY_AND_DO(mastergroup->addDSP(0, g_pan_dsp), return);

  SOUND_VERIFY_AND_DO(g_pan_dsp->setParameterFloat(FMOD_DSP_PAN_2D_DIRECTION, 0.f), return);
  SOUND_VERIFY_AND_DO(g_pan_dsp->setParameterFloat(FMOD_DSP_PAN_2D_EXTENT, 0.f), return);
  SOUND_VERIFY_AND_DO(g_pan_dsp->setParameterFloat(FMOD_DSP_PAN_3D_PAN_BLEND, 1.f - g_pan_3d_decay), return);
}

void init(const DataBlock &blk)
{
  SNDSYS_IS_MAIN_THREAD;
  DSP_BLOCK;
  const DataBlock &dspBlk = *blk.getBlockByNameEx("dsp");
  g_pan_3d_decay = saturate(dspBlk.getReal("pan3DDecay", 0.f));
}

void close()
{
  SNDSYS_IS_MAIN_THREAD;
  DSP_BLOCK;
  release_pan_dsp();
  release_fft_dsp();
}

void apply()
{
  DSP_BLOCK;
  create_pan_dsp();
}

void get_spectrum_bars(dag::Span<float> out)
{
  DSP_BLOCK;
  const int num_values = (int)out.size();
  for (int i = 0; i < num_values; i++)
    out[i] = 0.f;

  if (!ensure_fft_dsp())
    return;

  FMOD_DSP_PARAMETER_FFT *fft_data = nullptr;
  if (g_fft_dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&fft_data, nullptr, nullptr, 0) != FMOD_OK)
    return;
  if (!fft_data || fft_data->length == 0 || fft_data->numchannels == 0)
    return;

  const int fft_len = fft_data->length; // FFT_WINDOW_SIZE / 2 = 512
  const int channels = fft_data->numchannels;

  // Logarithmic bucketing: bass frequencies get proportionally more bins,
  // matching human hearing perception
  for (int bar = 0; bar < num_values; bar++)
  {
    const float t0 = (float)bar / num_values;
    const float t1 = (float)(bar + 1) / num_values;
    int bin0 = (int)(fft_len * (expf(t0 * logf((float)fft_len + 1.f)) - 1.f) / fft_len);
    int bin1 = (int)(fft_len * (expf(t1 * logf((float)fft_len + 1.f)) - 1.f) / fft_len);
    bin1 = max(bin0 + 1, bin1);
    bin0 = clamp(bin0, 0, fft_len - 1);
    bin1 = clamp(bin1, 1, fft_len);

    float sum = 0.f;
    for (int ch = 0; ch < channels; ch++)
      for (int b = bin0; b < bin1; b++)
        sum += fft_data->spectrum[ch][b];

    const float val = sum / float((bin1 - bin0) * channels);
    // sqrt amplifies quiet signals
    out[bar] = saturate(sqrtf(val));
  }
}
} // namespace dsp
} // namespace sndsys
