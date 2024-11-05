// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace capture_gbuffer
{
void make_gbuffer_capture();
bool is_gbuffer_capturing_in_progress();
void schedule_gbuffer_capture();
bool is_gbuffer_capture_scheduled();
} // namespace capture_gbuffer
