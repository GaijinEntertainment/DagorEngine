//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_hash.h>
#include <EASTL/type_traits.h>

namespace sndsys
{
typedef uint32_t str_hash_t;
} // namespace sndsys

#define SND_HASH(a)      eastl::integral_constant<sndsys::str_hash_t, str_hash_fnv1(a)>::value
#define SND_HASH_SLOW(a) sndsys::str_hash_t(str_hash_fnv1(a))
