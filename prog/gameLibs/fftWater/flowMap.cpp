// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_tex3d.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <fftWater/fftWater.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>

#define GLOBAL_VARS_LIST                     \
  VAR(world_to_heightmap)                    \
  VAR(world_to_flowmap)                      \
  VAR(world_to_flowmap_prev)                 \
  VAR(world_to_flowmap_add)                  \
  VAR(world_to_flowmap_add_0)                \
  VAR(world_to_flowmap_add_1a)               \
  VAR(world_to_flowmap_add_1b)               \
  VAR(world_to_flowmap_add_2a)               \
  VAR(world_to_flowmap_add_2b)               \
  VAR(height_texture_size)                   \
  VAR(flowmap_texture_size)                  \
  VAR(flowmap_temp_tex)                      \
  VAR(flowmap_temp_tex_samplerstate)         \
  VAR(water_flowmap_tex_add_0)               \
  VAR(water_flowmap_tex_add_0_samplerstate)  \
  VAR(water_flowmap_tex_add_1a)              \
  VAR(water_flowmap_tex_add_1a_samplerstate) \
  VAR(water_flowmap_tex_add_1b)              \
  VAR(water_flowmap_tex_add_1b_samplerstate) \
  VAR(water_flowmap_tex_add_2a)              \
  VAR(water_flowmap_tex_add_2a_samplerstate) \
  VAR(water_flowmap_tex_add_2b)              \
  VAR(water_flowmap_tex_add_2b_samplerstate) \
  VAR(water_flowmap_tex_blur_1a)             \
  VAR(water_flowmap_tex_blur_1b)             \
  VAR(water_flowmap_tex_blur_2a)             \
  VAR(water_flowmap_tex_blur_2b)             \
  VAR(water_flowmap_debug)                   \
  VAR(water_wind_strength)                   \
  VAR(water_flowmap_range)                   \
  VAR(water_flowmap_range_0)                 \
  VAR(water_flowmap_range_1)                 \
  VAR(water_flowmap_fading)                  \
  VAR(water_flowmap_damping)                 \
  VAR(water_flowmap_strength)                \
  VAR(water_flowmap_strength_add)            \
  VAR(water_flowmap_foam)                    \
  VAR(water_flowmap_foam_color)              \
  VAR(water_flowmap_foam_tiling)             \
  VAR(water_flowmap_foam_displacement)       \
  VAR(water_flowmap_foam_reflectivity_min)   \
  VAR(water_flowmap_depth)                   \
  VAR(water_flowmap_slope)                   \
  VAR(water_flowmap_cascades)                \
  VAR(water_flowmap_multiplier)              \
  VAR(water_flowmap_blend)                   \
  VAR(tex)                                   \
  VAR(tex_samplerstate)                      \
  VAR(water_flowmap_tex_samplerstate)        \
  VAR(texsz)                                 \
  VAR(water_flowmap_obstacles_add)           \
  VAR(water_flowmap_foam_detail)

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

