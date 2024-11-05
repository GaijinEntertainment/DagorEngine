//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <generic/dag_carray.h>
#include <EASTL/unique_ptr.h>

#include <render/wind/clusterWindCascade.h>
#include <osApiWrappers/dag_spinlock.h>

#define MAX_CLUSTER_CPP 254

class DataBlock;

class ClusterWindRenderer;
class float3;

class ClusterWind
{

public:
  struct ClusterDesc // used for cpu
  {
    Point4 sphere;
    float power; // mult bending [1,4]
    float time;
    float maxTime;
  };

  explicit ClusterWind(bool need_historical_buffer);
  ~ClusterWind();

private:
  eastl::unique_ptr<ClusterWindRenderer> clusterWindRenderer;
  carray<ClusterDesc, MAX_CLUSTER_CPP> clusterWinds;
  int actualClusterWindsCount;
  bool isInit;
  OSSpinlock clusterToGridSpinLock;

  bool checkAndFillGridBox(const IPoint2 &boxXY, int cascadeNo, int clusterId);
  bool isPriority(const ClusterDesc &value, const ClusterDesc &toCompare, int boxId, int cascadeNo);
  void updateAfterPositionChange(ClusterWindCascade::ClusterWindGridUpdateResult gridPoisitionResult, int cascadeNo);
  int addClusterToList(const Point3 &pos, float r, float power);
  void removeClusterFromList(int id);
  void recalculateWholeCascade(const Point3 &pos, int cascadeNo);

public:
  float particlesForceMul = 1.f;
  int actualDirectionnalClusterWindCount;
  carray<int, MAX_CLUSTER_CPP> directionnalWindId; // for faster directionnal wind search
  Tab<ClusterWindCascade> clusterWindCascades;
  void addClusterToGrid(const Point3 &pos, float r, float power, int clusterId = -1);
  void initClusterWinds(Tab<ClusterWindCascade::Desc> &desc);
  void updateGridPosition(const Point3 &pos);
  void updateClusterWinds(float dt);
  ClusterWind::ClusterDesc getClusterDescAt(int at);
  float3 getBlastAtPosForParticle(const float3 &pos);
  Point3 getBlastAtPosForParticle(const Point3 &pos);
  Point3 getBlastAtPosForAntenna(const Point3 &pos);
  void loadBendingMultConst(float treeMult, float impostorMult, float grassMult, float treeAnimationMult, float grassAnimationMult);

  void updateRenderer();

  void flipClusterWindSimArray();
};
