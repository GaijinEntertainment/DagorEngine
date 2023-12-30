#include <heightmap/lodGrid.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <FFT_CPU_Simulation.h>
#include <util/dag_string.h>
#include <math/dag_frustum.h>
#include <math/dag_half.h>
#include <shaders/dag_shaders.h>
#include "shaders/dag_computeShaders.h"
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
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
#include "waterRender.h"
#include "waterGPGPUData.h"
#include "waterCSGPUData.h"
#include "shaders/water_gradient_cs_const.hlsli"

#define MAX_LIGHT_SCATTER_DIST 900.f
#define LIGHT_SCATTER_DIST_MUL 80.f

static const char *water_shader_name[fft_water::RenderMode::MAX] = {"water_nv2", "water_depth_nv2", "water_ssr_nv2"};
static const char *water_heightmap_shader_name[fft_water::RenderMode::MAX] = {
  "water_nv2_heightmap", "water_depth_nv2_heightmap", "water_ssr_nv2_heightmap"};

static int water_gradients_texVarId[WaterNVRender::MAX_NUM_CASCADES] = {-1, -1, -1, -1, -1};
static int water_displacement_textureVarId[WaterNVRender::MAX_NUM_CASCADES] = {-1, -1, -1, -1, -1};
static int water_foam_textureVarId[WaterNVRender::MAX_NUM_CASCADES] = {-1, -1, -1, -1, -1};
static int water_gradient_scalesVarId[WaterNVRender::MAX_NUM_CASCADES] = {-1, -1, -1, -1, -1};
static int foam_dissipationVarId = -1;
static int foam_blur_extents0123VarId = -1;
static int foam_blur_extents4567VarId = -1;
static int foam_hats_scaleVarId = -1;
static int surface_folding_foam_paramsVarId = -1;
static int water_foam_passVarId = -1;
static int waterRefractionTexVarId = -1;
static int water_vs_cascadesVarId = -1;
static int water_cascadesVarId = -1;
static int water_levelVarId = -1;
static int shoreDistanceFieldTexVarId = -1;
static int wakeHtTexVarId = -1;
// fft
static int current_water_timeVarId = -1;
static int choppy_scaleVarId = -1;
static int fft_sizeVarId = -1;
static int k_texVarId = -1;
static int omega_texVarId = -1;
static int fft_source_texture_no = -1;
static int butterfly_texVarId = -1;
// update h0
static int gauss_texVarId = -1;
static int wind_dir_xVarId = -1, wind_dir_yVarId = -1;
static int wind_speedVarId = -1;

static bool use_texture_array = true;

int water_vertex_samplers = 4;
G_STATIC_ASSERT(fft_water::RENDER_VERY_LOW == 1 && fft_water::RENDER_GOOD == 3);
static const carray<float, fft_water::RENDER_GOOD> water_cells_size = {2.f, 1.f, 0.5f};
static const carray<int, fft_water::RENDER_GOOD> water_dim_bits = {4, 5, 6};
G_STATIC_ASSERT(fft_water::MAX_NUM_CASCADES >= 4);

static const carray<carray<eastl::pair<float, int>, 7>, fft_water::RENDER_GOOD> WAVE_HEIGHT_TO_LODS = {
  {{{{0.00f, 0}, {0.19f, 1}, {0.32f, 2}, {0.72f, 3}, {1.90f, 4}, {3.20f, 4}, {5.20f, 5}}},
    {{{0.00f, 1}, {0.19f, 1}, {0.32f, 2}, {0.48f, 4}, {1.60f, 5}, {3.20f, 5}, {5.20f, 6}}},
    {{{0.00f, 1}, {0.19f, 1}, {0.32f, 3}, {0.48f, 5}, {1.60f, 6}, {3.20f, 6}, {5.20f, 7}}}}};

static int water_gradient_cs_color_uav_slot = -1;
static int water_foam_blur_cs_color_uav_slot = -1;
static int water_gradient_mip_cs_color_uav_slot = -1;

static GPUFENCEHANDLE async_compute_gradients_fence = BAD_GPUFENCEHANDLE;

struct Ht0Vertex
{
  Point2 pos;
  Point2 tc;
  Point4 fft_tc;
  Point4 gaussTc__norm;
  Point4 windows;
};

void WaterNVRender::reset()
{
  del_it(renderQuad);
  currentSimulatedTime = 0;
  clearNeeded = true;
  wakeHtTex = BAD_TEXTUREID;
  gpGpu = NULL;
  csGpu = NULL;
  gradientCs.reset();
  gradientFoamCs.reset();
  gradientFoamCombineCs.reset();
  gradientMipCs.reset();
  computeGradientsEnabled = false;
  lastFoamTime = 0;
}

bool use_cs_water = true;

#define DEBUG_CS_WATER 0
#if DEBUG_CS_WATER
#define USE_CS_WATER use_cs_water
#else
#define USE_CS_WATER true
#endif


void WaterNVRender::initFoam()
{
  closeFoam();
  if (!numCascades)
    return;
  G_ASSERT(numCascades <= 5);
  const int N = 1 << fft[0].getParams().fft_resolution_bits;

  for (int cascadeNo = 1; cascadeNo < numCascades; ++cascadeNo)
  {
    G_ASSERT(N == (1 << fft[cascadeNo].getParams().fft_resolution_bits));
  }
  uint32_t texFmt = TEXFMT_A16B16G16R16F; // todo: could be TEXFMT_A8R8G8B8 is enough
  uint32_t texUsage = computeGradientsEnabled ? TEXCF_UNORDERED : TEXCF_RTARGET;
  foamGradient = dag::create_array_tex(N, N, numCascades > 4 ? 2 : 1, texUsage | texFmt, 1, "water3d_foam_temp");
  gradientFoamRenderer.init("water_foam_blur");
}

void WaterNVRender::closeFoam() { foamGradient.close(); }

void WaterNVRender::reinit(const Point2 &wind_dir, float wind_speed, float period)
{
  NVWaveWorks_FFT_CPU_Simulation::Params params = fft[0].getParams();
  params.wind_dir_x = wind_dir.x;
  params.wind_dir_y = wind_dir.y;
  params.wind_speed = wind_speed;
  params.fft_period = period;
  setCascades(params);

  if (csGpu)
    csGpu->driverReset(fft, numFftCascades);
  if (gpGpu)
    gpGpu->updateHt0WindowsVB(fft, numFftCascades);

  setLevel(waterLevel);

  cascadesRoughnessInvalid = true;
}

void WaterNVRender::closeGPU()
{
  del_it(gpGpu);
  del_it(csGpu);

  gradientCs.reset();
  gradientFoamCs.reset();
  gradientFoamCombineCs.reset();
  gradientMipCs.reset();
  computeGradientsEnabled = false;

  shaders::overrides::destroy(overrideAlpha);
  shaders::overrides::destroy(overrideRGB);
}

void WaterNVRender::initDisplacementCPU()
{
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    const int N = 1 << fft[0].getParams().fft_resolution_bits;
    String texName(128, "water3d_disp%d", cascadeNo);
    dispCPU[cascadeNo] = dag::create_tex(NULL, N, N, TEXCF_DYNAMIC | TEXFMT_A16B16G16R16F, 1, texName.str());
  }
}

void WaterNVRender::closeDisplacementCPU()
{
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    dispCPU[cascadeNo].close();
  }
}

bool WaterNVRender::initGPU()
{
  if (!numCascades)
    return true;

  csGpu = new CSGPUData;
  if (!csGpu->init(fft, numFftCascades))
  {
    del_it(csGpu);
  }
  else
  {
    debug("fftwater: use compute shader fft");

    gradientCs.reset(new_compute_shader("water_gradient_cs", true));
    gradientFoamCs.reset(new_compute_shader("water_foam_blur_cs", true));
    gradientFoamCombineCs.reset(new_compute_shader("water_gradient_foam_combine_cs", true));
    gradientMipCs.reset(new_compute_shader("water_gradient_mip_cs", true));

    computeGradientsEnabled = gradientCs && gradientFoamCs && gradientFoamCombineCs && gradientMipCs;

    computeGradientsEnabled =
      computeGradientsEnabled && ShaderGlobal::get_int_by_name("water_gradient_cs_color_uav_slot", water_gradient_cs_color_uav_slot);
    computeGradientsEnabled =
      computeGradientsEnabled && ShaderGlobal::get_int_by_name("water_foam_blur_cs_color_uav_slot", water_foam_blur_cs_color_uav_slot);
    computeGradientsEnabled = computeGradientsEnabled && ShaderGlobal::get_int_by_name("water_gradient_mip_cs_color_uav_slot",
                                                           water_gradient_mip_cs_color_uav_slot);

    debug("fftwater: compute gradients enabled: %d", computeGradientsEnabled);
  }

  if (!csGpu || DEBUG_CS_WATER)
  {
    gpGpu = new GPGPUData;
    if (!gpGpu->init(fft, numFftCascades))
    {
      del_it(gpGpu);
      return false;
    }
    debug("fftwater: use pixel shader fft");
  }

  return true;
}

