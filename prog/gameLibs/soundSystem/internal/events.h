// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/handle.h>
#include <generic/dag_span.h>
#include "framememString.h"

class DataBlock;

namespace FMOD
{
namespace Studio
{
class EventInstance;
class EventDescription;
} // namespace Studio
} // namespace FMOD

namespace sndsys
{
void events_init(const DataBlock &blk);
void events_close();
void events_update(float dt);
FrameStr get_debug_name(const FMOD::Studio::EventDescription &event_description);
FrameStr get_debug_name(const FMOD::Studio::EventInstance &event_instance);
FrameStr get_debug_name(EventHandle event_handle);
float adjust_stealing_volume(EventHandle event_handle, float delta);
const char *get_sample_loading_state(const FMOD::Studio::EventDescription &event_description);

using DbgSampleDataRef = eastl::pair<FMODGUID, int /*refCount*/>;
const dag::Span<DbgSampleDataRef> dbg_get_sample_data_refs();
} // namespace sndsys
