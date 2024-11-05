// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bhvMenuCameraControl.h"

class BhvReplayFreeCameraControl : public BhvMenuCameraControl
{
public:
  BhvReplayFreeCameraControl();
  virtual int update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/) override;
  virtual void onAttach(darg::Element *elem) override;
  virtual void onDetach(darg::Element *elem, DetachMode mode) override;
};

extern BhvReplayFreeCameraControl bhv_replay_free_camera_control;
