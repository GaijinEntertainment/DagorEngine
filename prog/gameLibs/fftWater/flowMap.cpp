#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_lockTexture.h>
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
  VAR(water_flowmap_foam_color)    \
  VAR(water_flowmap_foam_tiling)   \
  VAR(water_flowmap_depth)         \
  VAR(water_flowmap_slope)         \
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
    set_flowmap_tex(flowmap_params);

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

    windsBuf = dag::create_sbuffer(sizeof(FlowmapWind), MAX_FLOWMAP_WINDS, SBCF_BIND_CONSTANT | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC,
      0, "water_flowmap_winds");
  }

  if (!texA || !texB)
    return;

  set_flowmap_params(flowmap_params);
  set_flowmap_foam_params(flowmap_params);

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

void set_flowmap_tex(FlowmapParams &flowmap_params)
{
  if (flowmap_params.frame == 0)
    return;

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

void set_flowmap_params(FlowmapParams &flowmap_params)
{
  if (flowmap_params.frame == 0)
    return;

  Point4 flowmapStrength = flowmap_params.flowmapStrength;
  if (!flowmap_params.usingFoamFx)
    flowmapStrength.w = 0;

  ShaderGlobal::set_real(water_wind_strengthVarId, flowmap_params.windStrength);
  ShaderGlobal::set_real(water_flowmap_fadingVarId, flowmap_params.flowmapFading);
  ShaderGlobal::set_color4(water_flowmap_strengthVarId, flowmapStrength);
  ShaderGlobal::set_color4(water_flowmap_strength_addVarId, flowmap_params.flowmapStrengthAdd);
}

void set_flowmap_foam_params(FlowmapParams &flowmap_params)
{
  if (flowmap_params.frame == 0)
    return;

  ShaderGlobal::set_color4(water_flowmap_foamVarId, flowmap_params.flowmapFoam);
  ShaderGlobal::set_color4(water_flowmap_foam_colorVarId, flowmap_params.flowmapFoamColor);
  ShaderGlobal::set_real(water_flowmap_foam_tilingVarId, flowmap_params.flowmapFoamTiling);
  ShaderGlobal::set_color4(water_flowmap_depthVarId, flowmap_params.flowmapDepth);
  ShaderGlobal::set_real(water_flowmap_slopeVarId, flowmap_params.flowmapSlope);
}

void close_flowmap(FlowmapParams &flowmap_params)
{
  flowmap_params.tex.close();
  flowmap_params.texA.close();
  flowmap_params.texB.close();
  flowmap_params.frame = 0;
  flowmap_params.winds.clear();
  flowmap_params.windsBuf.close();
}

bool is_flowmap_active(const FlowmapParams &flowmap_params)
{
  return flowmap_params.tex || flowmap_params.texA || flowmap_params.texB;
}

void flowmap_floodfill(int texSize, Texture *heightmapTex, Texture *floodfillTex, uint16_t heightmapLevel)
{
  TIME_D3D_PROFILE(flowmap_floodfill);

  int heightmapStrideX = sizeof(uint16_t);
  int heightmapStrideY = texSize * heightmapStrideX;
  LockedTexture<uint8_t> heightmapLockedTex = lock_texture<uint8_t>(heightmapTex, heightmapStrideY, 0, TEXLOCK_READ);

  int floodfillStrideX = sizeof(uint16_t);
  int floodfillStrideY = texSize * floodfillStrideX;
  LockedTexture<uint8_t> floodfillLockedTex = lock_texture<uint8_t>(floodfillTex, floodfillStrideY, 0, TEXLOCK_WRITE);

  if (heightmapLockedTex && floodfillLockedTex)
  {
    uint8_t *heightmapData = heightmapLockedTex.get();
    uint8_t *floodfillData = floodfillLockedTex.get();
    memset(floodfillData, 0, texSize * floodfillStrideY);

    int queueSize = (texSize + 1) * 2;
    int queueBegin = 0;
    int queueEnd = 0;
    int *queue = new int[queueSize];

    for (int y = 0; y < texSize; y++)
    {
      for (int x = 0; x < texSize; x++)
      {
        uint16_t *height = (uint16_t *)(heightmapData + x * heightmapStrideX + y * heightmapStrideY);
        if (height[0] < heightmapLevel)
        {
          uint16_t *flood = (uint16_t *)(floodfillData + x * floodfillStrideX + y * floodfillStrideY);
          if (flood[0] == 0)
          {
            queue[queueEnd++] = x;
            queue[queueEnd++] = y;
            if (queueEnd >= queueSize)
              queueEnd = 0;

            while (queueBegin != queueEnd)
            {
              int u = queue[queueBegin++];
              int v = queue[queueBegin++];
              if (queueBegin >= queueSize)
                queueBegin = 0;

              height = (uint16_t *)(heightmapData + u * heightmapStrideX + v * heightmapStrideY);
              flood = (uint16_t *)(floodfillData + u * floodfillStrideX + v * floodfillStrideY);

              int fx = 0;
              int fy = 0;

              int nx = 0;
              int ny = 0;

              if ((u - 1 >= 0) && (height[-1] < heightmapLevel))
              {
                if (flood[-1] == 0)
                {
                  flood[-1] = 1;
                  queue[queueEnd++] = u - 1;
                  queue[queueEnd++] = v;
                  if (queueEnd >= queueSize)
                    queueEnd = 0;
                }
                else if (flood[-1] > 1)
                {
                  fx++;
                  nx += ((flood[-1] >> 0) & 0xff) - 0x80;
                  ny += ((flood[-1] >> 8) & 0xff) - 0x80;
                }
              }
              if ((u + 1 < texSize) && (height[1] < heightmapLevel))
              {
                if (flood[1] == 0)
                {
                  flood[1] = 1;
                  queue[queueEnd++] = u + 1;
                  queue[queueEnd++] = v;
                  if (queueEnd >= queueSize)
                    queueEnd = 0;
                }
                else if (flood[1] > 1)
                {
                  fx--;
                  nx += ((flood[1] >> 0) & 0xff) - 0x80;
                  ny += ((flood[1] >> 8) & 0xff) - 0x80;
                }
              }
              if ((v - 1 >= 0) && (height[-texSize] < heightmapLevel))
              {
                if (flood[-texSize] == 0)
                {
                  flood[-texSize] = 1;
                  queue[queueEnd++] = u;
                  queue[queueEnd++] = v - 1;
                  if (queueEnd >= queueSize)
                    queueEnd = 0;
                }
                else if (flood[-texSize] > 1)
                {
                  fy++;
                  nx += ((flood[-texSize] >> 0) & 0xff) - 0x80;
                  ny += ((flood[-texSize] >> 8) & 0xff) - 0x80;
                }
              }
              if ((v + 1 < texSize) && (height[texSize] < heightmapLevel))
              {
                if (flood[texSize] == 0)
                {
                  flood[texSize] = 1;
                  queue[queueEnd++] = u;
                  queue[queueEnd++] = v + 1;
                  if (queueEnd >= queueSize)
                    queueEnd = 0;
                }
                else if (flood[texSize] > 1)
                {
                  fy--;
                  nx += ((flood[texSize] >> 0) & 0xff) - 0x80;
                  ny += ((flood[texSize] >> 8) & 0xff) - 0x80;
                }
              }

              fx = fx * 6 + nx;
              fy = fy * 6 + ny;

              int i = fx * fx + fy * fy;
              if (i)
              {
                float f = 127.0f / sqrtf(float(i));
                fx = int(float(fx) * f);
                fy = int(float(fy) * f);
              }

              flood[0] = uint16_t(((fy + 0x80) << 8) | (fx + 0x80));
            }
          }
        }
      }
    }

    delete[] queue;

    for (int y = 0; y < texSize; y++)
    {
      for (int x = 0; x < texSize; x++)
      {
        uint16_t *flood = (uint16_t *)(floodfillData + x * floodfillStrideX + y * floodfillStrideY);
        if (flood[0] <= 1)
          flood[0] = 0x8080;
      }
    }
  }
}

} // namespace fft_water
