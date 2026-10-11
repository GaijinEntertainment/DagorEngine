//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <util/dag_simpleString.h>
#include <scene/dag_physMat.h>

// ri.dump_collres: the PhysMat table, then each RI-extra pool with a collision (pos-inst pools aside):
// its name, instances and resource
static constexpr uint32_t COLLRES_DUMP_MAGIC = 0xD0C0B3A5u; // MSB set: the ri_collisions reader refuses it
// bump on any change of the written layout; the MaterialData size is folded in
static constexpr int COLLRES_DUMP_VERSION = 3 | int(sizeof(PhysMat::MaterialData) << 8);

// MaterialData past its strings is plain data, written as it is
inline int collres_mat_pod_bytes(const PhysMat::MaterialData &m)
{
  return int((const char *)(&m + 1) - (const char *)&m.imp_absorb_k);
}

struct CollResDump
{
  struct Material : PhysMat::MaterialData
  {
    SimpleString bfClass; // bfid is -1: it indexes the writer's table only
  };
  struct Pool
  {
    SimpleString name;
    dag::Vector<mat43f> instances;
    Ptr<CollisionResource> collRes;
  };
  dag::Vector<Material> materials; // PhysMat id order: node ids index it
  dag::Vector<Pool> pools;
};
bool load_collres_dump(IGenLoad &cb, CollResDump &out);