bool water_debug = false;

void WaterNVRender::performGPUFFT()
{
  if (csGpu)
    csGpu->perform(fft, numFftCascades, currentSimulatedTime);
  if (gpGpu)
    gpGpu->perform(fft, numFftCascades, currentSimulatedTime);
  water_debug = false;
}

void WaterNVRender::setWakeHtTex(TEXTUREID wake_ht_tex) { wakeHtTex = wake_ht_tex; }

void WaterNVRender::setCascades(const NVWaveWorks_FFT_CPU_Simulation::Params &p)
{
  cascadesRoughnessInvalid = true;

  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    set_cascade_params(fft[cascadeNo],
      simulation_cascade_params(p, fftSimulationParams, cascadeNo, numCascades, cascadeWindowLength, cascadeFacetSize), true);
  }

  calcWaveHeight(maxWaveHeight, significantWaveHeight);
  applyShoreEnabled();

  applyWaterCell();

  float fft0Period = fft[0].getParams().fft_period;
  int fft_resolution = 1 << p.fft_resolution_bits;

  Color4 UVScaleCascade0123(1, 1, 1, 1), cascadesTexelScale0123(1, 1, 1, 1), cascadesTexelScale4567(1, 1, 1, 1),
    UVScaleCascade4567(1, 1, 1, 1);
  for (int i = 0; i < numCascades; ++i)
  {
    float &UVScaleCascade = (i < 4) ? UVScaleCascade0123[i] : UVScaleCascade4567[i - 4];
    UVScaleCascade = 1.0f / fft[i].getParams().fft_period;
    float &cascadesTexelScale = (i < 4) ? cascadesTexelScale0123[i] : cascadesTexelScale4567[i - 4];
    cascadesTexelScale = fft[0].getParams().fft_period / fft[i].getParams().fft_period;
  }

  static int fft_spectrumVarId = get_shader_variable_id("fft_spectrum");
  ShaderGlobal::set_int(fft_spectrumVarId, (int)fftSimulationParams.spectrum);

  cascadesTexelScale0123[0] = fft0Period / fft_resolution;
  static int cascadesTexelScale0123VarId = get_shader_variable_id("cascadesTexelScale0123");
  static int cascadesTexelScale4567VarId = get_shader_variable_id("cascadesTexelScale4567");
  static int UVScaleCascade0123VarId = get_shader_variable_id("UVScaleCascade0123");
  static int UVScaleCascade4567VarId = get_shader_variable_id("UVScaleCascade4567", true);
  ShaderGlobal::set_color4(cascadesTexelScale0123VarId, cascadesTexelScale0123);
  ShaderGlobal::set_color4(cascadesTexelScale4567VarId, cascadesTexelScale4567);
  ShaderGlobal::set_color4(UVScaleCascade0123VarId, UVScaleCascade0123);
  ShaderGlobal::set_color4(UVScaleCascade4567VarId, UVScaleCascade4567);

  ShaderGlobal::set_real(wind_dir_xVarId, fft[0].getParams().wind_dir_x);
  ShaderGlobal::set_real(wind_dir_yVarId, fft[0].getParams().wind_dir_y);
  ShaderGlobal::set_real(wind_speedVarId, fft[0].getParams().wind_speed);

  static int shoreDampVarId = get_shader_variable_id("shore_damp", true);
  ShaderGlobal::set_color4(shoreDampVarId,
    Color4(min(shoreDamp.x, significantWaveHeight), max(min(shoreDamp.y, significantWaveHeight * 2.0f), 1e-2f), 0, 0));
}

