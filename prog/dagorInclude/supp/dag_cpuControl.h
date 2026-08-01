//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

KRNLIMP void enable_float_exceptions(bool enable);
KRNLIMP bool is_float_exceptions_enabled();

// Sets FTZ+DAZ and round-to-nearest in the calling thread; exception masks are
// left intact. Applied automatically at process start and at DaThread/cpujobs
// thread entry (see cpuControl.cpp); idempotent and cheap, so it may be
// re-applied after code that resets FP state.
KRNLIMP void set_default_fp_control_this_thread();

class FloatingPointExceptionsKeeper
{
public:
  FloatingPointExceptionsKeeper()
  {
    savedFE = is_float_exceptions_enabled();
    if (savedFE)
      enable_float_exceptions(false);
  }

  ~FloatingPointExceptionsKeeper()
  {
    if (savedFE)
      enable_float_exceptions(true);
  }

protected:
  bool savedFE;
};

#include <supp/dag_undef_KRNLIMP.h>
