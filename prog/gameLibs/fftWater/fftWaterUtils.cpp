#include <fftWater/fftWater.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <util/dag_lookup.h>
#include <debug/dag_assert.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>

namespace fft_water
{
WavePreset::WavePreset() {}

G_STATIC_ASSERT(BEAUFORT_SCALES_NUM == 7);
G_STATIC_ASSERT((int)Spectrum::LAST == 1);

static const WavePresets def_waves_phillips = {
  // Beaufort 0
  WavePreset(10.0f, 0.3f, 0.0f, 0.15f,
    SimulationParams({0.01f, 0.01f, 0.01f, 0.01f, 0.01f}, 1.0f, 1.0f, 1.0f, 0.03f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f)),
  // Beaufort 1
  WavePreset(60.0f, 0.9f, 0.0f, 0.15f, SimulationParams({1.0f, 1.0f, 1.0f, 0.7f, 0.7f}, 1.0f, 0.9f, 1.5f, 0.05f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f)),
  // Beaufort 2
  WavePreset(250.0f, 2.5f, 0.0f, 0.03f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 2.0f, 0.08f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f)),
  // Beaufort 3
  WavePreset(750.0f, 4.5f, 0.0f, 0.01f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f)),
  // Beaufort 4
  WavePreset(750.0f, 6.7f, 0.0f, 0.01f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.07f, 0.75f, 0.985f, 10.0f, 0.55f, 0.6f)),
  // Beaufort 5
  WavePreset(1000.0f, 9.4f, 0.0f, 0.004f,
    SimulationParams({1.0f, 1.0f, 0.75f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.065f, 0.75f, 0.985f, 10.0f, 0.65f, 0.7f)),
  // Beaufort 6
  WavePreset(1150.0f, 12.3f, 0.0f, 0.003f,
    SimulationParams({1.0f, 1.0f, 0.75f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.05f, 0.75f, 0.985f, 10.0f, 0.7f, 0.75f))};

static const WavePresets def_waves_unified_directional = {{// Beaufort 0
  WavePreset(10.0f, 0.3f, 0.0f, 0.0f,
    SimulationParams({0.01f, 0.01f, 0.01f, 0.01f, 0.01f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f)),
  // Beaufort 1
  WavePreset(60.0f, 0.9f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f)),
  // Beaufort 2
  WavePreset(250.0f, 2.5f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f)),
  // Beaufort 3
  WavePreset(750.0f, 4.5f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f)),
  // Beaufort 4
  WavePreset(750.0f, 6.7f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.14f, 0.7f, 0.99f, 10.0f, 0.65f, 0.8f)),
  // Beaufort 5
  WavePreset(1000.0f, 9.4f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.12f, 0.7f, 0.99f, 10.0f, 0.6f, 0.8f)),
  // Beaufort 6
  WavePreset(1150.0f, 12.3f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.09f, 0.7f, 0.99f, 10.0f, 0.6f, 0.8f))}};

const WavePresets &defWaves(const Spectrum &spectrum)
{
  switch (spectrum)
  {
    case Spectrum::PHILLIPS: return def_waves_phillips;
    case Spectrum::UNIFIED_DIRECTIONAL: return def_waves_unified_directional;
  }
  return def_waves_phillips;
}

const WavePreset &defWaves(const Spectrum &spectrum, int def_preset_no) { return defWaves(spectrum)[def_preset_no]; }


