// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animBranches.h"

namespace AnimV20
{
extern GenericAnimStatesGraph *(*makeGraphFromRes)(const char *graphname);

inline GenericAnimStatesGraph *cloneGraph(GenericAnimStatesGraph *g, IPureAnimStateHolder *new_st)
{
  return g ? (GenericAnimStatesGraph *)g->clone(new_st) : nullptr;
}
inline void destroyGraph(GenericAnimStatesGraph *&g) { destroy_it(g); }
} // namespace AnimV20
