// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

dafg::NodeHandle makePrepareDepthForPostFxNode(bool withHistory)
{
  return dafg::register_node("prepare_depth_for_postfx", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    registry.renameTexture("depth_after_transparency", "depth_for_postfx",
      withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No);
  });
}

dafg::NodeHandle makePrepareDepthAfterTransparent()
{
  return dafg::register_node("prepare_depth_after_transparency", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("depth_for_transparency", "depth_after_transparency", dafg::History::No); });
}

dafg::NodeHandle makePrepareMotionVectorsAfterTransparent(bool withHistory)
{
  return dafg::register_node("prepare_motion_vectors_after_transparency", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    registry.orderMeBefore("after_world_render_node");
    registry.renameTexture("motion_vecs", "motion_vecs_after_transparency",
      withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No);
  });
}