WaterNVRender::WaterNVRender(const NVWaveWorks_FFT_CPU_Simulation::Params &p, const fft_water::SimulationParams &simulation, int q,
  int geom_quality, bool ssr_renderer, bool one_to_four_cascades, int num_cascades, float cascade_window_length,
  float cascade_facet_size, const fft_water::WaterHeightmap *water_heightmap) :
  fft(NULL),
  cascadeWindowLength(cascade_window_length),
  cascadeFacetSize(cascade_facet_size),
  lod0AreaRadius(0.f),
  lod0TesselationAdditional(0),
  cascadesRoughnessInvalid(true),
  roughnessBase(0.0f),
  cascadesRoughnessBase(0.0f),
  cascadesRoughnessMipBias(-3.0f),
  cascadesRoughnessDistBias(-2.0f),
  fftSimulationParams(simulation),
  turbulenceFoamEnabled(false),
  ssrRendererEnabled(ssr_renderer),
  detailsWeightDist(100.0f, 300.0f),
  detailsWeightMin(300.0f, 500.0f, 5000.0f, 6000.0f),
  detailsWeightMax(300.0f, 500.0f, 300.0f, 1300.0f),
  shoreDamp(3.0f, 6.0f),
  shoreWavesDist(600.0f, 1000.0f),
  lastLodExtension(40000.0f),
  shoreEnabled(false),
  renderPass(fft_water::WATER_RENDER_PASS_NORMAL),
  computeGradientsEnabled(false),
  forceTessellation(false),
  oneToFourCascades(one_to_four_cascades),
  waterHeightmap(water_heightmap)
{
  anisotropy = 0;
  mipBias = 0.f;
  waveDisplacementDist = Point2(0.0f, 0.0f);
  renderQuad = NULL;
  // num_cascades = min(d3d::get_driver_desc().maxvertexsamplers-1, num_cascades);
  water_levelVarId = get_shader_variable_id("water_level", true);
  renderQuality = q;
  geomQuality = max(geom_quality, waterHeightmap ? fft_water::RENDER_LOW : 0);
  autoVsamplersAdjust = true;
  memset(maxWaveSize, 0, sizeof(maxWaveSize));
  waterLevel = 0;
  G_ASSERT(num_cascades >= 0);
  numCascades = num_cascades;
  numFftCascades = oneToFourCascades ? 1 : num_cascades;

  G_ASSERT(numCascades <= MAX_NUM_CASCADES);
  G_STATIC_ASSERT(MAX_NUM_CASCADES <= 5);
  enableTurbulenceFoam(true);
  reset();

  setVertexSamplers(numCascades);

  G_ASSERT(renderQuality >= fft_water::RENDER_VERY_LOW && renderQuality <= fft_water::RENDER_GOOD);
  G_ASSERT(geomQuality >= fft_water::RENDER_VERY_LOW && geomQuality <= fft_water::RENDER_GOOD);

  waterCellSize = water_cells_size[(p.fft_resolution_bits == MIN_FFT_RESOLUTION && !waterHeightmap) ? 0 : geomQuality - 1];

  fft = new NVWaveWorks_FFT_CPU_Simulation[MAX_NUM_CASCADES];
  setCascades(p);
  int fft_resolution = 1 << p.fft_resolution_bits;

  if (waterHeightmap)
  {
    heightmapPagesTex = dag::create_tex(NULL, waterHeightmap->pagesX * waterHeightmap->PAGE_SIZE_PADDED,
      waterHeightmap->pagesY * waterHeightmap->PAGE_SIZE_PADDED, TEXFMT_L16, 1, "water_heightmap_pages");
    heightmapPagesTex->texaddr(TEXADDR_CLAMP);
    heightmapPagesTex->texfilter(TEXFILTER_SMOOTH);

    if (auto data = lock_texture(heightmapPagesTex.getTex2D(), 0, TEXLOCK_WRITE | TEXLOCK_DISCARD))
      if (waterHeightmap->pages.size())
        data.writeRows(make_span_const(waterHeightmap->pages));

    heightmapGridTex =
      dag::create_tex(NULL, waterHeightmap->gridSize, waterHeightmap->gridSize, TEXFMT_L16, 1, "water_heightmap_grid");
    heightmapGridTex->texaddr(TEXADDR_CLAMP);
    heightmapGridTex->texfilter(TEXFILTER_SMOOTH);
    if (auto data = lock_texture(heightmapGridTex.getTex2D(), 0, TEXLOCK_WRITE | TEXLOCK_DISCARD))
      if (waterHeightmap->grid.size())
        data.writeRows(make_span_const(waterHeightmap->grid));

    ShaderGlobal::set_color4(::get_shader_variable_id("water_height_offset_scale"),
      Color4(waterHeightmap->heightOffset, waterHeightmap->heightScale, 0, 0));
    ShaderGlobal::set_color4(::get_shader_variable_id("water_heightmap_page_count"), 1.0f / waterHeightmap->pagesX,
      1.0f / waterHeightmap->pagesY);
    ShaderGlobal::set_color4(::get_shader_variable_id("world_to_water_heightmap"), waterHeightmap->tcOffsetScale);
    ShaderGlobal::set_real(::get_shader_variable_id("water_heightmap_det_scale"), waterHeightmap->scale);
    ShaderGlobal::set_real(::get_shader_variable_id("water_heightmap_page_size"), waterHeightmap->HEIGHTMAP_PAGE_SIZE);

    waterHeightmapPatchesGridScale = 1;
    waterHeightmapUseTessellation = d3d::get_driver_desc().caps.hasQuadTessellation;
    if (!waterHeightmapUseTessellation)
    {
      waterHeightmapPatchesGridScale = 4;
      waterHeightmapVdata.init(waterHeightmapPatchesGridScale * waterHeightmapTessFactor);
    }
    waterHeightmapPatchesGridRes = waterHeightmap->PATCHES_GRID_SIZE / waterHeightmapPatchesGridScale;
    ShaderGlobal::set_real(::get_shader_variable_id("water_heightmap_patches_size", true), waterHeightmapPatchesGridRes);
    uint32_t mask = waterHeightmap->PATCHES_GRID_SIZE - 1;
    for (int y = 0; y < waterHeightmapPatchesGridRes; y++)
      for (int x = 0; x < waterHeightmapPatchesGridRes; x++)
      {
        Point2 minMax(FLT_MAX, -FLT_MAX);
        for (int j = y * waterHeightmapPatchesGridScale; j < (y + 1) * waterHeightmapPatchesGridScale; j++)
          for (int i = x * waterHeightmapPatchesGridScale; i < (x + 1) * waterHeightmapPatchesGridScale; i++)
          {
            minMax.x = min(minMax.x, waterHeightmap->patchHeights[j * waterHeightmap->PATCHES_GRID_SIZE + i].x);
            minMax.y = max(minMax.y, waterHeightmap->patchHeights[j * waterHeightmap->PATCHES_GRID_SIZE + i].y);
          }
        if (minMax.y > waterLevel)
          waterHeightmapPatchPositions.push_back(Point4(x, y, minMax.x, minMax.y));
      }

    waterHeightmapPatchesCount = waterHeightmapPatchPositions.size();
    if (waterHeightmapPatchesCount > 0 && waterHeightmapUseTessellation)
    {
      heightmapBuf = dag::buffers::create_ua_sr_structured(sizeof(Point4), waterHeightmapPatchesCount, "water_positions");
      heightmapBuf->updateData(0, sizeof(Point4) * waterHeightmapPatchesCount, waterHeightmapPatchPositions.data(), VBLOCK_WRITEONLY);
    }
    else
      heightmapBuf.close();

    for (int i = 0; i < fft_water::RenderMode::MAX; i++)
    {
      heightmapShmat[i].reset(new_shader_material_by_name(water_heightmap_shader_name[i], nullptr));
      if (heightmapShmat[i])
        heightmapShElem[i] = heightmapShmat[i]->make_elem();
    }
  }
  else
  {
    heightmapPagesTex.close();
    heightmapGridTex.close();
    heightmapBuf.close();
  }

  // render
  shoreDistanceFieldTexVarId = get_shader_variable_id("shore_distance_field_tex");
  wakeHtTexVarId = get_shader_variable_id("wake_ht_tex", true);

  if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_VERTEXTEXTURE))
  {
    int rendererFirst = fft_water::RenderMode::WATER_SHADER;
    int rendererLast = ssrRendererEnabled ? fft_water::RenderMode::WATER_SSR_SHADER : fft_water::RenderMode::WATER_SHADER;
    for (int i = rendererFirst; i <= rendererLast; i++)
    {
      if (!meshRenderer[i].init(water_shader_name[i], NULL, i == fft_water::RenderMode::WATER_SSR_SHADER,
            water_dim_bits[(p.fft_resolution_bits == MIN_FFT_RESOLUTION && !waterHeightmap) ? 0 : geomQuality - 1]))
      {
        return;
      }
    }
  }
  else
  {
    DAG_FATAL("fft_water: no vfetch!");
  }

  for (int cascadeNo = 0; cascadeNo < MAX_NUM_CASCADES; ++cascadeNo)
  {
    String texName;
    texName.printf(128, "water_gradients_tex%d", cascadeNo);
    water_gradients_texVarId[cascadeNo] = get_shader_variable_id(texName, cascadeNo != 0);
    texName.printf(128, "water_displacement_texture%d", cascadeNo);
    water_displacement_textureVarId[cascadeNo] = get_shader_variable_id(texName, cascadeNo != 0);
    texName.printf(128, "water_foam_texture%d", cascadeNo);
    water_foam_textureVarId[cascadeNo] = get_shader_variable_id(texName, cascadeNo != 0);
    texName.printf(128, "water_gradient_scales%d", cascadeNo);
    water_gradient_scalesVarId[cascadeNo] = get_shader_variable_id(texName, cascadeNo != 0);
  }
  foam_dissipationVarId = get_shader_variable_id("foam_dissipation");
  foam_blur_extents0123VarId = get_shader_variable_id("foam_blur_extents0123");
  foam_blur_extents4567VarId = get_shader_variable_id("foam_blur_extents4567", true);
  foam_hats_scaleVarId = get_shader_variable_id("foam_hats_scale", true);
  surface_folding_foam_paramsVarId = get_shader_variable_id("surface_folding_foam_params", true);
  water_foam_passVarId = get_shader_variable_id("water_foam_pass");
  waterRefractionTexVarId = get_shader_variable_id("water_refraction_tex", true);
  wind_dir_xVarId = get_shader_variable_id("wind_dir_x");
  wind_dir_yVarId = get_shader_variable_id("wind_dir_y");
  wind_speedVarId = get_shader_variable_id("wind_speed");

  if (!initGPU())
  {
    logerr("can not initialize GPU fft");
    closeGPU();
    initDisplacementCPU();
    for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
      fft[cascadeNo].allocateAllResources();
  }

  // generate mipmaps only once
  unsigned mipflag = TEXCF_GENERATEMIPS;
  unsigned usageFlag = TEXCF_RTARGET;
  if (computeGradientsEnabled) // Leave TEXCF_RTARGET so we are able to clear it easily if needed.
    usageFlag |= TEXCF_UNORDERED;
  unsigned gradientTexFlags = TEXFMT_A16B16G16R16F | usageFlag | mipflag;

  ArrayTexture *gradientTexArray = nullptr;
  if (use_texture_array)
    gradientArray = dag::create_array_tex(fft_resolution, fft_resolution, numFftCascades, gradientTexFlags, 0, "water3d_grad_array");

  else
  {
    if (computeGradientsEnabled)
    {
      computeGradientsEnabled = false;
      usageFlag &= ~TEXCF_UNORDERED;
      logerr("WaterRenderer: Compute gradients was enabled, but could not create gradients array, disabling compute gradients");
    }
    for (int cascadeNo = 0; cascadeNo < numFftCascades; ++cascadeNo)
    {
      String texName(128, "water3d_gradient%d", cascadeNo);
      gradient[cascadeNo] = dag::create_tex(NULL, fft_resolution, fft_resolution, gradientTexFlags, 0, texName.str());
    }
  }

  if (gradientArray)
    ShaderGlobal::set_texture(water_gradients_texVarId[0], gradientArray);
  else
    for (int i = 0; i < numFftCascades; ++i)
      ShaderGlobal::set_texture(water_gradients_texVarId[i], gradient[i]);

  gradientRenderer.init("water_gradient");
  normalsRenderer.init("water_normals");
  if (renderQuality >= fft_water::RENDER_GOOD)
    initFoam();
  water_vs_cascadesVarId = get_shader_variable_id("water_vs_cascades", true);
  water_cascadesVarId = get_shader_variable_id("water_cascades", true);
  ShaderGlobal::set_int(water_cascadesVarId, oneToFourCascades ? fft_water::MAX_NUM_CASCADES + 1 : numCascades);

  setAnisotropy(renderQuality >= fft_water::RENDER_GOOD ? 1 : 0, 0.f);

  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  zClampStateId = shaders::overrides::create(state);
  state.set(shaders::OverrideState::FLIP_CULL);
  zClampFlipStateId = shaders::overrides::create(state);
}

