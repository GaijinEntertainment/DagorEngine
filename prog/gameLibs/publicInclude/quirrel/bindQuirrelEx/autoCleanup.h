//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>
#include <stdint.h>

namespace sq
{

typedef void (*cleanup_unregfunc_cb_t)(HSQUIRRELVM vm);

class CleanupUnregRec // record of auto-binding registry
{
  cleanup_unregfunc_cb_t unregfuncCb;
  CleanupUnregRec *next;
  friend void cleanup_unreg_native_api(HSQUIRRELVM);

public:
  CleanupUnregRec(cleanup_unregfunc_cb_t cb);
};

// Actually perform cleanup from VM (usually before VM destruction)
void cleanup_unreg_native_api(HSQUIRRELVM vm);

}; // namespace sq

#define SQ_DEF_AUTO_CLEANUP_UNREGFUNC(Func)                  \
  static sq::CleanupUnregRec Func##_auto_cleanup_var(&Func); \
  extern const size_t sq_autocleanup_pull_##Func = (size_t)(&Func##_auto_cleanup_var);
