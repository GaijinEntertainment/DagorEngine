//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>


extern KRNLIMP void (*loading_progress_point_cb)();


__forceinline void loading_progress_point()
{
  if (loading_progress_point_cb)
    loading_progress_point_cb();
}


#include <supp/dag_undef_COREIMP.h>