void WaterNVRender::setLevel(float water_level)
{
  waterLevel = water_level;
  ShaderGlobal::set_real(water_levelVarId, water_level);
}

void WaterNVRender::setWaveDisplacementDistance(const Point2 &value) { waveDisplacementDist = value; }

void WaterNVRender::calcWaveHeight(float &out_max_wave_height, float &out_significant_wave_height)
{
  carray<NVWaveWorks_FFT_CPU_Simulation::Params, fft_water::MAX_NUM_CASCADES> fftParams;
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
    fftParams[cascadeNo] = fft[cascadeNo].getParams();
  calc_wave_height(fftParams.data(), numCascades, out_significant_wave_height, out_max_wave_height, maxWaveSize);

  static int max_wave_heightVarId = get_shader_variable_id("max_wave_height", true);
  ShaderGlobal::set_real(max_wave_heightVarId, out_max_wave_height);
}

void WaterNVRender::initRefraction(int w, int h)
{
  closeRefraction();
  if (waterRefractionTexVarId < 0 || renderQuality == fft_water::RENDER_VERY_LOW) // there is no refaction in compatibility, regardless
                                                                                  // of was it inited or not
    return;
  refraction = dag::create_tex(NULL, w, h, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "refractionTarget");
  refraction.getTex2D()->texaddr(TEXADDR_CLAMP);
}

void WaterNVRender::closeRefraction()
{
  refraction.close();
  ShaderGlobal::set_texture(waterRefractionTexVarId, BAD_TEXTUREID);
}

WaterNVRender::~WaterNVRender()
{
  if (fft)
    delete[] fft;
  fft = 0;
  for (int cascadeNo = 0; cascadeNo < MAX_NUM_CASCADES; ++cascadeNo)
  {
    ShaderGlobal::set_texture(water_displacement_textureVarId[cascadeNo], BAD_TEXTUREID);
    ShaderGlobal::set_texture(water_foam_textureVarId[cascadeNo], BAD_TEXTUREID);
  }
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    ShaderGlobal::set_texture(water_gradients_texVarId[cascadeNo], BAD_TEXTUREID);
  }
  closeGPU();
  closeDisplacementCPU();
  closeFoam();
  closeRefraction();
  reset();
  numCascades = 0;
}

void WaterNVRender::simulateAllAt(double time)
{
  currentSimulatedTime = time;
  if (!numCascades)
    return;
  const int N = 1 << fft[0].getParams().fft_resolution_bits;
#if 0
  bool updateH0Tex = false;
  for (int i = 0; i < numCascades; ++i)
  {
    if (fft[i].IsH0UpdateRequired())
    {
      for (int j = 0; j <= N; ++j)
        fft[i].UpdateH0(j);
      fft[i].SetH0UpdateNotRequired();
      updateH0Tex = true;
    }
  }
  if (updateH0Tex)
  {
    uint8_t *data; int stride;
    if (ht0Tex->lockimg((void**)&data, stride, 0, TEXLOCK_WRITE|TEXLOCK_DISCARD))
    {
      for (int y = 0; y <= N; ++y, data += stride)
      {
        Point4* ht0 = (Point4*)data;
        for (int i = 0; i < numCascades; i+=2, ht0+=N+1)
        {
          for (int x = 0; x <= N; ++x)
          {
            cpu_types::float2 a = fft[i].getH0Data(y)[x];
            cpu_types::float2 b = i+1 < numCascades ? fft[i+1].getH0Data(y)[x] : a;
            ht0[x] = Point4(a.x, a.y, b.x, b.y);
          }
        }
      }
      ht0Tex->unlockimg();
    }
  }
#endif

  if (csGpu || gpGpu)
    return;

  static int totalHt = 0, totalFft = 0, counter = 0;
  for (int i = 0; i < numCascades; ++i)
  {
    if (!fft[i].getHtData(0))
      G_VERIFY(fft[i].allocateAllResources());
    if (fft[i].IsH0UpdateRequired())
    {
      for (int j = 0; j <= N; ++j)
        fft[i].UpdateH0(j);
      fft[i].SetH0UpdateNotRequired();
    }
  }
  for (int i = 0; i < numCascades; ++i)
  {
    int64_t reft;
    fft[i].setTime(currentSimulatedTime);
    reft = ref_time_ticks();
    for (int j = 0; j < N; ++j)
      fft[i].UpdateHt(j);
    totalHt += get_time_usec(reft);
    // memcpy(savedHt[i].data(), fft[i].getHtData(), data_size(savedHt[i]));
    reft = ref_time_ticks();
    for (int j = 0; j < 3; ++j)
      fft[i].ComputeFFT(j);
    totalFft += get_time_usec(reft);
  }
  counter++;
  // debug("render ht = %gus, fft=%g us", double(totalHt)/counter, double(totalFft)/counter);
}

void WaterNVRender::updateTexturesAll()
{
  if (!dispCPU[0] || (csGpu || gpGpu))
    return;
  const int N = 1 << fft[0].getParams().fft_resolution_bits;
  for (int i = 0; i < numCascades; ++i)
  {
    uint8_t *data;
    int stride;
    if (dispCPU[i].getTex2D()->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE | TEXLOCK_DISCARD))
    {
      for (int j = 0; j < N; ++j, data += stride)
        fft[i].getHalfData(j, (cpu_types::half4 *)data);
      dispCPU[i].getTex2D()->unlockimg();
    }
    else
    {
      d3d_err(0);
    }
  }
}

CONSOLE_FLOAT_VAL("water", foam_generation_amount, 0);
CONSOLE_FLOAT_VAL("water", foam_falloff_speed, 0);
CONSOLE_FLOAT_VAL("water", foam_generation_threshold, 0);
CONSOLE_FLOAT_VAL("water", foam_dissipation_speed, 0);

void WaterNVRender::setAnisotropy(int aniso, float mip_bias)
{
  aniso = clamp(aniso, 0, 5);
  mip_bias = clamp(mip_bias, -1.f, 0.f);
  if (aniso == anisotropy && mip_bias != mipBias)
    return;
  anisotropy = aniso;
  mipBias = mip_bias;
  if (gradientArray.getArrayTex())
  {
    gradientArray.getArrayTex()->texfilter(aniso > 0 ? TEXFILTER_BEST : TEXFILTER_SMOOTH);
    gradientArray.getArrayTex()->setAnisotropy(1 << aniso);
    gradientArray.getArrayTex()->texlod(mip_bias);
  }
}

void WaterNVRender::setSmallWaveFraction(float smallWaveFraction)
{
  if (!fft)
    return;
  for (int i = 0; i < numCascades; ++i)
    fft[i].setSmallWaveFraction(smallWaveFraction);
}

#if DAGOR_DBGLEVEL > 0
void WaterNVRender::enableGraphicFeature(int feature, bool enable)
{
  static int waterMask[3] = {
    ::get_shader_variable_id("water_mask1"), ::get_shader_variable_id("water_mask2"), ::get_shader_variable_id("water_mask3")};

  int maskIndex = feature / 4;
  int maskSubIndex = feature % 4;
  G_ASSERT(maskIndex >= 0 && maskIndex <= 2);
  G_ASSERT(maskSubIndex >= 0 && maskSubIndex <= 3);

  Color4 maskValue = ShaderGlobal::get_color4_fast(waterMask[maskIndex]);
  maskValue[maskSubIndex] = enable ? 1.0f : 0.0f;
  ShaderGlobal::set_color4(waterMask[maskIndex], maskValue);
}

void WaterNVRender::getCascadePeriod(int cascade_no, float &out_period, float &out_window_in, float &out_window_out)
{
  out_period = 0.0f;
  out_window_in = 0.0f;

  if (cascade_no >= 0 && cascade_no < MAX_NUM_CASCADES)
  {
    out_period = fft[cascade_no].getParams().fft_period;
    out_window_in = fft[cascade_no].getParams().window_in;
    out_window_out = fft[cascade_no].getParams().window_out;
  }
}
#endif

