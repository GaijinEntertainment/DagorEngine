// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daBfg/bfg.h>
#include <render/world/defaultVrsSettings.h>

dabfg::NodeHandle makeCreateVrsTextureNode()
{
  const bool vrsSupported = d3d::get_driver_desc().caps.hasVariableRateShadingTexture;
  if (vrsSupported)
    return dabfg::register_node("create_vrs_texture_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      registry.create(VRS_RATE_TEXTURE_NAME, dabfg::History::No)
        .texture({TEXFMT_R8UI | TEXCF_VARIABLE_RATE | TEXCF_UNORDERED, registry.getResolution<2>("texel_per_vrs_tile")})
        .atStage(dabfg::Stage::COMPUTE)
        .useAs(dabfg::Usage::SHADER_RESOURCE);
    });
  else
    return dabfg::NodeHandle();
}
