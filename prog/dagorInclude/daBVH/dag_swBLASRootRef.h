//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// soa4::RootRef only: the dependency-free handle type for consumers that store or pass a SoA4
// BLAS root without walking the tree (e.g. rasterizer task structs). The format, walkers and
// everything else live in dag_swBLAS_soa4.h, which includes this header.

#include <util/dag_stdint.h>

namespace soa4
{

// Root of a SoA4 tree: a tagged child word (see the dag_swBLAS_soa4.h banner). Strong type so a
// root cannot be confused with a byte offset or a stackless blasStart. invalid() roots come from
// failed conversions.
struct RootRef
{
  int32_t v = -1;
  bool valid() const { return v >= 0; }
};

} // namespace soa4
