//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_hlsl_floatx.h>
#include <render/wind/rendinst_bend_desc.hlsli>

class ClusterWindCascade;

class ClusterWindRenderer
{
public:
  void updateClusterBuffer(const void *clusters, int num); // avoid circular definition
  void updateClusterWindGridsBuffer(const Tab<ClusterWindCascade> &clustersGrids);
  void updateClusterWindGridsDesc(const Tab<ClusterWindCascade> &clustersGrids); // load every time we change cascade
  void updateRenderer();
  void loadBendingMultConst(float treeMult, float impostorMult, float grassMult, float treeStaticMult, float impostorStaticMult,
    float grassStaticMult, float treeAnimationMult, float grassAnimationMult);

  unsigned int currentClusterIndex;
  carray<Tab<ClusterDescGpu>, 2u> clusterDescArr;
  carray<Tab<ClusterCascadeDescGpu>, 2u> cascadeDescArr;
  carray<Tab<uint4>, 2u> gridsIdArr;


  void updateCurrentRendererIndex() { currentClusterIndex = ((currentClusterIndex + 1u) % 2u); };
  bool getClusterDescForCpuSim(int num, ClusterDescGpu &outputClusterDescGpu);
  ClusterCascadeDescGpu getClusterCascadeDescForCpuSim(int num) { return cascadeDescArr[(currentClusterIndex + 1u) % 2u][num]; };
  const int getClusterCascadeDescNum() const { return cascadeDescArr[(currentClusterIndex + 1u) % 2u].size(); };
  bool getClusterGridsIdForCpuSim(int num, uint4 &outGridsId);
  ClusterWindRenderer(bool need_historical_buffer);
  ~ClusterWindRenderer();
};
