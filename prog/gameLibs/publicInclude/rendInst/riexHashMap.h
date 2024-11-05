//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <limits.h>

#include <rendInst/riexHandle.h>
#include <util/dag_hash.h>

#include <ska_hash_map/flat_hash_map2.hpp>

struct RiexHandlerHasher
{
  size_t operator()(rendinst::riex_handle_t rId) const
  {
    typedef FNV1Params<sizeof(rendinst::riex_handle_t) * CHAR_BIT> FNV;
    return (size_t)((FNV::prime * (rId ^ FNV::offset_basis)) >> (sizeof(rendinst::riex_handle_t) * CHAR_BIT - 32));
  }
};

template <typename T>
using RiexHashMap = ska::flat_hash_map<rendinst::riex_handle_t, T, RiexHandlerHasher>;
