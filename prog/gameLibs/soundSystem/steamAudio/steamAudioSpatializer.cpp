// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector.h>
#include <osApiWrappers/dag_critSec.h>
#include <fmod_studio.hpp>
#include <fmod_dsp.h>
#include <fmod.hpp>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>
#include <soundSystem/fmodApi.h>

#include "../internal/steamAudio/steamAudioSpatializer_internal.h"
#include "../internal/steamAudio/steamAudio_internal.h"
#include "../internal/events_internal.h"

static WinCritSec g_cs;
#define SPATIALIZER_BLOCK WinAutoLock lock(g_cs);

#define SPATIALIZER_MSG(MSG) "[SNDSYS] SteamAudio spatializer: " MSG

namespace sndsys::steam_audio::spatializer
{
struct Entry
{
  FMOD::Studio::EventInstance *instance;
  FMOD::DSP *dsp;
  Point3 pos;
};

static eastl::vector<Entry> g_entries;
static bool g_inited = false;

void init(const DataBlock &, FMOD::System *fmod_sys)
{
  SPATIALIZER_BLOCK;
  if (g_inited || !fmod_sys)
    return;

  FMOD_SPEAKERMODE speakermode = FMOD_SPEAKERMODE_DEFAULT;
  fmod_sys->getSoftwareFormat(nullptr, &speakermode, nullptr);
  if (speakermode != FMOD_SPEAKERMODE_STEREO)
  {
    debug(SPATIALIZER_MSG("disabled: output is not stereo (speakermode=%d)"), (int)speakermode);
    return;
  }

  g_inited = true;
  debug(SPATIALIZER_MSG("inited"));
}

void close()
{
  SPATIALIZER_BLOCK;
  for (Entry &entry : g_entries)
  {
    if (entry.dsp)
      entry.dsp->release();
  }
  g_entries.clear();
  g_inited = false;
}

void append(FMOD::Studio::EventInstance *instance, const Point3 &pos)
{
  if (!g_inited || !instance)
    return;

  SPATIALIZER_BLOCK;
  g_entries.push_back({instance, nullptr, pos});
}

void set_pos(FMOD::Studio::EventInstance &instance, const Point3 &pos)
{
  SPATIALIZER_BLOCK;
  if (!g_inited)
    return;
  for (auto &e : g_entries)
    if (e.instance == &instance)
    {
      e.pos = pos;
      return;
    }
}

void release(FMOD::Studio::EventInstance *instance)
{
  SPATIALIZER_BLOCK;
  if (!g_inited || !instance)
    return;
  for (Entry &entry : g_entries)
    if (entry.instance == instance)
    {
      if (entry.dsp)
        entry.dsp->release();
      g_entries.erase(&entry);
      return;
    }
}

static __declspec(noinline) bool validate_entry(Entry *entry)
{
  if (!entry->instance->isValid())
    return false;

  if (entry->dsp)
    return true;

  FMOD_DSP_DESCRIPTION desc = {};
  if (!steam_audio::get_spatializer_description(desc))
    return false;

  FMOD::ChannelGroup *group = nullptr;
  FMOD_RESULT res = entry->instance->getChannelGroup(&group);
  //
  if (res == FMOD_ERR_STUDIO_NOT_LOADED || res == FMOD_ERR_NOTREADY)
    return true;

  if (res != FMOD_OK || !group)
  {
    debug(SPATIALIZER_MSG("getChannelGroup failed: %d, group=%p"), res, group);
    return true;
  }

  if (!group)
    return false;

  int numDsps = 0;
  if (group->getNumDSPs(&numDsps) == FMOD_OK)
    for (int i = 0; i < numDsps; ++i)
    {
      FMOD::DSP *existingDsp = nullptr;
      FMOD_DSP_TYPE type = FMOD_DSP_TYPE_UNKNOWN;
      if (group->getDSP(i, &existingDsp) == FMOD_OK && existingDsp && existingDsp->getType(&type) == FMOD_OK &&
          (type == FMOD_DSP_TYPE_PAN || type == FMOD_DSP_TYPE_OBJECTPAN))
        logerr(SPATIALIZER_MSG("event \"%s\" has a built-in %s effect alongside the binaural spatializer, "
                               "expect double-panning; remove Spatializer/3D Panner/Pan/Stereo Pan from the event"),
          get_debug_name(*entry->instance).c_str(), type == FMOD_DSP_TYPE_PAN ? "Pan" : "3D Object Panner");
    }

  FMOD::System *fmod_sys = nullptr;
  if (group->getSystemObject(&fmod_sys) != FMOD_OK || !fmod_sys)
    return false;

  res = fmod_sys->createDSP(&desc, &entry->dsp);
  if (res != FMOD_OK || !entry->dsp)
  {
    debug(SPATIALIZER_MSG("createDSP failed: %d"), res);
    return false;
  }

  res = group->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, entry->dsp);
  if (res != FMOD_OK)
  {
    entry->dsp->release();
    debug(SPATIALIZER_MSG("addDSP failed: %d"), res);
    return false;
  }

  return true;
}

void update()
{
  SPATIALIZER_BLOCK;
  if (!g_inited)
    return;
  for (int i = (int)g_entries.size() - 1; i >= 0; --i)
  {
    Entry *entry = &g_entries[i];
    if (!validate_entry(entry))
    {
      if (entry->dsp)
        entry->dsp->release();
      g_entries.erase(entry);
    }
    else if (entry->dsp)
      steam_audio::set_spatialized_direction(entry->dsp, entry->pos);
  }
}

} // namespace sndsys::steam_audio::spatializer
