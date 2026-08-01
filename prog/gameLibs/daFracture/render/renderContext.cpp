// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/renderContext.h>

#include <generic/dag_initOnDemand.h>


namespace frx
{

GlobalRenderContext::GlobalRenderContext() : riMultidrawContext("frx_ri_multidraw") {}
GlobalRenderContext::~GlobalRenderContext() = default;

static InitOnDemand<GlobalRenderContext> g_render_context;

void init_render() { g_render_context.demandInit(); }
void shutdown_render() { g_render_context.demandDestroy(); }
GlobalRenderContext &get_render_ctx()
{
  G_ASSERT(g_render_context);
  return *g_render_context;
}


} // namespace frx