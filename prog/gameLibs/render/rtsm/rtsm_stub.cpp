// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm/rtsm_api.h>

namespace rtsm
{

void initialize() {}
void teardown() {}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtsm_frame_params").handle();
}

void register_nodes(const NodeParams &, dag::Vector<dafg::NodeHandle> &) {}

void render_dynamic_light_shadows(bvh::ContextId, const Point3 &, Texture *, float, bool, bool) {}

} // namespace rtsm
