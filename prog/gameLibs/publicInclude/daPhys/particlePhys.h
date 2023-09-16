//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>
#include <util/dag_index16.h>

class GeomNodeTree;
class DataBlock;


namespace daphys
{
struct ParticlePoint
{
  TMatrix tm;
  TMatrix initialTm;
  TMatrix invInitialTm;
  TMatrix helperTm;
  BBox3 limits;
  dag::Index16 gnNodeId;

  ParticlePoint(const TMatrix &initial_tm);

  Point3 limitDir(const Point3 &dir);
  void addDelta(const Point3 &delta);

  Point3 getPos() const { return tm.getcol(3); }
  Point3 getInitialPos() const { return initialTm.getcol(3); }
};

struct Constraint
{
  virtual bool solve() = 0;
  virtual void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm) = 0;
  virtual void renderDebug(const TMatrix &render_space_tm) const = 0;

  virtual ~Constraint() {}
};

class ParticlePhysSystem
{
  Tab<ParticlePoint *> particles;
  Tab<Constraint *> constraints;

  void loadData(const DataBlock *blk, const GeomNodeTree *tree);

public:
  ~ParticlePhysSystem();

  ParticlePoint *findParticle(dag::Index16 gn_node_id) const;
  int findParticleId(dag::Index16 gn_node_id) const;
  ParticlePoint *getParticle(int particle_id) const;

  void loadFromBlk(const DataBlock *blk, const GeomNodeTree *tree);

  void update();
  void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm);

  void renderDebug(const TMatrix &render_space_tm) const;
};
}; // namespace daphys
