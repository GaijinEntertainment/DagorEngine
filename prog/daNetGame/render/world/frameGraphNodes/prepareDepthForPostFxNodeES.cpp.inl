// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <render/antiAliasing_legacy.h>
#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>

dafg::NodeHandle makePrepareDepthForPostFxNode(bool withHistory)
{
  return dafg::register_node("prepare_depth_for_postfx", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    registry.renameTexture("depth_after_transparency", "depth_for_postfx")
      .withHistory(withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No);
  });
}

dafg::NodeHandle makePrepareDepthAfterTransparent()
{
  return dafg::register_node("prepare_depth_after_transparency", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("depth_for_transparency", "depth_after_transparency"); });
}

dafg::NodeHandle makePrepareMotionVectorsAfterTransparent(bool withHistory)
{
  return dafg::register_node("prepare_motion_vectors_after_transparency", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    registry.orderMeBefore("after_world_render_node");
    registry.renameTexture("motion_vecs", "motion_vecs_after_transparency")
      .withHistory(withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No);
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_prepare_depth_for_postfx_node_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makePrepareDepthForPostFxNode(evt.needDepthHistory));

  const AntiAliasing *aa = WRDispatcher::getAntialiasing();

  if (evt.hasMotionVectors)
  {
    const bool withHistory = aa && aa->needMotionVectorHistory();
    evt.nodes->push_back(makePrepareMotionVectorsAfterTransparent(withHistory));
  }
}
