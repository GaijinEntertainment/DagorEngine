//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physUserData.h>

struct CollisionObjectUserData : public PhysObjectUserData
{
  int matId = -1;

  MAKE_PHYS_OBJ_TYPE(CollisionObjectUserData, _MAKE4C('CLOB'));
};

struct CollisionObject
{
  PhysBody *body = nullptr;

  CollisionObject() = default;
  explicit CollisionObject(PhysBody *b) : body(b) {}

  explicit operator bool() const { return body != nullptr; }
  bool isValid() const { return body; }
  void clear_ptrs() { body = nullptr; }
};
