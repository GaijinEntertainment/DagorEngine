//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum GenProcCompMsg
{
  GPCM_Activate = 0x0006, // same as WM_ACTIVATE
  GPCM_SetFocus,
  GPCM_MouseMove,
  GPCM_MouseBtnPress,
  GPCM_MouseBtnRelease,
  GPCM_MouseWheel,
  GPCM_KeyPress,
  GPCM_KeyRelease,
  GPCM_Char,
  GPCM_TouchBegan,
  GPCM_TouchMoved,
  GPCM_TouchEnded,
  GPCM_JoystickMovement,
};

enum GenProcCompMsgPar1
{
  GPCMP1_Inactivate = 0,
  GPCMP1_Activate = 1,
  GPCMP1_ClickActivate = 2,
};

enum
{
  WM_DAGOR_USER = 0x0400,
  WM_DAGOR_SUSPEND_EVENT = WM_DAGOR_USER,
  WM_DAGOR_INPUT,
  WM_DAGOR_SETCURSOR,
  WM_DAGOR_CLOSING,
  WM_DAGOR_CLOSED,
  WM_DAGOR_DESTROY,
};
