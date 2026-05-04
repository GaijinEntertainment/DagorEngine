// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>


ECS_UNICAST_EVENT_TYPE(OnScopeCrosshairGuiRender,
  float /*lens_width*/,
  float /*lens_height*/,
  float /*hd font size*/,
  float * /*out_gui_offset_from_lens*/);
