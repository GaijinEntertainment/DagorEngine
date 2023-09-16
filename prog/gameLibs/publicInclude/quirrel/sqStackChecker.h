//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>
#include <debug/dag_assert.h>


class SqStackChecker
{
public:
  SqStackChecker(HSQUIRRELVM vm_) : vm(vm_) { prevTop = sq_gettop(vm_); }

  ~SqStackChecker() { check(); }

  void check(SQInteger offset = 0)
  {
    (void)offset;
    G_ASSERTF(sq_gettop(vm) == prevTop + offset, "Top = %d -> %d", prevTop, sq_gettop(vm));
  }

  void restore() { sq_settop(vm, prevTop); }

private:
  SQInteger prevTop;
  HSQUIRRELVM vm;
};
