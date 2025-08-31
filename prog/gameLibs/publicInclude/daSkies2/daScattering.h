//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <generic/dag_carray.h>
#include <shaders/dag_postFxRenderer.h>
#include <math.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>
#include <drv/3d/dag_consts.h>


struct AtmosphereParameters;

#define _PARAM(type, name, def_value)      type name = def_value;
#define _PARAM_RAND(type, name, def_value) type name = def_value;
struct SkyAtmosphereParams
{
#define GROUND_PARAMS                        \
  _PARAM(float, average_ground_albedo, 0.1f) \
  _PARAM(float, min_ground_offset, 0)        \
  _PARAM(Color3, ground_color, Color3(1, 1, 1))
#define SKY_PARAMS                                        \
  _PARAM_RAND(float, mie2_thickness, 300.f)               \
  _PARAM_RAND(float, mie2_altitude, 0.0123f)              \
  _PARAM_RAND(float, mie2_scale, 0.f)                     \
  _PARAM_RAND(float, mie_height, 1.2f)                    \
  _PARAM_RAND(float, mie_scale, 1.f)                      \
  _PARAM_RAND(float, mie_absorption_scale, 1.0f)          \
  _PARAM(Color3, mie_scattering_color, Color3(1, 1, 1))   \
  _PARAM(Color3, mie_absorption_color, Color3(1, 1, 1))   \
  _PARAM_RAND(float, mie_assymetry, 0.8f)                 \
  _PARAM_RAND(float, mie_back_assymetry, 0.2f)            \
  _PARAM(float, mie_forward_weight, 0.5f)                 \
  _PARAM(float, planet_scale, 1.f)                        \
  _PARAM(float, atmosphere_scale, 1.f)                    \
  _PARAM_RAND(float, rayleigh_scale, 1.f)                 \
  _PARAM_RAND(float, rayleigh_alt_scale, 1.f)             \
  _PARAM(Color3, rayleigh_color, Color3(1, 1, 1))         \
  _PARAM(Color3, solar_irradiance_scale, Color3(1, 1, 1)) \
  _PARAM(float, multiple_scattering_factor, 1.f)          \
  _PARAM(float, ozone_alt_dist, 1.f)                      \
  _PARAM(float, ozone_max_alt, 1.f)                       \
  _PARAM_RAND(float, ozone_scale, 1.f)                    \
  _PARAM(float, sun_brightness, 1.f)                      \
  _PARAM(float, haze_strength, 1.f)                       \
  _PARAM(float, haze_min_angle, 0.f)                      \
  _PARAM(float, haze_peak_hour_offset, 0.f)               \
  _PARAM(float, moon_brightness, 1.f)                     \
  _PARAM(Color3, moon_color, Color3(1, 1, 1))

  SKY_PARAMS
  GROUND_PARAMS
  bool operator==(const SkyAtmosphereParams &a) const;
  bool operator!=(const SkyAtmosphereParams &a) const { return !((*this) == a); }
};
#undef _PARAM
#undef _PARAM_RAND

#define _PARAM(type, name, def_value)      a.name == name &&
#define _PARAM_RAND(type, name, def_value) a.name == name &&
inline bool SkyAtmosphereParams::operator==(const SkyAtmosphereParams &a) const { return SKY_PARAMS GROUND_PARAMS true; }
#undef _PARAM
#undef _PARAM_RAND

struct PreparedSkies;
struct PreparedSkiesParams
{
  int skiesLUTQuality = 1;         // 1..8. the more the better skies will be. Almost any number is good enough, unless we are above
                                   // atmosphere. It is not used at all when we have panorama. (0 = means off)
  int scatteringScreenQuality = 2; // 1..8 the more the better scattering resolution will be. This is highly depend on usage - if you
                                   // are in plane, you better have it higher.
  int scatteringDepthSlices = 16;  // 8...128 the more the better scattering depth resolution will be. 32 is something which is enough
                                   // for most of cases, 64 is enough for 99% amount. For cubic use 16 (and shorter range).
  int transmittanceColorQuality = 1; // 1...8. This is quality of transmittance COLOR only. So can be independently small, like you can
                                     // keep it always 1..2.
  float scatteringRangeScale = 1.0f; // if you are rendering something always closer than zfar, such as when use for cubic rendering,
                                     // reduce this (but remember, it also used with clouds now)
  float minScatteringRange = 100.f;  // if you use SkeisData for rendering clouds, but zfar is close (i.e. ~30km) or you want to
                                     // increase quality of clouds scattering - set that to, say 80000
  enum class Panoramic
  {
    OFF,
    ON
  } panoramic = Panoramic::OFF;
  enum class Reprojection
  {
    OFF,
    ON
  } reprojection = Reprojection::ON;
  PreparedSkiesParams() = default;
  PreparedSkiesParams(int lut, int scr, int slices, int trans, float scale, float range, Panoramic pan, Reprojection rep) :
    skiesLUTQuality(lut),
    scatteringScreenQuality(scr),
    scatteringDepthSlices(slices),
    transmittanceColorQuality(trans),
    scatteringRangeScale(scale),
    minScatteringRange(range),
    panoramic(pan),
    reprojection(rep)
  {}
};

