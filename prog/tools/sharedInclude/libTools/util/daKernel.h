//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

namespace dakernel
{
KRNLIMP void reset_named_pointers();
KRNLIMP void set_named_pointer(const char *name, void *p);
KRNLIMP void *get_named_pointer(const char *name);
} // namespace dakernel

#include <supp/dag_undef_COREIMP.h>
