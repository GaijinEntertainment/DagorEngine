// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderRenderPass.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>


namespace shader_render_pass_mock
{

struct Subpass
{
  const char *name = "";
  eastl::vector<int> colors;
  int depth = -1;
  ShaderRenderPass::DepthUse depthUse = ShaderRenderPass::DepthUse::Write;
  eastl::vector<int> reads;
  int vrs = -1;
};

struct Framebuffer
{
  const char *name = "";
  eastl::vector<uint32_t> formats;
};

struct Shape
{
  const char *name = "";
  eastl::vector<const char *> targets;
  eastl::vector<Subpass> subpasses;
  eastl::vector<ShaderRenderPass::ResolvePair> resolves;
  eastl::vector<Framebuffer> framebuffers;
  ShaderRenderPass::ExecutionConstraint constraint = ShaderRenderPass::ExecutionConstraint::Any;
};

void reset();
void add_shape(const Shape &shape);
void bump_dump_generation();

const eastl::vector<eastl::string> &get_calls();

} // namespace shader_render_pass_mock
