//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <generic/dag_tab.h>

namespace gamephys
{
struct CollisionObjectInfo;

struct CollisionContactDataMin
{
  Point3 wpos;
  int matId;
};

struct CollisionContactData : CollisionContactDataMin //-V730
{
  Point3 wposB;
  Point3 wnormB;
  Point3 posA;
  Point3 posB;
  float depth;
  int riPool = -1; // Not riExtra
  void *userPtrA;
  void *userPtrB;
  gamephys::CollisionObjectInfo *objectInfo = nullptr;
};

inline void remove_contact(Tab<CollisionContactData> &contacts, int idx)
{
  // Remove duplicates
  for (int j = idx + 1; j < contacts.size(); ++j)
    if (contacts[j].userPtrB == contacts[idx].userPtrB)
      erase_items(contacts, j--, 1);

  // Remove this contact
  erase_items(contacts, idx, 1);
}
}; // namespace gamephys
DAG_DECLARE_RELOCATABLE(gamephys::CollisionContactData);