void WaterNVRender::calculateGradients()
{
  if (!numCascades)
    return;
  if (csGpu || gpGpu)
  {
    performGPUFFT();
    if (dispCPU[0])
      closeDisplacementCPU();
  }
  else
  {
    if (!dispCPU[0])
    {
      initDisplacementCPU();
      simulateAllAt(currentSimulatedTime);
    }
  }
  TIME_D3D_PROFILE(gradients);

  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);


  float foamSimDeltaTime = clamp(float(currentSimulatedTime - lastFoamTime), 0.f, 0.1f);
  lastFoamTime = currentSimulatedTime;

  const bool hasFoam = foamParams.generation_amount > 0.0f && turbulenceFoamEnabled && !clearNeeded;
  clearNeeded = false;

  const int N = 1 << fft[0].getParams().fft_resolution_bits;

  static int water_texture_sizeVarId = get_shader_variable_id("water_texture_size");
  float choppyMul = 1.0f;
  float fft0_period = fft[0].getParams().fft_period;
  if (fft0_period > 1000.0f)
    choppyMul = 1.0f + 0.2f * log(fft0_period / 1000.0f);

  d3d::set_render_target();

  ShaderGlobal::set_color4(water_texture_sizeVarId, Color4(N, N, 1.0 / N, 1.0 / N));

  UniqueTex *dispArray = NULL;
  if (csGpu || gpGpu)
  {
    if (csGpu && (!gpGpu || USE_CS_WATER))
    {
      dispArray = &csGpu->dispArray;
      if (!computeGradientsEnabled)
        d3d::insert_wait_on_fence(csGpu->asyncComputeFence, GpuPipeline::GRAPHICS);
    }
    else if (gpGpu && gpGpu->dispArray)
      dispArray = &gpGpu->dispArray;

    if (dispArray)
      ShaderGlobal::set_texture(water_displacement_textureVarId[0], *dispArray);
  }

  for (int i = 0; i < numCascades; ++i)
  {
    G_ASSERT((1 << fft[i].getParams().fft_resolution_bits) == N);
    const float fft_period = fft[i].getParams().fft_period;
    if (!dispArray)
      ShaderGlobal::set_texture(water_displacement_textureVarId[i], gpGpu ? gpGpu->dispGPU[i] : dispCPU[i]);
    float choppyScale = choppyMul * fft[i].getParams().choppy_scale * (N / fft_period);
    float g_GradMap2TexelWSScale = 0.5f * (N / fft_period);
    ShaderGlobal::set_color4(water_gradient_scalesVarId[i], Color4(choppyScale, g_GradMap2TexelWSScale, 0, 0));
  }

  if (hasFoam)
  {
    shaders::overrides::reset();

    ShaderGlobal::set_int(water_foam_passVarId, 0);
    float foamAccumulation, foamFadeout, foamGenerationThreshold;

    foamAccumulation = foamParams.generation_amount * (float)foamSimDeltaTime * 50.0f;
    foamFadeout = powf(foamParams.falloff_speed, (float)foamSimDeltaTime * 50.0f);
    foamGenerationThreshold = foamParams.generation_threshold;
#if DAGOR_DBGLEVEL > 0
    if (foam_generation_amount.get() > 0)
      foamAccumulation = foam_generation_amount.get() * (float)foamSimDeltaTime * 50.0f;
    if (foam_falloff_speed.get() > 0)
      foamFadeout = powf(foam_falloff_speed.get(), (float)foamSimDeltaTime * 50.0f);
    if (foam_generation_threshold.get() > 0)
      foamGenerationThreshold = foam_generation_threshold.get();
    if (foam_dissipation_speed.get() > 0)
      foamParams.dissipation_speed = foam_dissipation_speed.get();
#endif

    ShaderGlobal::set_color4(foam_dissipationVarId, Color4(foamFadeout, foamAccumulation, foamGenerationThreshold, 0));
    Color4 foam_blur_extents0123(0, 0, 0, 0);
    Color4 foam_blur_extents4567(0, 0, 0, 0);
    if (gradientArray)
      ShaderGlobal::set_texture(water_foam_textureVarId[0], gradientArray);

    for (int i = 0; i < numCascades; ++i)
    {
      if (!gradientArray)
        ShaderGlobal::set_texture(water_foam_textureVarId[i], gradient[i]);
      float &foam_blur_extents = (i < 4) ? foam_blur_extents0123[i] : foam_blur_extents4567[i - 4];
      foam_blur_extents =
        min(0.5f, foamParams.dissipation_speed * (float)foamSimDeltaTime * 1000.0f / fft[i].getParams().fft_period) / N;
    }
    ShaderGlobal::set_color4(foam_blur_extents0123VarId, foam_blur_extents0123);
    ShaderGlobal::set_color4(foam_blur_extents4567VarId, foam_blur_extents4567);
  }

  if (computeGradientsEnabled)
  {
    if (hasFoam)
    {
      {
        TIME_D3D_PROFILE(foamFirstPass);

        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, water_foam_blur_cs_color_uav_slot, VALUE, 0, 0), foamGradient.getArrayTex());
        int groupSize = (N + WATER_FOAM_BLUR_CS_WARP_SIZE - 1) / WATER_FOAM_BLUR_CS_WARP_SIZE;
        gradientFoamCs->dispatch(groupSize, groupSize, 1, GpuPipeline::ASYNC_COMPUTE);
      }
      {
        TIME_D3D_PROFILE(gradientFoamCombine)

        d3d::resource_barrier({foamGradient.getArrayTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
        ShaderGlobal::set_texture(water_foam_textureVarId[0], foamGradient);

        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, water_foam_blur_cs_color_uav_slot, VALUE, 0, 0), gradientArray.getArrayTex());
        int groupSize = (N + WATER_GRADIENT_FOAM_COMBINE_WARP_SIZE - 1) / WATER_GRADIENT_FOAM_COMBINE_WARP_SIZE;
        gradientFoamCombineCs->dispatch(groupSize, groupSize, 1, GpuPipeline::ASYNC_COMPUTE);
      }
    }
    else
    {
      TIME_D3D_PROFILE(gradientRenderer);

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, water_gradient_cs_color_uav_slot, VALUE, 0, 0), gradientArray.getArrayTex());
      int groupSize = (N + WATER_GRADIENT_CS_WARP_SIZE - 1) / WATER_GRADIENT_CS_WARP_SIZE;
      gradientCs->dispatch(groupSize, groupSize, 1, GpuPipeline::ASYNC_COMPUTE);
    }
  }
  else
  {
    if (hasFoam)
    {
      if (!overrideRGB)
      {
        shaders::OverrideState state;
        state.colorWr = WRITEMASK_RGB;
        overrideRGB.reset(shaders::overrides::create(state));
        state.colorWr = WRITEMASK_ALPHA;
        overrideAlpha.reset(shaders::overrides::create(state));
      }
      shaders::overrides::set(overrideRGB);
    }

    for (int i = 0; i < numFftCascades; ++i)
    {
      if (gradientArray)
        d3d::set_render_target(i, gradientArray.getArrayTex(), i, 0);
      else
        d3d::set_render_target(i, gradient[i].getTex2D(), 0);
    }

    gradientRenderer.render();

    if (hasFoam)
    {
      TIME_D3D_PROFILE(gradientFoamRenderer);
      if (gradientArray)
        d3d::resource_barrier({gradientArray.getArrayTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

      // First pass
      shaders::overrides::reset();
      ShaderGlobal::set_int(water_foam_passVarId, 0);

      d3d::set_render_target(foamGradient.getArrayTex(), 0, 0);
      if (numFftCascades > 4)
        d3d::set_render_target(1, foamGradient.getArrayTex(), 1, 0);

      d3d::clearview(CLEAR_DISCARD, 0, 0.f, 0);

      gradientFoamRenderer.render(); // vblur and render

      // Second pass
      ShaderGlobal::set_texture(water_foam_textureVarId[0], foamGradient);
      ShaderGlobal::set_int(water_foam_passVarId, 1);
      shaders::overrides::set(overrideAlpha);

      for (int i = 0; i < numFftCascades; ++i)
      {
        if (gradientArray)
          d3d::set_render_target(i, gradientArray.getArrayTex(), i, 0);
        else
          d3d::set_render_target(i, gradient[i].getTex2D(), 0);
      }

      gradientFoamRenderer.render(); // hblur and render

      shaders::overrides::reset();
    }
  }

  ShaderGlobal::set_color4(foam_hats_scaleVarId, Color4(foamParams.hats_mul, foamParams.hats_threshold, foamParams.hats_folding));
  ShaderGlobal::set_color4(surface_folding_foam_paramsVarId,
    Color4(foamParams.surface_folding_foam_mul, foamParams.surface_folding_foam_pow, 0.f, 0.f));

  // generate mipmaps
  {
    TIME_D3D_PROFILE(gradientMipRenderer);

    if (computeGradientsEnabled)
    {
      static int water_texture_size_mipVarId = get_shader_variable_id("water_texture_size_mip");
      int mipCount = gradientArray.getArrayTex()->level_count();
      int mipN = N;
      for (int mipI = 1; mipI < mipCount; ++mipI)
      {
        d3d::set_rwtex(STAGE_CS, water_gradient_mip_cs_color_uav_slot, gradientArray.getArrayTex(), 0, mipI);
        gradientArray.getArrayTex()->texmiplevel(mipI - 1, mipI - 1);
        for (unsigned layer = 0; layer < numFftCascades; layer++)
        {
          d3d::resource_barrier(
            {gradientArray.getArrayTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, mipI - 1 + mipCount * layer, 1});
        }

        mipN /= 2;
        ShaderGlobal::set_color4(water_texture_size_mipVarId, Color4(mipN, mipN, 1.0 / mipN, 1.0 / mipN));

        int groupSize = (mipN + WATER_GRADIENT_MIP_CS_WARP_SIZE - 1) / WATER_GRADIENT_MIP_CS_WARP_SIZE;
        gradientMipCs->dispatch(groupSize, groupSize, 1, GpuPipeline::ASYNC_COMPUTE);
      }
      for (unsigned layer = 0; layer < numFftCascades; layer++)
      {
        d3d::resource_barrier(
          {gradientArray.getArrayTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, mipCount - 1 + mipCount * layer, 1});
      }
      gradientArray.getArrayTex()->texmiplevel(-1, -1);

      d3d::set_rwtex(STAGE_CS, water_gradient_mip_cs_color_uav_slot, nullptr, 0, 0);
      async_compute_gradients_fence = d3d::insert_fence(GpuPipeline::ASYNC_COMPUTE);
    }
    else if (gradientArray)
    {
      gradientArray.getArrayTex()->generateMips();
    }
    else
    {
      for (int i = 0; i < numFftCascades; ++i)
        if (gradient[i].getTex2D())
          gradient[i].getTex2D()->generateMips();
    }
  }

  d3d::set_render_target(prevRt);

  if (cascadesRoughnessInvalid)
  {
    cascadesRoughnessInvalid = false;
    d3d::insert_wait_on_fence(async_compute_gradients_fence, GpuPipeline::GRAPHICS);
    calculateCascadesRoughness();
  }
}

void WaterNVRender::calculateCascadesRoughness()
{
  static int cascadesRoughness0123VarId = get_shader_variable_id("water_cascades_roughness0123", true);
  static int cascadesRoughness4567VarId = get_shader_variable_id("water_cascades_roughness4567", true);
  if (cascadesRoughness0123VarId < 0 || cascadesRoughness4567VarId < 0 || !normalsRenderer.getMat())
    return;

  TIME_D3D_PROFILE(calculateCascadesRoughness);

  static int waterNormalsSizeVarId = get_shader_variable_id("water_normals_size", true);
  static int roughnessBaseVarId = get_shader_variable_id("water_roughness_base");
  static int cascadesDistMulVarId = get_shader_variable_id("water_cascades_dist_mul");
  static int waterNormalsCascadeNoVarId = get_shader_variable_id("water_normals_cascade_no");

  int fft_resolution_bits = fft[0].getParams().fft_resolution_bits;
  int fft_resolution = 1 << fft_resolution_bits;
  int normalsWidth = fft_resolution / 2;
  int normalsHeight = fft_resolution / 2;
  UniqueTex normals;
  // generate mipmaps only once
  normals = dag::create_tex(NULL, normalsWidth, normalsHeight, TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS, 0,
    "water3d_normals");

  ShaderGlobal::set_color4(waterNormalsSizeVarId, Color4(normalsWidth, normalsHeight, 1.0f / normalsWidth, 1.0f / normalsHeight));
  ShaderGlobal::set_real(cascadesDistMulVarId, 1.0f / max(fft_resolution_bits + cascadesRoughnessDistBias, 1.0f));
  ShaderGlobal::set_real(roughnessBaseVarId, roughnessBase);
  Point4 roughVec0123(0, 0, 0, 0);
  Point4 roughVec4567(0, 0, 0, 0);

  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  d3d::set_render_target();
  d3d::set_render_target(normals.getTex2D(), 0);

  for (int cascadeNo = 0; cascadeNo < numFftCascades; ++cascadeNo)
  {
    ShaderGlobal::set_int(waterNormalsCascadeNoVarId, cascadeNo);

    normalsRenderer.render();
    // generate mipmaps
    normals.getTex2D()->generateMips();

    d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, NULL, NULL, NULL);

    uint16_t *data;
    int stride;
    int res = normals.getTex2D()->lockimg((void **)&data, stride, normals.getTex2D()->level_count() - 1, TEXLOCK_READ);
    if (res && data)
    {
      Point3 norm(half_to_float(data[0]), half_to_float(data[1]), half_to_float(data[2]));
      float avgNormalLen = 1.0f;
      if (!isnan(norm.x) && !isnan(norm.y) && !isnan(norm.z))
      {
        avgNormalLen = norm.lengthSq();
        if (!isnan(avgNormalLen))
          avgNormalLen = safe_sqrt(avgNormalLen);
      }
      float &roughVal = cascadeNo < 4 ? roughVec0123[cascadeNo] : roughVec4567[cascadeNo - 4];
      roughVal = adjust_roughness(cascadeNo == 0 ? cascadesRoughnessBase : 0.0f, avgNormalLen);
      normals.getTex2D()->unlockimg();
    }
  }

  for (int cascadeNo = numFftCascades; cascadeNo < numCascades; cascadeNo++)
  {
    float &roughVal = cascadeNo < 4 ? roughVec0123[cascadeNo] : roughVec4567[cascadeNo - 4];
    roughVal = roughVec0123[0] / fft[0].getParams().fft_period * fft[cascadeNo].getParams().fft_period;
  }

  ShaderGlobal::set_color4(cascadesRoughness0123VarId, Color4::xyzw(roughVec0123));
  ShaderGlobal::set_color4(cascadesRoughness4567VarId, Color4::xyzw(roughVec4567));

  d3d::set_render_target(prevRt);
}

