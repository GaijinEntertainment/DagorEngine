// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/lodGrid.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <FFT_CPU_Simulation.h>
#include <util/dag_string.h>
#include <math/dag_frustum.h>
#include <math/dag_half.h>
#include <shaders/dag_shaders.h>
#include "shaders/dag_computeShaders.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathUtils.h>
#include <util/dag_convar.h>
#include <memory/dag_framemem.h>
#include <3d/dag_render.h>
#include <3d/dag_lockTexture.h>
#include <waterDecals/waterDecalsRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/utility.h>
#include <3d/dag_lockSbuffer.h>
#include "chopWaterRender.h"

static const char *water_hmap_tess_factor_name = "water_tess_factor";
static const char *chop_water_shader_name = "water_chop";

static int waterChopBlockId = -1;

G_STATIC_ASSERT(fft_water::RENDER_VERY_LOW == 1 && fft_water::RENDER_GOOD == 3);
static const carray<float, fft_water::RENDER_GOOD> water_cells_size = {2.0f, 1.0f, 0.5f};
static const carray<int, fft_water::RENDER_GOOD> water_dim_bits = {4, 5, 6}; // clamped to 6 in HeightmapRenderer

static const carray<carray<eastl::pair<float, int>, 7>, fft_water::RENDER_GOOD> WAVE_HEIGHT_TO_LODS = {
  {{{{0.00f, 1}, {0.1f, 3}, {0.2f, 4}, {0.6f, 4}, {1.90f, 5}, {2.5f, 5}, {3.0f, 5}}},
    {{{0.00f, 1}, {0.1f, 4}, {0.2f, 5}, {0.6f, 5}, {1.60f, 5}, {2.5f, 6}, {3.0f, 6}}},
    {{{0.00f, 3}, {0.1f, 5}, {0.2f, 6}, {1.2f, 6}, {1.60f, 7}, {2.5f, 7}, {3.0f, 7}}}}};

#define GLOBAL_VARS_LIST      \
  VAR(chop_fov)               \
  VAR(water_origin)           \
  VAR(water_heightmap_region) \
  VAR(water_vertical_lod)     \
  VAR(tess_distance)          \
  VAR(chop_output_normal)     \
  VAR(chop_detail_waves_enabled)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

void ChopWaterRender::reset() { del_it(renderQuad); }

void ChopWaterRender::reinit()
{
  calcWaveHeight(maxWaveHeight, significantWaveHeight);
  setLevel(waterLevel);
}

ChopWaterRender::ChopWaterRender(ChopWaterGenerator &chop_gen, int render_quality, int geom_quality, bool depth_renderer,
  bool ssr_renderer, const fft_water::WaterHeightmap *water_heightmap) :
  chopGen(chop_gen),
  renderQuality(render_quality),
  lod0AreaRadius(0.f),
  lod0TesselationAdditional(0),
  depthRendererEnabled(depth_renderer),
  ssrRendererEnabled(ssr_renderer),
  lastLodExtension(80000.0f),
  forceTessellation(false),
  maxWaveHeight(0.0f),
  significantWaveHeight(0.0f),
  renderQuad(NULL)
{
  waterHeightmap = water_heightmap;
  geomQuality = max(geom_quality, 0);
  memset(maxWaveSize, 0, sizeof(maxWaveSize));
  setLevel(0);
  numCascades = WATER_VISUAL_CASCADES_COUNT;
  G_STATIC_ASSERT(MAX_NUM_CASCADES <= 5);
  reset();

  G_ASSERT(geomQuality >= fft_water::RENDER_VERY_LOW && geomQuality <= fft_water::RENDER_GOOD);

  waterCellSize = water_cells_size[geomQuality - 1];

  reinit();

  if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_VERTEXTEXTURE))
  {
    int rendererEnabled = 1 << fft_water::RenderMode::WATER_SHADER;
    if (depthRendererEnabled)
      rendererEnabled |= 1 << fft_water::RenderMode::WATER_DEPTH_SHADER;
    if (ssrRendererEnabled)
      rendererEnabled |= 1 << fft_water::RenderMode::WATER_SSR_SHADER;
    for (int i = fft_water::RenderMode::WATER_SHADER; i < fft_water::RenderMode::MAX; i++)
    {
      if (rendererEnabled & (1 << i))
      {
        const bool isDrawSSR = i == fft_water::RenderMode::WATER_SSR_SHADER;
        if (!chopWaterRenderer[i].init(chop_water_shader_name, NULL, water_hmap_tess_factor_name, isDrawSSR,
              water_dim_bits[geomQuality - 1]))
        {
          return;
        }
      }
    }
  }
  else if (!d3d::device_lost(nullptr))
  {
    DAG_FATAL("fft_water: no vfetch!");
  }

  waterChopBlockId = ShaderGlobal::getBlockId("water_chop_block");
}

bool ChopWaterRender::isDetailWavesEnabled() const { return renderQuality >= fft_water::RENDER_EXCELLENT; }

