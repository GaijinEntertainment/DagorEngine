// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/nodeHandle.h>

namespace d3d
{
struct RenderPass;
}

dabfg::NodeHandle mk_scope_begin_rp_mobile_node(d3d::RenderPass *opaqueWithScopeRenderPass);
dabfg::NodeHandle mk_scope_prepass_mobile_node();
dabfg::NodeHandle mk_scope_mobile_node();
dabfg::NodeHandle mk_scope_lens_mask_mobile_node();
dabfg::NodeHandle mk_scope_depth_cut_mobile_node();

dabfg::NodeHandle mk_scope_forward_node();
dabfg::NodeHandle mk_scope_prepass_forward_node();
dabfg::NodeHandle mk_scope_lens_mask_forward_node();
dabfg::NodeHandle mk_scope_vrs_mask_forward_node();
dabfg::NodeHandle mk_scope_lens_hole_forward_node();

dabfg::NodeHandle mk_scope_lens_mobile_node();