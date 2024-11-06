// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debugGbuffer.h>
#include <render/debugMesh.h>
#include <render/deferredRT.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

using OptionsMap = eastl::array<eastl::string_view, (size_t)DebugGbufferMode::Count>;
const OptionsMap gbuffer_debug_options = {
#define MODE(mode, num)             #mode,
#define MODE_HAS_VECTORS(mode, num) #mode,
#define LAST_MODE(mode)
#define DEBUG_MESH_MODE(mode) #mode,
#include <render/debugGbufferModes.h>
#undef DEBUG_MESH_MODE
#undef LAST_MODE
#undef MODE_HAS_VECTORS
#undef MODE
};

const OptionsMap gbuffer_debug_with_vectors_options = {
#define MODE(mode, num)             "",
#define MODE_HAS_VECTORS(mode, num) #mode,
#define LAST_MODE(mode)
#define DEBUG_MESH_MODE(mode) "",
#include <render/debugGbufferModes.h>
#undef DEBUG_MESH_MODE
#undef LAST_MODE
#undef MODE_HAS_VECTORS
#undef MODE
};

DebugGbufferMode show_gbuffer = DebugGbufferMode::None;
DebugGbufferMode show_gbuffer_with_vectors = DebugGbufferMode::None;
const int USE_DEBUG_GBUFFER_MODE = -2;

int debug_vectors_count = 1000.;
float debug_vectors_scale = 0.05f;

bool shouldRenderGbufferDebug()
{
  return show_gbuffer != DebugGbufferMode::None || show_gbuffer_with_vectors != DebugGbufferMode::None;
}

static void setModeHelper(eastl::string_view str, DebugGbufferMode &mode_out)
{
  if (str.empty())
  {
    mode_out = DebugGbufferMode::None;
    return;
  }

  int fnd = -2, next_found = -2;
  for (int i = 0; i < (int)gbuffer_debug_options.size(); ++i)
  {
    eastl::string_view option = gbuffer_debug_options[i];
    if (option == str)
    {
      fnd = i;
      break;
    }
    else if (option.starts_with(str))
    {
      next_found = next_found == -2 || (int)mode_out == next_found ? i : next_found;
    }
  }
  if (fnd >= -1)
    mode_out = (DebugGbufferMode)fnd;
  else if (next_found >= -1)
    mode_out = (DebugGbufferMode)next_found;
}

void setDebugGbufferMode(eastl::string_view mode) { setModeHelper(mode, show_gbuffer); }

void setDebugGbufferWithVectorsMode(eastl::string_view mode, int vectorsCount, float vectorsScale)
{
  setModeHelper(mode, show_gbuffer_with_vectors);
  debug_vectors_count = max(vectorsCount, 0);
  debug_vectors_scale = max(vectorsScale, 0.f);
}

static eastl::string getUsageHelper(const OptionsMap &options)
{
  eastl::string str;
  int counter = 0;
  for (eastl::string_view mode : options)
  {
    if (mode.empty())
      continue;
    if (counter % 16 == 0 && counter > 0)
      str.append("\n                     ");
    counter++;
    str.append(" ").append(eastl::string(mode));
  }
  return str;
}

eastl::string getDebugGbufferUsage() { return getUsageHelper(gbuffer_debug_options); }

eastl::string getDebugGbufferWithVectorsUsage() { return getUsageHelper(gbuffer_debug_with_vectors_options); }

class DebugGbufferRenderScope
{
public:
  DebugGbufferRenderScope(Texture *in_depth) : depth(in_depth)
  {
    d3d::get_render_target(renderTarget);
    rwDepth = renderTarget.isDepthReadOnly();
    renderTarget.setDepthReadOnly();
    d3d::set_render_target(renderTarget);

    // TODO: make a more flexible solution for stencil access
    useStencil = debug_mesh::is_enabled();
    if (useStencil)
      depth->setReadStencil(true);
  }

  ~DebugGbufferRenderScope()
  {
    if (useStencil)
      depth->setReadStencil(false);

    if (!rwDepth)
      renderTarget.setDepthRW();
    d3d::set_render_target(renderTarget);
  }

private:
  Texture *depth;
  bool rwDepth;
  bool useStencil;
  Driver3dRenderTarget renderTarget;
};

void debug_render_gbuffer(const PostFxRenderer &debugRenderer, DeferredRT &gbuffer, int mode)
{
  gbuffer.setVar();
  debug_render_gbuffer(debugRenderer, gbuffer.getDepth(), mode);
}

void debug_render_gbuffer_with_vectors(const DynamicShaderHelper &debugVecShader, DeferredRT &gbuffer, int mode, int vec_count,
  float vec_scale)
{
  gbuffer.setVar();
  debug_render_gbuffer_with_vectors(debugVecShader, gbuffer.getDepth(), mode, vec_count, vec_scale);
}

void debug_render_gbuffer(const PostFxRenderer &debugRenderer, Texture *depth, int mode)
{
  if (mode == USE_DEBUG_GBUFFER_MODE)
    mode = (int)show_gbuffer;

  if (mode >= 0)
  {
    static int show_gbufferVarId = get_shader_variable_id("show_gbuffer");
    ShaderGlobal::set_int(show_gbufferVarId, mode);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    DebugGbufferRenderScope scope(depth);
    debugRenderer.render();
  }
}

void debug_render_gbuffer_with_vectors(const DynamicShaderHelper &debugVecShader, Texture *depth, int mode, int vec_count,
  float vec_scale)
{
  if (!debugVecShader.shader)
    return;
  if (mode == USE_DEBUG_GBUFFER_MODE)
    mode = (int)show_gbuffer_with_vectors;
  if (vec_count < 0)
    vec_count = debug_vectors_count;
  if (abs(vec_scale) < FLT_EPSILON)
    vec_scale = debug_vectors_scale;

  if (mode >= 0)
  {
    static int vec_countVarId = get_shader_variable_id("gbuffer_debug_vec_count");
    ShaderGlobal::set_real(vec_countVarId, float(vec_count));
    static int vec_scaleVarId = get_shader_variable_id("gbuffer_debug_vec_scale");
    ShaderGlobal::set_real(vec_scaleVarId, vec_scale);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    DebugGbufferRenderScope scope(depth);
    debugVecShader.shader->setStates();
    d3d::setvsrc(0, 0, 0);
    d3d::draw_instanced(PRIM_LINELIST, 0, 2, debug_vectors_count);
  }
}
