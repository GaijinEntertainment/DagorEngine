//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

//! creates named inter-process mutex
KRNLIMP void *global_mutex_create(const char *mutex_name);

//! locks named inter-process mutex
KRNLIMP int global_mutex_enter(void *mutex);

//! unlocks named inter-process mutex
KRNLIMP int global_mutex_leave(void *mutex);

//! destroys named inter-process mutex
KRNLIMP int global_mutex_destroy(void *mutex, const char *mutex_name);

//! combined create+lock
inline void *global_mutex_create_enter(const char *mutex_name)
{
  void *m = global_mutex_create(mutex_name);
  if (m)
    global_mutex_enter(m);
  return m;
}

//! combined unlock+destroy
inline int global_mutex_leave_destroy(void *mutex, const char *mutex_name)
{
  int ret = global_mutex_leave(mutex);
  if (ret)
    return ret;
  return global_mutex_destroy(mutex, mutex_name);
}

#include <supp/dag_undef_COREIMP.h>
