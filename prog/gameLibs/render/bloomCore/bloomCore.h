// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <EASTL/fixed_function.h>

struct Color3;

namespace bloom
{
void set_input_tex_name(const eastl::string &tex_name);

using RenderFunc = eastl::fixed_function<16, void()>;
using RenderCallback = eastl::fixed_function<16, void(RenderFunc render_func)>;
void regenerate_downsample_chain(
  dag::Vector<dafg::NodeHandle> &nodes, uint32_t mips_count, bool use_esram = true,
  RenderCallback cb = [](RenderFunc render_func) { render_func(); });
void regenerate_upsample_chain(
  dag::Vector<dafg::NodeHandle> &nodes, uint32_t mips_count, float upsample_factor, const Color3 &halation_color,
  float halation_mip_factor, int halation_end_mip_ofs, RenderCallback cb = [](RenderFunc render_func) { render_func(); });

uint32_t calculate_bloom_mips(uint32_t width, uint32_t height, int max_mips = 7);
} // namespace bloom