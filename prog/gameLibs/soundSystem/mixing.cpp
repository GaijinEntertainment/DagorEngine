// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <string.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <fmod_studio.hpp>
#include "internal/framememString.h"
#include "internal/debug.h"
#include <fmod.hpp>

namespace sndsys
{
using namespace fmodapi;

static FMOD::Studio::VCA *get_vca(const char *vca_name)
{
  SNDSYS_IF_NOT_INITED_RETURN_(nullptr);
  G_ASSERT_RETURN(vca_name, nullptr);

  FrameStr path;
  path.sprintf("vca:/%s", vca_name);

  FMOD::Studio::VCA *vca = nullptr;
  FMOD_RESULT result = get_studio_system()->getVCA(path.c_str(), &vca);

  if (FMOD_OK != result)
    logerr("Get VCA \"%s\" failed, fmod result is \"%s\"", path.c_str(), FMOD_ErrorString(result));

  return vca;
}

void set_volume(const char *vca_name, float volume)
{
  if (FMOD::Studio::VCA *vca = get_vca(vca_name))
    SOUND_VERIFY(vca->setVolume(volume));
}

float get_volume(const char *vca_name)
{
  float volume = 0.f;
  if (FMOD::Studio::VCA *vca = get_vca(vca_name))
    SOUND_VERIFY_AND_DO(vca->getVolume(&volume), return 0.f);
  return volume;
}

void set_volume_bus(const char *bus_name, float volume)
{
  if (FMOD::Studio::Bus *bus = get_bus(bus_name))
    SOUND_VERIFY(bus->setVolume(volume));
}

float get_volume_bus(const char *bus_name)
{
  float volume = 0.f;
  if (FMOD::Studio::Bus *bus = get_bus(bus_name))
    SOUND_VERIFY_AND_DO(bus->getVolume(&volume), return 0.f);
  return volume;
}

bool is_bus_exists(const char *bus_name)
{
  FMOD::Studio::Bus *bus = get_bus(bus_name);
  return bus ? true : false;
}

void set_mute(const char *bus_name, bool mute)
{
  if (FMOD::Studio::Bus *bus = get_bus(bus_name))
    SOUND_VERIFY(bus->setMute(mute));
}

bool get_mute(const char *bus_name)
{
  FMOD::Studio::Bus *bus = get_bus(bus_name);
  if (!bus)
    return false;
  bool mute = false;
  SOUND_VERIFY_AND_DO(bus->getMute(&mute), return false);
  return mute;
}

/*
 * by default Studio creates and destroys the ChannelGroup on demand.
 * this means it only exists if at least one event instance routes into the bus.
 * if it doesnt exist, getChannelGroup() will return FMOD_ERR_STUDIO_NOT_LOADED.
 *
 * lock_channel_group() is to force bank channel group to be created and to persist for the duration
 */
void lock_channel_group(const char *bus_name, bool immediately)
{
  if (FMOD::Studio::Bus *bus = get_bus(bus_name))
  {
    SOUND_VERIFY(bus->lockChannelGroup());
    if (immediately)
      SOUND_VERIFY(get_studio_system()->flushCommands());
  }
}

void set_pitch(const char *bus_name, float pitch) { fmodapi::set_pitch(get_bus(bus_name), pitch); }

void set_pitch(float pitch) { fmodapi::set_pitch(get_bus(""), pitch); }

namespace fmodapi
{
void set_channel_group_volume(FMOD::ChannelGroup *channel_group, float volume)
{
  if (channel_group)
    channel_group->setVolume(volume);
}

float get_channel_group_volume(FMOD::ChannelGroup *channel_group)
{
  float value = 0.f;
  if (channel_group)
    channel_group->getVolume(&value);
  return value;
}

bool set_pitch(FMOD::Studio::Bus *bus, float pitch)
{
  if (bus)
  {
    FMOD::ChannelGroup *channelGroup = nullptr;
    FMOD_RESULT result = bus->getChannelGroup(&channelGroup);
    if (result == FMOD_OK)
      return channelGroup->setPitch(pitch) == FMOD_OK;
  }
  return false;
}

FMOD::Studio::Bus *get_bus(const char *bus_name)
{
  SNDSYS_IF_NOT_INITED_RETURN_(nullptr);

  FrameStr path;
  path.sprintf("bus:/%s", bus_name ? bus_name : "");
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = get_studio_system()->getBus(path.c_str(), &bus);

  if (FMOD_OK != result)
  {
    logerr("Get BUS \"%s\" failed, fmod result is \"%s\"", path.c_str(), FMOD_ErrorString(result));
    return nullptr;
  }

  return bus;
}

FMOD::Studio::Bus *get_bus_nullable(const char *bus_name)
{
  SNDSYS_IF_NOT_INITED_RETURN_(nullptr);
  FrameStr path;
  path.sprintf("bus:/%s", bus_name ? bus_name : "");
  FMOD::Studio::Bus *bus = nullptr;
  if (FMOD_OK == get_studio_system()->getBus(path.c_str(), &bus))
    return bus;
  return nullptr;
}

bool get_channel_group_by_bus(FMOD::Studio::Bus *bus, FMOD::ChannelGroup *&group)
{
  group = nullptr;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::System *system = get_studio_system();
  if (!system || !bus)
    return false;
  FMOD_RESULT lockRes = bus->lockChannelGroup();
  if (lockRes != FMOD_ERR_ALREADY_LOCKED)
  {
    SOUND_VERIFY_AND_DO(lockRes, return false);
    SOUND_VERIFY_AND_DO(system->flushCommands(), return false);
  }
  SOUND_VERIFY_AND_DO(bus->getChannelGroup(&group), return false);
  return true;
}

bool get_channel_group(EventHandle event_handle, FMOD::ChannelGroup *&group)
{
  group = nullptr;
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::EventInstance *eventInstance = get_instance(event_handle);
  if (!eventInstance)
    return false;
  SOUND_VERIFY_AND_DO(eventInstance->getChannelGroup(&group), return false);
  return true;
}

bool get_rms_levels(FMOD::ChannelGroup *&group, short &numchannels, eastl::vector<float> &rmslevel)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  G_ASSERT(rmslevel.size() == 32);
  FMOD::DSP *chanDSP = nullptr;
  group->getDSP(FMOD_CHANNELCONTROL_DSP_HEAD, &chanDSP);
  if (chanDSP)
  {
    chanDSP->setMeteringEnabled(false, true);
    FMOD_DSP_METERING_INFO levels = {};
    chanDSP->getMeteringInfo(0, &levels);
    if (levels.numsamples > 0)
    {
      numchannels = levels.numchannels;
      for (int i = 0; i < 32; ++i)
      {
        rmslevel[i] = levels.rmslevel[i];
      }
      return true;
    }
  }
  return false;
}

void disable_rms_metering(FMOD::ChannelGroup *&group)
{
  SNDSYS_IF_NOT_INITED_RETURN
  FMOD::DSP *chanDSP = nullptr;
  group->getDSP(FMOD_CHANNELCONTROL_DSP_HEAD, &chanDSP);
  if (chanDSP)
  {
    chanDSP->setMeteringEnabled(false, false);
  }
}

bool unlock_channel_group_by_bus(const char *bus_name)
{
  SNDSYS_IF_NOT_INITED_RETURN_(false);
  FMOD::Studio::System *system = get_studio_system();
  if (!system || !bus_name)
    return false;
  FMOD::Studio::Bus *bus = nullptr;

  FrameStr path;
  path.sprintf("bus:/%s", bus_name);

  SOUND_VERIFY_AND_DO(system->getBus(path.c_str(), &bus), return false);
  SOUND_VERIFY_AND_DO(bus->unlockChannelGroup(), return false);
  return true;
}

void set_dsp_parameter(FMOD::DSP *dsp, int index, float value)
{
  if (dsp)
    dsp->setParameterFloat(index, value);
}
} // namespace fmodapi
} // namespace sndsys
