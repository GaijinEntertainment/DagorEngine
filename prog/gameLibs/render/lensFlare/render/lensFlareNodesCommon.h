// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/fixed_function.h>

class LensFlareRenderer;

inline auto get_lens_flare_namespace() { return dafg::root() / "lens_flare"; }

struct LensFlareQualityParameters
{
  float resolutionScaling = 1; // multiplier of post_fx resolution
};

using LensFlareRenderNodeFunc =
  eastl::fixed_function<32, dafg::NodeHandle(const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality)>;
using LensFlarePrepareLightsNodeFunc = eastl::fixed_function<32, dafg::NodeHandle(LensFlareRenderer *renderer)>;

void set_up_lens_flare_renderer(bool enabled, LensFlareRenderer &lens_flare_renderer,
  dafg::NodeHandle &lens_flare_renderer__render_node, LensFlareRenderNodeFunc create_render_node_func,
  dafg::NodeHandle &lens_flare_renderer__prepare_lights_node, LensFlarePrepareLightsNodeFunc create_prepare_lights_node_func);
