//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once


#include <daECS/core/entityManager.h>

namespace ecs
{
struct NestedQueryRestorer
{
  int nestedQuery;

  NestedQueryRestorer(const ecs::EntityManager *mgr) : nestedQuery(!mgr->isConstrainedMTMode() ? mgr->getNestedQuery() : -1) {}
  void restore(ecs::EntityManager *mgr)
  {
    if (nestedQuery >= 0)
      mgr->setNestedQuery(nestedQuery);
  }
};

}; // namespace ecs
