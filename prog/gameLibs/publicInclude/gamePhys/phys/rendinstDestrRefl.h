//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daNet/mpi.h>

namespace rendinstdestr
{
void init_refl_object(mpi::ObjectID oid);
void destroy_refl_object();
void enable_reflection_refl_object(bool);
void refl_object_on_changed();
mpi::IObject *get_refl_object();
}; // namespace rendinstdestr
