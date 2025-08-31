// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>
#include <EASTL/shared_ptr.h>
#include <render/lensFlare/render/lensFlareNodesCommon.h>

class LensFlareRenderer;

dafg::NodeHandle create_lens_flare_render_node(const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality);
dafg::NodeHandle create_lens_flare_prepare_lights_node(LensFlareRenderer *renderer);
