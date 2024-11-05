//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

namespace dakernel
{
KRNLIMP void reset_named_pointers();
KRNLIMP void set_named_pointer(const char *name, void *p);
KRNLIMP void *get_named_pointer(const char *name);
} // namespace dakernel

#include <supp/dag_undef_KRNLIMP.h>
