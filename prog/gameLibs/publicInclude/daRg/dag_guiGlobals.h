//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace darg
{

enum DebugFramesMode
{
  DFM_NONE = 0,
  DFM_ELEM_CREATE,
  DFM_ELEM_UPDATE,

  DFM_NUM,
  DFM_FIRST = DFM_NONE,
  DFM_LAST = DFM_NUM - 1
};

extern DebugFramesMode debug_frames_mode;

bool set_debug_frames_mode_by_str(const char *str);

} // namespace darg
