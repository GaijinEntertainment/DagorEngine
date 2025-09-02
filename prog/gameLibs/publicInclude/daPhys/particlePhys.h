//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  int8_t subVehicleIdx = -1;

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

  typedef Tab<int> ParticleGroupIds;
  Tab<ParticleGroupIds> particleGroups;

  void loadData(const DataBlock *blk, const GeomNodeTree *tree);

public:
  ParticlePhysSystem() = default;
  ParticlePhysSystem(ParticlePhysSystem &&) = default;
  ~ParticlePhysSystem();

  dag::ConstSpan<ParticlePoint *> getParticles() const { return make_span_const(particles); }

  ParticlePoint *findParticle(dag::Index16 gn_node_id) const;
  int findParticleId(dag::Index16 gn_node_id) const;
  ParticlePoint *getParticle(int particle_id) const;

  int particleGroupsCount() const { return particleGroups.size(); }
  dag::ConstSpan<const int> getParticleGroupIds(int group_id) const { return particleGroups[group_id]; }

  void loadFromBlk(const DataBlock *blk, const GeomNodeTree *tree);

  void update();
  void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm);

  void renderDebug(const TMatrix &render_space_tm) const;
};
}; // namespace daphys
