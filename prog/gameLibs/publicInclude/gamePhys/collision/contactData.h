//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <generic/dag_tab.h>

namespace gamephys
{
struct CollisionObjectInfo;

struct CollisionContactData
{
  Point3 wpos;
  Point3 wposB;
  Point3 wnormB;
  Point3 posA;
  Point3 posB;
  float depth;
  void *userPtrA;
  void *userPtrB;
  int matId;
  gamephys::CollisionObjectInfo *objectInfo;
};

inline void remove_contact(Tab<CollisionContactData> &contacts, int idx)
{
  // Remove duplicates
  for (int j = idx + 1; j < contacts.size(); ++j)
    if (contacts[j].objectInfo == contacts[idx].objectInfo)
      erase_items(contacts, j--, 1);

  // Remove this contact
  erase_items(contacts, idx, 1);
}
}; // namespace gamephys