PreparedSkies *create_prepared_skies(const char *name, const PreparedSkiesParams &params);
// legacy way
/*inline PreparedSkies *create_prepared_skies(const char *name, int prepared_quality_texsize = 128, bool panoramic = false)
{
  const int quality = clamp(prepared_quality_texsize/64, 1, 4);
  return create_prepared_skies(name,
    PreparedSkiesParams
    {
      quality,//skies
      quality*2,//scattering
      (quality+1)*32,//slices
      quality,//transmittance
      1.0f,//range
      panoramic ? PreparedSkiesParams::Panoramic::ON : PreparedSkiesParams::Panoramic::OFF,
      PreparedSkiesParams::Reprojection::ON
    }
  );
}*/

void destroy_prepared_skies(PreparedSkies *&);
void use_prepared_skies(PreparedSkies *skies, PreparedSkies *panorama_skies); // sets variables

class DaScattering
{
public:
  void setParams(const SkyAtmosphereParams &params); // sets params to shader. Some of them will have immediate effect (first order of
                                                     // scattering), some (other orders) will need precompute
  SkyAtmosphereParams getParams() const { return current; }
  void init();
  void close();
  void precompute(bool invalidate_cpu_data); // if invalidate_cpu_data, immediate requests will be cause heavy cpu computations
  void renderSky();
  bool prepareSkyForAltitude(const Point3 &origin, float tolerance, uint32_t gen, PreparedSkies *skies, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, uint32_t prepare_frame, PreparedSkies *panorama_skies, const Point3 &primary_light_dir,
    const Point3 &secondary_light_dir);

  //
  static Color3 getBaseSunColor();
  static Color3 getBaseMoonColor();
  static float getEarthRadius();

  Color3 getCpuTransmittance(float ht, float mu) const;
  Color3 getCpuIrradiance(float ht, float mu_s) const;
  Color3 getCpuFogLoss(const Point3 &camera, const Point3 &viewdir, float d) const;

  Color3 getCpuTransmittance(const Point3 &at, const Point3 &lightDir) const;
  Color3 getCpuIrradiance(const Point3 &camera, const Point3 &skies_sun_light_dir) const;

  // normalized! mul by sun or moon color
  Color3 getCpuFogSingleMieInscatter(const Point3 &cameraPosition, const Point3 &viewdir, float d, const Point3 &skies_sun_light_dir,
    int steps) const;
  Color3 getCpuFogSingleInscatter(const Point3 &cameraPosition, const Point3 &viewdir, float d, const Point3 &skies_sun_light_dir,
    int steps) const;
  Color3 getCpuFogLossIntegrate(const Point3 &cameraPosition, const Point3 &viewdir, float d) const; // this is done with direct
                                                                                                     // integration
  bool updateColors(); // to be called once per frame. return true if old vals are invalid

  void setCPUConsts();
  bool setStatisticalCloudsInfo(float clouds_ms_contribution, float clouds_ms_attenuation, float clouds_start_altitude2,
    float clouds_end_altitude2, float clouds_shadow_coverage, float global_clouds_sigma);

protected:
  Color3 getCpuSingleInscatter(const Point3 &camera, const Point3 &viewdir, float d, const Point3 &skies_sun_light_dir, int steps,
    bool need_rayleigh) const;
  void initMsApproximation();
  void initCommon();
  bool finishGpuReadback(bool force_sync_readback = false);
  void startGpuReadback();
  void invalidateCPUData();
  void updateCPUIrradiance(const uint16_t *, int stride); // read halves
  void prepareScattering(PreparedSkies &skies, int w, int h, int d);
  void setParamsToShader();
  void prepareFrustumScattering(PreparedSkies &skies, PreparedSkies *panorama_skies, const Point3 &primary_dir, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, bool restart_temporal);
  static float getPreparedDistSq();
  float getAtmosphereAltitudeKm() const;
  AtmosphereParameters getAtmosphere() const;
  void precompute_ms_approximation();

  SkyAtmosphereParams current;

  UniqueTexHolder skies_irradiance_texture, skies_transmittance_texture;
  UniqueBufHolder gpuAtmosphereCb;

  // approximated MS https://sebh.github.io/publications/egsr2020.pdf
  UniqueTexHolder skies_ms_texture;
  PostFxRenderer indirect_irradiance_ms, skies_approximate_ms_lut_ps;
  eastl::unique_ptr<ComputeShaderElement> skies_approximate_ms_lut_cs, skies_generate_transmittance_cs, indirect_irradiance_ms_cs;
  //--

  PostFxRenderer skies_generate_transmittance_ps;

  PostFxRenderer skies_view_lut_tex_prepare_ps;
  PostFxRenderer skies_render_screen;
  PostFxRenderer skies_prepare_transmittance_for_altitude_ps;
  eastl::unique_ptr<ComputeShaderElement> skies_view_lut_tex_prepare_cs;
  eastl::unique_ptr<ComputeShaderElement> skies_prepare_transmittance_for_altitude_cs;
  PostFxRenderer skies_integrate_froxel_scattering_ps;
  eastl::unique_ptr<ComputeShaderElement> skies_integrate_froxel_scattering_cs;

  bool gpuReadbackStarted = false;
  uint32_t multipleScatteringGeneration = 0;
  // for precompute
};