void build_flowmap(FFTWater *handle, int flowmap_texture_size, int heightmap_texture_size, const Point3 &camera_pos, int cascade,
  bool obstacles)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap)
    return;
  if ((cascade > 0) && !waterFlowmap->flowmapDetail)
    return;

  G_ASSERT_RETURN((cascade >= 0) && (cascade <= waterFlowmap->cascades.size()), );
  if (cascade == waterFlowmap->cascades.size())
    waterFlowmap->cascades.push_back();
  WaterFlowmapCascade &waterFlowmapCascade = waterFlowmap->cascades[cascade];

  ShaderGlobal::set_real(water_flowmap_debugVarId, debug_water_flowmap.get() ? 1 : 0);

  if (waterFlowmapCascade.frameCount == 0)
  {
    waterFlowmapCascade.frameCount = 1;

    if (cascade == 0)
    {
      init_shader_vars();
      set_flowmap_tex(handle);
      set_flowmap_params(handle);
      set_flowmap_foam_params(handle);

      waterFlowmap->builder.init("water_flowmap");

      waterFlowmap->circularObstacles.reserve(MAX_FLOWMAP_CIRCULAR_OBSTACLES);
      waterFlowmap->rectangularObstacles.reserve(MAX_FLOWMAP_RECTANGULAR_OBSTACLES);
      waterFlowmap->circularObstaclesBuf = dag::create_sbuffer(d3d::buffers::CBUFFER_REGISTER_SIZE,
        dag::buffers::cb_array_reg_count<FlowmapCircularObstacle>(MAX_FLOWMAP_CIRCULAR_OBSTACLES),
        SBCF_BIND_CONSTANT | SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, 0, "water_flowmap_circular_obstacles");
      waterFlowmap->rectangularObstaclesBuf = dag::create_sbuffer(d3d::buffers::CBUFFER_REGISTER_SIZE,
        dag::buffers::cb_array_reg_count<FlowmapRectangularObstacle>(MAX_FLOWMAP_RECTANGULAR_OBSTACLES),
        SBCF_BIND_CONSTANT | SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE, 0, "water_flowmap_rectangular_obstacles");

      waterFlowmap->circularObstaclesRenderer.init("water_flowmap_circular_obstacles");
      waterFlowmap->rectangularObstaclesRenderer.init("water_flowmap_rectangular_obstacles");
    }
    if (cascade == 1)
    {
      waterFlowmap->flowmapBlurX.init("flowmap_blur_x");
      waterFlowmap->flowmapBlurY.init("flowmap_blur_y");
    }
    if (cascade == 2)
    {
      waterFlowmap->fluidSolver.init("water_flowmap_fluid");
    }

    if (cascade >= 1)
    {
      int blurTextureSize = flowmap_texture_size;
      if (cascade == 1)
        blurTextureSize /= 8;

      waterFlowmapCascade.blurTexA.close();
      waterFlowmapCascade.blurTexB.close();
      waterFlowmapCascade.tempTex.close();
      waterFlowmapCascade.blurTexA = dag::create_tex(NULL, blurTextureSize, blurTextureSize,
        TEXCF_RTARGET | TEXFMT_R16F | TEXCF_CLEAR_ON_CREATE, 1, String(128, "water_flowmap_tex_blur_a%d", cascade));
      waterFlowmapCascade.blurTexB = dag::create_tex(NULL, blurTextureSize, blurTextureSize,
        TEXCF_RTARGET | TEXFMT_R16F | TEXCF_CLEAR_ON_CREATE, 1, String(128, "water_flowmap_tex_blur_b%d", cascade));
      waterFlowmapCascade.tempTex = dag::create_tex(NULL, blurTextureSize, flowmap_texture_size,
        TEXCF_RTARGET | TEXFMT_R16F | TEXCF_CLEAR_ON_CREATE, 1, String(128, "water_flowmap_tex_temp_%d", cascade));
      waterFlowmapCascade.blurTexA.getTex2D()->texaddr(TEXADDR_CLAMP);
      waterFlowmapCascade.blurTexB.getTex2D()->texaddr(TEXADDR_CLAMP);
      waterFlowmapCascade.blurTexA.getTex2D()->texfilter(TEXFILTER_LINEAR);
      waterFlowmapCascade.blurTexB.getTex2D()->texfilter(TEXFILTER_LINEAR);
      waterFlowmapCascade.tempTex->disableSampler();
    }

    waterFlowmapCascade.texA.close();
    waterFlowmapCascade.texB.close();
    waterFlowmapCascade.texA = dag::create_tex(NULL, flowmap_texture_size, flowmap_texture_size,
      TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_CLEAR_ON_CREATE, 1, String(128, "water_flowmap_tex_a%d", cascade));
    waterFlowmapCascade.texB = dag::create_tex(NULL, flowmap_texture_size, flowmap_texture_size,
      TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_CLEAR_ON_CREATE, 1, String(128, "water_flowmap_tex_b%d", cascade));
    waterFlowmapCascade.texA->disableSampler();
    waterFlowmapCascade.texB->disableSampler();
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(flowmap_temp_tex_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(water_flowmap_tex_add_0_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(water_flowmap_tex_add_1a_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(water_flowmap_tex_add_1b_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(water_flowmap_tex_add_2a_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(water_flowmap_tex_add_2b_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(tex_samplerstateVarId, sampler);
  }

  if (!waterFlowmapCascade.texA || !waterFlowmapCascade.texB)
    return;

  float range = waterFlowmap->flowmapRange;
  float damping = waterFlowmap->flowmapDamping;
  Point4 depth = waterFlowmap->flowmapDepth;
  float slope = waterFlowmap->flowmapSlope;

  if (cascade != 0)
  {
    // these parameters are better for the detailed cascade
    range = 20;
    damping = 0.99f;
    depth.y = 5;
    depth.z = 1;
    slope = 0.1f;
  }

  ShaderGlobal::set_real(water_flowmap_rangeVarId, range);
  ShaderGlobal::set_real((cascade == 0) ? water_flowmap_range_0VarId : water_flowmap_range_1VarId, range);
  ShaderGlobal::set_real(water_flowmap_dampingVarId, damping);
  ShaderGlobal::set_color4(water_flowmap_depthVarId, depth);
  ShaderGlobal::set_real(water_flowmap_slopeVarId, slope);

  ShaderGlobal::set_int(flowmap_texture_sizeVarId, flowmap_texture_size);
  ShaderGlobal::set_int(height_texture_sizeVarId, heightmap_texture_size);
  ShaderGlobal::set_int(water_flowmap_cascadesVarId, cascade + 1);

  float multiplier =
    (waterFlowmap->flowmapWaveFade.y - get_max_wave(handle)) / (waterFlowmap->flowmapWaveFade.y - waterFlowmap->flowmapWaveFade.x);
  multiplier = clamp(multiplier, 0.0f, 1.0f);
  ShaderGlobal::set_real(water_flowmap_multiplierVarId, multiplier);

  Point4 foamDetail = Point4(0.025f, 0.01f, 10.0f * waterFlowmap->flowmapFoamDetail, 10.0f);
  ShaderGlobal::set_color4(water_flowmap_foam_detailVarId, foamDetail);

  if (cascade != 0)
  {
    float blend = 0;
    float frameTime = waterFlowmapCascade.frameRate * get_shader_global_time();
    bool update = waterFlowmapCascade.frameTime < 0;
    if (update)
    {
      if (waterFlowmapCascade.frameCount == 2) // texA and texB are initialized
        waterFlowmapCascade.frameTime = int(frameTime);
    }
    else
    {
      update = waterFlowmapCascade.frameTime != int(frameTime);
      waterFlowmapCascade.frameTime = int(frameTime);
      blend = frameTime - float(waterFlowmapCascade.frameTime);
    }
    ShaderGlobal::set_real(water_flowmap_blendVarId, blend);
    if (!update)
      return;
  }

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
  ShaderGlobal::set_color4(world_to_flowmap_prevVarId, waterFlowmapCascade.flowmapArea);
  ShaderGlobal::set_color4(world_to_flowmap_addVarId, area);
  if (cascade == 0)
  {
    ShaderGlobal::set_color4(world_to_flowmap_add_0VarId, area);
  }
  else if (cascade == 1)
  {
    Point4 areaA = Point4(waterFlowmapCascade.flowmapArea.x, waterFlowmapCascade.flowmapArea.y, area.x, area.y);
    Point4 areaB = Point4(waterFlowmapCascade.flowmapArea.z, waterFlowmapCascade.flowmapArea.w, area.z, area.w);
    ShaderGlobal::set_color4(world_to_flowmap_add_1aVarId, areaA);
    ShaderGlobal::set_color4(world_to_flowmap_add_1bVarId, areaB);
  }
  else if (cascade == 2)
  {
    Point4 areaA = Point4(waterFlowmapCascade.flowmapArea.x, waterFlowmapCascade.flowmapArea.y, area.x, area.y);
    Point4 areaB = Point4(waterFlowmapCascade.flowmapArea.z, waterFlowmapCascade.flowmapArea.w, area.z, area.w);
    ShaderGlobal::set_color4(world_to_flowmap_add_2aVarId, areaA);
    ShaderGlobal::set_color4(world_to_flowmap_add_2bVarId, areaB);
  }
  waterFlowmapCascade.flowmapArea = area;

  d3d::GpuAutoLock gpuLock;

  d3d::set_render_target();
  int frameId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  int sceneId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  {
    bool evenFrame = waterFlowmapCascade.frameCount <= 2;
    UniqueTex &flowmapSrc = evenFrame ? waterFlowmapCascade.texA : waterFlowmapCascade.texB;
    UniqueTex &flowmapDst = evenFrame ? waterFlowmapCascade.texB : waterFlowmapCascade.texA;

    d3d::set_render_target(flowmapDst.getTex2D(), 0);
    ShaderGlobal::set_texture(flowmap_temp_texVarId, flowmapSrc);
    if (cascade <= 1)
      waterFlowmap->builder.render();
    else
      waterFlowmap->fluidSolver.render();

    int numCircularObstacles = min(int(waterFlowmap->circularObstacles.size()), MAX_FLOWMAP_CIRCULAR_OBSTACLES);
    int numRectangularObstacles = min(int(waterFlowmap->rectangularObstacles.size()), MAX_FLOWMAP_RECTANGULAR_OBSTACLES);

    if (cascade == 0)
    {
      if (numCircularObstacles > 0)
        waterFlowmap->circularObstaclesBuf->updateData(0, numCircularObstacles * sizeof(*waterFlowmap->circularObstacles.data()),
          waterFlowmap->circularObstacles.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);

      if (numRectangularObstacles > 0)
        waterFlowmap->rectangularObstaclesBuf->updateData(0,
          numRectangularObstacles * sizeof(*waterFlowmap->rectangularObstacles.data()), waterFlowmap->rectangularObstacles.data(),
          VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    }

    if (obstacles)
    {
      ShaderGlobal::set_real(water_flowmap_obstacles_addVarId, (cascade <= 1) ? 0 : 1);

      if (numCircularObstacles > 0)
      {
        ShaderElement *shElem = waterFlowmap->circularObstaclesRenderer.getElem();
        if (shElem)
        {
          d3d::setvsrc(0, 0, 0);
          shElem->render(0, 0, RELEM_NO_INDEX_BUFFER, numCircularObstacles * 2, 0, PRIM_TRILIST);
        }
      }

      if (numRectangularObstacles > 0)
      {
        ShaderElement *shElem = waterFlowmap->rectangularObstaclesRenderer.getElem();
        if (shElem)
        {
          d3d::setvsrc(0, 0, 0);
          shElem->render(0, 0, RELEM_NO_INDEX_BUFFER, numRectangularObstacles * 2, 0, PRIM_TRILIST);
        }
      }
    }

    ShaderGlobal::set_texture(flowmap_temp_texVarId, BAD_TEXTUREID);

    if (cascade == 0)
    {
      ShaderGlobal::set_texture(water_flowmap_tex_add_0VarId, flowmapDst);
      d3d::resource_barrier({flowmapDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    }
    else if (cascade == 1)
    {
      ShaderGlobal::set_texture(water_flowmap_tex_add_1aVarId, flowmapSrc);
      ShaderGlobal::set_texture(water_flowmap_tex_add_1bVarId, flowmapDst);
      d3d::resource_barrier({flowmapSrc.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
      d3d::resource_barrier({flowmapDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    }
    else if (cascade == 2)
    {
      ShaderGlobal::set_texture(water_flowmap_tex_add_2aVarId, flowmapSrc);
      ShaderGlobal::set_texture(water_flowmap_tex_add_2bVarId, flowmapDst);
      d3d::resource_barrier({flowmapSrc.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
      d3d::resource_barrier({flowmapDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    }

    if (cascade >= 1)
    {
      TIME_D3D_PROFILE(blur_texture)

      UniqueTex &blurSrc = evenFrame ? waterFlowmapCascade.blurTexA : waterFlowmapCascade.blurTexB;
      UniqueTex &blurDst = evenFrame ? waterFlowmapCascade.blurTexB : waterFlowmapCascade.blurTexA;

      ShaderGlobal::set_color4(texszVarId, Color4(0.5f, -0.5f, 1.0f / flowmap_texture_size, 1.0f / flowmap_texture_size));

      d3d::set_render_target(waterFlowmapCascade.tempTex.getTex2D(), 0);
      ShaderGlobal::set_texture(texVarId, flowmapDst);
      waterFlowmap->flowmapBlurX.render();

      d3d::set_render_target(blurDst.getTex2D(), 0);
      ShaderGlobal::set_texture(texVarId, waterFlowmapCascade.tempTex);
      waterFlowmap->flowmapBlurY.render();

      if (cascade == 1)
      {
        ShaderGlobal::set_texture(water_flowmap_tex_blur_1aVarId, blurSrc);
        ShaderGlobal::set_texture(water_flowmap_tex_blur_1bVarId, blurDst);
        d3d::resource_barrier({blurSrc.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
        d3d::resource_barrier({blurDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
      }
      else if (cascade == 2)
      {
        ShaderGlobal::set_texture(water_flowmap_tex_blur_2aVarId, blurSrc);
        ShaderGlobal::set_texture(water_flowmap_tex_blur_2bVarId, blurDst);
        d3d::resource_barrier({blurSrc.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
        d3d::resource_barrier({blurDst.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
      }
    }
  }

  if (frameId >= 0)
    ShaderGlobal::setBlock(frameId, ShaderGlobal::LAYER_FRAME);
  if (sceneId >= 0)
    ShaderGlobal::setBlock(sceneId, ShaderGlobal::LAYER_SCENE);

  if (waterFlowmapCascade.frameCount < 2)
    waterFlowmapCascade.frameCount = 2;
  waterFlowmapCascade.frameCount = 5 - waterFlowmapCascade.frameCount;
}

void set_flowmap_tex(FFTWater *handle)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap || waterFlowmap->cascades.empty())
    return;

  waterFlowmap->tex.close();
  if (waterFlowmap->enabled)
  {
    if (!waterFlowmap->texName.empty())
      waterFlowmap->tex = dag::get_tex_gameres(waterFlowmap->texName.c_str(), "water_flowmap_tex");
    else
      waterFlowmap->tex = dag::create_tex(NULL, 1, 1, TEXFMT_A8R8G8B8 | TEXCF_CLEAR_ON_CREATE, 1, "water_flowmap_tex");
    ShaderGlobal::set_sampler(water_flowmap_tex_samplerstateVarId, d3d::request_sampler({}));

    ShaderGlobal::set_color4(world_to_flowmapVarId, waterFlowmap->texArea);
  }
}

void set_flowmap_params(FFTWater *handle)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap || waterFlowmap->cascades.empty())
    return;

  Point4 flowmapStrength = waterFlowmap->flowmapStrength;
  if (waterFlowmap->texName.empty())
  {
    flowmapStrength.x = 0;
    flowmapStrength.y = 0;
  }
  if (!waterFlowmap->usingFoamFx)
    flowmapStrength.w = 0;

  ShaderGlobal::set_real(water_wind_strengthVarId, waterFlowmap->windStrength);
  ShaderGlobal::set_real(water_flowmap_fadingVarId, waterFlowmap->flowmapFading);
  ShaderGlobal::set_color4(water_flowmap_strengthVarId, flowmapStrength);
  ShaderGlobal::set_color4(water_flowmap_strength_addVarId, waterFlowmap->flowmapStrengthAdd);
}

void set_flowmap_foam_params(FFTWater *handle)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap || waterFlowmap->cascades.empty())
    return;

  ShaderGlobal::set_color4(water_flowmap_foamVarId, waterFlowmap->flowmapFoam);
  ShaderGlobal::set_real(water_flowmap_foam_reflectivity_minVarId, waterFlowmap->flowmapFoamReflectivityMin);
  ShaderGlobal::set_color4(water_flowmap_foam_colorVarId, waterFlowmap->flowmapFoamColor);
  ShaderGlobal::set_real(water_flowmap_foam_tilingVarId, waterFlowmap->flowmapFoamTiling);
  ShaderGlobal::set_real(water_flowmap_foam_displacementVarId, waterFlowmap->flowmapFoamDisplacement);
}

void close_flowmap(FFTWater *handle)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap)
    return;

  waterFlowmap->tex.close();

  waterFlowmap->circularObstacles.clear();
  waterFlowmap->rectangularObstacles.clear();
  waterFlowmap->circularObstaclesBuf.close();
  waterFlowmap->rectangularObstaclesBuf.close();

  for (int cascade = 0; cascade < waterFlowmap->cascades.size(); cascade++)
  {
    WaterFlowmapCascade &waterFlowmapCascade = waterFlowmap->cascades[cascade];
    waterFlowmapCascade.texA.close();
    waterFlowmapCascade.texB.close();
    waterFlowmapCascade.blurTexA.close();
    waterFlowmapCascade.blurTexB.close();
    waterFlowmapCascade.tempTex.close();
  }
  waterFlowmap->cascades.clear();
}

bool is_flowmap_active(FFTWater *handle)
{
  WaterFlowmap *waterFlowmap = get_flowmap(handle);
  if (!waterFlowmap)
    return false;

  return waterFlowmap->tex || !waterFlowmap->cascades.empty();
}

enum FloodType
{
  WATER = 0,
  QUEUED = 1,
  GROUND = 2
};

void flowmap_floodfill(int texSize, const LockedImage2DView<const uint16_t> &heightmapTexView,
  LockedImage2DView<uint16_t> &floodfillTexView, uint16_t heightmapLevel)
{
  TIME_D3D_PROFILE(flowmap_floodfill);

  if (heightmapTexView && floodfillTexView)
  {
    int queueSize = (texSize + 1) * 2;
    int queueBegin = 0;
    int queueEnd = 0;
    Tab<int> queue(framemem_ptr());
    queue.resize(queueSize);

    for (int y = 0; y < texSize; y++)
    {
      for (int x = 0; x < texSize; x++)
      {
        if (
          (heightmapTexView[IPoint2(x, y)] < heightmapLevel) ||
          ((x - 1 >= 0) && (heightmapTexView[IPoint2(x - 1, y)] < heightmapLevel) && (heightmapTexView[IPoint2(x - 1, y)] > 0)) ||
          ((x + 1 < texSize) && (heightmapTexView[IPoint2(x + 1, y)] < heightmapLevel) && (heightmapTexView[IPoint2(x + 1, y)] > 0)) ||
          ((y - 1 >= 0) && (heightmapTexView[IPoint2(x, y - 1)] < heightmapLevel) && (heightmapTexView[IPoint2(x, y - 1)] > 0)) ||
          ((y + 1 < texSize) && (heightmapTexView[IPoint2(x, y + 1)] < heightmapLevel) && (heightmapTexView[IPoint2(x, y + 1)] > 0)))
          floodfillTexView[IPoint2(x, y)] = FloodType::WATER;
        else
          floodfillTexView[IPoint2(x, y)] = FloodType::GROUND;
      }
    }

    for (int y = 0; y < texSize; y++)
    {
      for (int x = 0; x < texSize; x++)
      {
        if (floodfillTexView[IPoint2(x, y)] != FloodType::WATER)
          continue;

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

          int fx = 0;
          int fy = 0;

          int nx = 0;
          int ny = 0;

          if (u - 1 >= 0)
          {
            uint16_t &flood = floodfillTexView[IPoint2(u - 1, v)];
            if (flood == FloodType::WATER)
            {
              flood = FloodType::QUEUED;
              queue[queueEnd++] = u - 1;
              queue[queueEnd++] = v;
              if (queueEnd >= queueSize)
                queueEnd = 0;
            }
            else if (flood > FloodType::GROUND)
            {
              fx++;
              nx += ((flood >> 0) & 0xff) - 0x80;
              ny += ((flood >> 8) & 0xff) - 0x80;
            }
          }
          if (u + 1 < texSize)
          {
            uint16_t &flood = floodfillTexView[IPoint2(u + 1, v)];
            if (flood == FloodType::WATER)
            {
              flood = FloodType::QUEUED;
              queue[queueEnd++] = u + 1;
              queue[queueEnd++] = v;
              if (queueEnd >= queueSize)
                queueEnd = 0;
            }
            else if (flood > FloodType::GROUND)
            {
              fx--;
              nx += ((flood >> 0) & 0xff) - 0x80;
              ny += ((flood >> 8) & 0xff) - 0x80;
            }
          }
          if (v - 1 >= 0)
          {
            uint16_t &flood = floodfillTexView[IPoint2(u, v - 1)];
            if (flood == FloodType::WATER)
            {
              flood = FloodType::QUEUED;
              queue[queueEnd++] = u;
              queue[queueEnd++] = v - 1;
              if (queueEnd >= queueSize)
                queueEnd = 0;
            }
            else if (flood > FloodType::GROUND)
            {
              fy++;
              nx += ((flood >> 0) & 0xff) - 0x80;
              ny += ((flood >> 8) & 0xff) - 0x80;
            }
          }
          if (v + 1 < texSize)
          {
            uint16_t &flood = floodfillTexView[IPoint2(u, v + 1)];
            if (flood == FloodType::WATER)
            {
              flood = FloodType::QUEUED;
              queue[queueEnd++] = u;
              queue[queueEnd++] = v + 1;
              if (queueEnd >= queueSize)
                queueEnd = 0;
            }
            else if (flood > FloodType::GROUND)
            {
              fy--;
              nx += ((flood >> 0) & 0xff) - 0x80;
              ny += ((flood >> 8) & 0xff) - 0x80;
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

          floodfillTexView[IPoint2(u, v)] = uint16_t(((fy + 0x80) << 8) | (fx + 0x80));
        }
      }
    }

    for (int y = 0; y < texSize; y++)
    {
      for (int x = 0; x < texSize; x++)
      {
        uint16_t &flood = floodfillTexView[IPoint2(x, y)];
        if (flood <= FloodType::GROUND)
          flood = 0x8080;
      }
    }
  }
}

} // namespace fft_water
