//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>


extern KRNLIMP void (*loading_progress_point_cb)();


__forceinline void loading_progress_point()
{
  if (loading_progress_point_cb)
    loading_progress_point_cb();
}


#include <supp/dag_undef_KRNLIMP.h>
