//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// To differentiate samples in a single frame, when subpixel or supersampling enabled
enum class SubFrameSample
{
  Single,      // one and only in this frame
  First,       // first in this frame out of multiple
  Last,        // last in this frame out of multiple
  Intermediate // middle frame out of multiple
};
