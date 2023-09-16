#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <fftWater/fftWater.h>
#include <util/dag_convar.h>

#define GLOBAL_VARS_LIST           \
  VAR(world_to_heightmap)          \
  VAR(world_to_flowmap)            \
  VAR(world_to_flowmap_prev)       \
  VAR(world_to_flowmap_add)        \
  VAR(height_texture_size)         \
  VAR(flowmap_texture_size)        \
  VAR(flowmap_texture_size_meters) \
  VAR(flowmap_temp_tex)            \
  VAR(water_flowmap_tex_add)       \
  VAR(water_flowmap_num_winds)     \
  VAR(water_flowmap_debug)         \
  VAR(water_wind_strength)         \
  VAR(water_flowmap_fading)        \
  VAR(water_flowmap_strength)      \
  VAR(water_flowmap_strength_add)  \
  VAR(water_flowmap_foam)          \
  VAR(water_flowmap_foam_tiling)   \
  VAR(water_flowmap_depth)         \
  VAR(world_to_depth_ao)           \
  VAR(depth_ao_texture_size)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
}

CONSOLE_BOOL_VAL("render", debug_water_flowmap, false);

namespace fft_water
{

static Point4 flowmapArea(1, 1, 0, 0);

void build_flowmap(FFTWater *handle, FlowmapParams &flowmap_params, int flowmap_texture_size, int heightmap_texture_size,
  const Point3 &camera_pos, float range)
{
  UniqueTex &texA = flowmap_params.texA;
  UniqueTex &texB = flowmap_params.texB;
  PostFxRenderer &builder = flowmap_params.builder;
  int &frame = flowmap_params.frame;
  eastl::vector<FlowmapWind> &winds = flowmap_params.winds;
  UniqueBufHolder &windsBuf = flowmap_params.windsBuf;

  ShaderGlobal::set_real(water_flowmap_debugVarId, debug_water_flowmap.get() ? 1 : 0);

  if (frame == 0)
  {
    frame = 1;

    init_shader_vars();
    set_flowmap_params(flowmap_params);

    texA.close();
    texB.close();

    // build flowmap
    texA = dag::create_tex(NULL, flowmap_texture_size, flowmap_texture_size,
      TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_CLEAR_ON_CREATE, 1, "water_flowmap_tex_a");
    texB = dag::create_tex(NULL, flowmap_texture_size, flowmap_texture_size,
      TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_CLEAR_ON_CREATE, 1, "water_flowmap_tex_b");
    texA.getTex2D()->texaddr(TEXADDR_CLAMP);
    texB.getTex2D()->texaddr(TEXADDR_CLAMP);
    texA.getTex2D()->texfilter(TEXFILTER_SMOOTH);
    texB.getTex2D()->texfilter(TEXFILTER_SMOOTH);

    builder.init("water_flowmap");

    windsBuf = dag::create_sbuffer(sizeof(FlowmapWind), MAX_FLOWMAP_WINDS,
      SBCF_BIND_CONSTANT | SBCF_CPU_ACCESS_WRITE | SBCF_MAYBELOST | SBCF_DYNAMIC, 0, "water_flowmap_winds");
  }

  if (!texA || !texB)
    return;

  float cameraSnap = float(flowmap_texture_size) / (range * 2);
  Point3 cameraPos = camera_pos;
  if (cameraSnap != 0)
  {
    cameraPos.x = floorf(cameraPos.x * cameraSnap) / cameraSnap;
    cameraPos.z = floorf(cameraPos.z * cameraSnap) / cameraSnap;
  }

  Point4 area = Point4(cameraPos.x - range, cameraPos.z - range, cameraPos.x + range, cameraPos.z + range);
  area.z -= area.x;
  area.w -= area.y;
  G_ASSERT(area.z && area.w);
  if (area.z && area.w)
    area = Point4(1.0f / area.z, 1.0f / area.w, -area.x / area.z, -area.y / area.w);
  ShaderGlobal::set_color4(world_to_flowmap_prevVarId, flowmapArea);
  ShaderGlobal::set_color4(world_to_flowmap_addVarId, area);
  flowmapArea = area;

  ShaderGlobal::set_real(flowmap_texture_size_metersVarId, range * 2);
  ShaderGlobal::set_int(flowmap_texture_sizeVarId, flowmap_texture_size);
  ShaderGlobal::set_int(height_texture_sizeVarId, heightmap_texture_size);

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

  d3d::set_render_target();
  int frameId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  int sceneId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  {
    TIME_D3D_PROFILE(build_flowmap)

    bool evenFrame = frame <= 2;
    UniqueTex &flowmapSrc = evenFrame ? texA : texB;
    UniqueTex &flowmapDst = evenFrame ? texB : texA;

    ShaderGlobal::set_texture(flowmap_temp_texVarId, flowmapSrc);
    d3d::resource_barrier({flowmapSrc.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::set_render_target(flowmapDst.getTex2D(), 0);

    int numWinds = min(int(winds.size()), MAX_FLOWMAP_WINDS);
    ShaderGlobal::set_int(water_flowmap_num_windsVarId, numWinds);

    if (frame < 2)
    {
      frame = 2;
      d3d::clearview(CLEAR_TARGET, E3DCOLOR(127, 127, 127, 127), 0, 0);

      if (numWinds > 0)
        windsBuf->updateData(0, numWinds * sizeof(*winds.data()), winds.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    }

    builder.render();

    ShaderGlobal::set_texture(flowmap_temp_texVarId, BAD_TEXTUREID);

    ShaderGlobal::set_texture(water_flowmap_tex_addVarId, flowmapDst);
    d3d::resource_barrier({flowmapDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});

    frame = 5 - frame;
  }

  if (frameId >= 0)
    ShaderGlobal::setBlock(frameId, ShaderGlobal::LAYER_FRAME);
  if (sceneId >= 0)
    ShaderGlobal::setBlock(sceneId, ShaderGlobal::LAYER_SCENE);

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
}

void set_flowmap_params(FlowmapParams &flowmap_params)
{
  if (flowmap_params.frame == 0)
    return;

  ShaderGlobal::set_real(water_wind_strengthVarId, flowmap_params.windStrength);
  ShaderGlobal::set_real(water_flowmap_fadingVarId, flowmap_params.flowmapFading);
  ShaderGlobal::set_color4(water_flowmap_strengthVarId, flowmap_params.flowmapStrength);
  ShaderGlobal::set_color4(water_flowmap_strength_addVarId, flowmap_params.flowmapStrengthAdd);
  ShaderGlobal::set_color4(water_flowmap_foamVarId, flowmap_params.flowmapFoam);
  ShaderGlobal::set_real(water_flowmap_foam_tilingVarId, flowmap_params.flowmapFoamTiling);
  ShaderGlobal::set_color4(water_flowmap_depthVarId, flowmap_params.flowmapDepth);

  SharedTexHolder &tex = flowmap_params.tex;
  String &texName = flowmap_params.texName;
  Point4 &texArea = flowmap_params.texArea;

  tex.close();

  if (texName && !texName.empty())
  {
    tex = dag::get_tex_gameres(texName.c_str(), "water_flowmap_tex");
    ShaderGlobal::set_color4(world_to_flowmapVarId, texArea);
  }
}

void close_flowmap(FlowmapParams &flowmap_params)
{
  clear_and_shrink(flowmap_params.texName);
  flowmap_params.tex.close();
  flowmap_params.texA.close();
  flowmap_params.texB.close();
  flowmap_params.frame = 0;
  flowmap_params.winds.clear();
  flowmap_params.windsBuf.close();
}

} // namespace fft_water
