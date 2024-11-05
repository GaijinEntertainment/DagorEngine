// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>

namespace bvh::cables
{
void on_cables_changed(Cables *, ContextId) {}
const dag::Vector<UniqueBLAS> &get_blases(ContextId, int &)
{
  static dag::Vector<UniqueBLAS> blases;
  return blases;
}

void init() {}
void init(ContextId) {}
void teardown() {}
void teardown(ContextId) {}
} // namespace bvh::cables