void ChopWaterRender::setLevel(float water_level)
{
  waterLevel = water_level;
  minWaterLevel = waterLevel;
  maxWaterLevel = waterLevel;
  if (waterHeightmap)
  {
    minWaterLevel = min(minWaterLevel, waterHeightmap->heightOffset);
    maxWaterLevel = max(maxWaterLevel, waterHeightmap->heightMax);
  }
  minWaterLevel -= maxWaveHeight;
  maxWaterLevel += maxWaveHeight;
}

void ChopWaterRender::setMinMaxLevel(float min_water_level, float max_water_level)
{
  minWaterLevel = min_water_level - maxWaveHeight;
  maxWaterLevel = max_water_level + maxWaveHeight;
}

void ChopWaterRender::calcWaveHeight(float &out_max_wave_height, float &out_significant_wave_height)
{
  calc_wave_height_chop(chopGen, numCascades, out_significant_wave_height, out_max_wave_height, maxWaveSize);
}

ChopWaterRender::~ChopWaterRender()
{
  reset();
  numCascades = 0;
}

void ChopWaterRender::setWaterDim(int dim_bits)
{
  for (int i = 0; i < fft_water::RenderMode::MAX; i++)
  {
    if (chopWaterRenderer[i].isInited() && chopWaterRenderer[i].getDim() != 1 << dim_bits)
    {
      chopWaterRenderer[i].close();
      chopWaterRenderer[i].init(chop_water_shader_name, NULL, water_hmap_tess_factor_name, true, dim_bits);
    }
  }
}

