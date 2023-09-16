#include "waterPhys.h"
#include "waterRender.h"
#include <limits.h>
#include <fftWater/fftWater.h>
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

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("water", phys_tex_on, false);
#endif

class FFTWater
{
  eastl::unique_ptr<fft_water::WaterHeightmap> waterHeightmap;
  WaterNVRender *render;
  WaterNVPhysics *physics;
#if DAGOR_DBGLEVEL > 0
  UniqueTex physTex;
#endif
  double currentPhysTime;
  double lastTime;
  NVWaveWorks_FFT_CPU_Simulation::Params params;
  float waterLevel;
  int numRenderCascades;
  int minRenderResBits;

public:
  void setCurrentTime(double time) { currentPhysTime = time; }
  double getCurrentTime() const { return currentPhysTime; }
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
  }
  float getPeriod() { return params.fft_period; }
  FFTWater(int num_render_cascades, int min_render_res_bits) : render(NULL), physics(NULL), numRenderCascades(num_render_cascades)
  {
    minRenderResBits = max<int>(min_render_res_bits, MIN_FFT_RESOLUTION);
    params.fft_resolution_bits = minRenderResBits;
    params.wind_dependency = 0.98f;
    params.wind_alignment = 1.0f;
    params.small_wave_fraction = 0.001f;
    params.choppy_scale = 1.0f;
    params.wave_amplitude = 0.7f;
    params.fft_period = 1000.0f;
    lastTime = currentPhysTime = 0.0;
    waterLevel = 0;
    setWind(1.0f, Point2(0.8, 0.6));
  }
  void getWind(float &out_speed, Point2 &out_wind_dir)
  {
    out_speed = params.wind_speed;
    out_wind_dir = Point2(params.wind_dir_x, params.wind_dir_y);
  }
  void setWind(float speed, const Point2 &wind_dir)
  {
    if (fabsf(params.wind_speed - speed) < 0.05f && wind_dir * Point2(params.wind_dir_x, params.wind_dir_y) > 0.999f)
      return;

    Point2 windDirNorm = normalize(wind_dir);
    params.wind_dir_x = windDirNorm.x;
    params.wind_dir_y = windDirNorm.y;
    params.wind_speed = speed;

    if (render)
      render->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    if (physics)
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
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
  }
  void closeRender() { del_it(render); }

  int getActualFFTResolutionBits(int quality)
  {
    if (quality <= fft_water::RENDER_VERY_LOW)
      return minRenderResBits;
    return params.fft_resolution_bits;
  }

  void initRender(int quality, int geom_quality, bool ssr_renderer, bool one_to_four_cascades)
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
    Point2 shoreWavesDist = saveParams ? render->getShoreWavesDist() : Point2(0, 0);
    Point2 waveDisplacementDistance = saveParams ? render->getWaveDisplacementDistance() : Point2(0, 0);
    bool shoreEnable = saveParams ? render->isShoreEnabled() : false;

    closeRender();
    NVWaveWorks_FFT_CPU_Simulation::Params newParams = params;
    newParams.fft_resolution_bits = getActualFFTResolutionBits(quality);
    render = new WaterNVRender(newParams, simulation, quality, geom_quality, ssr_renderer, one_to_four_cascades, numRenderCascades,
      cascadeWindowLength, cascadeFacetSize, waterHeightmap.get());

    if (saveParams)
    {
      render->setLevel(waterLevel);
      render->setAnisotropy(aniso, mipBias);
      render->setFoamParams(foam);
      render->setRoughness(roughnessBase, cascadesRoughnessBase);
      render->setShoreWavesDist(shoreWavesDist);
      render->setWaveDisplacementDistance(waveDisplacementDistance);
      render->shoreEnable(shoreEnable);
    }
  }
  void resetRender()
  {
    if (render)
      initRender(render->getQuality(), render->getGeomQuality(), render->isSSRRendererEnabled(), render->getOneToFourCascades());
  }
  int getNumCascades() const { return numRenderCascades; }
  void setNumCascades(int cascades)
  {
    if (numRenderCascades == cascades)
      return;
    numRenderCascades = cascades;
    if (render)
      initRender(render->getQuality(), render->getGeomQuality(), render->isSSRRendererEnabled(), render->getOneToFourCascades());
  }
  void resetPhysics()
  {
    if (physics)
      physics->reset();
  }
  void closePhysics() { del_it(physics); }
  bool validateNextTimeTick(double time) { return physics ? physics->validateNextTimeTick(time) : true; }
  void initPhysics()
  {
    G_ASSERT(physics == NULL);

    NVWaveWorks_FFT_CPU_Simulation::Params newParams = params;
    newParams.fft_resolution_bits = DEF_PHYS_FFT_RESOLUTION;
    physics = new WaterNVPhysics(newParams, fft_water::SimulationParams(), NUM_PHYS_CASCADES, getCascadeWindowLength(),
      getCascadeFacetSize(), waterHeightmap.get());

    lastTime = 0;
    currentPhysTime = 0;
  }
  const fft_water::WaterHeightmap *getHeightmap() { return waterHeightmap.get(); }
  void setHeightmap(eastl::unique_ptr<fft_water::WaterHeightmap> &&water_heightmap)
  {
    waterHeightmap = eastl::move(water_heightmap);
    waterHeightmap->heightMax = waterHeightmap->heightOffset + waterHeightmap->heightScale;
    if (physics)
    {
      physics->setHeightmap(waterHeightmap.get());
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
  }
  void removeHeightmap()
  {
    waterHeightmap.reset(nullptr);
    if (physics)
    {
      physics->setHeightmap(waterHeightmap.get());
      physics->reinit(Point2(params.wind_dir_x, params.wind_dir_y), params.wind_speed, params.fft_period);
    }
  }
  ~FFTWater()
  {
    del_it(render);
    del_it(physics);
  }
  void simulateAllAt(double time)
  {
    lastTime = time;
    setCurrentTime(time);
    if (physics)
      physics->increaseTime(time);

#if DAGOR_DBGLEVEL > 0
    if (physics && phys_tex_on)
    {
      const int TEX_R = (1 << DEF_PHYS_FFT_RESOLUTION);
      if (!physTex.getTex2D())
      {
        physTex = dag::create_tex(NULL, TEX_R, TEX_R, TEXCF_DYNAMIC | TEXFMT_A16B16G16R16F, 1, "water_phys_tex");
      }

      int fifo1, fifo2;
      float fifo2Part;
      physics->getFifoIndex(currentPhysTime, fifo1, fifo2, fifo2Part);
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
  float getLevel() const { return waterLevel; }
  void setLevel(float level)
  {
    waterLevel = level;
    if (render)
      render->setLevel(level);
    if (physics)
      physics->setLevel(level);
  }
  float getHeight(const Point3 &point)
  {
    if (physics)
    {
      vec4f disp = physics->getHeightmapDataBilinear(point.x, point.z);
      return v_extract_z(disp);
    }
    return waterLevel;
  }
  float getMaxLevel()
  {
    if (waterHeightmap)
      return waterHeightmap->heightMax;
    return waterLevel;
  }
  float getMaxWaveHeight() const { return physics ? physics->getMaxWaveHeight() : 0.0f; }
  float getSignificantWaveHeight() const { return physics ? physics->getSignificantWaveHeight() : 0.0f; }
  WaterNVRender *getRender() const { return render; }
  WaterNVPhysics *getPhysics() const { return physics; }
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

FFTWater *create_water(RenderQuality quality, float period, int res_bits, bool ssr_renderer, bool one_to_four_cascades,
  int min_render_res_bits, RenderQuality geom_quality)
{
  FFTWater *water = new FFTWater(fft_water::DEFAULT_NUM_CASCADES, min_render_res_bits);
  water->setFFTRenderResolution(res_bits);
  water->setPeriod(period);
  if (quality != DONT_RENDER)
    water->initRender(quality, (int)geom_quality > 0 ? geom_quality : quality, ssr_renderer, one_to_four_cascades);
  water->initPhysics();
  water->simulateAllAt(0);

#if DAGOR_DBGLEVEL > 0
  water_consoleproc.demandInit();
  water_consoleproc->water = water;
  add_con_proc(water_consoleproc);
#endif

  return water;
}
void init_render(FFTWater *handle, int quality, bool ssr_renderer, bool one_to_four_cascades, RenderQuality geom_quality)
{
  if (!handle)
    return;
  if (quality != DONT_RENDER)
    handle->initRender(quality, (int)geom_quality > 0 ? geom_quality : quality, ssr_renderer, one_to_four_cascades);
  else
    handle->closeRender();
}
bool one_to_four_render_enabled(FFTWater *handle)
{
  if (handle && handle->getRender())
    return handle->getRender()->getOneToFourCascades();
  return false;
}
void set_grid_lod0_additional_tesselation(FFTWater *a, float amount)
{
  if (a)
    a->getRender()->setGridLod0AdditionalTesselation(amount);
}
void set_grid_lod0_area_radius(FFTWater *a, float radius)
{
  if (a)
    a->getRender()->setLod0AreaSize(radius);
}
void set_period(FFTWater *a, float period)
{
  if (a)
    a->setPeriod(period);
}
float get_period(FFTWater *handle) { return handle ? handle->getPeriod() : 0.f; }
void delete_water(FFTWater *&a)
{
#if DAGOR_DBGLEVEL > 0
  if (water_consoleproc && water_consoleproc->water == a)
    water_consoleproc->water = NULL;
#endif
  del_it(a);
}
void set_render_quad(FFTWater *handle, const BBox2 &quad)
{
  if (handle && handle->getRender())
    handle->getRender()->setRenderQuad(quad);
}
void simulate(FFTWater *handle, double time)
{
  if (!handle)
    return;

  handle->simulateAllAt(time);
}
void before_render(FFTWater *handle)
{
  if (!handle->getRender())
    return;
  handle->getRender()->simulateAllAt(handle->getLastTime());
  handle->getRender()->updateTexturesAll();
  handle->getRender()->calculateGradients();
}

float getGridLod0AreaSize(FFTWater *handle)
{
  if (!handle->getRender())
    return 0.f;
  return handle->getRender()->getLod0AreaSize();
}

void setGridLod0AdditionalTesselation(FFTWater *handle, float additional_tesselation)
{
  if (handle->getRender() != NULL)
    handle->getRender()->setGridLod0AdditionalTesselation(additional_tesselation);
}

void render(FFTWater *handle, const Point3 &pos, TEXTUREID distance_tex_id, int geom_lod_quality, int survey_id,
  IWaterDecalsRenderHelper *decals_renderer, RenderMode render_mode)
{
  TIME_D3D_PROFILE(fft_water_render);

  if (!handle->getRender())
    return;

  // render() call runs begin_survey, becouse we have a fence inside function, fence cannot be between
  // start survey and end survey. So here we should just call end_survey.
  handle->getRender()->render(pos, distance_tex_id, geom_lod_quality, survey_id, decals_renderer, render_mode);

  d3d::end_survey(survey_id);
}

void prepare_refraction(FFTWater *handle, Texture *scene_target_tex)
{
  if (!handle->getRender())
    return;
  handle->getRender()->prepareRefraction(scene_target_tex);
}
float get_level(FFTWater *handle) { return handle->getLevel(); }
float get_max_level(FFTWater *handle) { return handle->getMaxLevel(); }
void set_level(FFTWater *handle, float level) { handle->setLevel(level); }
float get_height(FFTWater *handle, const Point3 &point) { return handle->getHeight(point); }
float get_max_wave(FFTWater *handle) { return handle->getMaxWaveHeight(); }
float get_significant_wave_height(FFTWater *handle) { return handle->getSignificantWaveHeight(); }
void set_wave_displacement_distance(FFTWater *handle, const Point2 &value)
{
  if (handle && handle->getRender())
    handle->getRender()->setWaveDisplacementDistance(value);
}

void shore_enable(FFTWater *handle, bool enable)
{
  if (handle && handle->getRender())
    handle->getRender()->shoreEnable(enable);
}

bool is_shore_enabled(FFTWater *handle)
{
  if (handle && handle->getRender())
    return handle->getRender()->isShoreEnabled();
  return false;
}

void set_shore_waves_dist(FFTWater *handle, const Point2 &value)
{
  if (handle && handle->getRender())
    handle->getRender()->setShoreWavesDist(value);
}

float get_shore_wave_threshold(FFTWater *handle)
{
  if (handle && handle->getRender())
    return handle->getRender()->getShoreWaveThreshold();
  return 0;
}

void set_shore_wave_threshold(FFTWater *handle, float value)
{
  if (handle && handle->getRender())
    handle->getRender()->setShoreWaveThreshold(value);
}

int get_fft_resolution(FFTWater *handle)
{
  if (handle)
    return handle->getFFTRenderResolution();
  return TARGET_FFT_RESOLUTION;
}

void set_fft_resolution(FFTWater *handle, int res_bits)
{
  if (handle)
    handle->setFFTRenderResolution(res_bits);
}

void set_render_pass(FFTWater *handle, int pass)
{
  if (handle && handle->getRender())
    handle->getRender()->setRenderPass(pass);
}

void reset_render(FFTWater *handle)
{
  if (handle)
    handle->resetRender();
}

int get_num_cascades(FFTWater *handle)
{
  if (handle)
    return handle->getNumCascades();
  return fft_water::DEFAULT_NUM_CASCADES;
}

void set_num_cascades(FFTWater *handle, int cascades)
{
  if (handle)
    handle->setNumCascades(cascades);
}

void set_render_quality(FFTWater *handle, int quality, bool ssr_renderer)
{
  if (handle && handle->getRender() && handle->getRender()->getQuality() != quality)
    handle->initRender(quality, handle->getRender()->getGeomQuality(), ssr_renderer, handle->getRender()->getOneToFourCascades());
}

void setAnisotropy(FFTWater *handle, int aniso, float mip_bias)
{
  if (handle && handle->getRender())
    handle->getRender()->setAnisotropy(aniso, mip_bias);
}

float get_small_wave_fraction(FFTWater *handle)
{
  if (handle)
    return handle->getSmallWaveFraction();
  return 0.0f;
}

void set_small_wave_fraction(FFTWater *handle, float smallWaveFraction)
{
  if (handle)
    handle->setSmallWaveFraction(smallWaveFraction);
}

float get_cascade_window_length(FFTWater *handle)
{
  if (handle)
    return handle->getCascadeWindowLength();
  return 0.0f;
}

void set_cascade_window_length(FFTWater *handle, float value)
{
  if (handle)
    handle->setCascadeWindowLength(value);
}

float get_cascade_facet_size(FFTWater *handle)
{
  if (handle)
    return handle->getCascadeFacetSize();
  return 0.0f;
}

void set_cascade_facet_size(FFTWater *handle, float value)
{
  if (handle)
    handle->setCascadeFacetSize(value);
}

SimulationParams get_simulation_params(FFTWater *handle)
{
  if (handle)
    return handle->getSimulationParams();
  return SimulationParams();
}

void set_simulation_params(FFTWater *handle, const SimulationParams &scales)
{
  if (handle)
    handle->setSimulationParams(scales);
}

void set_foam(FFTWater *handle, const FoamParams &params)
{
  if (handle && handle->getRender())
    handle->getRender()->setFoamParams(params);
}

FoamParams get_foam(FFTWater *handle)
{
  if (handle && handle->getRender())
    return handle->getRender()->getFoamParams();
  return FoamParams();
}

void enable_graphic_feature(FFTWater *handle, GraphicFeature feature, bool enable)
{
#if DAGOR_DBGLEVEL > 0
  if (feature < 0 || feature >= GRAPHIC_FEATURE_END)
    return;
  if (handle && handle->getRender())
    handle->getRender()->enableGraphicFeature(feature, enable);
#else
  G_UNREFERENCED(handle);
  G_UNREFERENCED(feature);
  G_UNREFERENCED(enable);
#endif
}

void get_cascade_period(FFTWater *handle, int cascade_no, float &out_period, float &out_window_in, float &out_window_out)
{
#if DAGOR_DBGLEVEL > 0
  if (handle && handle->getRender())
    handle->getRender()->getCascadePeriod(cascade_no, out_period, out_window_in, out_window_out);
#else
  G_UNREFERENCED(handle);
  G_UNREFERENCED(cascade_no);
  G_UNREFERENCED(out_period);
  G_UNREFERENCED(out_window_in);
  G_UNREFERENCED(out_window_out);
#endif
}

void set_current_time(FFTWater *handle, double time) { return handle->setCurrentTime(time); }
void reset_physics(FFTWater *handle) { handle->resetPhysics(); }
bool validate_next_time_tick(FFTWater *handle, double next_time) { return handle->validateNextTimeTick(next_time); }
int intersect_segment(FFTWater *handle, const Point3 &start, const Point3 &end, float &result)
{
  return handle->getPhysics()->intersectSegment(handle->getCurrentTime(), start, end, result);
}
int intersect_segment_at_time(FFTWater *handle, double time, const Point3 &start, const Point3 &end, float &result)
{
  return handle->getPhysics()->intersectSegment(time, start, end, result);
}
int getHeightAboveWater(FFTWater *handle, const Point3 &in_point, float &result, bool matchRenderGrid)
{
  return handle->getPhysics()->getHeightAboveWater(handle->getCurrentTime(), in_point, result, nullptr, matchRenderGrid);
}
void setRenderParamsToPhysics(FFTWater *handle)
{
  if (!handle->getRender() || !handle->getPhysics())
    return;
  float gridAlign;
  Point2 gridOffset;
  handle->getRender()->getGridDataAtCamera(gridAlign, gridOffset);
  handle->getPhysics()->setRenderParams(gridAlign, gridOffset);
}
int getHeightAboveWaterAtTime(FFTWater *handle, double at_time, const Point3 &in_point, float &result, Point3 *displacement)
{
  return handle->getPhysics()->getHeightAboveWater(at_time, in_point, result, displacement);
}
void get_wind_speed(FFTWater *handle, float &out_speed, Point2 &out_wind_dir) { handle->getWind(out_speed, out_wind_dir); }
void set_wind_speed(FFTWater *handle, float speed, const Point2 &wind_dir) { handle->setWind(speed, wind_dir); }
void get_roughness(FFTWater *handle, float &out_roughness_base, float &out_cascades_roughness_base)
{
  out_roughness_base = 0;
  out_cascades_roughness_base = 0;
  if (handle && handle->getRender())
    handle->getRender()->getRoughness(out_roughness_base, out_cascades_roughness_base);
}
void set_roughness(FFTWater *handle, float roughness_base, float cascades_roughness_base)
{
  if (handle && handle->getRender())
    handle->getRender()->setRoughness(roughness_base, cascades_roughness_base);
}

void setVertexSamplers(FFTWater *handle, int samplersCount)
{
  if (!handle->getRender())
    return;
  handle->getRender()->setVertexSamplers(samplersCount);
}
int setWaterCell(FFTWater *handle, float water_cell_size, bool auto_set_samplers_cnt)
{
  if (!handle->getRender())
    return 0;
  return handle->getRender()->setWaterCell(water_cell_size, auto_set_samplers_cnt);
}
void set_water_dim(FFTWater *handle, int dim_bits)
{
  if (!handle->getRender())
    return;
  return handle->getRender()->setWaterDim(dim_bits);
}

void setWakeHtTex(FFTWater *handle, TEXTUREID wake_ht_tex_id)
{
  if (handle->getRender())
    handle->getRender()->setWakeHtTex(wake_ht_tex_id);
}

void force_actual_waves(FFTWater *handle, bool enforce)
{
  if (handle->getRender())
  {
    handle->getPhysics()->setForceActualWaves(enforce);
    handle->getRender()->setForceTessellation(enforce);
  }
}

const fft_water::WaterHeightmap *get_heightmap(FFTWater *handle) { return handle->getHeightmap(); }
void set_heightmap(FFTWater *handle, eastl::unique_ptr<WaterHeightmap> &&heightmap) { handle->setHeightmap(eastl::move(heightmap)); }
void remove_heightmap(FFTWater *handle) { handle->removeHeightmap(); }

void load_heightmap(IGenLoad &loadCb, FFTWater *water)
{
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

  heightmap->pages.resize(heightmap->pagesX * heightmap->pagesY * heightmap->PAGE_SIZE_PADDED * heightmap->PAGE_SIZE_PADDED);
  heightmap->grid.resize(heightmap->gridSize * heightmap->gridSize);
  uint16_t *pages_data = heightmap->pages.data();
  uint16_t *grid_data = heightmap->grid.data();
  IGenLoad *zcrd_p = NULL;
  unsigned fmt = 0;
  loadCb.beginBlock(&fmt);
  if (fmt == btag_compr::OODLE)
    zcrd_p = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE)
      OodleLoadCB(loadCb, loadCb.getBlockRest(), (heightmap->pages.size() + heightmap->grid.size()) * sizeof(uint16_t));
  else if (fmt == btag_compr::ZSTD)
    zcrd_p = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(loadCb, loadCb.getBlockRest());
  else
    zcrd_p = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(loadCb, loadCb.getBlockRest());
  IGenLoad &zcrd = *zcrd_p;
  zcrd.read(pages_data, heightmap->pages.size() * sizeof(uint16_t));
  zcrd.read(grid_data, heightmap->grid.size() * sizeof(uint16_t));
  zcrd.ceaseReading();
  zcrd.~IGenLoad();
  int h = heightmap->pagesY * heightmap->PAGE_SIZE_PADDED;
  int w = heightmap->pagesX * heightmap->PAGE_SIZE_PADDED;
  for (int y = 0; y < h; ++y, pages_data += w)
    for (int x = 1; x < w; ++x)
      pages_data[x] = (uint16_t)(int(pages_data[x]) + pages_data[x - 1]);
  loadCb.endBlock();
  int patchHeightsSize = heightmap->PATCHES_GRID_SIZE * heightmap->PATCHES_GRID_SIZE;
  heightmap->patchHeights.resize(patchHeightsSize);
  int rest = loadCb.getBlockRest();
  if (rest >= patchHeightsSize * sizeof(Point2))
    loadCb.readExact(heightmap->patchHeights.data(), heightmap->patchHeights.size() * sizeof(Point2));
  else
    logerr("Missing water heightmap patches data. Level re-export is needed for correct water heightmap rendering");
  set_heightmap(water, eastl::move(heightmap));
}

void WaterHeightmap::getHeightmapDataBilinear(float x, float z, float &result) const
{
  x = (x * tcOffsetScale.z + tcOffsetScale.x) * gridSize;
  z = (z * tcOffsetScale.w + tcOffsetScale.y) * gridSize;
  if (x < 0.0f || z < 0.0f || x > gridSize - 1 || z > gridSize - 1)
    return;
  uint16_t cellData = grid[((int)z) * gridSize + (int)x];
  if (cellData == 0xFFFF)
    return;
  int pageSize = PAGE_SIZE_PADDED;
  int gridX = (cellData >> 1) & 0x7F;
  int gridZ = (cellData >> 8) & 0xFF;
  float crdScale = (cellData & 1) ? 1.0 : scale;
  x = fmod(x / crdScale, 1.0);
  z = fmod(z / crdScale, 1.0);
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
os_spinlock_t waterQuerySpinlock;

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
  os_spinlock_init(&waterQuerySpinlock);
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
  os_spinlock_destroy(&waterQuerySpinlock);
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
} // namespace fft_water
