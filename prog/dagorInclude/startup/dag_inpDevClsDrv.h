//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiDecl.h>

class WinCritSec;

// global pointers to class drivers; can be NULL
extern HumanInput::IGenKeyboardClassDrv *global_cls_drv_kbd;
extern HumanInput::IGenPointingClassDrv *global_cls_drv_pnt;
extern HumanInput::IGenJoystickClassDrv *global_cls_drv_joy;
extern WinCritSec global_cls_drv_update_cs;

extern bool dgs_inpdev_disable_acessibility_shortcuts;
extern bool dgs_inpdev_allow_rawinput;
extern bool dgs_inpdev_rawinput_kbd_inited, dgs_inpdev_rawinput_mouse_inited;

void acquire_all_inpdev();
void reset_all_inpdev(bool report_btn_release = true);
void destroy_all_inpdev();