void ChopWaterRender::render(const Point3 &origin, TEXTUREID distanceTex, int geom_lod_quality, int survey_id, const Frustum &frustum,
  const Driver3dPerspective &persp, const WaterRenderCommon &waterRenderCommon, IWaterDecalsRenderHelper *decals_renderer,
  fft_water::RenderMode render_mode)
{
  HeightmapRenderer *renderer = nullptr;
  if (render_mode < fft_water::RenderMode::MAX)
  {
    renderer = &chopWaterRenderer[render_mode];
    if (!chopWaterRenderer[render_mode].isInited())
    {
      const bool isDrawSSR = render_mode == fft_water::RenderMode::WATER_SSR_SHADER;
      chopWaterRenderer[render_mode].init(chop_water_shader_name, NULL, water_hmap_tess_factor_name, isDrawSSR,
        water_dim_bits[geomQuality - 1]);
    }
  }
  else
    logerr("Non-existent water rendering mode %d", render_mode);

  float waterHeight = waterLevel;
  if (waterHeightmap)
    waterHeightmap->getHeightmapDataBilinear(origin.x, origin.z, waterHeight);
  else if (geom_lod_quality == fft_water::GEOM_HIGH)
    waterHeight = maxWaterLevel;
  float originAlt = origin.y - waterHeight;

  ShaderGlobal::set_float(tess_distanceVarId, lod0AreaRadius * 0.66f);
  ShaderGlobal::set_int(chop_output_normalVarId, render_mode == fft_water::RenderMode::WATER_DEPTH_SHADER ? 1 : 0);

  float chopFov = 2.0f * atanf(1.0f / persp.hk);
  ShaderGlobal::set_float(chop_fovVarId, chopFov);

  ShaderGlobal::set_int(chop_detail_waves_enabledVarId, isDetailWavesEnabled() ? 1 : 0);

  dag::ConstSpan<eastl::pair<float, int>> waveLods = WAVE_HEIGHT_TO_LODS[geomQuality - 1];
  int lodCount = waveLods[0].second;
  for (int itemNo = 1; itemNo < waveLods.size(); ++itemNo)
  {
    if (significantWaveHeight > waveLods[itemNo].first)
      lodCount = waveLods[itemNo].second;
    else
      break;
  }

  if (forceTessellation || waterHeightmap || (geom_lod_quality == fft_water::GEOM_HIGH))
    lodCount = (int)waveLods.size() - 1;

  const int lod0Rad = 1;
  const int lastLodRad = 1;
  const float maxLodWithHeightmap = 3.0f;
  float nextLod = originAlt * 0.015f;
  if (waterHeightmap)
    nextLod = min(nextLod, maxLodWithHeightmap);
  if (geom_lod_quality == fft_water::GEOM_INVISIBLE && !renderQuad)
    nextLod = lodCount;
  else if (geom_lod_quality == fft_water::GEOM_HIGH)
    nextLod = 0;
  int lod = (int)floorf(max(nextLod, 0.f));
  if (lodCount <= lod)
  {
    nextLod = lod = max(max<int>(log2f(max(originAlt, 1.0f)) - waterCellSize * 2.0f, lodCount), 4);
    lodCount = 1;
  }
  else
    lodCount = max(lodCount - lod, 0);
  float scaledCell = waterCellSize * (1 << min(11, lod));

  Point2 centerOfHmap = Point2::xz(origin);

  int gridDim = chopWaterRenderer[fft_water::RenderMode::WATER_SHADER].getDim();

  if (renderQuad)
  {
    if (!frustum.testBoxB(BBox3(Point3(renderQuad->lim[0].x, minWaterLevel, renderQuad->lim[0].y),
          Point3(renderQuad->lim[1].x, maxWaterLevel, renderQuad->lim[1].y))))
      return;
    centerOfHmap.x = clamp(centerOfHmap.x, renderQuad->lim[0].x, renderQuad->lim[1].x);
    centerOfHmap.y = clamp(centerOfHmap.y, renderQuad->lim[0].y, renderQuad->lim[1].y);
    float maxSize = min(renderQuad->lim[1].x - renderQuad->lim[0].x, renderQuad->lim[1].y - renderQuad->lim[0].y);
    maxSize *= 0.5f / gridDim;
    while (scaledCell > maxSize && lod > 0)
    {
      scaledCell *= 0.5;
      lod--;
    }
  }

  const float lod0_tesselation_additional = nextLod < 1.0f ? lod0TesselationAdditional : 0.0f;
  const int lod0TessFactor = eastl::max(lod0_tesselation_additional, 0.0f);
  float alignSize = -1;

  LodGrid lodGrid;
  lodGrid.init(lodCount, lod0Rad, lod0TessFactor, lastLodRad, lastLodExtension);
  LodGridCullData defaultCullData(framemem_ptr());
  int hmap_tess_factorVarId = renderer ? renderer->get_hmap_tess_factorVarId() : -1;
  BBox2 lodsRegion;
  cull_lod_grid(lodGrid, lodGrid.lodsCount, centerOfHmap.x, centerOfHmap.y, scaledCell, scaledCell, alignSize, alignSize, // alignment
    minWaterLevel, maxWaterLevel, &frustum, renderQuad, defaultCullData, NULL, lod0AreaRadius, hmap_tess_factorVarId, gridDim,
    false /*not used*/, nullptr, &lodsRegion);
  if (!defaultCullData.getCount())
    return;

  LodGridPatchParams cameraPatch(1, 0, 0, 0);
  for (auto &patch : defaultCullData.patches)
  {
    Point2 lt(patch.originX, patch.originY);
    float size = gridDim * patch.size;
    Point2 rb = lt + Point2(size, size);
    if (lt.x <= origin.x && rb.x >= origin.x && lt.y <= origin.z && rb.y >= origin.z)
    {
      cameraPatch = patch;
      break;
    }
  }
  cameraPatchOffset = Point2(cameraPatch.originX, cameraPatch.originY);
  cameraPatchAlign = cameraPatch.size;

  BBox2 heightmapRegion;
  if (geom_lod_quality == fft_water::GEOM_HIGH)
  {
    for (auto &patch : defaultCullData.patches)
    {
      float size = gridDim * patch.size;
      float border = 5.0f * 1.6f * patch.size; // aligned to LAST_LOD_HEIGHTMAP_BORDER with some overlap
      Point2 lt(patch.originX, patch.originY);
      Point2 rb = lt + Point2(size, size);
      heightmapRegion += lt + Point2(border, border);
      heightmapRegion += rb - Point2(border, border);
    }
  }

  ShaderGlobal::set_float4(water_originVarId, Color4(origin.x, origin.y, origin.z));
  ShaderGlobal::set_float4(water_heightmap_regionVarId,
    Color4(heightmapRegion.left(), heightmapRegion.top(), heightmapRegion.right(), heightmapRegion.bottom()));

  float lod0RadiusMeters = lod0AreaRadius / (1 << lod0TessFactor);
  float lodVertical = clamp((nextLod - lod) * 2 - 1, 0.0f, 1.0f);
  float lod0ExVertical = max(1.0f - lod0_tesselation_additional, lodVertical);
  ShaderGlobal::set_float4(water_vertical_lodVarId,
    Color4(lodVertical, lod0ExVertical, (lod0RadiusMeters * 0.5f) > lastLodExtension ? -1.0f : lodCount - 1,
      max(lastLodExtension - lod0RadiusMeters * 0.5f, 0.0f)));

  const WaterRenderCommon::SavedStates states = waterRenderCommon.setCachedStates(distanceTex);
  int prevGlobalBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(waterChopBlockId, ShaderGlobal::LAYER_FRAME);

  if (renderer)
    renderer->render(lodGrid, defaultCullData);

  if (decals_renderer != NULL)
    decals_renderer->render();

  waterRenderCommon.resetCachedStates(states);
  ShaderGlobal::setBlock(prevGlobalBlockId, ShaderGlobal::LAYER_FRAME);
}

void ChopWaterRender::setRenderQuad(const BBox2 &b)
{
  if (b.isempty())
  {
    del_it(renderQuad);
    return;
  }
  if (!renderQuad)
    renderQuad = new BBox2;
  *renderQuad = b;
}
