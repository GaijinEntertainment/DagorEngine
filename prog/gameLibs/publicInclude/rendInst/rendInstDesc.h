//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>

namespace rendinst
{
struct RendInstDesc
{
  int cellIdx;
  int idx;
  int pool;
  uint32_t offs;

  int layer;

  RendInstDesc() { reset(); }

  RendInstDesc(int cell_idx, int index, int pool_id, uint32_t offset, int layer_idx) :
    cellIdx(cell_idx), idx(index), pool(pool_id), offs(offset), layer(layer_idx)
  {}

  RendInstDesc(riex_handle_t handle);

  void reset()
  {
    cellIdx = idx = offs = 0;
    pool = -1;
    layer = 0;
  }

  bool doesReflectedInstanceMatches(const RendInstDesc &rhs) const;

  bool operator==(const RendInstDesc &rhs) const
  {
    return cellIdx == rhs.cellIdx && idx == rhs.idx && pool == rhs.pool && offs == rhs.offs && layer == rhs.layer;
  }

  bool operator!=(const RendInstDesc &rhs) const { return !(*this == rhs); }

  void setRiExtra() { cellIdx = -1; }

  bool isValid() const { return pool >= 0; }
  void invalidate() { pool = -1; }
  bool isRiExtra() const { return cellIdx < 0; }
  bool isDynamicRiExtra() const;

  riex_handle_t getRiExtraHandle() const;
};

bool isRiGenDescValid(const RendInstDesc &desc);
bool isRgLayerPrimary(const RendInstDesc &desc);

} // namespace rendinst
