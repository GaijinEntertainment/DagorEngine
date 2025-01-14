// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/shared_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <render/daBfg/nodeHandle.h>
#include <render/world/dynamicShadowRenderExtender.h>
#include "../common.h"

namespace dagdp
{

struct RiexRenderableInfo
{
  RenderableInstanceLodsResource *lodsRes;
  int lodIndex;
  bool isTree;
};

struct RiexResource
{
  int riExId;
  uint32_t riPoolOffset;
  dag::Vector<RenderableId> lods_rId;
  eastl::shared_ptr<GameResource> gameRes; // We could use unique_ptr, but it does not play nicely with
                                           // ECS_DECLARE_BOXED_TYPE(dagdp::RiexManager)
};

struct RiexBuilder
{
  NameMap resourceAssetNameMap;
  dag::Vector<RiexResource> resources;
  dag::VectorMap<RenderableId, RiexRenderableInfo> renderablesInfo;
};

struct RiexPersistentData;

struct RiexManager
{
  RiexBuilder currentBuilder;                                               // Only valid while building a view.
  dag::Vector<eastl::shared_ptr<RiexPersistentData>> currentDynShadowViews; // Only valid after finalize_view, and before finalize.
  DynamicShadowRenderExtender::Handle shadowExtensionHandle;
};

void riex_finalize_view(const ViewInfo &view_info, const ViewBuilder &view_builder, RiexManager &manager, NodeInserter node_inserter);

void riex_finalize(RiexManager &manager);

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::RiexManager);
