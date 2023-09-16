//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum AnimcharVisbits : unsigned char
{
  VISFLG_WITHIN_RANGE = 1 << 0,
  VISFLG_MAIN_VISIBLE = 1 << 1,
  VISFLG_MAIN_AND_SHADOW_VISIBLE = 1 << 2,
  VISFLG_COCKPIT_VISIBLE = 1 << 3,
  VISFLG_OUTLINE_RENDERED = 1 << 4,
  VISFLG_SEMI_TRANS_RENDERED = 1 << 5,
  VISFLG_CSM_SHADOW_RENDERED = 1 << 6,
  VISFLG_MAIN_CAMERA_RENDERED = 1 << 7
};