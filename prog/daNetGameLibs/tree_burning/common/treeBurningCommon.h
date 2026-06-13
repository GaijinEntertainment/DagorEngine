// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <daECS/net/msgDecl.h>
#include <dag/dag_vectorSet.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <rendInst/riexHandle.h>

using BurntTreesRiExHandles = dag::VectorSet<rendinst::riex_handle_t>;

ECS_BROADCAST_EVENT_TYPE(EventDestroyBurnedTree, rendinst::riex_handle_t /*handle*/);
ECS_UNICAST_EVENT_TYPE(EventCreateTreeBurningChain,
  rendinst::riex_handle_t /*riExHandle*/,
  TMatrix /*treeTm*/,
  int /*canopyType*/,
  Point3 /*canopyBoxMin*/,
  Point3 /*canopyBoxMax*/,
  Point3 /*burningSourceWpos*/,
  bool /*isBush*/);
ECS_UNICAST_EVENT_TYPE(EventAddTreeBurningChainSource, Point3 /*burningSourceWpos*/);

ECS_NET_DECL_MSG(TreeBurningInitialSyncMsg, danet::BitStream);

// True if the rendinst's collision uses a "soft" (slowing) material like bushes, i.e. has
// SoftMaterialProps with physViscosity != 1 - the same property that slows the soldier down.
bool riex_collision_is_soft(rendinst::riex_handle_t handle);

void serialize_tree_burning_data(danet::BitStream &bs, const BurntTreesRiExHandles &burnt_trees);
void deserialize_tree_burning_data(const danet::BitStream &bs);
void apply_tree_burning_data_for_render(dag::ConstSpan<rendinst::riex_handle_t> burnt_trees);
