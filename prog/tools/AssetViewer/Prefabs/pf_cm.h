#pragma once

#include "../av_cm.h"


enum
{
  CM_ = CM_PLUGIN_BASE,

  CM_AUTO_OBJ_PIVOT,
  CM_CENTER_OBJ_PIVOT,
  CM_RESET_OBJ_PIVOT,
};


// object specific parameters
// Static objects
enum
{
  ID_SO_CLIPPING_GRP = ID_SPEC_GRP + 1,
  ID_SO_CLIP_IN_GAME,
  ID_SO_CLIP_IN_EDITOR,

  PID_STATIC_GEOM_GUI_START,
};


enum
{
  ID_NEW_OBJ_NAME = 1,
  ID_NEW_OBJ_PATH,
};
