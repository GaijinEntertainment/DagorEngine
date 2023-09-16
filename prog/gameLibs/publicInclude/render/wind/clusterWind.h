//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <generic/dag_carray.h>
#include <EASTL/unique_ptr.h>

#include <render/wind/clusterWindCascade.h>
#include <render/wind/dynamicWind.h>
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
    Point3 direction;
    float power; // mult bending [1,4]
    float time;
    float maxTime;
    int directionnalId; // for directionnale if -1 then blast
  };

  explicit ClusterWind(bool need_historical_buffer);
  ~ClusterWind();

private:
  eastl::unique_ptr<ClusterWindRenderer> clusterWindRenderer;
  eastl::unique_ptr<DynamicWind> dynamicWindMgr;
  carray<ClusterDesc, MAX_CLUSTER_CPP> clusterWinds;
  int actualClusterWindsCount;
  bool isInit;
  OSSpinlock clusterToGridSpinLock;

  bool checkAndFillGridBox(const IPoint2 &boxXY, int cascadeNo, int clusterId);
  bool isPriority(const ClusterDesc &value, const ClusterDesc &toCompare, int boxId, int cascadeNo);
  void updateAfterPositionChange(ClusterWindCascade::ClusterWindGridUpdateResult gridPoisitionResult, int cascadeNo);
  int addClusterToList(const Point3 &pos, float r, float power, const Point3 &dir, bool directionnal = false);
  void removeClusterFromList(int id);
  void recalculateWholeCascade(const Point3 &pos, int cascadeNo);

public:
  float particlesForceMul = 1.f;
  int actualDirectionnalClusterWindCount;
  carray<int, MAX_CLUSTER_CPP> directionnalWindId; // for faster directionnal wind search
  Tab<ClusterWindCascade> clusterWindCascades;
  void addClusterToGrid(const Point3 &pos, float r, float power, const Point3 &dir = Point3(0, 0, 0), bool directionnal = false,
    int clusterId = -1);
  void initClusterWinds(Tab<ClusterWindCascade::Desc> &desc);
  void updateGridPosition(const Point3 &pos);
  void updateClusterWinds(float dt);
  void updateDirectionnalWinds(const Point3 &pos, float dt);
  ClusterWind::ClusterDesc getClusterDescAt(int at);
  float3 getBlastAtPosForParticle(const float3 &pos);
  Point3 getBlastAtPosForParticle(const Point3 &pos);
  Point3 getBlastAtPosForAntenna(const Point3 &pos);
  void loadBendingMultConst(float treeMult, float impostorMult, float grassMult, float treeStaticMult, float impostorStaticMult,
    float grassStaticMult, float treeAnimationMult, float grassAnimationMult);

  Tab<WindSpawner> getDynamicWindGenerator();
  void initDynamicWind(int beaufortScale, const Point3 &direction);

  void enableDynamicWind(bool isEnabled);

  void loadDynamicWindDesc(DynamicWind::WindDesc &_desc);

  void updateRenderer();

  void flipClusterWindSimArray();
};
