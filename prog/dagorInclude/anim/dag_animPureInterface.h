//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animDecl.h>
#include <util/dag_stdint.h>

// forward declarations for external classes
class Point3;
namespace ecs
{
class EntityManager;
}


namespace AnimV20
{

//
// Generic IRQ interface
//
class IGenericIrq
{
public:
  virtual intptr_t irq(int type, intptr_t p1, intptr_t p2, intptr_t p3, ecs::EntityManager *mgr = nullptr) = 0;
};

} // end of namespace AnimV20
