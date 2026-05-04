//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <generic/dag_tab.h>
#include <rendInst/rendInstDesc.h>

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
  void *userPtrA;
  void *userPtrB;
  gamephys::CollisionObjectInfo *objectInfo = nullptr;
  rendinst::RendInstDesc riDesc;
};

inline void remove_contact(Tab<CollisionContactData> &contacts, int idx)
{
  // Remove duplicates
  if (auto objectInfo = contacts[idx].objectInfo)
    for (int j = idx + 1; j < contacts.size(); ++j)
      if (contacts[j].objectInfo == objectInfo)
        erase_items(contacts, j--, 1);

  // Remove this contact
  erase_items(contacts, idx, 1);
}
}; // namespace gamephys
DAG_DECLARE_RELOCATABLE(gamephys::CollisionContactData);
