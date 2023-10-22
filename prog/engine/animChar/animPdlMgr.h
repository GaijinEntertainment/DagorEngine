// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_ANIM_PDL_MGR_H
#define _GAIJIN_ANIM_PDL_MGR_H
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

#endif