void load_wave_preset(const DataBlock *presetBlk, WavePreset &preset, int def_preset_no)
{
  G_ASSERT(presetBlk);

  Spectrum spectrum = (Spectrum)lup(presetBlk->getStr("spectra", ""), spectrum_names, countof(spectrum_names), (int)Spectrum::DEFAULT);

  String blkName;
  preset.period = presetBlk->getReal("period", defWaves(spectrum, def_preset_no).period);
  preset.wind = presetBlk->getReal("wind", defWaves(spectrum, def_preset_no).wind);
  preset.roughness = presetBlk->getReal("roughness", defWaves(spectrum, def_preset_no).roughness);
  preset.smallWaveFraction = presetBlk->getReal("smallWaveFraction", defWaves(spectrum, def_preset_no).smallWaveFraction);

  preset.simulation.choppiness = presetBlk->getReal("choppiness", defWaves(spectrum, def_preset_no).simulation.choppiness);
  preset.simulation.facetSize = presetBlk->getReal("facetSize", defWaves(spectrum, def_preset_no).simulation.facetSize);
  preset.simulation.windDependency = presetBlk->getReal("windDependency", defWaves(spectrum, def_preset_no).simulation.windDependency);
  preset.simulation.windAlignment = presetBlk->getReal("windAlignment", defWaves(spectrum, def_preset_no).simulation.windAlignment);
  preset.simulation.spectrum = spectrum;

  for (int ampNo = 0; ampNo < preset.simulation.amplitude.size(); ++ampNo)
  {
    blkName.printf(0, "amplitude%d", ampNo);
    const DataBlock *ampBlk = presetBlk->getBlockByName(blkName);
    if (!ampBlk)
    {
      preset.simulation.amplitude[ampNo] = defWaves(spectrum, def_preset_no).simulation.amplitude[ampNo];
      continue;
    }
    preset.simulation.amplitude[ampNo] = ampBlk->getReal("amplitude", defWaves(spectrum, def_preset_no).simulation.amplitude[ampNo]);
  }

  preset.foam.generation_threshold =
    presetBlk->getReal("foam_generation_threshold", defWaves(spectrum, def_preset_no).foam.generation_threshold);
  preset.foam.generation_amount =
    presetBlk->getReal("foam_generation_amount", defWaves(spectrum, def_preset_no).foam.generation_amount);
  preset.foam.dissipation_speed =
    presetBlk->getReal("foam_dissipation_speed", defWaves(spectrum, def_preset_no).foam.dissipation_speed);
  preset.foam.falloff_speed = presetBlk->getReal("foam_falloff_speed", defWaves(spectrum, def_preset_no).foam.falloff_speed);
  preset.foam.hats_mul = presetBlk->getReal("foam_hats_mul", defWaves(spectrum, def_preset_no).foam.hats_mul);
  preset.foam.hats_threshold = presetBlk->getReal("foam_hats_threshold", defWaves(spectrum, def_preset_no).foam.hats_threshold);
  preset.foam.hats_folding = presetBlk->getReal("foam_hats_folding", defWaves(spectrum, def_preset_no).foam.hats_folding);
  preset.foam.surface_folding_foam_mul =
    presetBlk->getReal("surface_folding_foam_mul", defWaves(spectrum, def_preset_no).foam.surface_folding_foam_mul);
  preset.foam.surface_folding_foam_pow =
    presetBlk->getReal("surface_folding_foam_pow", defWaves(spectrum, def_preset_no).foam.surface_folding_foam_pow);
}

void save_wave_preset(DataBlock *presetBlk, WavePreset &preset)
{
  G_ASSERT(presetBlk);

  String blkName;
  presetBlk->setReal("period", preset.period);
  presetBlk->setReal("wind", preset.wind);
  presetBlk->setReal("roughness", preset.roughness);
  presetBlk->setReal("smallWaveFraction", preset.smallWaveFraction);

  presetBlk->setReal("choppiness", preset.simulation.choppiness);
  presetBlk->setReal("facetSize", preset.simulation.facetSize);
  presetBlk->setReal("windDependency", preset.simulation.windDependency);
  presetBlk->setReal("windAlignment", preset.simulation.windAlignment);
  presetBlk->setStr("spectra", spectrum_names[(int)preset.simulation.spectrum]);

  for (int ampNo = 0; ampNo < preset.simulation.amplitude.size(); ++ampNo)
  {
    blkName.printf(0, "amplitude%d", ampNo);
    DataBlock *ampBlk = presetBlk->addBlock(blkName);
    G_ASSERT(ampBlk);
    ampBlk->setReal("amplitude", preset.simulation.amplitude[ampNo]);
  }

  presetBlk->setReal("foam_generation_threshold", preset.foam.generation_threshold);
  presetBlk->setReal("foam_generation_amount", preset.foam.generation_amount);
  presetBlk->setReal("foam_dissipation_speed", preset.foam.dissipation_speed);
  presetBlk->setReal("foam_falloff_speed", preset.foam.falloff_speed);
  presetBlk->setReal("foam_hats_mul", preset.foam.hats_mul);
  presetBlk->setReal("foam_hats_threshold", preset.foam.hats_threshold);
  presetBlk->setReal("foam_hats_folding", preset.foam.hats_folding);
  presetBlk->setReal("surface_folding_foam_mul", preset.foam.surface_folding_foam_mul);
  presetBlk->setReal("surface_folding_foam_pow", preset.foam.surface_folding_foam_pow);
}

void load_wave_presets(const DataBlock &blk, WavePresets &out_waves)
{
  for (int presetNo = 0; presetNo < out_waves.presets.size(); ++presetNo)
  {
    fft_water::WavePreset &preset = out_waves[presetNo];

    String blkName = String(0, "preset%d", presetNo);
    const DataBlock *presetBlk = blk.getBlockByName(blkName);
    if (!presetBlk)
    {
      preset = defWaves(Spectrum::DEFAULT)[presetNo];
      continue;
    }

    load_wave_preset(presetBlk, preset, presetNo);
  }
}

