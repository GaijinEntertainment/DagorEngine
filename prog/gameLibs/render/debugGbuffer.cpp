// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debugGbuffer.h>
#include <render/debugMesh.h>
#include <render/deferredRT.h>
#include <render/viewportTiles.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_info.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_span.h>
#include <debug/dag_textMarks.h>

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
DebugGbufferComposition show_gbuffer_composition = DebugGbufferComposition::Single;
const int USE_DEBUG_GBUFFER_MODE = -2;

const eastl::array<eastl::string_view, 2> gbuffer_debug_composition_options = {"grid", "overview"};

// debug-mesh modes (lod, drawElements, overdraw) must not be used in these tables:
// they read stencil through depth_gbuf, which cannot be switched per tile
static const eastl::array<DebugGbufferMode, 16> gbuffer_debug_grid_modes = {DebugGbufferMode::diffuseColor,
  DebugGbufferMode::specularColor, DebugGbufferMode::normal, DebugGbufferMode::smoothness, DebugGbufferMode::metalness,
  DebugGbufferMode::materialType, DebugGbufferMode::finalAO, DebugGbufferMode::preshadow, DebugGbufferMode::translucency,
  DebugGbufferMode::depth, DebugGbufferMode::ssr, DebugGbufferMode::reflectance, DebugGbufferMode::emission, DebugGbufferMode::lights,
  DebugGbufferMode::contactShadows, DebugGbufferMode::shadowCascades};

static const eastl::array<DebugGbufferMode, 12> gbuffer_debug_overview_modes = {DebugGbufferMode::diffuseColor,
  DebugGbufferMode::specularColor, DebugGbufferMode::normal, DebugGbufferMode::smoothness, DebugGbufferMode::emission,
  DebugGbufferMode::metalness, DebugGbufferMode::materialType, DebugGbufferMode::finalAO, DebugGbufferMode::preshadow,
  DebugGbufferMode::translucency, DebugGbufferMode::depth, DebugGbufferMode::ssr};

static dag::ConstSpan<DebugGbufferMode> composition_modes(DebugGbufferComposition composition)
{
  switch (composition)
  {
    case DebugGbufferComposition::Grid: return make_span_const(gbuffer_debug_grid_modes);
    case DebugGbufferComposition::Overview: return make_span_const(gbuffer_debug_overview_modes);
    default: return {};
  }
}

int debug_vectors_count = 1000.;
float debug_vectors_scale = 0.05f;


bool shouldRenderGbufferDebug()
{
  return show_gbuffer != DebugGbufferMode::None || show_gbuffer_with_vectors != DebugGbufferMode::None ||
         show_gbuffer_composition != DebugGbufferComposition::Single;
}

static void setModeHelper(eastl::string_view str, DebugGbufferMode &mode_out, bool with_compositions = false)
{
  if (str.empty())
  {
    mode_out = DebugGbufferMode::None;
    show_gbuffer_composition = DebugGbufferComposition::Single;
    return;
  }

  if (with_compositions)
    for (int i = 0; i < (int)gbuffer_debug_composition_options.size(); ++i)
      if (gbuffer_debug_composition_options[i] == str)
      {
        const DebugGbufferComposition newMode = (DebugGbufferComposition)(i + 1);
        show_gbuffer_composition = newMode == show_gbuffer_composition ? DebugGbufferComposition::Single : newMode;
        show_gbuffer = DebugGbufferMode::None;
        show_gbuffer_with_vectors = DebugGbufferMode::None;
        return;
      }

  show_gbuffer_composition = DebugGbufferComposition::Single;

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
  DebugGbufferMode newMode = DebugGbufferMode::None;
  if (fnd >= -1)
    newMode = (DebugGbufferMode)fnd;
  else if (next_found >= -1)
    newMode = (DebugGbufferMode)next_found;

  mode_out = newMode == mode_out ? DebugGbufferMode::None : newMode;
}


void setDebugGbufferMode(eastl::string_view mode, bool with_compositions) { setModeHelper(mode, show_gbuffer, with_compositions); }

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

eastl::string getDebugGbufferModeName(DebugGbufferMode mode) { return eastl::string(gbuffer_debug_options[(int)mode]); }

