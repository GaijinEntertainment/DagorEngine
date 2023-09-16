//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

KRNLIMP void enable_float_exceptions(bool enable);
KRNLIMP bool is_float_exceptions_enabled();

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

#include <supp/dag_undef_COREIMP.h>
