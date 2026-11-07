// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao/rtao_api.h>

namespace rtao
{

void initialize(bool) {}
void teardown() {}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtao_frame_params").handle();
}

void register_nodes(const NodeParams &, dag::Vector<dafg::NodeHandle> &) {}

} // namespace rtao
