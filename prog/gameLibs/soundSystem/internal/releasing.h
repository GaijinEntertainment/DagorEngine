// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/handle.h>

namespace FMOD
{
namespace Studio
{
class EventInstance;
} // namespace Studio
} // namespace FMOD

namespace sndsys
{
namespace releasing
{
void track(FMOD::Studio::EventInstance *instance);
void update(float cur_time);
void close();
int debug_get_num_events();
void debug_warn_about_deadline(const FMOD::Studio::EventInstance &instance);
void release_immediate_impl(FMOD::Studio::EventInstance &instance);

static constexpr float g_max_duration = 20.f;
static constexpr float g_check_interval = 0.2f;
} // namespace releasing
} // namespace sndsys
