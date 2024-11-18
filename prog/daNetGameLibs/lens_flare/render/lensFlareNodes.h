// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include <render/daBfg/bfg.h>
#include <EASTL/shared_ptr.h>

class LensFlareRenderer;

inline auto get_lens_flare_namespace() { return dabfg::root() / "lens_flare"; }

struct LensFlareQualityParameters
{
  float resolutionScaling = 1; // multiplier of post_fx resolution
};

dabfg::NodeHandle create_lens_flare_render_node(const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality);
dabfg::NodeHandle create_lens_flare_prepare_lights_node(LensFlareRenderer *renderer);
