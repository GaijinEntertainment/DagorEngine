// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "waterPhys.h"
#include "waterRender.h"
#include "chopWaterPhysics.h"
#include "chopWaterRender.h"
#include "waterRenderCommon.h"
#include <limits.h>
#include <fftWater/fftWater.h>
#include <fftWater/chopWaterGen.h>
#include <fftWater/gpuFetch.h>
#include <debug/dag_debug3d.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <waterDecals/waterDecalsRenderer.h>
#include <math/dag_adjpow2.h>
#include <math/dag_mathUtils.h>
#include <math/dag_hlsl_floatx.h>
#include <generic/dag_initOnDemand.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_spinlock.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_btagCompr.h>
#include <supp/dag_alloca.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("water", phys_tex_on, false);
#endif

namespace convar
{
CONSOLE_BOOL_VAL("water", chop_gen, false);
CONSOLE_FLOAT_VAL("chop", wind_speed_chop, 1.0f);
} // namespace convar

#define GLOBAL_VARS_LIST VAR(chop_water_enabled)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

class FFTWater
{
  eastl::unique_ptr<fft_water::WaterFlowmap> waterFlowmap;
  eastl::unique_ptr<fft_water::WaterHeightmap> waterHeightmap;
  eastl::unique_ptr<HeightmapHeightCulling> heightmapCulling;

  WaterRenderCommon renderCommon;

  WaterNVRender *render;
  WaterNVPhysics *physics;

  ChopWaterRender *renderChop;
  ChopWaterPhysics *physicsChop;
  ChopWaterGenerator *chopWaterGenerator;

#if DAGOR_DBGLEVEL > 0
  UniqueTex physTex;
#endif
  dag::AtomicFloat<double> currentPhysTime;
  double lastTime;
  NVWaveWorks_FFT_CPU_Simulation::Params params;
  float minWaterLevel, maxWaterLevel;
  int numRenderCascades;
  int minRenderResBits;
  int enforceRenderCascadeCount;

public:
  void setCurrentTime(double time) { currentPhysTime.store(time, dag::memory_order_release); }
  double getCurrentTime() const { return currentPhysTime.load(dag::memory_order_acquire); }
  double getLastTime() const { return lastTime; }
  void setPeriod(float period)
  {
    if (params.fft_period == period)
      return;

    params.fft_period = period;

    if (render)
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    if (physics)
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);

