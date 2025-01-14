// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/nodeHandle.h>

#include <EASTL/unique_ptr.h>


dabfg::NodeHandle mk_frame_data_setup_node();

dabfg::NodeHandle mk_panorama_prepare_mobile_node();
dabfg::NodeHandle mk_panorama_apply_mobile_node();
dabfg::NodeHandle mk_panorama_apply_forward_node();

dabfg::NodeHandle mk_opaque_setup_mobile_node();
dabfg::NodeHandle mk_opaque_begin_rp_node();
dabfg::NodeHandle mk_opaque_mobile_node();
dabfg::NodeHandle mk_opaque_resolve_mobile_node();
dabfg::NodeHandle mk_opaque_end_rp_mobile_node();

dabfg::NodeHandle mk_opaque_setup_forward_node();
dabfg::NodeHandle mk_rename_depth_opaque_forward_node();
dabfg::NodeHandle mk_static_opaque_forward_node();
dabfg::NodeHandle mk_dynamic_opaque_forward_node();
dabfg::NodeHandle mk_decals_on_dynamic_forward_node();

dabfg::NodeHandle mk_decals_mobile_node();

dabfg::NodeHandle mk_water_prepare_mobile_node();
dabfg::NodeHandle mk_water_mobile_node();
dabfg::NodeHandle mk_under_water_fog_mobile_node();

dabfg::NodeHandle mk_transparent_effects_setup_mobile_node();
dabfg::NodeHandle mk_transparent_particles_mobile_node();

dabfg::NodeHandle mk_occlusion_preparing_mobile_node();
dabfg::NodeHandle mk_postfx_target_producer_mobile_node();
dabfg::NodeHandle mk_postfx_mobile_node();
dabfg::NodeHandle mk_finalize_frame_mobile_node();
dabfg::NodeHandle mk_debug_render_mobile_node();
