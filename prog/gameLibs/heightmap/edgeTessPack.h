// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

// decode_edge_tesselation() of edge_tesselation.dshl unpacks this.
static inline uint32_t uint_pack_edges_tess(const int edge_tess[4])
{
  return edge_tess[0] | (edge_tess[1] << 4) | (edge_tess[2] << 8) | (edge_tess[3] << 12);
}