    renderCommon.setMaxWaveHeight(getMaxWaveHeight()); // FFT maxWaveHeight depends on fft_period
    updateHeightCullingWaveHeight();
  }
  float getPeriod() const { return params.fft_period; }
  FFTWater(int num_render_cascades, int min_render_res_bits) :
    render(NULL),
    physics(NULL),
    renderChop(NULL),
    physicsChop(NULL),
    chopWaterGenerator(NULL),
    numRenderCascades(num_render_cascades),
    currentPhysTime(0.0)
  {
    minRenderResBits = max<int>(min_render_res_bits, MIN_FFT_RESOLUTION);
    params.fft_resolution_bits = minRenderResBits;
    params.wind_dependency = 0.98f;
    params.wind_alignment = 1.0f;
    params.small_wave_fraction = 0.001f;
    params.choppy_scale = 1.0f;
    params.wave_amplitude = 0.7f;
    params.fft_period = 1000.0f;
    lastTime = 0.0;
    minWaterLevel = 0.0f;
    maxWaterLevel = 0.0f;
    enforceRenderCascadeCount = -1;
    setWind(1.0f, 1.0f, Point2(0.8, 0.6));
  }
  void getWind(float &out_speed, Point2 &out_wind_dir) const
  {
    out_speed = params.wind_speed;
    out_wind_dir = Point2(params.wind_dir_x, params.wind_dir_y);
  }
  void setWind(float speed, float chop_wind_speed, const Point2 &wind_dir)
  {
    Point2 windDirNorm = normalize(wind_dir);
    const bool chopWindSpeedChanged = chopWaterGenerator && fabs(chop_wind_speed - chopWaterGenerator->getWindSpeed()) > 0.05f;
    if (!chopWindSpeedChanged && fabsf(params.wind_speed - speed) < 0.05f &&
        windDirNorm * Point2(params.wind_dir_x, params.wind_dir_y) > 0.999f)
      return;

    params.wind_dir_x = windDirNorm.x;
    params.wind_dir_y = windDirNorm.y;
    params.wind_speed = speed;

    if (render)
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    if (physics)
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);

    if (chopWaterGenerator)
      chopWaterGenerator->setWind(chop_wind_speed, Point2(params.wind_dir_x, params.wind_dir_y));
    if (renderChop)
      renderChop->reinit();
    if (physicsChop)
      physicsChop->calcWaveHeight();

    renderCommon.setWind(params.wind_dir_x, params.wind_dir_y, params.wind_speed); // use FFT wind_speed for shader (todo: check)
    renderCommon.setMaxWaveHeight(getMaxWaveHeight());
    updateHeightCullingWaveHeight();

    convar::wind_speed_chop = chop_wind_speed;
  }
  int getFFTRenderResolution() const { return params.fft_resolution_bits; }
  void setFFTRenderResolution(float resolution_bits)
  {
    if (params.fft_resolution_bits == resolution_bits)
      return;

    params.fft_resolution_bits = clamp<int>(resolution_bits, minRenderResBits, MAX_FFT_RESOLUTION);
    if (render && render->getFFTResolutionBits() != params.fft_resolution_bits)
      resetRender();
  }
  float getSmallWaveFraction() const { return params.small_wave_fraction; }
  void setSmallWaveFraction(float smallWaveFraction)
  {
    if (params.small_wave_fraction == smallWaveFraction)
      return;
    params.small_wave_fraction = smallWaveFraction;

    if (physics)
    {
      physics->setSmallWaveFraction(smallWaveFraction);
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (render)
    {
      render->setSmallWaveFraction(smallWaveFraction);
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    updateHeightCullingWaveHeight();
  }
  fft_water::SimulationParams getSimulationParams() const
  {
    if (physics)
      return physics->getSimulationParams();
    if (render)
      return render->getSimulationParams();
    return fft_water::SimulationParams();
  }
  void setSimulationParams(const fft_water::SimulationParams &simulation)
  {
    if (physics && physics->getSimulationParams() != simulation)
    {
      physics->setSimulationParams(simulation);
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (render && render->getSimulationParams() != simulation)
    {
      render->setSimulationParams(simulation);
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    updateHeightCullingWaveHeight();
  }
  float getCascadeWindowLength() const
  {
    if (physics)
      return physics->getCascadeWindowLength();
    if (render)
      return render->getCascadeWindowLength();
    return kCascadeMinFacetTexels;
  }
  void setCascadeWindowLength(float value)
  {
    if (getCascadeWindowLength() == value)
      return;
    if (physics)
    {
      physics->setCascadeWindowLength(value);
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (render)
    {
      render->setCascadeWindowLength(value);
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    updateHeightCullingWaveHeight();
  }
  float getCascadeFacetSize() const
  {
    if (physics)
      return physics->getCascadeFacetSize();
    if (render)
      return render->getCascadeFacetSize();
    return kCascadeFacetSize;
  }
  void setCascadeFacetSize(float value)
  {
    if (getCascadeFacetSize() == value)
      return;
    if (physics)
    {
      physics->setCascadeFacetSize(value);
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (render)
    {
      render->setCascadeFacetSize(value);
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    updateHeightCullingWaveHeight();
  }
  void closeRender()
  {
    del_it(render);
    del_it(renderChop);
  }

  int getActualFFTResolutionBits(int quality)
  {
    if (quality <= fft_water::RENDER_VERY_LOW)
      return minRenderResBits;
    return params.fft_resolution_bits;
  }

  int renderCascadeCountValidation(int v)
  {
    if (enforceRenderCascadeCount >= 0)
    {
      if (v != enforceRenderCascadeCount)
      {
        logerr("fftwater: enforced cascade count dont match with required count: %d %d", enforceRenderCascadeCount, v);
        return enforceRenderCascadeCount;
      }
    }
    return v;
  }

  void initRender(int quality, int geom_quality, bool depth_renderer, bool ssr_renderer, bool one_to_four_cascades,
    bool water_heightmap_draw_patches, int enforce_render_cascade_count)
  {
    bool saveParams = render != NULL;
    int aniso = saveParams ? render->getAnisotropy() : 0;
    float mipBias = saveParams ? render->getMipBias() : 0.f;
    fft_water::SimulationParams simulation = saveParams ? render->getSimulationParams() : fft_water::SimulationParams();
    fft_water::FoamParams foam = saveParams ? render->getFoamParams() : fft_water::FoamParams();
    float cascadeFacetSize = getCascadeFacetSize();
    float cascadeWindowLength = getCascadeWindowLength();
    float roughnessBase = 0, cascadesRoughnessBase = 0;
    if (saveParams)
      render->getRoughness(roughnessBase, cascadesRoughnessBase);
    Point2 waveDisplacementDistance = saveParams ? render->getWaveDisplacementDistance() : Point2(0, 0);

    enforceRenderCascadeCount = enforce_render_cascade_count;
    numRenderCascades = renderCascadeCountValidation(numRenderCascades);

    closeRender();
    NVWaveWorks_FFT_CPU_Simulation::Params newParams = params;
    newParams.fft_resolution_bits = getActualFFTResolutionBits(quality);
    render = new WaterNVRender(newParams, simulation, quality, geom_quality, depth_renderer, ssr_renderer, one_to_four_cascades,
      numRenderCascades, cascadeWindowLength, cascadeFacetSize, waterHeightmap.get(), heightmapCulling.get(),
      water_heightmap_draw_patches);
    if (chopWaterGenerator)
      initRenderChop();

    renderCommon.init();
    if (saveParams)
    {
      bool shoreEnable = renderCommon.isShoreEnabled();
      render->setMinMaxLevel(minWaterLevel, maxWaterLevel);
      render->setAnisotropy(aniso, mipBias);
      render->setFoamParams(foam);
      render->setRoughness(roughnessBase, cascadesRoughnessBase);
      render->setWaveDisplacementDistance(waveDisplacementDistance);
      renderCommon.shoreEnable(shoreEnable);
      renderCommon.setWaterLevel(minWaterLevel);
    }
    else
    {
      renderCommon.shoreEnable(false);
      renderCommon.setWaterLevel(0.0f);
    }
    if (renderChop)
      renderChop->setMinMaxLevel(minWaterLevel, maxWaterLevel);

    updateHeightCullingWaveHeight();
  }
  void initChopWaterGen()
  {
    if (chopWaterGenerator)
    {
      del_it(chopWaterGenerator);
    }
    chopWaterGenerator = new ChopWaterGenerator();
  }
  void initRenderChop()
  {
    G_ASSERT(chopWaterGenerator);
    G_ASSERT(!renderChop);
    renderChop = new ChopWaterRender(*chopWaterGenerator, render->getQuality(), render->getGeomQuality(),
      render->isDepthRendererEnabled(), render->isSSRRendererEnabled(), waterHeightmap.get(), heightmapCulling.get());
    renderChop->setMinMaxLevel(minWaterLevel, maxWaterLevel);
    updateHeightCullingWaveHeight();
  }
  void initPhysicsChop()
  {
    G_ASSERT(chopWaterGenerator);
    G_ASSERT(!physicsChop);
    physicsChop = new ChopWaterPhysics(*chopWaterGenerator, WATER_CPU_CASCADES_COUNT, waterHeightmap.get());
  }
  void resetRender()
  {
    if (render)
      initRender(render->getQuality(), render->getGeomQuality(), render->isDepthRendererEnabled(), render->isSSRRendererEnabled(),
        render->getOneToFourCascades(), render->isWaterHeightmapDrawPatches(), enforceRenderCascadeCount);
  }
  int getNumCascades() const { return numRenderCascades; }
  void setNumCascades(int cascades)
  {
    if (numRenderCascades == cascades)
      return;
    numRenderCascades = renderCascadeCountValidation(cascades);
    if (render)
      initRender(render->getQuality(), render->getGeomQuality(), render->isDepthRendererEnabled(), render->isSSRRendererEnabled(),
        render->getOneToFourCascades(), render->isWaterHeightmapDrawPatches(), enforceRenderCascadeCount);
  }
  void resetPhysics()
  {
    if (physics)
      physics->reset();
    if (physicsChop)
      physicsChop->reset();
  }
  void waitPhysics()
  {
    if (physics)
      physics->wait();
    if (physicsChop)
      physicsChop->wait();
  }
  void closePhysics()
  {
    del_it(physics);
    del_it(physicsChop);
  }
  bool validateNextTimeTick(double time) const
  {
    if (chopEnabled())
      return physicsChop ? physicsChop->validateNextTimeTick(time) : true;
    else
      return physics ? physics->validateNextTimeTick(time) : true;
  }
  void initPhysics()
  {
    G_ASSERT(physics == NULL);

    NVWaveWorks_FFT_CPU_Simulation::Params newParams = params;
    newParams.fft_resolution_bits = DEF_PHYS_FFT_RESOLUTION;
    physics = new WaterNVPhysics(newParams, fft_water::SimulationParams(), NUM_PHYS_CASCADES, getCascadeWindowLength(),
      getCascadeFacetSize(), waterHeightmap.get());

    if (chopWaterGenerator)
      initPhysicsChop();

    lastTime = 0;
    currentPhysTime.store(0);
  }

  fft_water::WaterFlowmap *getFlowmap() const { return waterFlowmap.get(); }
  void createFlowmap() { waterFlowmap.reset(new fft_water::WaterFlowmap()); }
  void removeFlowmap() { waterFlowmap.reset(nullptr); }

  const fft_water::WaterHeightmap *getHeightmap() const { return waterHeightmap.get(); }
  const HeightmapHeightCulling *getHeightmapCulling() const { return heightmapCulling.get(); }
  void setHeightmap(eastl::unique_ptr<fft_water::WaterHeightmap> &&water_heightmap)
  {
    waterHeightmap = eastl::move(water_heightmap);
    waterHeightmap->heightMax = waterHeightmap->heightOffset + waterHeightmap->heightScale;
    waterHeightmap->waterLevel = minWaterLevel;
    setMinMaxLevel(minWaterLevel, maxWaterLevel);

    heightmapCulling = eastl::make_unique<HeightmapHeightCulling>();
    heightmapCulling->init(waterHeightmap.get());
    updateHeightCullingWaveHeight();

    if (physics)
    {
      physics->setHeightmap(waterHeightmap.get());
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (physicsChop)
    {
      physicsChop->setHeightmap(waterHeightmap.get());
      physicsChop->calcWaveHeight();
    }
  }
  void setHeightmapExtraBoundings(const dag::Vector<BBox3> &extra_boundings)
  {
    if (waterHeightmap && heightmapCulling)
    {
      waterHeightmap->extraBoundings = extra_boundings;
      heightmapCulling->init(waterHeightmap.get());
    }
  }
  void removeHeightmap()
  {
    heightmapCulling.reset(nullptr);
    waterHeightmap.reset(nullptr);
    setMinMaxLevel(minWaterLevel, minWaterLevel);

    if (physics)
    {
      physics->setHeightmap(waterHeightmap.get());
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
    if (physicsChop)
    {
      physicsChop->setHeightmap(nullptr);
      physicsChop->calcWaveHeight();
    }
  }
  ~FFTWater()
  {
    del_it(render);
    del_it(physics);
    del_it(renderChop);
    del_it(physicsChop);
    del_it(chopWaterGenerator);
  }
  void simulateAllAt(double time, bool is_chop)
  {
    lastTime = time;
    setCurrentTime(time);
    if (physics && !is_chop)
      physics->increaseTime(time);
    if (physicsChop && is_chop)
      physicsChop->increaseTime(time);

#if DAGOR_DBGLEVEL > 0
    if (physics && phys_tex_on)
    {
      const int TEX_R = (1 << DEF_PHYS_FFT_RESOLUTION);
      if (!physTex.getTex2D())
      {
        physTex = dag::create_tex(NULL, TEX_R, TEX_R, TEXCF_DYNAMIC | TEXFMT_A16B16G16R16F, 1, "water_phys_tex", RESTAG_WATER);
      }

      int fifo1, fifo2;
      float fifo2Part;
      physics->getFifoIndex(currentPhysTime.load(), fifo1, fifo2, fifo2Part);
      vec3f v_fifo2Part = v_splats(fifo2Part);
      Point3_vec4 displacements;

      uint8_t *data;
      int stride;
      if (physTex.getTex2D()->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE | TEXLOCK_DISCARD))
      {
        for (int j = 0; j < TEX_R; ++j, data += stride)
          for (int k = 0; k < TEX_R; ++k)
          {
            vec3f disp1, disp2;
            vec4f xz = v_make_vec4f((k + 0.5f) * params.fft_period / TEX_R, (j + 0.5f) * params.fft_period / TEX_R, 0.0f, 0.0f);
            physics->getDisplacementsBilinear(0, disp1, disp2, xz, fifo1, fifo2);
            v_st(&displacements.x, v_madd(v_sub(disp2, disp1), v_fifo2Part, disp1));

            ((Half4 *)data)[k] = Half4(displacements.x, displacements.y, displacements.z, 1.0f);
          }
        physTex.getTex2D()->unlockimg();
      }
    }
    else
      physTex.close();
#endif
  }
  float getLevel() const { return minWaterLevel; }
  void setLevel(float level)
  {
    minWaterLevel = level;
    maxWaterLevel = level;
    if (render)
      render->setMinMaxLevel(level, level);
    if (renderChop)
      renderChop->setMinMaxLevel(level, level);
    if (physics)
      physics->setLevel(level);
    if (physicsChop)
      physicsChop->setLevel(level);
    if (render || renderChop)
      renderCommon.setWaterLevel(level);
    // Flat ocean culling bounds depend on the water level, so rebake the height-culling LUT when the level changes.
    if (waterHeightmap && heightmapCulling && waterHeightmap->waterLevel != level)
    {
      waterHeightmap->waterLevel = level;
      heightmapCulling->init(waterHeightmap.get());
    }
  }
  void updateHeightCullingWaveHeight()
  {
    if (!waterHeightmap || !heightmapCulling)
      return;
    // At dead calm wave == 0, so flat ocean keeps a zero-thickness box at the water level. This is correct (no waves to
    // contain); do not force a minimum here or elevated water bodies lose their tight culling bounds.
    const float maxWaveHeight =
      chopEnabled() && renderChop ? renderChop->getMaxWaveHeight() : (render ? render->getMaxWaveHeight() : 0.0f);
    waterHeightmap->waveCullingMargin = maxWaveHeight;
    heightmapCulling->setUpDisplacement(maxWaveHeight);
    heightmapCulling->setDownDisplacement(maxWaveHeight);
  }
  float getMinLevel() const { return minWaterLevel; }
  float getMaxLevel() const { return maxWaterLevel; }
  void setMinMaxLevel(float min_level, float max_level)
  {
    minWaterLevel = min_level;
    maxWaterLevel = max_level;
    if (waterHeightmap)
      maxWaterLevel = max(maxWaterLevel, waterHeightmap->heightMax);

    if (render)
      render->setMinMaxLevel(minWaterLevel, maxWaterLevel);
    if (renderChop)
      renderChop->setMinMaxLevel(minWaterLevel, maxWaterLevel);
  }
  float getHeight(const Point3 &point) const
  {
    if (chopEnabled() && physicsChop)
    {
      vec4f disp = physicsChop->getHeightmapDataBilinear(point.x, point.z);
      return v_extract_z(disp);
    }
    if (physics)
    {
      vec4f disp = physics->getHeightmapDataBilinear(point.x, point.z);
      return v_extract_z(disp);
    }
    return minWaterLevel;
  }
  float getMinHeight() const
  {
    if (render)
      return render->getMinHeight();
    if (renderChop)
      return renderChop->getMinHeight();
    return minWaterLevel;
  }
  float getMaxHeight() const
  {
    if (render)
      return render->getMaxHeight();
    if (renderChop)
      return renderChop->getMaxHeight();
    return maxWaterLevel;
  }
  float getMaxWaveHeight() const
  {
    if (chopEnabled())
      return physicsChop ? physicsChop->getMaxWaveHeight() : 0.0f;
    else
      return physics ? physics->getMaxWaveHeight() : 0.0f;
  }
  float getSignificantWaveHeight() const
  {
    if (chopEnabled())
      return physicsChop ? physicsChop->getSignificantWaveHeight() : 0.0f;
    else
      return physics ? physics->getSignificantWaveHeight() : 0.0f;
  }

  bool chopEnabled() const { return chopWaterGenerator && convar::chop_gen; }

  WaterRenderCommon &getRenderCommon() { return renderCommon; }
  const WaterRenderCommon &getRenderCommon() const { return renderCommon; }

  WaterNVRender *getRender() const { return render; }
  WaterNVPhysics *getPhysics() const { return physics; }

  ChopWaterGenerator *getChopWaterGen() const { return chopWaterGenerator; }
  ChopWaterRender *getRenderChop() const { return renderChop; }
  ChopWaterPhysics *getPhysicsChop() const { return physicsChop; }
};

#if DAGOR_DBGLEVEL > 0
class FFTWaterCmdProcessor : public console::ICommandProcessor
{
public:
  FFTWater *water = NULL;

  FFTWaterCmdProcessor() : console::ICommandProcessor(1000) {}
  void destroy() {}

  Point2 getDir()
  {
    G_ASSERT(water);
    if (!water)
      return Point2(1.0f, 0.0f);
    float speed;
    Point2 dir;
    water->getWind(speed, dir);
    return dir;
  }

  virtual bool processCommand(const char *argv[], int argc)
  {
    int found = 0;
    if (!water)
      return found;

    CONSOLE_CHECK_NAME("water", "hq", 1, 5)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc > 1)
        preset.simulation.amplitude[0] = preset.simulation.amplitude[1] = preset.simulation.amplitude[2] =
          preset.simulation.amplitude[3] = atof(argv[1]);
      else
        preset.simulation.amplitude[0] = preset.simulation.amplitude[1] = preset.simulation.amplitude[2] =
          preset.simulation.amplitude[3] = 0.7f;
      if (argc > 2)
        preset.simulation.facetSize = atof(argv[2]);
      if (argc > 3)
        preset.foam.generation_threshold = atof(argv[3]);
      if (argc > 4)
        preset.foam.hats_threshold = atof(argv[4]);

      fft_water::apply_wave_preset(water, preset, getDir());
      fft_water::setWaterCell(water, 0.25f, true);
      fft_water::set_water_dim(water, 7);
    }
    CONSOLE_CHECK_NAME("water", "foam_hats", 1, 4)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.foam_hats <hats_mul> <hats_threshold> <hats_folding>");
        console::print_d("Current: water.foam_hats %0.3f %0.3f %0.3f", preset.foam.hats_mul, preset.foam.hats_threshold,
          preset.foam.hats_folding);
        return true;
      }

      if (argc > 1)
        preset.foam.hats_mul = atof(argv[1]);
      if (argc > 2)
        preset.foam.hats_threshold = atof(argv[2]);
      if (argc > 3)
        preset.foam.hats_folding = atof(argv[3]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "surface_folding_foam", 1, 4)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.surface_folding_foam <mul> <pow>");
        console::print_d("Current: water.surface_folding_foam %0.3f %0.3f", preset.foam.surface_folding_foam_mul,
          preset.foam.surface_folding_foam_pow);
        return true;
      }

      if (argc > 1)
        preset.foam.surface_folding_foam_mul = atof(argv[1]);
      if (argc > 2)
        preset.foam.surface_folding_foam_pow = atof(argv[2]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "foam_turbulent", 1, 5)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.foam_turbulent <generation_threshold> <generation_amount> <dissipation_speed> <falloff_speed>");
        console::print_d("Current: water.foam_turbulent %0.3f %0.3f %0.3f %0.3f", preset.foam.generation_threshold,
          preset.foam.generation_amount, preset.foam.dissipation_speed, preset.foam.falloff_speed);
        return true;
      }

      if (argc > 1)
        preset.foam.generation_threshold = atof(argv[1]);
      if (argc > 2)
        preset.foam.generation_amount = atof(argv[2]);
      if (argc > 3)
        preset.foam.dissipation_speed = atof(argv[3]);
      if (argc > 4)
        preset.foam.falloff_speed = atof(argv[4]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "dependency_wind", 1, 2)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.wind_dependency <size>");
        console::print_d("Current: water.wind_dependency %0.3f", saturate(preset.simulation.windDependency));
        return true;
      }

      preset.simulation.windDependency = atof(argv[1]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "alignment_wind", 1, 2)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.alignment_wind <size>");
        console::print_d("Current: water.alignment_wind %0.3f", preset.simulation.windAlignment);
        return true;
      }

      preset.simulation.windAlignment = atof(argv[1]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "choppiness", 1, 2)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.choppiness <size>");
        console::print_d("Current: water.choppiness %0.3f", saturate(preset.simulation.choppiness));
        return true;
      }

      preset.simulation.choppiness = atof(argv[1]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "facet_size", 1, 2)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.facet_size <size>");
        console::print_d("Current: water.facetSize %0.3f", preset.simulation.facetSize);
        return true;
      }

      preset.simulation.facetSize = atof(argv[1]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "amplitude", 1, 6)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.amplitude <a01234> or water.amplitude <a0> <a1> <a2> <a3> <a4>");
        console::print_d("Current: water.amplitude %0.3f %0.3f %0.3f %0.3f %0.3f", preset.simulation.amplitude[0],
          preset.simulation.amplitude[1], preset.simulation.amplitude[2], preset.simulation.amplitude[3],
          preset.simulation.amplitude[4]);
        return true;
      }

      if (argc == 2)
      {
        float amplitude = atof(argv[1]);
        for (int i = 0; i < fft_water::MAX_NUM_CASCADES; i++)
          preset.simulation.amplitude[i] = amplitude;
      }
      for (int i = 1; i < eastl::min(argc, (int)fft_water::MAX_NUM_CASCADES + 1); i++)
        preset.simulation.amplitude[i - 1] = atof(argv[i]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "small_wave_fraction", 1, 2)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc == 1)
      {
        console::print("Usage: water.smallWaveFraction <size>");
        console::print_d("Current: water.smallWaveFraction %0.5f", preset.smallWaveFraction);
        return true;
      }

      preset.smallWaveFraction = atof(argv[1]);

      fft_water::apply_wave_preset(water, preset, getDir());
    }
    CONSOLE_CHECK_NAME("water", "cascade_window_length", 1, 2)
    {
      if (argc == 1)
      {
        console::print("Usage: water.cascade_window_length <size>");
        console::print_d("Current: water.cascade_window_length %0.5f", fft_water::get_cascade_window_length(water));
        return true;
      }
      fft_water::set_cascade_window_length(water, atof(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "cascade_facet_size", 1, 2)
    {
      if (argc == 1)
      {
        console::print("Usage: water.cascade_facet_size <size>");
        console::print_d("Current: water.cascade_facet_size %0.5f", fft_water::get_cascade_facet_size(water));
        return true;
      }
      fft_water::set_cascade_facet_size(water, atof(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "roughness", 1, 3)
    {
      if (argc < 3)
      {
        float roughnessBase = 0, cascadesRoughnessBase = 0;
        console::print("Usage: water.roughness <base> <cascadesBase>");
        console::print_d("Current: water.roughness %0.3f %0.3f", roughnessBase, cascadesRoughnessBase);
        return true;
      }
      fft_water::set_roughness(water, atof(argv[1]), atof(argv[2]));
    }
    CONSOLE_CHECK_NAME("water", "fft_resolution", 1, 2)
    {
      if (argc == 1)
      {
        console::print("Usage: water.fft_resolution <size>");
        console::print_d("Current: water.fft_resolution %0d", fft_water::get_fft_resolution(water));
        return true;
      }
      fft_water::set_fft_resolution(water, atoi(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "tesselation", 2, 2)
    {
      fft_water::setWaterCell(water, argc > 1 && atoi(argv[1]) == 2 ? 0.25f : (argc > 1 && atoi(argv[1]) == 1 ? 0.5f : 1.0f), true);
      fft_water::set_water_dim(water, argc > 1 && atoi(argv[1]) == 2 ? 7 : (argc > 1 && atoi(argv[1]) == 1 ? 6 : 5));
    }
    CONSOLE_CHECK_NAME("water", "fft_period", 1, 2)
    {
      if (argc == 1)
      {
        console::print("Usage: water.fft_period <size>");
        console::print_d("Current: water.fft_period %0.5f", fft_water::get_period(water));
        return true;
      }
      fft_water::set_period(water, atof(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "spectrum", 1, 3)
    {
      fft_water::WavePreset preset;
      fft_water::get_wave_preset(water, preset);

      if (argc < 3)
      {
        console::print("Usage: water.spectrum <spectra index> <bf_scale>");
        for (int i = 0; i < countof(fft_water::spectrum_names); ++i)
          console::print("%s = %d", fft_water::spectrum_names[i], i);
        console::print_d("Current: water.spectrum %d(%s)", (int)preset.simulation.spectrum,
          fft_water::spectrum_names[(int)preset.simulation.spectrum]);
        return true;
      }

      fft_water::apply_wave_preset(water, atof(argv[2]), getDir(), (fft_water::Spectrum)atoi(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "vs_samplers", 2, 2)
    {
      fft_water::setVertexSamplers(water, atoi(argv[1]));
      console::print_d("water wind vertex samplers %d", atoi(argv[1]));
    }
    CONSOLE_CHECK_NAME("water", "reset_render", 1, 1) { fft_water::reset_render(water); }
    CONSOLE_CHECK_NAME("water", "num_cascades", 1, 2)
    {
      if (argc == 1)
      {
        console::print("Usage: water.num_cascades <size>");
        console::print_d("Current: water.num_cascades %d", fft_water::get_num_cascades(water));
        return true;
      }
      fft_water::set_num_cascades(water, atof(argv[1]));
    }
    return found;
  }
};

static InitOnDemand<FFTWaterCmdProcessor> water_consoleproc;
#endif

namespace fft_water
{
void init() { init_nv_wave_works(); }
void close() { close_nv_wave_works(); }

FFTWater *create_water(RenderQuality quality, float period, int res_bits, bool depth_renderer, bool ssr_renderer,
  bool one_to_four_cascades, int min_render_res_bits, RenderQuality geom_quality, bool water_heightmap_draw_patches,
  int enforce_render_cascade_count, bool chop_generator_init)
{
  FFTWater *water = new FFTWater(fft_water::DEFAULT_NUM_CASCADES, min_render_res_bits);
  water->setFFTRenderResolution(res_bits);
  water->setPeriod(period);
  if (quality != DONT_RENDER)
    water->initRender(quality, geom_quality != RenderQuality::UNDEFINED ? geom_quality : min(quality, RENDER_GOOD), depth_renderer,
      ssr_renderer, one_to_four_cascades, water_heightmap_draw_patches, enforce_render_cascade_count);
  water->initPhysics();
  water->simulateAllAt(0, false); // sim only FFT on creation

  if (chop_generator_init)
  {
    water->initChopWaterGen();
    water->initPhysicsChop();
  }

#if DAGOR_DBGLEVEL > 0
  water_consoleproc.demandInit();
  water_consoleproc->water = water;
  add_con_proc(water_consoleproc);
#endif

  return water;
}
void init_render(FFTWater *water, int quality, bool depth_renderer, bool ssr_renderer, bool one_to_four_cascades,
  RenderQuality geom_quality, bool water_heightmap_draw_patches, int enforce_render_cascade_count)
{
  if (!water)
    return;
  if (quality != DONT_RENDER)
    water->initRender(quality,
      geom_quality != RenderQuality::UNDEFINED ? geom_quality : min(static_cast<RenderQuality>(quality), RenderQuality::RENDER_GOOD),
      depth_renderer, ssr_renderer, one_to_four_cascades, water_heightmap_draw_patches, enforce_render_cascade_count);
  else
    water->closeRender();
}
bool one_to_four_render_enabled(const FFTWater *water)
{
  if (water && water->getRender())
    return water->getRender()->getOneToFourCascades();
  return false;
}
void set_grid_lod0_additional_tesselation(FFTWater *water, float amount)
{
  if (water && water->getRender())
    water->getRender()->setGridLod0AdditionalTesselation(amount);
  if (water && water->getRenderChop())
    water->getRenderChop()->setGridLod0AdditionalTesselation(amount);
}
void set_grid_lod0_area_radius(FFTWater *water, float radius)
{
  if (water && water->getRender())
    water->getRender()->setLod0AreaSize(radius);
  if (water && water->getRenderChop())
    water->getRenderChop()->setLod0AreaSize(radius);
}
void set_last_lod_extension(FFTWater *water, float extension)
{
  if (water && water->getRender())
    water->getRender()->setLastLodExtension(extension);
  if (water && water->getRenderChop())
    water->getRenderChop()->setLastLodExtension(extension);
}
void set_period(FFTWater *water, float period)
{
  if (water)
    water->setPeriod(period);
}
float get_period(const FFTWater *water) { return water ? water->getPeriod() : 0.f; }
void delete_water(FFTWater *&water)
{
#if DAGOR_DBGLEVEL > 0
  if (water_consoleproc && water_consoleproc->water == water)
    water_consoleproc->water = NULL;
#endif
  del_it(water);
}
void set_render_quad(FFTWater *water, const BBox2 &quad)
{
  if (water && water->getRender())
    water->getRender()->setRenderQuad(quad);
  if (water && water->getRenderChop())
    water->getRenderChop()->setRenderQuad(quad);
}
void simulate(FFTWater *water, double time)
{
  if (!water)
    return;

  const bool chopEnabled = water->chopEnabled();
  const bool hasAnyRenderer = water->getRender() || water->getRenderChop();

  // update shader consts after changing generators
  if (hasAnyRenderer && chopEnabled != water->getRenderCommon().getChopEnabled())
  {
    water->getRenderCommon().setMaxWaveHeight(water->getMaxWaveHeight());
    water->getRenderCommon().setChopEnabled(chopEnabled);
    water->updateHeightCullingWaveHeight();
  }
  // apply wind_speed_chop changed via convar
  if (chopEnabled && water->getChopWaterGen() && fabs(water->getChopWaterGen()->getWindSpeed() - convar::wind_speed_chop) > 0.05f)
  {
    float fftSpeed;
    Point2 windDir;
    get_wind_speed(water, fftSpeed, windDir);
    set_wind_speed(water, fftSpeed, convar::wind_speed_chop, windDir);
  }

  if (chopEnabled && !hasAnyRenderer) // server-case, for client see before_render
  {
    if (!water->getChopWaterGen() || !water->getPhysicsChop())
    {
      water->initChopWaterGen();
      water->initPhysicsChop();
    }
    water->getChopWaterGen()->Update(water->getLastTime(), false);
  }
  water->simulateAllAt(time, chopEnabled);
}
void before_render(const FFTWater *water)
{
  if (water->getRender() || water->getRenderChop())
  {
    ShaderGlobal::set_int(chop_water_enabledVarId, water->chopEnabled() ? 1 : 0);
    water->getRenderCommon().setGlobalShaderConsts(water->chopEnabled());
  }
  if (water->chopEnabled())
  {
    if (water->getRenderChop()) // client-case
    {
      water->getChopWaterGen()->Update(water->getLastTime(), water->getRenderChop()->isDetailWavesEnabled());
    }
  }
  else
  {
    if (!water->getRender())
      return;
    water->getRender()->simulateAllAt(water->getLastTime());
    water->getRender()->updateTexturesAll();
    water->getRender()->calculateGradients();
  }
}

float getGridLod0AreaSize(const FFTWater *water)
{
  if (water->chopEnabled() && water->getRenderChop())
  {
    return water->getRenderChop()->getLod0AreaSize();
  }
  if (!water->getRender())
    return 0.f;
  return water->getRender()->getLod0AreaSize();
}

void setGridLod0AdditionalTesselation(FFTWater *water, float additional_tesselation)
{
  if (water->getRender() != NULL)
    water->getRender()->setGridLod0AdditionalTesselation(additional_tesselation);
  if (water->getRenderChop() != NULL)
    water->getRenderChop()->setGridLod0AdditionalTesselation(additional_tesselation);
}

void render(const FFTWater *water, const Point3 &pos, TEXTUREID distance_tex_id, const Frustum &frustum, Occlusion *occlusion,
  const Driver3dPerspective &persp, int geom_lod_quality, int survey_id, IWaterDecalsRenderHelper *decals_renderer,
  RenderMode render_mode, eastl::function<bool(const Point3_vec4 &pos, const Point3_vec4 &posRB)> cullCb)
{
  TIME_D3D_PROFILE(fft_water_render);

  if (water->chopEnabled())
  {
    if (water->getRenderChop())
      water->getRenderChop()->render(pos, distance_tex_id, geom_lod_quality, survey_id, frustum, occlusion, persp,
        water->getRenderCommon(), decals_renderer, render_mode);
  }
  else
  {
    if (!water->getRender())
      return;
    // render() call runs begin_survey, because we have a fence inside function, fence cannot be between
    // start survey and end survey. So here we should just call end_survey.
    water->getRender()->render(pos, distance_tex_id, geom_lod_quality, survey_id, frustum, occlusion, persp, water->getRenderCommon(),
      decals_renderer, render_mode, cullCb);

    d3d::end_survey(survey_id);
  }
}

float get_level(const FFTWater *water) { return water->getLevel(); }
void set_level(FFTWater *water, float level) { water->setLevel(level); }
float get_min_level(const FFTWater *water) { return water->getMinLevel(); }
float get_max_level(const FFTWater *water) { return water->getMaxLevel(); }
void set_min_max_level(FFTWater *water, float min_level, float max_level) { water->setMinMaxLevel(min_level, max_level); }
float get_height(const FFTWater *water, const Point3 &point) { return water->getHeight(point); }
float get_min_height(const FFTWater *water) { return water->getMinHeight(); }
float get_max_height(const FFTWater *water) { return water->getMaxHeight(); }
float get_max_wave_height(const FFTWater *water) { return water->getMaxWaveHeight(); }
float get_significant_wave_height(const FFTWater *water) { return water->getSignificantWaveHeight(); }
void set_wave_displacement_distance(FFTWater *water, const Point2 &value)
{
  if (water && water->getRender())
    water->getRender()->setWaveDisplacementDistance(value);
}

void shore_enable(FFTWater *water, bool enable)
{
  if (water && water->getRenderCommon().isInited())
    water->getRenderCommon().shoreEnable(enable);
}

bool is_shore_enabled(const FFTWater *water)
{
  if (water && water->getRenderCommon().isInited())
    return water->getRenderCommon().isShoreEnabled();
  return false;
}

float get_shore_wave_threshold(const FFTWater *water)
{
  if (water && water->getRenderCommon().isInited())
    return water->getRenderCommon().getShoreWaveThreshold();
  return 0;
}

void set_shore_wave_threshold(FFTWater *water, float value)
{
  if (water && water->getRenderCommon().isInited())
    water->getRenderCommon().setShoreWaveThreshold(value);
}

float get_shore_damp_min(const FFTWater *water)
{
  if (water && water->getRenderCommon().isInited())
    return water->getRenderCommon().getShoreDampMin();
  return 0.f;
}

void set_shore_damp_min(FFTWater *water, float value)
{
  if (water && water->getRenderCommon().isInited())
    water->getRenderCommon().setShoreDampMin(value);
}

int get_fft_resolution(const FFTWater *water)
{
  if (water)
    return water->getFFTRenderResolution();
  return TARGET_FFT_RESOLUTION;
}

void set_fft_resolution(FFTWater *water, int res_bits)
{
  if (water)
    water->setFFTRenderResolution(res_bits);
}

void reset_render(FFTWater *water)
{
  if (water)
    water->resetRender();
}

int get_num_cascades(const FFTWater *water)
{
  if (water)
    return water->getNumCascades();
  return fft_water::DEFAULT_NUM_CASCADES;
}

void set_num_cascades(FFTWater *water, int cascades)
{
  if (water)
    water->setNumCascades(cascades);
}

void set_render_quality(FFTWater *water, int quality, bool depth_renderer, bool ssr_renderer, int enforce_render_cascade_count)
{
  if (water && water->getRender() && water->getRender()->getQuality() != quality)
    water->initRender(quality, water->getRender()->getGeomQuality(), depth_renderer, ssr_renderer,
      water->getRender()->getOneToFourCascades(), water->getRender()->isWaterHeightmapDrawPatches(), enforce_render_cascade_count);
}

void setAnisotropy(FFTWater *water, int aniso, float mip_bias)
{
  if (water && water->getRender())
    water->getRender()->setAnisotropy(aniso, mip_bias);
}

float get_small_wave_fraction(const FFTWater *water)
{
  if (water)
    return water->getSmallWaveFraction();
  return 0.0f;
}

void set_small_wave_fraction(FFTWater *water, float smallWaveFraction)
{
  if (water)
    water->setSmallWaveFraction(smallWaveFraction);
}

float get_cascade_window_length(const FFTWater *water)
{
  if (water)
    return water->getCascadeWindowLength();
  return 0.0f;
}

void set_cascade_window_length(FFTWater *water, float value)
{
  if (water)
    water->setCascadeWindowLength(value);
}

float get_cascade_facet_size(const FFTWater *water)
{
  if (water)
    return water->getCascadeFacetSize();
  return 0.0f;
}

void set_cascade_facet_size(FFTWater *water, float value)
{
  if (water)
    water->setCascadeFacetSize(value);
}

SimulationParams get_simulation_params(const FFTWater *water)
{
  if (water)
    return water->getSimulationParams();
  return SimulationParams();
}

void set_simulation_params(FFTWater *water, const SimulationParams &scales)
{
  if (water)
    water->setSimulationParams(scales);
}

void set_foam(FFTWater *water, const FoamParams &params)
{
  if (water && water->getRender())
    water->getRender()->setFoamParams(params);
}

FoamParams get_foam(const FFTWater *water)
{
  if (water && water->getRender())
    return water->getRender()->getFoamParams();
  return FoamParams();
}

void set_chop_water_props(FFTWater *water, const ChopWaterProps &props)
{
  if (water && water->getChopWaterGen())
  {
    water->getChopWaterGen()->setProps(props);
    const float curChopWindSpeed = water->getChopWaterGen()->getWindSpeed();
    if (fabs(curChopWindSpeed - props.wind_speed) > 0.05f)
    {
      float fftSpeed;
      Point2 windDir;
      get_wind_speed(water, fftSpeed, windDir);
      set_wind_speed(water, fftSpeed, props.wind_speed, windDir);
    }
  }
}

ChopWaterProps get_chop_water_props(const FFTWater *water)
{
  if (water && water->getChopWaterGen())
    return water->getChopWaterGen()->getProps();
  return ChopWaterProps();
}

void create_chop_water_renderer(FFTWater *water, TEXTUREID chopWaterDetailCombined, TEXTUREID foamDissolveTex,
  TEXTUREID whiteNoise64Tex, TEXTUREID detailWaveletTexture)
{
  water->getChopWaterGen()->setWaveletTextures(chopWaterDetailCombined, foamDissolveTex, whiteNoise64Tex, detailWaveletTexture);
  if (!water->getRenderChop())
    water->initRenderChop();
}

void set_chop_water_enabled(bool chop_enabled) { convar::chop_gen = chop_enabled; }
bool get_chop_water_enabled() { return convar::chop_gen; }

void enable_graphic_feature(FFTWater *water, GraphicFeature feature, bool enable)
{
#if DAGOR_DBGLEVEL > 0
  if (feature < 0 || feature >= GRAPHIC_FEATURE_END)
    return;
  if (water && water->getRender())
    water->getRender()->enableGraphicFeature(feature, enable);
#else
  G_UNREFERENCED(water);
  G_UNREFERENCED(feature);
  G_UNREFERENCED(enable);
#endif
}

void get_cascade_period(const FFTWater *water, int cascade_no, float &out_period, float &out_window_in, float &out_window_out)
{
#if DAGOR_DBGLEVEL > 0
  if (water && water->getRender())
    water->getRender()->getCascadePeriod(cascade_no, out_period, out_window_in, out_window_out);
#else
  G_UNREFERENCED(water);
  G_UNREFERENCED(cascade_no);
  G_UNREFERENCED(out_period);
  G_UNREFERENCED(out_window_in);
  G_UNREFERENCED(out_window_out);
#endif
}

void set_current_time(FFTWater *water, double time) { return water->setCurrentTime(time); }
void reset_physics(FFTWater *water) { water->resetPhysics(); }
void wait_physics(FFTWater *water) { water->waitPhysics(); }
bool validate_next_time_tick(const FFTWater *water, double next_time) { return water->validateNextTimeTick(next_time); }
int intersect_segment(const FFTWater *water, const Point3 &start, const Point3 &end, float &result)
{
  if (water->chopEnabled())
  {
    return water->getPhysicsChop()->intersectSegment(water->getCurrentTime(), start, end, result);
  }
  return water->getPhysics()->intersectSegment(water->getCurrentTime(), start, end, result);
}
int intersect_segment_at_time(const FFTWater *water, double time, const Point3 &start, const Point3 &end, float &result)
{
  if (water->chopEnabled())
  {
    return water->getPhysicsChop()->intersectSegment(time, start, end, result);
  }
  return water->getPhysics()->intersectSegment(time, start, end, result);
}
int getHeightAboveWater(const FFTWater *water, const Point3 &in_point, float &result, bool matchRenderGrid)
{
  if (water->chopEnabled())
  {
    return water->getPhysicsChop()->getHeightAboveWater(water->getCurrentTime(), in_point, result, nullptr, matchRenderGrid);
  }
  return water->getPhysics()->getHeightAboveWater(water->getCurrentTime(), in_point, result, nullptr, matchRenderGrid);
}
void setRenderParamsToPhysics(FFTWater *water)
{
  if (!water->getRender())
    return;
  float gridAlign;
  Point2 gridOffset;
  water->getRender()->getGridDataAtCamera(gridAlign, gridOffset);
  if (water->getPhysics())
    water->getPhysics()->setRenderParams(gridAlign, gridOffset);
  if (water->getPhysicsChop())
    water->getPhysicsChop()->setRenderParams(gridAlign, gridOffset);
}
int getHeightAboveWaterAtTime(const FFTWater *water, double at_time, const Point3 &in_point, float &result, Point3 *displacement)
{
  if (water->chopEnabled())
  {
    return water->getPhysicsChop()->getHeightAboveWater(at_time, in_point, result, displacement);
  }
  return water->getPhysics()->getHeightAboveWater(at_time, in_point, result, displacement);
}
void get_wind_speed(const FFTWater *water, float &out_speed, Point2 &out_wind_dir) { water->getWind(out_speed, out_wind_dir); }
void set_wind_speed(FFTWater *water, float speed, float chop_wind_speed, const Point2 &wind_dir)
{
  water->setWind(speed, chop_wind_speed, wind_dir);
}
void get_roughness(const FFTWater *water, float &out_roughness_base, float &out_cascades_roughness_base)
{
  out_roughness_base = 0;
  out_cascades_roughness_base = 0;
  if (water && water->getRender())
    water->getRender()->getRoughness(out_roughness_base, out_cascades_roughness_base);
}
void set_roughness(FFTWater *water, float roughness_base, float cascades_roughness_base)
{
  if (water && water->getRender())
    water->getRender()->setRoughness(roughness_base, cascades_roughness_base);
}

void setVertexSamplers(FFTWater *water, int samplersCount)
{
  if (!water->getRender()) // FFT only
    return;
  water->getRender()->setVertexSamplers(samplersCount); // FFT only
}
int setWaterCell(FFTWater *water, float water_cell_size, bool auto_set_samplers_cnt)
{
  if (!water->getRender())
    return 0;
  return water->getRender()->setWaterCell(water_cell_size, auto_set_samplers_cnt);
}
void set_water_dim(FFTWater *water, int dim_bits)
{
  if (water->getRender())
    water->getRender()->setWaterDim(dim_bits);
  if (water->getRenderChop())
    water->getRenderChop()->setWaterDim(dim_bits);
}

float get_render_significant_wave_height(FFTWater *water)
{
  if (water && water->getRender())
    return water->getRender()->getSignificantWaveHeight();
  return 0;
}

void setWakeHtTex(FFTWater *water, TEXTUREID wake_ht_tex_id) { water->getRenderCommon().setWakeHtTex(wake_ht_tex_id); }

void force_actual_waves(FFTWater *water, bool enforce)
{
  if (water->getRender())
  {
    water->getPhysics()->setForceActualWaves(enforce);
    water->getRender()->setForceTessellation(enforce);
  }
}

void force_physics_waves(FFTWater *water, bool enforce)
{
  if (water->getPhysics())
    water->getPhysics()->setForceActualWaves(enforce);
}

void force_render_waves(FFTWater *water, bool enforce)
{
  if (water->getRender())
    water->getRender()->setForceTessellation(enforce);
}

fft_water::WaterFlowmap *get_flowmap(const FFTWater *water)
{
  if (water)
    return water->getFlowmap();
  return nullptr;
}

void create_flowmap(FFTWater *water)
{
  if (water)
    water->createFlowmap();
}

void remove_flowmap(FFTWater *water)
{
  if (water)
    water->removeFlowmap();
}

const fft_water::WaterHeightmap *get_heightmap(const FFTWater *water) { return water->getHeightmap(); }
const HeightmapHeightCulling *get_heightmap_culling(const FFTWater *water) { return water->getHeightmapCulling(); }
void set_heightmap(FFTWater *water, eastl::unique_ptr<WaterHeightmap> &&heightmap) { water->setHeightmap(eastl::move(heightmap)); }
void remove_heightmap(FFTWater *water) { water->removeHeightmap(); }
void set_heightmap_extra_boundings(FFTWater *water, const dag::Vector<BBox3> &extra_boundings)
{
  water->setHeightmapExtraBoundings(extra_boundings);
}

size_t WaterHeightmap::calcDumpSize() const
{
  return //
    POW2_ALIGN(elem_size(pages) * pagesX * pagesY * PAGE_SIZE_PADDED * PAGE_SIZE_PADDED + elem_size(grid) * gridSize * gridSize, 4) +
    elem_size(patchHeights) * PATCHES_GRID_SIZE * PATCHES_GRID_SIZE;
}
void WaterHeightmap::arrangeDataLayout(size_t dump_sz)
{
  G_ASSERT(dataPtr);
  pages.set(POW2_ALIGN_PTR(dataPtr, 2, uint16_t), pagesX * pagesY * PAGE_SIZE_PADDED * PAGE_SIZE_PADDED);
  grid.set(POW2_ALIGN_PTR(pages.end(), 2, uint16_t), gridSize * gridSize);
  patchHeights.set(POW2_ALIGN_PTR(grid.end(), 4, Point2), PATCHES_GRID_SIZE * PATCHES_GRID_SIZE);
  G_ASSERTF(uintptr_t(patchHeights.end()) <= uintptr_t(dataPtr) + dump_sz,
    "data={%p..%p, %d} pages={%p..%p, %d} grid={%p..%p, %d} patchHeights={%p..%p, %d}", //
    dataPtr, dump_sz + (char *)dataPtr, dump_sz, pages.data(), pages.end(), pages.size(), grid.data(), grid.end(), grid.size(),
    patchHeights.data(), patchHeights.end(), patchHeights.size());
}
void WaterHeightmap::loadData(IGenLoad &crd)
{
  unsigned fmt = 0;
  crd.beginBlock(&fmt);
  IGenLoad *zcrd = NULL;
  if (fmt == btag_compr::OODLE)
    zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(crd, crd.getBlockRest(), data_size(pages) + data_size(grid));
  else if (fmt == btag_compr::ZSTD)
    zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, crd.getBlockRest());
  else
    zcrd = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, crd.getBlockRest());

  uint16_t *pages_data = const_cast<uint16_t *>(pages.data());
  uint16_t *grid_data = const_cast<uint16_t *>(grid.data());
  zcrd->read(pages_data, data_size(pages));
  zcrd->read(grid_data, data_size(grid));
  zcrd->ceaseReading();
  zcrd->~IGenLoad();
  for (int w = pagesX * PAGE_SIZE_PADDED, h = pagesY * PAGE_SIZE_PADDED, y = 0; y < h; ++y, pages_data += w)
    for (int x = 1; x < w; ++x)
      pages_data[x] = (uint16_t)(int(pages_data[x]) + pages_data[x - 1]);
  crd.endBlock();
  int rest = crd.getBlockRest();
  if (rest >= data_size(patchHeights))
    crd.readExact(const_cast<Point2 *>(patchHeights.data()), data_size(patchHeights));
  else
  {
    memset(const_cast<Point2 *>(patchHeights.data()), 0, data_size(patchHeights));
    logerr("Missing water heightmap patches data. Level re-export is needed for correct water heightmap rendering");
  }
}
WaterHeightmap::~WaterHeightmap()
{
  pages.reset();
  grid.reset();
  patchHeights.reset();
  if (dataPtr && sharedMem && sharedMem->doesPtrBelong(dataPtr))
    sharedMem->releasePtr(SM_DATA_TAG, dataPtr);
  else if (dataPtr)
    memfree(dataPtr, midmem);
  extraBoundings.clear();
}

void load_heightmap(IGenLoad &loadCb, FFTWater *water)
{
  char sm_ptr_name[256];
  SNPRINTF(sm_ptr_name, sizeof(sm_ptr_name), "%s:%X", loadCb.getTargetName(), loadCb.tell());

  eastl::unique_ptr<WaterHeightmap> heightmap(new WaterHeightmap());
  heightmap->gridSize = loadCb.readInt();
  heightmap->pagesX = loadCb.readInt();
  heightmap->pagesY = loadCb.readInt();
  heightmap->scale = loadCb.readInt();
  heightmap->heightOffset = loadCb.readReal();
  heightmap->heightScale = loadCb.readReal();
  heightmap->tcOffsetScale.x = loadCb.readReal();
  heightmap->tcOffsetScale.y = loadCb.readReal();
  heightmap->tcOffsetScale.z = loadCb.readReal();
  heightmap->tcOffsetScale.w = loadCb.readReal();

  size_t dump_sz = heightmap->calcDumpSize();
  if (GlobalSharedMemStorage *sm = WaterHeightmap::sharedMem)
  {
    bool smNew = false;
    heightmap->dataPtr = sm->findOrAlloc(sm_ptr_name, WaterHeightmap::SM_DATA_TAG, dump_sz, smNew);
    if (heightmap->dataPtr && !smNew)
    {
      logmessage(_MAKE4C('SHMM'), "reusing Water-HMAP dump from shared mem: %p, %dK, '%s'", heightmap->dataPtr, dump_sz >> 10,
        sm_ptr_name);
      G_ASSERTF_RETURN(dump_sz <= sm->getPtrSize(heightmap->dataPtr), , "dump_sz=%d, foundPtrSize=%d", dump_sz,
        sm->getPtrSize(heightmap->dataPtr));
      heightmap->arrangeDataLayout(dump_sz);
      return set_heightmap(water, eastl::move(heightmap));
    }
    if (heightmap->dataPtr)
    {
      logmessage(_MAKE4C('SHMM'), "allocated Water-HMAP dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d)",
        heightmap->dataPtr, dump_sz >> 10, sm_ptr_name, ((uint64_t)sm->getMemUsed()) >> 10, ((uint64_t)sm->getMemSize()) >> 10,
        sm->getRecUsed());
      heightmap->arrangeDataLayout(dump_sz);
      heightmap->loadData(loadCb);
      mark_global_shared_mem_readonly(heightmap->dataPtr, dump_sz, true);
      sm->markPtrDataReady(heightmap->dataPtr);
      return set_heightmap(water, eastl::move(heightmap));
    }
    logmessage(_MAKE4C('SHMM'),
      "failed to allocate HMAP dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d); "
      "falling back to conventional allocator",
      heightmap->dataPtr, dump_sz >> 10, sm_ptr_name, ((uint64_t)sm->getMemUsed()) >> 10, ((uint64_t)sm->getMemSize()) >> 10,
      sm->getRecUsed());
  }
  if (!heightmap->dataPtr)
    heightmap->dataPtr = memalloc(dump_sz, midmem);
  G_ASSERT_RETURN(heightmap->dataPtr, );
  heightmap->arrangeDataLayout(dump_sz);
  heightmap->loadData(loadCb);
  set_heightmap(water, eastl::move(heightmap));
}

bool WaterHeightmap::getHeightmapDataBilinear(float x, float z, float &result) const
{
  x = (x * tcOffsetScale.z + tcOffsetScale.x) * gridSize;
  z = (z * tcOffsetScale.w + tcOffsetScale.y) * gridSize;
  if (x < 0.0f || z < 0.0f || (int)x > gridSize - 1 || (int)z > gridSize - 1)
    return false;
  uint16_t cellData = grid[((int)z) * gridSize + (int)x];
  if (cellData == 0xFFFF)
    return false;
  int pageSize = PAGE_SIZE_PADDED;
  int gridX = (cellData >> 1) & 0x7F;
  int gridZ = (cellData >> 8) & 0xFF;
  float crdScale = (cellData & 1) ? 1.0f : scale;
  x = fmod(x / crdScale, 1.0f);
  z = fmod(z / crdScale, 1.0f);
  float pageScale = (HEIGHTMAP_PAGE_SIZE / (HEIGHTMAP_PAGE_SIZE + 2.0f));
  float pageOffset = 1.0f / (HEIGHTMAP_PAGE_SIZE + 2.0f);
  x = (x * pageScale + pageOffset) * pageSize;
  z = (z * pageScale + pageOffset) * pageSize;
  int crdX = clamp((int)x, 1, pageSize - 2);
  int crdZ = clamp((int)z, 1, pageSize - 2);
  crdX += gridX * pageSize;
  crdZ += gridZ * pageSize;
  int w = pageSize * pagesX;
  int lb = crdZ * w + crdX;
  int lt = (crdZ + 1) * w + crdX;
  int rb = crdZ * w + crdX + 1;
  int rt = (crdZ + 1) * w + crdX + 1;
  float wr = x - (int)x;
  float wt = z - (int)z;
  float wl = 1.0f - wr;
  float wb = 1.0f - wt;
  const uint16_t *heights = pages.data();
  G_ASSERT(rt < pages.size());
  float height = (heights[lb] * wb + heights[lt] * wt) * wl + (heights[rb] * wb + heights[rt] * wt) * wr;
  result = round(height) * (heightScale / UINT16_MAX) + heightOffset;
  return true;
}

bool WaterHeightmap::isFlat(int x, int z) const
{
  uint16_t cellData = grid[((int)z) * gridSize + (int)x];
  return cellData == 0xFFFF;
}

bool WaterHeightmap::isDetailed(int x, int z) const
{
  uint16_t cellData = grid[((int)z) * gridSize + (int)x];
  return cellData & 1;
}

float WaterHeightmap::getMaxUpwardDisplacement() const { return waveCullingMargin; }

float WaterHeightmap::getMaxDownwardDisplacement() const { return waveCullingMargin; }

IPoint2 WaterHeightmap::getHeightmapSize() const
{
  // Cell = page, return the count of pages
  return {gridSize, gridSize};
}

Point3 WaterHeightmap::getHeightmapOffset() const
{
  return {-tcOffsetScale.x / tcOffsetScale.z, heightOffset, -tcOffsetScale.y / tcOffsetScale.w};
}

float WaterHeightmap::getHeightmapCellSize() const { return max(1.f / tcOffsetScale.z / gridSize, 1.f / tcOffsetScale.w / gridSize); }

bool WaterHeightmap::getHeightmapHeightMinMaxInChunk(const Point2 &pos, const real &chunk_size, real &hmin, real &hmax) const
{
  const Point2 origin = Point2(-tcOffsetScale.x / tcOffsetScale.z, -tcOffsetScale.y / tcOffsetScale.w);
  const Point2 pageSize = Point2(1.f / tcOffsetScale.z, 1.f / tcOffsetScale.w) / gridSize;
  const Point2 texelSize = pageSize / HEIGHTMAP_PAGE_SIZE;

  // Add a texel to account for region of interpolation between two texels
  const Point2 extendedStartPos = pos - texelSize;
  const Point2 extendedChunkSize = Point2(chunk_size, chunk_size) + texelSize;

  IPoint2 pageStart = IPoint2((extendedStartPos.x * tcOffsetScale.z + tcOffsetScale.x) * gridSize,
    (extendedStartPos.y * tcOffsetScale.w + tcOffsetScale.y) * gridSize);
  IPoint2 pageEnd = IPoint2(((extendedStartPos.x + extendedChunkSize.x) * tcOffsetScale.z + tcOffsetScale.x) * gridSize,
    ((extendedStartPos.y + extendedChunkSize.y) * tcOffsetScale.w + tcOffsetScale.y) * gridSize);
  if (pageStart.x >= gridSize || pageStart.y >= gridSize || pageEnd.x < 0 || pageEnd.y < 0 || pageEnd.x < pageStart.x ||
      pageEnd.y < pageStart.y)
  {
    hmin = hmax = waterLevel;
    return false;
  }
  pageStart = {max(0, pageStart.x), max(0, pageStart.y)};
  pageEnd = {min(pageEnd.x, gridSize - 1), min(pageEnd.y, gridSize - 1)};

  hmin = 1000000;
  hmax = -1000000;
  for (int pageY = pageStart.y; pageY <= pageEnd.y; ++pageY)
  {
    for (int pageX = pageStart.x; pageX <= pageEnd.x; ++pageX)
    {
      // Flat (no-data) pages are rendered at the global water level, not at heightOffset, so treat such a page as a sample at
      // waterLevel. Otherwise flat ocean gets a culling box offset from where it is actually drawn and pops out of the frustum.
      if (grid[pageY * gridSize + pageX] == 0xFFFF)
      {
        hmin = min(hmin, waterLevel);
        hmax = max(hmax, waterLevel);
        continue;
      }

      const Point2 pageStartPos = origin + mul(pageSize, Point2(pageX, pageY));
      const Point2 pageEndPos = pageStartPos + pageSize;

      Point2 from = extendedStartPos;
      from = mul(floor(div(from - pageStartPos, texelSize)), texelSize) + pageStartPos;
      from = max(pageStartPos, from);
      Point2 to = extendedStartPos + extendedChunkSize;
      to = mul(ceil(div(to - pageStartPos, texelSize)), texelSize) + pageStartPos;
      to = min(to, pageEndPos);

      for (float y = from.y; y < to.y; y += texelSize.y) //-V1034
      {
        for (float x = from.x; x < to.x; x += texelSize.x) //-V1034
        {
          float height;
          if (getHeightmapDataBilinear(x, y, height))
          {
            hmin = min(hmin, height);
            hmax = max(hmax, height);
          }
        }
      }
    }
  }

  BBox2 chunkBox(pos, pos + Point2(chunk_size, chunk_size));
  for (int i = 0; i < extraBoundings.size(); i++)
  {
    const BBox3 &bbox = extraBoundings[i];
    if ((chunkBox.lim[0].x <= bbox.lim[1].x) && (chunkBox.lim[0].y <= bbox.lim[1].z) && (chunkBox.lim[1].x >= bbox.lim[0].x) &&
        (chunkBox.lim[1].y >= bbox.lim[0].z))
    {
      hmin = min(hmin, bbox.lim[0].y);
      hmax = max(hmax, bbox.lim[1].y);
    }
  }

  // No page contributed anything (data pages whose bilinear samples were all out of range): fall back to the water level.
  if (hmin > hmax)
    hmin = hmax = waterLevel;
  return true;
}


struct WaterQueryUser
{
  uint32_t handle;
  Point3 pos;
  float timeLife;
  float maxHeight;

  bool hasResult;
  bool waitResult;
  float depth;
  int priority;
};

static const uint32_t query_gpu_fetch_max_point_count = 128;
static const uint32_t query_gpu_one_max_point_count = 1;
static const uint32_t query_gpu_update_frequence = 10;

fft_water::GpuOnePointQuery *fftWaterPointQuery = NULL;
fft_water::GpuFetchQuery *fftWaterFetchQuery = NULL;
carray<Point3, query_gpu_fetch_max_point_count> waterQueryPointsRequests;
SmallTab<WaterQueryUser, MidmemAlloc> waterQueryUsers;
uint32_t waterQueryUserNextHandle = 0;
uint32_t waterQueryUserCount = 0;
int waterQueryUpdateFrame = 0;
OSSpinlock waterQuerySpinlock;

void init_gpu_queries()
{
  if (fftWaterFetchQuery || fftWaterPointQuery)
    return;

  if (fft_water::init_gpu_fetch())
  {
    debug("gpu fetch was inited successfully");

    fftWaterFetchQuery = fft_water::create_gpu_fetch_query(query_gpu_fetch_max_point_count);
    if (fftWaterFetchQuery)
    {
      clear_and_resize(waterQueryUsers, query_gpu_fetch_max_point_count);
      mem_set_0(waterQueryUsers);
    }
    else
      fft_water::close_gpu_fetch();
  }

  if (!fftWaterFetchQuery)
  {
    debug("gpu fetch was not supported, use one point query instead");

    fftWaterPointQuery = fft_water::create_one_point_query();
    clear_and_resize(waterQueryUsers, query_gpu_one_max_point_count);
    mem_set_0(waterQueryUsers);
  }

  G_ASSERT(fftWaterFetchQuery || fftWaterPointQuery);
}

void release_gpu_queries()
{
  if (!fftWaterFetchQuery && !fftWaterPointQuery)
    return;

  fft_water::destroy_query(fftWaterFetchQuery);
  fft_water::destroy_query(fftWaterPointQuery);
  fft_water::close_gpu_fetch();
  clear_and_shrink(waterQueryUsers);
  waterQueryUserNextHandle = 0;
  waterQueryUserCount = 0;
}

void update_gpu_queries(float dt)
{
  // handling more than one point is too costly however we can use several points on
  // one query but it is still needed to do
  G_STATIC_ASSERT(query_gpu_one_max_point_count == 1);

  if (waterQueryUserCount > 0 && ((fftWaterFetchQuery && fft_water::update_query_status(fftWaterFetchQuery)) ||
                                   (fftWaterPointQuery && fft_water::update_query_status(fftWaterPointQuery))))
  {
    bool isResultReady = false;
    uint32_t queryResultCount = 0;
    uint32_t queryUserCount = 0;
    uint32_t lastQueryUserIndex = -1;

    dag::ConstSpan<float> results =
      fftWaterFetchQuery ? fft_water::get_results(fftWaterFetchQuery, &isResultReady) : dag::ConstSpan<float>();
    float result = fftWaterPointQuery ? fft_water::get_result(fftWaterPointQuery, &isResultReady) : 0.0f;
    bool queryNewResults = --waterQueryUpdateFrame < 0;

    for (uint32_t i = 0; i < waterQueryUserCount; ++i)
    {
      WaterQueryUser &queryUser = waterQueryUsers[i];

      if (queryUser.waitResult)
      {
        if (fftWaterFetchQuery && isResultReady && queryResultCount < results.size())
          queryUser.depth = results[queryResultCount];
        else if (fftWaterPointQuery && isResultReady)
          queryUser.depth = result;
        queryUser.hasResult |= isResultReady;
        queryUser.waitResult = false;
        ++queryResultCount;
      }

      if (queryUser.timeLife > 0 && queryNewResults)
      {
        waterQueryPointsRequests[queryUserCount] = queryUser.pos;
        queryUser.waitResult = true;
        lastQueryUserIndex = i;
        ++queryUserCount;
      }

      queryUser.timeLife = max(queryUser.timeLife - dt, 0.0f);
    }

    if (queryNewResults)
    {
      waterQueryUpdateFrame = query_gpu_update_frequence;
      waterQueryUserCount = lastQueryUserIndex + 1;
      if (fftWaterFetchQuery)
        fft_water::update_query(fftWaterFetchQuery, waterQueryPointsRequests.data(), queryUserCount);
      else if (fftWaterPointQuery)
        fft_water::update_query(fftWaterPointQuery, waterQueryUsers[0].pos, waterQueryUsers[0].maxHeight);
    }
  }
}

void render_debug_gpu_queries()
{
  if (waterQueryUserCount > 0)
  {
    begin_draw_cached_debug_lines(false, false);
    for (int i = 0; i < waterQueryUserCount; ++i)
    {
      if (!waterQueryUsers[i].hasResult)
        continue;

      E3DCOLOR col = waterQueryUsers[i].depth < 0 ? 0xFF0000FF : 0xFFFF00FF;
      draw_cached_debug_sphere(waterQueryUsers[i].pos, 0.1, col);
      draw_cached_debug_line(waterQueryUsers[i].pos, waterQueryUsers[i].pos + Point3(0, waterQueryUsers[i].depth, 0), col);
    }
    end_draw_cached_debug_lines();
  }
}

void before_reset_gpu_queries()
{
  if (fftWaterPointQuery)
    fft_water::before_reset(fftWaterPointQuery);
}

uint32_t make_gpu_query_user() { return waterQueryUserNextHandle++; }

bool query_gpu_water_height(uint32_t query_handle, uint32_t &ref_query_result, const Point3 &from_y_up, float max_height,
  bool &out_underwater, float &out_height, int priority, float time_life)
{
  G_ASSERT(fftWaterFetchQuery || fftWaterPointQuery);

  OSSpinlockScopedLock lock(waterQuerySpinlock);

  if (ref_query_result >= waterQueryUsers.size() || waterQueryUsers[ref_query_result].handle != query_handle)
  {
    ref_query_result = UINT_MAX;
    uint32_t lastQueryIndexLowP = UINT_MAX;

    for (uint32_t i = 0; i < waterQueryUsers.size(); ++i)
    {
      WaterQueryUser &queryUser = waterQueryUsers[i];

      if (queryUser.priority < priority)
        lastQueryIndexLowP = i;

      if (queryUser.timeLife == 0.0f && !queryUser.waitResult)
      {
        queryUser.handle = query_handle;
        queryUser.hasResult = false;
        ref_query_result = i;
        break;
      }
    }

    if (ref_query_result == UINT_MAX && lastQueryIndexLowP != UINT_MAX)
    {
      WaterQueryUser &queryUser = waterQueryUsers[lastQueryIndexLowP];
      queryUser.handle = query_handle;
      queryUser.hasResult = false;
      ref_query_result = lastQueryIndexLowP;
    }
  }

  if (ref_query_result == UINT_MAX)
    return false;

  WaterQueryUser &userRef = waterQueryUsers[ref_query_result];
  if (userRef.timeLife == 0.0f)
    userRef.hasResult = false;

  if (userRef.hasResult && !isnan(userRef.depth))
  {
    out_underwater = userRef.depth >= 0;
    float height = userRef.depth > 0 ? min(userRef.depth, max_height) : max(userRef.depth, -max_height);
    out_height = userRef.pos.y + height;
  }

  userRef.pos = from_y_up;
  userRef.timeLife = time_life;
  userRef.priority = priority;
  userRef.maxHeight = max_height;
  waterQueryUserCount = max(waterQueryUserCount, ref_query_result + 1);
  return userRef.hasResult;
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  using namespace convar;

  bool chopGen = chop_gen;
  if (ImGui::Checkbox("Chop generator enabled", &chopGen))
    chop_gen = chopGen;

  float chopWindSpeed = wind_speed_chop;
  if (ImGui::SliderFloat("Chop wind speed", &chopWindSpeed, 0.0f, 10.0f, "%.2f"))
    wind_speed_chop = chopWindSpeed;
}

REGISTER_IMGUI_WINDOW("Render", "Water", imguiWindow);
#endif

} // namespace fft_water

GlobalSharedMemStorage *fft_water::WaterHeightmap::sharedMem = nullptr;