int WaterNVRender::getAutoDisplacementSamplers(float threshold)
{
  if (oneToFourCascades)
    return 1;

  int maxCascade = 0;
  for (int i = 0; i < numCascades; i++)
    if (maxWaveSize[i] > (i ? threshold : 0.5f * threshold))
      maxCascade = i;
  return maxCascade + 1;
}

void WaterNVRender::applyWaterCell()
{
  if (autoVsamplersAdjust)
  {
    int vs_samplers = getAutoDisplacementSamplers(waterCellSize * 0.25f);
    setVertexSamplers(vs_samplers);
  }
}

void WaterNVRender::applyShoreEnabled()
{
  bool isOn = shoreEnabled && significantWaveHeight > shoreWaveThreshold;
  ShaderGlobal::set_int(get_shader_variable_id("shore_waves_on", true), isOn ? 1 : 0);
}

int WaterNVRender::setWaterCell(float water_cell_size, bool auto_set_samplers_cnt)
{
  if (waterCellSize == water_cell_size)
    return vertexDisplaceSamplers;
  waterCellSize = max(water_cell_size, 0.0001f);
  autoVsamplersAdjust = auto_set_samplers_cnt;
  applyWaterCell();
  return vertexDisplaceSamplers;
}

void WaterNVRender::setWaterDim(int dim_bits)
{
  for (int i = 0; i < fft_water::RenderMode::MAX; i++)
  {
    if (meshRenderer[i].isInited() && meshRenderer[i].getDim() != 1 << dim_bits)
    {
      meshRenderer[i].close();
      meshRenderer[i].init(water_shader_name[i], NULL, true, dim_bits);
    }
  }
}

