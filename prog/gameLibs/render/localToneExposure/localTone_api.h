// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/daFrameGraph/stage.h>

namespace dafg
{
class Registry;
}

// Local exposure nodes and their tonemap hook.
namespace local_tone
{
// The nodes read the resource names in root; empty names are not allowed.
// The resolution names must outlive the nodes.
struct Inputs
{
  // HDR frame for the nodes' own downsample; required when downsampledFrame is null.
  const char *frame = nullptr;
  // Its frame_bloom_downsample; when null, the nodes make it.
  const char *downsampledFrame = nullptr;
  // Uv offsets to the previous frame; null means none.
  const char *motionVectors = nullptr;
  const char *frameResolution = "post_fx";
  const char *viewResolution = "main_view";
};

// Reversed depth at viewResolution, bound by the game.
inline constexpr const char *DEPTH_SHADER_VAR = "local_tone_depth";

// local_tone_enabled is not 0.
bool is_enabled();
bool has_shaders(const Inputs &inputs);
void register_nodes(const Inputs &inputs, dag::Vector<dafg::NodeHandle> &nodes);
// For the hook's node; else no gain.
void read_hook_inputs(dafg::Registry registry, dafg::Stage stage);
} // namespace local_tone
