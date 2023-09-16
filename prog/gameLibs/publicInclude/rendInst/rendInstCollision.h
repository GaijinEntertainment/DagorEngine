//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gameRes/dag_collisionResource.h>

#include <rendInst/rendInstDesc.h>


namespace rendinst
{

struct RendInstCollisionInfo
{
  void *handle = NULL;
  CollisionResource *collRes = NULL;
  SimpleString destrFxTemplate;
  RendInstDesc desc;
  int pool = -1;
  TMatrix tm = TMatrix::IDENT;
  BBox3 localBBox = BBox3::IDENT;
  float destrImpulse = 0.0f;
  float hp = 0.0f;
  float initialHp = 0.0f;
  int destrFxId = -1;
  int32_t userData = -1;
  bool isImmortal = false;
  bool stopsBullets = true;
  bool isDestr = false;
  bool bushBehaviour = false;
  bool treeBehaviour = false;
  bool isParent = false;
  bool destructibleByParent = false;
  int destroyNeighbourDepth = 1;

  explicit RendInstCollisionInfo(const RendInstDesc &ri_desc = RendInstDesc()) : desc(ri_desc) {}
};

struct RendInstCollisionCB
{
  using CollisionInfo = RendInstCollisionInfo; // alias for compat

  virtual void addCollisionCheck(const CollisionInfo &coll_info) = 0;
  virtual void addTreeCheck(const CollisionInfo &coll_info) = 0;
};

} // namespace rendinst