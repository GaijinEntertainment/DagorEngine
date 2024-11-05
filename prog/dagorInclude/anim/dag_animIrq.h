//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace AnimV20
{
enum GenericIrqType
{
  // Irq types lower than GIRQT_FIRST_SERVICE_IRQ are treated as USER and
  // and are not transmitted to statesDirector by IAnimCharacter2

  GIRQT_TraceFootStepMultiRay = 0x0FFF02,

  GIRQT_GetMotionMatchingPose = 0x0FFF10,

  GIRQT_FIRST_SERVICE_IRQ = 0x100000,
  GIRQT_EndOfSingleAnim = GIRQT_FIRST_SERVICE_IRQ,
  GIRQT_EndOfContinuousAnim,
};

enum GenericIrqResponse
{
  GIRQR_NoResponse,

  GIRQR_RewindSingleAnim,
  GIRQR_StopSingleAnim,
  GIRQR_ResumeSingleAnim,

  GIRQR_TraceOK,

  GIRQR_MotionMatchingPoseApplied,
};
} // end of namespace AnimV20
