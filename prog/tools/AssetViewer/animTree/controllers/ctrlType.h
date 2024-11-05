// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

#define CONTROLLERS_LIST                                                                                                       \
  DECL_CTRL(null), DECL_CTRL(setParam), DECL_CTRL(compoundRotateShift), DECL_CTRL(attachNode), DECL_CTRL(alias),               \
    DECL_CTRL(paramFromNode), DECL_CTRL(effFromAttachment), DECL_CTRL(effectorFromChildIK), DECL_CTRL(multiChainFABRIK),       \
    DECL_CTRL(nodesFromAttachement), DECL_CTRL(matFromNode), DECL_CTRL(hub), DECL_CTRL(paramSwitch), DECL_CTRL(linearPoly),    \
    DECL_CTRL(rotateNode), DECL_CTRL(rotateAroundNode), DECL_CTRL(paramsCtrl), DECL_CTRL(randomSwitch), DECL_CTRL(alignEx),    \
    DECL_CTRL(alignNode), DECL_CTRL(moveNode), DECL_CTRL(fifo3), DECL_CTRL(legsIK), DECL_CTRL(twistCtrl), DECL_CTRL(condHide), \
    DECL_CTRL(lookat), DECL_CTRL(scaleNode), DECL_CTRL(defClampCtrl), DECL_CTRL(lookatNode), DECL_CTRL(deltaRotateShiftCalc),  \
    DECL_CTRL(setMotionMatchingTag), DECL_CTRL(footLockerIK)

#define DECL_CTRL(x) String(#x)
inline const Tab<String> ctrl_type = {CONTROLLERS_LIST};
#undef DECL_CTRL

#define DECL_CTRL(x) ctrl_type_##x
enum CtrlType
{
  ctrl_type_inner_include = -3,
  ctrl_type_include = -2,
  ctrl_type_not_found = -1,
  CONTROLLERS_LIST,
  ctrl_type_size,
};
#undef DECL_CTRL

const char *get_ctrl_icon_name(CtrlType type);
