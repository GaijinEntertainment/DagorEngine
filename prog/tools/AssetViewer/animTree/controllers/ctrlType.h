// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

#define CONTROLLERS_LIST                                                                                                       \
  DECL_CTRL(null), DECL_CTRL(alias), DECL_CTRL(hub), DECL_CTRL(paramSwitch), DECL_CTRL(paramSwitchS), DECL_CTRL(linearPoly),   \
    DECL_CTRL(randomSwitch), DECL_CTRL(fifo3), DECL_CTRL(setMotionMatchingTag), DECL_CTRL(animateAndProcNode), DECL_CTRL(BIS), \
    DECL_CTRL(blender), DECL_CTRL(stub), DECL_CTRL(linear)

#define POST_BLEND_CONTROLLERS_LIST                                                                                                \
  DECL_CTRL(deltaRotateShiftCalc), DECL_CTRL(deltaAnglesCalc), DECL_CTRL(alignNode), DECL_CTRL(alignEx), DECL_CTRL(rotateNode),    \
    DECL_CTRL(rotateAroundNode), DECL_CTRL(scaleNode), DECL_CTRL(moveNode), DECL_CTRL(condHide), DECL_CTRL(aim),                   \
    DECL_CTRL(paramsCtrl), DECL_CTRL(defClampCtrl), DECL_CTRL(animateNode), DECL_CTRL(legsIK), DECL_CTRL(multiChainFABRIK),        \
    DECL_CTRL(attachNode), DECL_CTRL(lookat), DECL_CTRL(lookatNode), DECL_CTRL(effFromAttachment), DECL_CTRL(effectorFromChildIK), \
    DECL_CTRL(matFromNode), DECL_CTRL(nodesFromAttachement), DECL_CTRL(paramFromNode), DECL_CTRL(compoundRotateShift),             \
    DECL_CTRL(setParam), DECL_CTRL(twistCtrl), DECL_CTRL(eyeCtrl), DECL_CTRL(footLockerIK), DECL_CTRL(hasAttachment),              \
    DECL_CTRL(humanAim)

#define PROC_CONTROLLERS_LIST                                                                                        \
  DECL_CTRL(moveNode), DECL_CTRL(scaleNode), DECL_CTRL(rotateNode), DECL_CTRL(rotateAroundNode), DECL_CTRL(alignEx), \
    DECL_CTRL(alignNode), DECL_CTRL(lookat), DECL_CTRL(lookatNode), DECL_CTRL(compoundRotateShift)

#define DECL_CTRL(x) String(#x)
inline const Tab<String> ctrl_type = {CONTROLLERS_LIST, POST_BLEND_CONTROLLERS_LIST};
inline const Tab<String> proc_ctrl_type = {PROC_CONTROLLERS_LIST};
#undef DECL_CTRL

#define DECL_CTRL(x) ctrl_type_##x
enum CtrlType
{
  ctrl_type_inner_include = -3,
  ctrl_type_include = -2,
  ctrl_type_not_found = -1,
  CONTROLLERS_LIST,
  POST_BLEND_CONTROLLERS_LIST,
  ctrl_type_size,
};
inline const Tab<CtrlType> post_blend_ctrl_type = {POST_BLEND_CONTROLLERS_LIST};
#undef DECL_CTRL

const char *get_ctrl_icon_name(CtrlType type);
bool is_post_blend_ctrl(CtrlType type);
