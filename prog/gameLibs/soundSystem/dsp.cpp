#include <fmod_studio.hpp>
#include <fmod_studio_common.h>
#include <fmod_errors.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/dsp.h>
#include "internal/debug.h"

namespace sndsys
{
namespace dsp
{
static float g_pan_3d_decay = 0.f;
static FMOD::DSP *g_pan_dsp = nullptr;


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
  const DataBlock &dspBlk = *blk.getBlockByNameEx("dsp");
  g_pan_3d_decay = saturate(dspBlk.getReal("pan3DDecay", 0.f));
}

void close() { release_pan_dsp(); }

void apply() { create_pan_dsp(); }
} // namespace dsp
} // namespace sndsys