void apply_wave_preset(FFTWater *water, const WavePreset &preset, const Point2 &wind_dir)
{
  set_period(water, preset.period);
  set_wind_speed(water, preset.wind, wind_dir);
  set_roughness(water, preset.roughness, 0);
  set_small_wave_fraction(water, preset.smallWaveFraction);
  set_simulation_params(water, preset.simulation);
  set_foam(water, preset.foam);
  reset_physics(water);
}

void apply_wave_preset(FFTWater *water, const WavePreset &preset1, const WavePreset &preset2, float alpha, const Point2 &wind_dir)
{
  G_STATIC_ASSERT(MAX_NUM_CASCADES == 5);
  WavePreset preset(lerp(preset1.period, preset2.period, alpha), lerp(preset1.wind, preset2.wind, alpha),
    lerp(preset1.roughness, preset2.roughness, alpha), lerp(preset1.smallWaveFraction, preset2.smallWaveFraction, alpha),
    SimulationParams({lerp(preset1.simulation.amplitude[0], preset2.simulation.amplitude[0], alpha),
                       lerp(preset1.simulation.amplitude[1], preset2.simulation.amplitude[1], alpha),
                       lerp(preset1.simulation.amplitude[2], preset2.simulation.amplitude[2], alpha),
                       lerp(preset1.simulation.amplitude[3], preset2.simulation.amplitude[3], alpha),
                       lerp(preset1.simulation.amplitude[4], preset2.simulation.amplitude[4], alpha)},
      lerp(preset1.simulation.choppiness, preset2.simulation.choppiness, alpha),
      lerp(preset1.simulation.windDependency, preset2.simulation.windDependency, alpha),
      lerp(preset1.simulation.windAlignment, preset2.simulation.windAlignment, alpha),
      lerp(preset1.simulation.facetSize, preset2.simulation.facetSize, alpha), preset2.simulation.spectrum),
    FoamParams(lerp(preset1.foam.generation_threshold, preset2.foam.generation_threshold, alpha),
      lerp(preset1.foam.generation_amount, preset2.foam.generation_amount, alpha),
      lerp(preset1.foam.dissipation_speed, preset2.foam.dissipation_speed, alpha),
      lerp(preset1.foam.falloff_speed, preset2.foam.falloff_speed, alpha), lerp(preset1.foam.hats_mul, preset2.foam.hats_mul, alpha),
      lerp(preset1.foam.hats_threshold, preset2.foam.hats_threshold, alpha),
      lerp(preset1.foam.hats_folding, preset2.foam.hats_folding, alpha),
      lerp(preset1.foam.surface_folding_foam_mul, preset2.foam.surface_folding_foam_mul, alpha),
      lerp(preset1.foam.surface_folding_foam_pow, preset2.foam.surface_folding_foam_pow, alpha)));
  apply_wave_preset(water, preset, wind_dir);
}

void apply_wave_preset(FFTWater *water, const WavePresets &waves, float bf_scale, const Point2 &wind_dir)
{
  int preset1 = clamp<int>(bf_scale, 0, waves.presets.size() - 1);
  int preset2 = clamp<int>(bf_scale + 1, 0, waves.presets.size() - 1);
  float presetAlpha = bf_scale - preset1;

  apply_wave_preset(water, waves.presets[preset1], waves.presets[preset2], presetAlpha, wind_dir);
}

void apply_wave_preset(FFTWater *water, float bf_scale, const Point2 &wind_dir, Spectrum spectrum)
{
  apply_wave_preset(water, defWaves(spectrum), bf_scale, wind_dir);
}

void get_wave_preset(FFTWater *water, WavePreset &out_preset)
{
  Point2 windDir;
  float cascadeR;
  out_preset.period = get_period(water);
  get_wind_speed(water, out_preset.wind, windDir);
  get_roughness(water, out_preset.roughness, cascadeR);
  out_preset.smallWaveFraction = get_small_wave_fraction(water);
  out_preset.simulation = get_simulation_params(water);
  out_preset.foam = get_foam(water);
}

void set_wind(FFTWater *handle, float bf_scale, const Point2 &wind_dir)
{
  G_STATIC_ASSERT(BEAUFORT_SCALES_NUM == 7);
  const float BeaufortWindSpeed[BEAUFORT_SCALES_NUM] = {0.0f, 0.6f, 2.0f, 3.0f, 6.0f, 8.1f, 10.9};
  int speed = clamp<int>(bf_scale, 0, BEAUFORT_SCALES_NUM - 2);
  set_wind_speed(handle, lerp<float>(BeaufortWindSpeed[speed], BeaufortWindSpeed[speed + 1], saturate(bf_scale - speed)), wind_dir);
}
} // namespace fft_water