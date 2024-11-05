// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>

dabfg::NodeHandle makePrepareDepthForPostFxNode(bool withHistory)
{
  return dabfg::register_node("prepare_depth_for_postfx", DABFG_PP_NODE_SRC, [withHistory](dabfg::Registry registry) {
    registry.renameTexture("depth_after_transparency", "depth_for_postfx",
      withHistory ? dabfg::History::DiscardOnFirstFrame : dabfg::History::No);
  });
}

dabfg::NodeHandle makePrepareDepthAfterTransparent()
{
  return dabfg::register_node("prepare_depth_after_transparency", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.renameTexture("depth_for_transparency", "depth_after_transparency", dabfg::History::No);
  });
}

dabfg::NodeHandle makePrepareMotionVectorsAfterTransparent(bool withHistory)
{
  return dabfg::register_node("prepare_motion_vectors_after_transparency", DABFG_PP_NODE_SRC, [withHistory](dabfg::Registry registry) {
    registry.orderMeBefore("after_world_render_node");
    registry.renameTexture("motion_vecs", "motion_vecs_after_transparency",
      withHistory ? dabfg::History::DiscardOnFirstFrame : dabfg::History::No);
  });
}