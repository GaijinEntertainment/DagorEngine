//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace AnimV20
{
enum GenericIrqType
{
  // Irq types lower than GIRQT_FIRST_SERVICE_IRQ are treated as USER and
  // and are not transmitted to statesDirector by IAnimCharacter2

  GIRQT_TraceFootStepDown = 0x0FFF00,

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
};
} // end of namespace AnimV20