eastl::string getDebugGbufferWithVectorsUsage() { return getUsageHelper(gbuffer_debug_with_vectors_options); }

class DebugGbufferRenderScope
{
public:
  DebugGbufferRenderScope(Texture *in_depth) : depth(in_depth)
  {
    // TODO: make a more flexible solution for stencil access
    useStencil = debug_mesh::is_enabled();
    if (useStencil)
      depth->setReadStencil(true);
  }

  ~DebugGbufferRenderScope()
  {
    if (useStencil)
      depth->setReadStencil(false);
  }

private:
  Texture *depth;
  bool useStencil;
};

static int prevMode = -1;

void debug_render_gbuffer(const PostFxRenderer &debugRenderer, DeferredRT &gbuffer, Texture *depth, int mode)
{
  gbuffer.setVar();
  if (depth)
    ShaderGlobal::set_texture_unsafe(get_shader_variable_id("depth_gbuf"), depth);

  if (mode == USE_DEBUG_GBUFFER_MODE)
    mode = (int)show_gbuffer;

  static int dbgGbuffMode_VarId = get_shader_variable_id("gbuff_dbg_mode", true);

  const bool needsDebugDump = mode == (int)DebugGbufferMode::mip || mode == (int)DebugGbufferMode::texelDensity;
  const bool hadDebugDump = prevMode == (int)DebugGbufferMode::mip || prevMode == (int)DebugGbufferMode::texelDensity;
  bool released = true;

  if (needsDebugDump)
  {
    if (!hadDebugDump && !require_debug_shaders())
    {
      LOGERR_ONCE("Debug shader dump could not be loaded!");
      return;
    }

    static int dbgGbuffer_TexId = get_shader_variable_id("dbg_gbuff_tex");

    gbuffer.setShouldRenderDbgTex(true);
    ShaderGlobal::set_int(dbgGbuffMode_VarId, mode == (int)DebugGbufferMode::texelDensity);
    ShaderGlobal::set_texture(dbgGbuffer_TexId, gbuffer.getDbgTex());
  }
  else
  {
    if (hadDebugDump)
      released = release_debug_shaders();

    ShaderGlobal::set_int(dbgGbuffMode_VarId, -1);
    gbuffer.setShouldRenderDbgTex(false);
  }

  // keeping prevMode on a failed restore makes the next frame retry it
  if (released)
    prevMode = mode;
  debug_render_gbuffer(debugRenderer, depth ? depth : gbuffer.getDepth(), mode);
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

void debug_render_gbuffer_tiles(const PostFxRenderer &debugRenderer, DebugGbufferComposition composition, bool with_labels)
{
  const dag::ConstSpan<DebugGbufferMode> modes = composition_modes(composition);
  if (!modes.size())
    return;

  const bool border_only = composition == DebugGbufferComposition::Overview;
  const int grid_cols = 4;
  const int grid_rows = 4;

  static int show_gbufferVarId = get_shader_variable_id("show_gbuffer");
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  const auto gbufGridCallback = [&modes, &debugRenderer, with_labels](int tile_x, int tile_y, int tile_w, int tile_h, int index) {
    if (index >= modes.size())
      return;

    G_UNUSED(tile_h);
    const DebugGbufferMode mode = modes[index];
    ShaderGlobal::set_int(show_gbufferVarId, (int)mode);
    debugRenderer.render();

    if (!with_labels)
      return;

    const eastl::string_view name = gbuffer_debug_options[(int)mode];
    add_debug_text_mark(tile_x + tile_w * 0.5f, tile_y + 12, name.data(), (int)name.size(), 0.8f);
  };

  for_each_viewport_tile(grid_cols, grid_rows, border_only, gbufGridCallback);
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
    ShaderGlobal::set_float(vec_countVarId, float(vec_count));
    static int vec_scaleVarId = get_shader_variable_id("gbuffer_debug_vec_scale");
    ShaderGlobal::set_float(vec_scaleVarId, vec_scale);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    DebugGbufferRenderScope scope(depth);
    debugVecShader.shader->setStates();
    d3d::setvsrc(0, 0, 0);
    d3d::draw_instanced(PRIM_LINELIST, 0, 2, debug_vectors_count);
  }
}

DebugGbufferMode get_debug_gbuffer_mode() { return show_gbuffer; }
