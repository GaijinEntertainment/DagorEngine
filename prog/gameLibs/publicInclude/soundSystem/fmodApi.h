//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/handle.h>
#include <EASTL/vector.h>

namespace FMOD
{
class System;
namespace Studio
{
class System;
class EventInstance;
class EventDescription;
class Bus;
} // namespace Studio
class ChannelGroup;
class DSP;
} // namespace FMOD


struct FMOD_STUDIO_USER_PROPERTY;
struct FMOD_GUID;

namespace sndsys
{
namespace fmodapi
{
using DescAndInstance = eastl::pair<FMOD::Studio::EventDescription *, FMOD::Studio::EventInstance *>;

FMOD::System *get_system();
FMOD::Studio::System *get_studio_system();
void setup_dagor_audiosys();

typedef bool (*EventInstanceCallback)(uint32_t cb_type, FMOD::Studio::EventInstance *event_instance, void *parameters, void *cb_data);
void set_instance_cb(EventHandle event_handle, EventInstanceCallback cb, void *cb_data, uint32_t cb_mask);
bool get_instance_cb_data(FMOD::Studio::EventInstance *event_instance, void *&cb_data);

FMOD::Studio::EventInstance *get_instance(EventHandle event_handle);
FMOD::Studio::EventDescription *get_description(EventHandle event_handle);
FMOD::Studio::EventDescription *get_description(const FMODGUID &event_id);
FMOD::Studio::EventDescription *get_description(const char *name, const char *path);

bool get_channel_group(EventHandle event_handle, FMOD::ChannelGroup *&group);
bool get_channel_group_by_bus(FMOD::Studio::Bus *bus, FMOD::ChannelGroup *&group);
bool unlock_channel_group_by_bus(const char *bus_name);
void set_channel_group_volume(FMOD::ChannelGroup *channel_group, float volume);
float get_channel_group_volume(FMOD::ChannelGroup *channel_group);
void set_dsp_parameter(FMOD::DSP *dsp, int index, float value);
bool get_rms_levels(FMOD::ChannelGroup *&group, short &numchannels, eastl::vector<float> &rmslevel);
void disable_rms_metering(FMOD::ChannelGroup *&group);
FMOD::Studio::Bus *get_bus(const char *bus_name);
FMOD::Studio::Bus *get_bus_nullable(const char *bus_name);
bool set_pitch(FMOD::Studio::Bus *bus, float pitch);
} // namespace fmodapi
} // namespace sndsys
