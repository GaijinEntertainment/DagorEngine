// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/ptgi/ptgi_api.h>

namespace ptgi
{

void initialize(bool) {}
void teardown() {}

void set_second_chance_history(bool) {}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("ptgi_frame_params").handle();
}

void register_nodes(const NodeParams &, dag::Vector<dafg::NodeHandle> &) {}

bool is_validation_layer_enabled() { return false; }
bool is_second_chance_history_enabled() { return false; }
void render_validation_layer() {}

} // namespace ptgi
