//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <util/dag_index16.h>
#include <EASTL/vector.h>

class GeomNodeTree;
class DataBlock;

namespace chain_ik
{
struct ParticlePoint
{
  TMatrix tm = TMatrix::IDENT;
  TMatrix initialTm = TMatrix::IDENT;
  dag::Index16 nodeId;
  Point3 originalVec = ZERO<Point3>();

  ParticlePoint() : tm(TMatrix::IDENT), initialTm(TMatrix::IDENT), nodeId(-1), originalVec(ZERO<Point3>()) {}

  ParticlePoint(const GeomNodeTree *tree, dag::Index16 node_id);
  void updateFromTree(const GeomNodeTree *tree);

  Point3 getPos() const { return tm.getcol(3); }
  Point3 getOriginalPos() const { return initialTm.getcol(3); }
};

struct Joint
{
  float angleMinRad = -1.f;
  float angleMaxRad = -1.f;

  bool needCheckRange() const { return angleMinRad >= 0.f && angleMaxRad >= 0.f; }

  bool isInRange(float angle) const { return angle >= angleMinRad && angle <= angleMaxRad; }

  bool trimAngle(float &angle_in_out) const
  {
    if (isInRange(angle_in_out))
      return false;
    angle_in_out = clamp(angle_in_out, angleMinRad, angleMaxRad);
    return true;
  }
};

struct Chain
{
  eastl::vector<ParticlePoint> points;
  eastl::vector<float> edges;

  bool needSelectInverseSolution = false;
  TMatrix tmToLocal2DSpace = TMatrix::IDENT;
  float shoulder = -1.f;

  Joint upperJoint;
  Joint bottomJoint;

  void updateControlPoint(const TMatrix &tm) { points.back().tm = tm; }
  void solve();
  void initWithEdges(const eastl::vector<float> &edges_lengths, const TMatrix &tm_start, const Point3 &pt_finish);

private:
  void calculate3EdgesSolution(const Point2 &control_point_local, eastl::vector<Point2> &local_points_out);
};

class ChainIKSolver
{
public:
  eastl::vector<Chain> chains;

  void loadFromBlk(const DataBlock *blk, const GeomNodeTree *tree);
  void solve();
  void updateGeomNodeTree(GeomNodeTree *tree, const TMatrix &render_space_tm = TMatrix::IDENT) const;

#if DAGOR_DBGLEVEL > 0
  void renderDebug(const TMatrix &render_space_tm) const;
#endif

private:
  void loadPoints(const DataBlock *blk, const GeomNodeTree *tree);
};

} // namespace chain_ik
