#include <render/debugGbuffer.h>
#include <render/debugMesh.h>
#include <render/deferredRT.h>
#include <shaders/dag_shaderBlock.h>

const eastl::array<eastl::string_view, (size_t)DebugGbufferMode::Count> gbuffer_debug_options = {
#define MODE(mode, num) #mode,
#define LAST_MODE(mode)
#define DEBUG_MESH_MODE(mode) #mode,
#include <render/debugGbufferModes.h>
};

DebugGbufferMode show_gbuffer = DebugGbufferMode::None;
const int USE_DEBUG_GBUFFER_MODE = -2;

void setDebugGbufferMode(eastl::string_view mode)
{
  if (mode.empty())
  {
    show_gbuffer = DebugGbufferMode::None;
    return;
  }

  int fnd = -2, next_found = -2;
  for (int i = 0; i < (int)gbuffer_debug_options.size(); ++i)
  {
    eastl::string_view option = gbuffer_debug_options[i];
    if (option == mode)
    {
      fnd = i;
      break;
    }
    else if (option.starts_with(mode))
    {
      next_found = next_found == -2 || (int)show_gbuffer == next_found ? i : next_found;
    }
  }
  if (fnd >= -1)
    show_gbuffer = (DebugGbufferMode)fnd;
  else if (next_found >= -1)
    show_gbuffer = (DebugGbufferMode)next_found;
}

eastl::string getDebugGbufferUsage()
{
  eastl::string str;
  int counter = 0;
  for (eastl::string_view mode : gbuffer_debug_options)
  {
    if (counter % 16 == 0 && counter > 0)
      str.append("\n                     ");
    counter++;
    str.append(" ").append(eastl::string(mode));
  }
  return str;
}

void debug_render_gbuffer(PostFxRenderer debugRenderer, DeferredRT &gbuffer, int mode)
{
  gbuffer.setVar();
  debug_render_gbuffer(debugRenderer, gbuffer.getDepth(), mode);
}

void debug_render_gbuffer(PostFxRenderer debugRenderer, Texture *depth, int mode)
{
  if (mode == USE_DEBUG_GBUFFER_MODE)
    mode = (int)show_gbuffer;

  if (mode >= 0)
  {
    static int show_gbufferVarId = get_shader_variable_id("show_gbuffer");
    ShaderGlobal::set_int(show_gbufferVarId, mode);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    Driver3dRenderTarget renderTarget;
    d3d::get_render_target(renderTarget);
    bool rw = renderTarget.isDepthReadOnly();
    renderTarget.setDepthReadOnly();
    d3d::set_render_target(renderTarget);

    // TODO: make a more flexible solution for stencil access
    bool useStencil = debug_mesh::is_enabled();
    if (useStencil)
      depth->setReadStencil(true);
    debugRenderer.render();
    if (useStencil)
      depth->setReadStencil(false);

    if (!rw)
      renderTarget.setDepthRW();
    d3d::set_render_target(renderTarget);
  }
}
