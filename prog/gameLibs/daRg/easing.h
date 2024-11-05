// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace darg
{

namespace easing
{

enum Mode
{
  Linear = 0,

  InQuad,
  OutQuad,
  InOutQuad,

  InCubic,
  OutCubic,
  InOutCubic,

  InQuintic,
  OutQuintic,
  InOutQuintic,

  InQuart,
  OutQuart,
  InOutQuart,

  InSine,
  OutSine,
  InOutSine,

  InCirc,
  OutCirc,
  InOutCirc,

  InExp,
  OutExp,
  InOutExp,

  InElastic,
  OutElastic,
  InOutElastic,

  InBack,
  OutBack,
  InOutBack,

  InBounce,
  OutBounce,
  InOutBounce,

  InOutBezier,
  CosineFull,

  InStep,
  OutStep,

  Blink,
  DoubleBlink,
  BlinkSin,
  BlinkCos,

  Discrete8,

  Shake4,
  Shake6,

  NUM_FUNCTIONS
};

typedef float (*Func)(float);

extern Func functions[];

} // namespace easing

} // namespace darg
