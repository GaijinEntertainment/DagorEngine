// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <daECS/core/componentType.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>
#include <rendInst/riexHandle.h>


struct TreeBurningManager
{
  dag::Vector<Point4> burningSources;
  UniqueBufHolder burningSourcesConstBuffer;
  dag::VectorMap<rendinst::riex_handle_t, uint32_t> burningTreesAdditionalData;
  dag::VectorSet<rendinst::riex_handle_t> burntTrees;
  dag::VectorSet<uint32_t> burnableRiExtraTypes;
  dag::VectorSet<uint32_t> leavesOnlyRiExtra;
  float staticShadowsInvalidationTimer = 0;
};

ECS_DECLARE_RELOCATABLE_TYPE(TreeBurningManager);
