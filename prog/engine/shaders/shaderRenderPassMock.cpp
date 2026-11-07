// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderRenderPassMock.h"
#include "shaderRenderPassPrivate.h"

#include <drv/3d/dag_texFlags.h>
#include <util/dag_hash.h>
#include <debug/dag_assert.h>
#include <string.h>


using namespace ShaderRenderPass;

namespace shader_vars_mock
{
void bump_generation();
}

namespace
{

struct MockShape
{
  shader_render_pass_mock::Shape desc;
  Id id = Id::Invalid;
  eastl::vector<FramebufferId> framebufferIds;
};

eastl::vector<MockShape> shapes;
eastl::vector<eastl::string> calls;

const MockShape *find_shape(Id id)
{
  for (const auto &shape : shapes)
    if (shape.id == id)
      return &shape;
  return nullptr;
}

const shader_render_pass_mock::Framebuffer *find_framebuffer(const MockShape &shape, FramebufferId fb)
{
  for (int i = 0; i < int(shape.framebufferIds.size()); ++i)
    if (shape.framebufferIds[i] == fb)
      return &shape.desc.framebuffers[i];
  return nullptr;
}

} // namespace

namespace shader_render_pass_mock
{

void reset()
{
  shapes.clear();
  calls.clear();
  shader_vars_mock::bump_generation();
}

void add_shape(const Shape &shape)
{
  MockShape &added = shapes.emplace_back();
  added.desc = shape;
  added.id = Id(uint32_t(str_hash_fnv1a(shape.name)));
  for (const auto &fb : shape.framebuffers)
    added.framebufferIds.push_back(FramebufferId(uint32_t(str_hash_fnv1a(fb.name))));
}

void bump_dump_generation() { shader_vars_mock::bump_generation(); }

const eastl::vector<eastl::string> &get_calls() { return calls; }

} // namespace shader_render_pass_mock

Id ShaderRenderPass::find_id(const char *name)
{
  const Id id = Id(uint32_t(str_hash_fnv1a(name)));
  return find_shape(id) ? id : Id::Invalid;
}

FramebufferId ShaderRenderPass::find_framebuffer_id(const char *name)
{
  const FramebufferId fb = FramebufferId(uint32_t(str_hash_fnv1a(name)));
  for (const auto &shape : shapes)
    if (find_framebuffer(shape, fb))
      return fb;
  return FramebufferId::Invalid;
}

int ShaderRenderPass::get_target_count(Id id)
{
  const MockShape *shape = find_shape(id);
  return shape ? int(shape->desc.targets.size()) : 0;
}

const char *ShaderRenderPass::get_target_name(Id id, int i)
{
  const MockShape *shape = find_shape(id);
  return shape && i >= 0 && i < int(shape->desc.targets.size()) ? shape->desc.targets[i] : nullptr;
}

int ShaderRenderPass::get_target_slot(Id id, const char *name)
{
  const MockShape *shape = find_shape(id);
  if (!shape || !name)
    return -1;
  for (int i = 0; i < int(shape->desc.targets.size()); ++i)
    if (strcmp(shape->desc.targets[i], name) == 0)
      return i;
  return -1;
}

bool ShaderRenderPass::is_target_multisampled(Id id, int i)
{
  const MockShape *shape = find_shape(id);
  if (!shape || shape->desc.framebuffers.empty() || i < 0 || i >= int(shape->desc.targets.size()) ||
      i >= int(shape->desc.framebuffers[0].formats.size()))
    return false;
  return get_sample_count(shape->desc.framebuffers[0].formats[i]) > 1;
}

dag::ConstSpan<ResolvePair> ShaderRenderPass::get_resolves(Id id)
{
  const MockShape *shape = find_shape(id);
  return shape ? make_span_const(shape->desc.resolves.data(), shape->desc.resolves.size()) : dag::ConstSpan<ResolvePair>{};
}

int ShaderRenderPass::get_subpass_count(Id id)
{
  const MockShape *shape = find_shape(id);
  return shape ? int(shape->desc.subpasses.size()) : 0;
}

const char *ShaderRenderPass::get_subpass_name(Id id, int subpass)
{
  const MockShape *shape = find_shape(id);
  return shape && subpass >= 0 && subpass < int(shape->desc.subpasses.size()) ? shape->desc.subpasses[subpass].name : nullptr;
}

SubpassInfo ShaderRenderPass::get_subpass_info(Id id, int subpass)
{
  const MockShape *shape = find_shape(id);
  if (!shape || subpass < 0 || subpass >= int(shape->desc.subpasses.size()))
    return {};
  const auto &sp = shape->desc.subpasses[subpass];
  return SubpassInfo{
    .colors = make_span_const(sp.colors.data(), sp.colors.size()),
    .depth = sp.depth,
    .depthUse = sp.depthUse,
    .reads = make_span_const(sp.reads.data(), sp.reads.size()),
    .vrs = sp.vrs,
  };
}

dag::ConstSpan<FramebufferId> ShaderRenderPass::get_pass_framebuffers(Id id)
{
  const MockShape *shape = find_shape(id);
  return shape ? make_span_const(shape->framebufferIds.data(), shape->framebufferIds.size()) : dag::ConstSpan<FramebufferId>{};
}

FramebufferId ShaderRenderPass::get_only_framebuffer(Id id)
{
  const auto fbs = get_pass_framebuffers(id);
  G_ASSERT_RETURN(fbs.size() == 1, FramebufferId::Invalid);
  return fbs[0];
}

const char *ShaderRenderPass::get_framebuffer_name(FramebufferId fb)
{
  for (const auto &shape : shapes)
    if (const auto *layout = find_framebuffer(shape, fb))
      return layout->name;
  return nullptr;
}

TargetFormatInfo ShaderRenderPass::get_framebuffer_target_format(Id id, FramebufferId fb, int target)
{
  const MockShape *shape = find_shape(id);
  const auto *layout = shape ? find_framebuffer(*shape, fb) : nullptr;
  if (!layout || target < 0 || target >= int(layout->formats.size()))
    return {0, 0};
  return {
    .format = layout->formats[target],
    .samples = uint8_t(get_sample_count(layout->formats[target])),
  };
}

ExecutionConstraint ShaderRenderPass::get_execution_constraint(Id id)
{
  const MockShape *shape = find_shape(id);
  return shape ? shape->desc.constraint : ExecutionConstraint::NativeOnly;
}

bool ShaderRenderPass::device_breaks_subpass_reads() { return false; }

#if DAGOR_DBGLEVEL > 0
Marker::Marker(Id, int) {}
Marker::~Marker() {}

void ShaderRenderPass::verify_active_pass(ShaderBindumpHandle, int, int, const char *) {}
void ShaderRenderPass::verify_active_pass_group(ShaderBindumpHandle, int, const char *) {}
#endif

ActivePass ShaderRenderPass::get_active_pass() { return {}; }

bool check_render_passes_against_main_dump(const ScriptedShadersBinDumpOwner &) { return true; }

void dafg_set_active_shader_render_pass(Id id, int subpass)
{
  const MockShape *shape = find_shape(id);
  if (shape)
    calls.push_back(eastl::string(eastl::string::CtorSprintf{}, "active %s %d", shape->desc.name, subpass));
  else
    calls.push_back("inactive");
}
