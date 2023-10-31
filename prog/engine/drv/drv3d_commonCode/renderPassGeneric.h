#pragma once
#include <3d/dag_drv3d.h>
#include <EASTL/vector.h>

class String;

namespace render_pass_generic
{
struct RenderPass
{
#if DAGOR_DBGLEVEL > 0
  String dbgName;
#endif
  eastl::vector<RenderPassBind> actions;
  eastl::vector<uint32_t> sequence;
  int32_t subpassCnt;
  int32_t targetCnt;
  int32_t bindingOffset;

  struct ClearAccumulator
  {
    int32_t mask = 0;
    int32_t colorTarget = -1;
    int32_t depthTarget = -1;
  };

  const char *getDebugName();
  void addSubpassToList(const RenderPassDesc &rp_desc, int32_t subpass);
  void execute(uint32_t idx, ClearAccumulator &clear_acm);
  void resolveMSAATargets();
};

RenderPass *create_render_pass(const RenderPassDesc &rp_desc);
void delete_render_pass(RenderPass *rp);

void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets);
void next_subpass();
void end_render_pass();
} // namespace render_pass_generic

#define IMPLEMENT_D3D_RENDER_PASS_API_USING_GENERIC()                                                               \
  d3d::RenderPass *d3d::create_render_pass(const RenderPassDesc &rp_desc)                                           \
  {                                                                                                                 \
    return reinterpret_cast<d3d::RenderPass *>(render_pass_generic::create_render_pass(rp_desc));                   \
  }                                                                                                                 \
  void d3d::delete_render_pass(d3d::RenderPass *rp)                                                                 \
  {                                                                                                                 \
    render_pass_generic::delete_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp));               \
  }                                                                                                                 \
  void d3d::begin_render_pass(d3d::RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)      \
  {                                                                                                                 \
    render_pass_generic::begin_render_pass(reinterpret_cast<render_pass_generic::RenderPass *>(rp), area, targets); \
  }                                                                                                                 \
  void d3d::next_subpass()                                                                                          \
  {                                                                                                                 \
    render_pass_generic::next_subpass();                                                                            \
  }                                                                                                                 \
  void d3d::end_render_pass()                                                                                       \
  {                                                                                                                 \
    render_pass_generic::end_render_pass();                                                                         \
  }