static void set_vertex_samplers_to_shader(int vs) { ShaderGlobal::set_int(water_vs_cascadesVarId, vs); }

void WaterNVRender::setVertexSamplers(int water_vertex_samplers)
{
  if (vertexDisplaceSamplers == water_vertex_samplers)
    return;

  vertexDisplaceSamplers = min(water_vertex_samplers, d3d::get_driver_desc().maxvertexsamplers - 1);
  vertexDisplaceSamplers = min(vertexDisplaceSamplers, numCascades);
  if (geomQuality == fft_water::RENDER_VERY_LOW)
  {
    debug("water is in compatbility vs samplers is 2");
    vertexDisplaceSamplers = min(vertexDisplaceSamplers, 2);
  }
  else if (geomQuality == fft_water::RENDER_LOW)
  {
    debug("water is in low, vs samplers is 3");
    vertexDisplaceSamplers = min(vertexDisplaceSamplers, 3);
  }
  set_vertex_samplers_to_shader(vertexDisplaceSamplers);
}

WaterNVRender::SavedStates WaterNVRender::setStates(TEXTUREID distanceTex, int vs_samplers)
{
  SavedStates states;

  TEXTUREID dispArrayId = BAD_TEXTUREID;

  if (csGpu && (!gpGpu || USE_CS_WATER))
    dispArrayId = csGpu->dispArray.getTexId();
  else if (gpGpu)
    dispArrayId = gpGpu->dispArray.getTexId();


  if (d3d::get_driver_code().is(d3d::dx11 || d3d::ps4) && dispArrayId != BAD_TEXTUREID && wakeHtTex != BAD_TEXTUREID)
  {
    G_ASSERT(VariableMap::isGlobVariablePresent(wakeHtTexVarId));
    ShaderGlobal::set_texture(wakeHtTexVarId, wakeHtTex);
  }

  states.shoreDistanceFieldTexId = ShaderGlobal::get_tex(shoreDistanceFieldTexVarId);
  if (dispArrayId != BAD_TEXTUREID || vs_samplers >= 0)
    ShaderGlobal::set_texture(shoreDistanceFieldTexVarId, distanceTex);

  if (dispArrayId != BAD_TEXTUREID)
  {
    ShaderGlobal::set_texture(water_displacement_textureVarId[0], dispArrayId);
  }
  else
  {
    for (int i = 0; i < vs_samplers; ++i)
      ShaderGlobal::set_texture(water_displacement_textureVarId[i], gpGpu ? gpGpu->dispGPU[i] : dispCPU[i]);
  }
  return states;
}

void WaterNVRender::prepareRefraction(Texture *scene_target_tex)
{
  if (renderQuality == fft_water::RENDER_VERY_LOW) // no refraction in compatibility ever
    scene_target_tex = NULL;
  if (scene_target_tex)
  {
    if (!refraction)
    {
      TextureInfo tinfo;
      scene_target_tex->getinfo(tinfo, 0);
      initRefraction(tinfo.w / 2, tinfo.h / 2);
    }
    d3d::stretch_rect(scene_target_tex, refraction.getTex2D());
  }
  else
    closeRefraction();
  ShaderGlobal::set_texture(waterRefractionTexVarId, refraction);
}

