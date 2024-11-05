// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlType.h"
#include "../animTreeIcons.h"

const char *get_ctrl_icon_name(CtrlType type)
{
  switch (type)
  {
    case ctrl_type_paramSwitch: return CTRL_PARAM_SWITCH_ICON;
    case ctrl_type_condHide: return CTRL_COND_HIDE_ICON;
    case ctrl_type_attachNode: return CTRL_ATTACH_NODE_ICON;
    case ctrl_type_hub: return CTRL_HUB_ICON;
    case ctrl_type_linearPoly: return CTRL_LINEAR_POLY_ICON;
    case ctrl_type_effFromAttachment: return CTRL_EFF_FROM_ATT_ICON;
    case ctrl_type_randomSwitch: return CTRL_RANDOM_SWITCH_ICON;
    case ctrl_type_paramsCtrl: return CTRL_PARAMS_CTRL_ICON;
    case ctrl_type_fifo3: return CTRL_FIFO3_ICON;
    case ctrl_type_setParam: return CTRL_SET_PARAM_ICON;
    case ctrl_type_moveNode: return CTRL_MOVE_NODE_ICON;
    case ctrl_type_rotateNode: return CTRL_ROTATE_NODE_ICON;
    case ctrl_type_rotateAroundNode: return CTRL_ROTATE_AROUND_NODE_ICON;

    default: return ANIM_BLEND_CTRL_ICON;
  }
}