void WaterNVRender::render(const Point3 &origin, TEXTUREID distanceTex, int geom_lod_quality, int survey_id, const Frustum &frustum,
  IWaterDecalsRenderHelper *decals_renderer, fft_water::RenderMode render_mode)
{
  d3d::insert_wait_on_fence(async_compute_gradients_fence, GpuPipeline::GRAPHICS);
  // We cannot have fence between begin_survey and end_survey.
  // Fence splits command list into two command lists, but survey should be in whole command list.
  // So we have to begin survey after wait on fence. End of survey is outside this function, becouse it have
  // several returns.
  d3d::begin_survey(survey_id);

  float minWaterLevel = waterLevel;
  float maxWaterLevel = waterLevel;
  float waterHeight = waterLevel;
  if (waterHeightmap)
  {
    minWaterLevel = min(minWaterLevel, waterHeightmap->heightOffset);
    maxWaterLevel = max(maxWaterLevel, waterHeightmap->heightMax);
    waterHeightmap->getHeightmapDataBilinear(origin.x, origin.z, waterHeight);
  }
  minWaterLevel -= significantWaveHeight;
  maxWaterLevel += significantWaveHeight;
  float originAlt = origin.y - waterHeight;
  const int fftResBits = fft[0].getParams().fft_resolution_bits;

  static int water_originVarId = get_shader_variable_id("water_origin", true);
  static int water_vertical_lodVarId = get_shader_variable_id("water_vertical_lod", true);

  static int cascadesLodResolution0123VarId = get_shader_variable_id("water_cascades_lod_resolution0123", true);
  static int cascadesLodResolution4567VarId = get_shader_variable_id("water_cascades_lod_resolution4567", true);

  static int tess_distanceVarId = get_shader_variable_id("tess_distance", true);

  ShaderGlobal::set_real(tess_distanceVarId, lod0AreaRadius * 0.66f);

  Driver3dPerspective persp;
  if (!d3d::getpersp(persp))
    persp.wk = persp.hk = 1;

  if (cascadesLodResolution0123VarId >= 0 && cascadesLodResolution4567VarId >= 0)
  {
    Color4 cascadesLodResolution0123 = Color4(0, 0, 0, 0);
    Color4 cascadesLodResolution4567 = Color4(0, 0, 0, 0);
    for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
    {
      float &cascadesLodResolution = cascadeNo < 4 ? cascadesLodResolution0123[cascadeNo] : cascadesLodResolution4567[cascadeNo - 4];
      cascadesLodResolution =
        log2(fft[cascadeNo].getParams().fft_period * max(0.0001f, min(persp.wk, persp.hk))) + cascadesRoughnessMipBias;
    }
    ShaderGlobal::set_color4(cascadesLodResolution0123VarId, cascadesLodResolution0123);
    ShaderGlobal::set_color4(cascadesLodResolution4567VarId, cascadesLodResolution4567);
  }

  static int detailsWeightVarId = get_shader_variable_id("details_weight", true);
  ShaderGlobal::set_color4(detailsWeightVarId,
    Color4::xyzw(lerp(detailsWeightMin, detailsWeightMax, cvt(originAlt, detailsWeightDist.x, detailsWeightDist.y, 0.0f, 1.0f))));

  //
  dag::ConstSpan<eastl::pair<float, int>> waveLods =
    WAVE_HEIGHT_TO_LODS[(fftResBits == MIN_FFT_RESOLUTION && !waterHeightmap) ? 0 : geomQuality - 1];
  int lodCount = waveLods[0].second;
  for (int itemNo = 1; itemNo < waveLods.size(); ++itemNo)
  {
    if (significantWaveHeight > waveLods[itemNo].first)
      lodCount = waveLods[itemNo].second;
    else
      break;
  }

  if (forceTessellation || waterHeightmap)
    lodCount = (int)waveLods.size() - 1;

  const int lod0Rad = 1;
  const int lastLodRad = 1;
  const float maxLodWithHeightmap = 3.0f;
  float nextLod = originAlt * 0.015f;
  if (waterHeightmap)
    nextLod = min(nextLod, maxLodWithHeightmap);
  if (geom_lod_quality == fft_water::GEOM_INVISIBLE && !renderQuad)
    nextLod = lodCount;
  int lod = (int)floorf(max(nextLod, 0.f));
  if (lodCount <= lod)
  {
    nextLod = lod = max(max<int>(log2f(max(originAlt, 1.0f)) - waterCellSize * 2.0f, lodCount), 4);
    lodCount = 1;
  }
  else
    lodCount = max(lodCount - lod, 0);
  float scaledCell = waterCellSize * (1 << min(11, lod));
  int vs_samplers = vertexDisplaceSamplers;
  float maxScatterDist = min(MAX_LIGHT_SCATTER_DIST, LIGHT_SCATTER_DIST_MUL * (maxWaterLevel - waterLevel));
  if (lod != 0 && autoVsamplersAdjust)
  {
    vs_samplers = min(vs_samplers, getAutoDisplacementSamplers(scaledCell * 0.25f));
    if (!vs_samplers && origin.y < maxWaterLevel + maxScatterDist)
      vs_samplers = 1;
  }
  Point2 centerOfHmap = Point2::xz(origin);

  int gridDim = meshRenderer[fft_water::RenderMode::WATER_SHADER].getDim();

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
  BBox2 lodsRegion;
  cull_lod_grid(lodGrid, lodGrid.lodsCount, centerOfHmap.x, centerOfHmap.y, scaledCell, scaledCell, alignSize, alignSize, // alignment
    minWaterLevel, maxWaterLevel, &frustum, renderQuad, defaultCullData, NULL, lod0AreaRadius, gridDim, false /*not used*/, nullptr,
    nullptr, &lodsRegion);
  if (!defaultCullData.getCount())
    return;

  Point4 cameraPatch(1, 0, 0, 0);
  for (auto &patch : defaultCullData.patches)
  {
    Point2 lt(patch.z, patch.w);
    float size = gridDim * patch.x;
    Point2 rb = lt + Point2(size, size);
    if (lt.x <= origin.x && rb.x >= origin.x && lt.y <= origin.z && rb.y >= origin.z)
    {
      cameraPatch = patch;
      break;
    }
  }
  cameraPatchOffset = Point2(cameraPatch.z, cameraPatch.w);
  cameraPatchAlign = cameraPatch.x;

  if (autoVsamplersAdjust)
    set_vertex_samplers_to_shader(vs_samplers);
  static int scatter_disappear_factorVarId = get_shader_variable_id("scatter_disappear_factor", true);
  float scatterFactor = clamp(safediv(origin.y - maxWaterLevel, maxScatterDist), 0.f, 1.f);
  ShaderGlobal::set_real(scatter_disappear_factorVarId, 1 - scatterFactor * scatterFactor * scatterFactor);

  ShaderGlobal::set_color4(water_originVarId, Color4(origin.x, origin.y, origin.z));

  float lod0RadiusMeters = lod0AreaRadius / (1 << lod0TessFactor);
  float lodVertical = clamp((nextLod - lod) * 2 - 1, 0.0f, 1.0f);
  float lod0ExVertical = max(1.0f - lod0_tesselation_additional, lodVertical);
  ShaderGlobal::set_color4(water_vertical_lodVarId,
    Color4(lodVertical, lod0ExVertical, (lod0RadiusMeters * 0.5f) > lastLodExtension ? -1.0f : lodCount - 1,
      max(lastLodExtension - lod0RadiusMeters * 0.5f, 0.0f)));

  const SavedStates states = setStates(distanceTex, vs_samplers);
  int prevGlobalBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  static int mWater3dBlockId = ShaderGlobal::getBlockId("water3d_block");
  ShaderGlobal::setBlock(mWater3dBlockId, ShaderGlobal::LAYER_FRAME);
  if (origin.y <= maxWaterLevel && renderPass == fft_water::WATER_RENDER_PASS_NORMAL)
  {
    shaders::OverrideStateId prevStateId = shaders::overrides::get_current();
    if (prevStateId)
    {
      shaders::overrides::reset();
      G_ASSERT(shaders::overrides::get(prevStateId).bits == shaders::OverrideState::FLIP_CULL);
      shaders::overrides::set(zClampFlipStateId);
    }
    else
      shaders::overrides::set(zClampStateId);
  }

  ShaderElement *shElem = nullptr;
  HeightmapRenderer *renderer = nullptr;
  if ((render_mode >= 0) && (render_mode < fft_water::RenderMode::MAX))
  {
    shElem = heightmapShElem[render_mode];
    renderer = &meshRenderer[render_mode];
  }
  else
    logerr("Non-existent water rendering mode %d", render_mode);
  if (renderer)
    renderer->render(lodGrid, defaultCullData);
  if (waterHeightmapPatchesCount > 0)
  {
    if (waterHeightmapUseTessellation)
    {
      ShaderGlobal::set_real(::get_shader_variable_id("hmap_tess_factor"),
        waterHeightmapUseTessellation ? waterHeightmapTessFactor : 0);
      if (shElem && shElem->setStates(0, true))
      {
        d3d::setvsrc(0, 0, 0);
        ShaderGlobal::set_color4(::get_shader_variable_id("water_lods_region"), lodsRegion[0].x, lodsRegion[0].y, lodsRegion[1].x,
          lodsRegion[1].y);
        int heightmap_scale_offset_varId = ::get_shader_variable_id("heightmap_scale_offset");
        d3d::draw(PRIM_4_CONTROL_POINTS, 0, waterHeightmapPatchesCount);
      }
    }
    else
    {
      float ofsScale = 1.0f / (float)(waterHeightmap->tcOffsetScale.z * waterHeightmapPatchesGridRes);
      float scale = ofsScale / (float)(waterHeightmapPatchesGridScale * waterHeightmapTessFactor);
      Point2 offset(-waterHeightmap->tcOffsetScale.x / waterHeightmap->tcOffsetScale.z,
        -waterHeightmap->tcOffsetScale.y / waterHeightmap->tcOffsetScale.w);
      defaultCullData.eraseAll();
      lodsRegion.inflate(-ofsScale);
      for (int i = 0; i < waterHeightmapPatchesCount; i++)
      {
        Point4 pos = Point4(waterHeightmapPatchPositions[i]);
        pos.x = pos.x * ofsScale + offset.x;
        pos.y = pos.y * ofsScale + offset.y;
        Point4 min = Point4(pos.x, pos.z, pos.y, 0.0f);
        Point4 max = Point4(pos.x + ofsScale, pos.w, pos.y + ofsScale, 0.0f);
        if (!(lodsRegion & BBox2(min.x, min.z, max.x, max.z)) && frustum.testBoxB(v_ld(&min.x), v_ld(&max.x)))
          defaultCullData.patches.push_back(Point4(scale, 0.0, pos.x, pos.y));
      }
      defaultCullData.scaleX = 1.0 / waterHeightmap->tcOffsetScale.x;
      defaultCullData.lod0PatchesCount = defaultCullData.patches.size();
      lodGrid.lod0SubDiv = 0;
      int cnt = waterHeightmapTessFactor * waterHeightmapPatchesGridScale;
      if (renderer)
        renderer->render(lodGrid, defaultCullData, &waterHeightmapVdata, cnt);
    }
  }

  if (origin.y <= maxWaterLevel && renderPass == fft_water::WATER_RENDER_PASS_NORMAL)
    shaders::overrides::reset();

  if (decals_renderer != NULL)
    decals_renderer->render();

  resetStates(states, vs_samplers);
  ShaderGlobal::setBlock(prevGlobalBlockId, ShaderGlobal::LAYER_FRAME);
}

void WaterNVRender::resetStates(const SavedStates &states, int vs_samplers)
{
  if (vs_samplers < 0)
    return;
  // if (!vertexDisplaceSamplers)// requires changes in vertex shader
  //   return;

  const UniqueTex *dispArray = NULL;
  if (csGpu && (!gpGpu || USE_CS_WATER))
    dispArray = &csGpu->dispArray;
  else if (gpGpu)
    dispArray = &gpGpu->dispArray;

  if (VariableMap::isGlobVariablePresent(wakeHtTexVarId))
    ShaderGlobal::set_texture(wakeHtTexVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(shoreDistanceFieldTexVarId, states.shoreDistanceFieldTexId);

  if (dispArray)
  {
    ShaderGlobal::set_texture(water_displacement_textureVarId[0], *dispArray);
  }
  else
  {
    for (int i = 0; i < vs_samplers; ++i)
      ShaderGlobal::set_texture(water_displacement_textureVarId[i], BAD_TEXTUREID);
  }
}

void WaterNVRender::setRenderQuad(const BBox2 &b)
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

void WaterNVRender::setShoreWavesDist(const Point2 &value)
{
  static int shoreWavesDistVarId = ::get_shader_glob_var_id("shore_waves_dist");
  shoreWavesDist = value;
  ShaderGlobal::set_color4(shoreWavesDistVarId, shoreWavesDist.x, shoreWavesDist.y, 0, 50.0f);
}

void WaterNVRender::setShoreWaveThreshold(float value)
{
  shoreWaveThreshold = value;
  applyShoreEnabled();
}

bool WaterNVRender::isShoreEnabled() const { return shoreEnabled; }

void WaterNVRender::shoreEnable(bool enable)
{
  shoreEnabled = enable;
  applyShoreEnabled();
}

void WaterNVRender::setRenderPass(int pass)
{
  renderPass = pass;
  static int waterRenderPassVarId = get_shader_variable_id("water_render_pass", true);
  ShaderGlobal::set_int(waterRenderPassVarId, pass);
}
