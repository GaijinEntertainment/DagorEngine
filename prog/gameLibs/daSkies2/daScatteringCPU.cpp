// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>

#include <daSkies2/daScattering.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h>

#include "cpu_definitions.hlsli"
#include "shaders/atmosphere/atmosphere_params.hlsli"

#define TRANSMITTANCE_TEXTURE_SIZE_DEFINED 1
static constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 128;
static constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 32;
#include "shaders/atmosphere/texture_sizes.hlsli"

struct TransmittanceTexture
{
  static constexpr int Width = TRANSMITTANCE_TEXTURE_WIDTH;
  static constexpr int Height = TRANSMITTANCE_TEXTURE_HEIGHT;
  typedef float3 Data;
  mutable Data data[Width * Height];
  void invalidate()
  {
    for (int i = 0; i < Width * Height; ++i)
      data[i] = Data(-1, -1, -1);
  }
  TransmittanceTexture() { invalidate(); }
  inline Data eval(int x, int y) const;
  Data calc(int x, int y) const;
};

struct IrradianceTexture
{
  static constexpr int Width = IRRADIANCE_TEXTURE_WIDTH;
  static constexpr int Height = IRRADIANCE_TEXTURE_HEIGHT;
  typedef float3 Data;
  mutable Data data[Width * Height];
  inline Data eval(int x, int y) const;
  float3 calc(int x, int y) const;
  void invalidate()
  {
    for (int i = 0; i < Width * Height; ++i)
      data[i] = Data(-1, -1, -1);
  }
  IrradianceTexture() { invalidate(); }
};
static TransmittanceTexture lazy_transmittance_texture;
static IrradianceTexture lazy_irradiance_texture;


inline TransmittanceTexture::Data TransmittanceTexture::eval(int x, int y) const
{
  Data val = data[x + y * Width];
  if (val.x >= 0)
    return val;
  return calc(x, y);
}

inline TransmittanceTexture::Data IrradianceTexture::eval(int x, int y) const
{
  Data val = data[x + y * Width];
  if (val.x >= 0)
    return val;
  return calc(x, y);
}

template <typename Texture>
inline typename Texture::Data sample(const Texture &t, float2 uv)
{
  uv.x = clamp(uv.x * float(Texture::Width) - 0.5f, 0.f, float(Texture::Width - 1));
  uv.y = clamp(uv.y * float(Texture::Height) - 0.5f, 0.f, float(Texture::Height - 1));
  int uvX0 = int(uv.x), uvY0 = int(uv.y), uvX1 = min(uvX0 + 1, Texture::Width - 1), uvY1 = min(uvY0 + 1, Texture::Height - 1);
  const typename Texture::Data lt = t.eval(uvX0, uvY0), rt = t.eval(uvX1, uvY0), lb = t.eval(uvX0, uvY1), rb = t.eval(uvX1, uvY1);
  const float xf = uv.x - float(uvX0), yf = uv.y - float(uvY0);
  return lerp(lerp(lt, rt, xf), lerp(lb, rb, xf), yf);
}


inline float4 sample_texture(const TransmittanceTexture &t, const float2 &uv) { return float4(sample(t, uv), 0.f); }
inline float4 sample_texture(const IrradianceTexture &t, const float2 &uv) { return float4(sample(t, uv), 0.f); }

inline float max(float a, double b) { return max(a, (float)b); }
#define LOOP

#include "shaders/atmosphere/statistical_clouds_shadow.hlsli"
inline float3 exp2(const float3 &a) { return float3(exp2f(a.x), exp2f(a.y), exp2f(a.z)); }
#define SHADOWMAP_ENABLED 1
static float3 clouds_stat_shadows_sigma = {0, 0, 0};
static float3 clouds_stat_shadows_contribution = {0, 0, 0};
static float2 clouds_stat_shadows_params = {1, 1};
float getShadow(float3, float, float r, float mu)
{
  float len = getCloudsLength(r, mu, clouds_stat_shadows_params.x, clouds_stat_shadows_params.y);
  return dot(exp2(clouds_stat_shadows_sigma * len), clouds_stat_shadows_contribution);
}

#include "cpu_definitions_units.hlsli"
#include "shaders/atmosphere/transmittance.hlsli"
#include "shaders/atmosphere/functions.hlsli"

// used only on CPU part!
// probably we should not do that on CPU at all
__forceinline void ComputeSingleScatteringLimited(IN(AtmosphereParameters) atmosphere_p,
  IN(TransmittanceTexture) transmittance_texture, Length r, Number mu, Number mu_s, Number nu, Length maxDist, IN(int) SAMPLE_COUNT,
  bool ray_r_mu_intersects_ground, OUT(IrradianceSpectrum) rayleigh, OUT(IrradianceSpectrum) mie)
{
#if DAGOR_DBGLEVEL > 0
  if (r < atmosphere_p.bottom_radius || r > atmosphere_p.top_radius)
    logerr("exceeding atmosphere limits: %g (%g/%g)", r, atmosphere_p.bottom_radius, atmosphere_p.top_radius);
#endif
  r = max(r, atmosphere_p.bottom_radius);
  r = min(r, atmosphere_p.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(mu_s >= -1.0 && mu_s <= 1.0);
  assert(nu >= -1.0 && nu <= 1.0);
  auto ss = IntegrateScatteredLuminanceMS(atmosphere_p, transmittance_texture, MultipleScatteringTexture(), Position(0, 0, 0),
    Direction(0, 0, 0), 0, // only for shadows
    SAMPLE_COUNT, true, float2(SAMPLE_COUNT, SAMPLE_COUNT), r, mu, nu, mu_s,
    ray_r_mu_intersects_ground, // RayIntersectsGround(atmosphere_p, r, mu)
    maxDist);
  rayleigh = ss.ray * atmosphere_p.solar_irradiance;
  mie = ss.mie * atmosphere_p.solar_irradiance;
}

static AtmosphereParameters atmosphere;
AtmosphereParameters DaScattering::getAtmosphere() const { return atmosphere; }

TransmittanceTexture::Data TransmittanceTexture::calc(int x, int y) const
{
  Length r;
  Number mu;
  GetRMuFromTransmittanceTextureUv(atmosphere, float2((x + 0.5f) / Width, (y + 0.5f) / Height), r, mu);
  Data val =
    ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu, 2. * atmosphere.top_radius, 196, float3(0, 0, 0), float3(0, 0, 0));
  data[x + y * Width] = val;
  return val;
}

IrradianceTexture::Data IrradianceTexture::calc(int x, int y) const
{
  Length r;
  Number mu_s;
  GetRMuSFromIrradianceTextureUv(atmosphere, float2((x + 0.5f) / Width, (y + 0.5f) / Height), r, mu_s);
  Data val = ComputeIndirectIrradianceSingle(atmosphere, lazy_transmittance_texture, r, mu_s, 8, 16);
  data[x + y * Width] = max(val, Data(0, 0, 0));
  return val;
}

void DaScattering::invalidateCPUData()
{
  debug("invalidate CPU data");
  lazy_transmittance_texture.invalidate();
  lazy_irradiance_texture.invalidate();
}

float DaScattering::getEarthRadius() { return atmosphere.bottom_radius; }

Color3 DaScattering::getCpuFogLossIntegrate(const Point3 &camera, const Point3 &viewdir, float d) const
{
  d *= 0.001f;
  float distAbove = max(0.01, (camera.y - current.min_ground_offset) * 0.001f);
  float mu = viewdir.y, r = atmosphere.bottom_radius + distAbove;
  DimensionlessSpectrum loss =
    ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu, d, min(128, int(3 + d * 0.25)), float3(0, 0, 0), float3(0, 0, 0));
  loss = min(loss, float3(1.0f, 1.0f, 1.0f));
  return Color3(loss.x, loss.y, loss.z);
}

Color3 DaScattering::getCpuTransmittance(float ht, float mu) const
{
  Length r = atmosphere.bottom_radius + max(0.01, (ht - current.min_ground_offset) * 0.001f);
  DimensionlessSpectrum loss = GetTransmittanceToTopAtmosphereBoundary(atmosphere, lazy_transmittance_texture, r, mu);
  return Color3(loss.x, loss.y, loss.z);
}

Color3 DaScattering::getCpuFogLoss(const Point3 &camera, const Point3 &viewdir, float d) const
{
  if (d < 1000.f)
    return getCpuFogLossIntegrate(camera, viewdir, d);
  d *= 0.001f;
  float distAbove = max(0.01, (camera.y - current.min_ground_offset) * 0.001f);
  Number mu = viewdir.y;
  Length r = atmosphere.bottom_radius + distAbove;
  DimensionlessSpectrum loss =
    GetTransmittance(atmosphere, lazy_transmittance_texture, r, mu, d, RayIntersectsGround(atmosphere, r, mu));
  return Color3(loss.x, loss.y, loss.z);
}

Color3 DaScattering::getCpuIrradiance(float ht, float mu_s) const
{
  Length r = atmosphere.bottom_radius + max(0.001, (ht - current.min_ground_offset) * 0.001f);
  IrradianceSpectrum irr = GetIrradiance(atmosphere, lazy_irradiance_texture, r, mu_s);
  return Color3(irr.x, irr.y, irr.z);
}

void DaScattering::updateCPUIrradiance(const uint16_t *s, int stride)
{
  float3 *dst = lazy_irradiance_texture.data;
  for (int y = 0; y < IRRADIANCE_TEXTURE_HEIGHT;
       ++y, s = (const uint16_t *)(((const uint8_t *)s) + (stride - IRRADIANCE_TEXTURE_WIDTH * 4 * sizeof(*s))))
    for (int x = 0; x < IRRADIANCE_TEXTURE_WIDTH; ++x, s += 4, ++dst)
    {
      Point3_vec4 v;
      v_st(&v.x, v_half_to_float(s));
      *dst = v;
    }
}

__forceinline Color3 DaScattering::getCpuSingleInscatter(const Point3 &camera, const Point3 &viewdir, float d,
  const Point3 &skies_sun_light_dir, int steps, bool need_rayleigh) const
{
  const float maxDist = 64.f;
  d *= 0.001f;

  int csteps = min(steps, int(2 + d * (steps - 2.f) * (1.f / maxDist)));
  Length distAbove = max(0.01, (camera.y - current.min_ground_offset) * 0.001f);
  Length r = atmosphere.bottom_radius + distAbove;
  Number mu = viewdir.y, nu = clamp(viewdir * skies_sun_light_dir, -1.f, 1.f), mu_s = skies_sun_light_dir.y;

  if (r > atmosphere.top_radius)
  {
    Length rmu = r * mu;
    Length distance_to_top_atmosphere_boundary = -rmu - SafeSqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
    // If the viewer is in space and the view ray intersects the atmosphere, move
    // the viewer to the top atmosphere_p boundary (along the view ray):
    if (distance_to_top_atmosphere_boundary > 0.0 * meter)
    {
      r = atmosphere.top_radius;
      rmu += distance_to_top_atmosphere_boundary;
      mu = rmu / r;
    }
    else
      return Color3(0, 0, 0); // no mie
  }

  bool ray_r_theta_intersects_ground = false; //
  IrradianceSpectrum rayleigh, mie;
  ComputeSingleScatteringLimited(atmosphere, lazy_transmittance_texture, r, mu, mu_s, nu, d, csteps, ray_r_theta_intersects_ground,
    rayleigh, mie);
  if (!need_rayleigh)
    rayleigh = IrradianceSpectrum(0, 0, 0);
  RadianceSpectrum result = GetPhasedRadiance(atmosphere, rayleigh, mie, nu);
  return Color3(result.x, result.y, result.z);
}


Color3 DaScattering::getCpuFogSingleInscatter(const Point3 &camera, const Point3 &viewdir, float d, const Point3 &skies_sun_light_dir,
  int steps) const
{
  return getCpuSingleInscatter(camera, viewdir, d, skies_sun_light_dir, steps, true);
}

Color3 DaScattering::getCpuFogSingleMieInscatter(const Point3 &camera, const Point3 &viewdir, float d,
  const Point3 &skies_sun_light_dir, int steps) const
{
  return getCpuSingleInscatter(camera, viewdir, d, skies_sun_light_dir, steps, false);
}

Color3 DaScattering::getCpuTransmittance(const Point3 &at, const Point3 &lightDir) const
{
  return getCpuTransmittance(at.y, lightDir.y);
}

Color3 DaScattering::getCpuIrradiance(const Point3 &camera, const Point3 &skies_sun_light_dir) const
{
  return getCpuIrradiance(camera.y, skies_sun_light_dir.y);
}


static inline Point3 point3(const Color3 &a) { return Point3(a.r, a.g, a.b); }
static inline Color3 color3(const Point3 &a) { return Color3(a.x, a.y, a.z); }
static inline Color3 pow(const Color3 &a, float v) { return Color3(powf(a.r, v), powf(a.g, v), powf(a.b, v)); }

void DaScattering::setCPUConsts()
{
  static const AtmosphereParameters earthAtmosphere = {// The solar irradiance at the top of the atmosphere.
    float3(1.474000f, 1.850400f, 1.911980f),
    // The sun's angular radius. Warning: the implementation uses approximations
    0.004675f,

    // The scattering coefficient of air molecules at the altitude where their
    float3(0.005802f, 0.013558f, 0.033100f),
    // The density profile of air molecules, i.e. a function from altitude to
    -0.125000f,

    // The scattering coefficient of aerosols at the altitude where their density
    float3(0.003996f, 0.003996f, 0.003996f),
    // The density profile of aerosols, i.e. a function from altitude to
    -0.833333f,
    // The extinction coefficient of aerosols at the altitude where their density
    float3(0.004440f, 0.004440f, 0.004440f),
    // The asymetry parameter for the Cornette-Shanks phase function forward weight
    0.5f,

    // mie2
    0.f,        // mie2_strength
    0.1f,       // mie2_density_altitude
    -0.833333f, // mie2_density_altitude_exp_term
    0.f,        // padding

    // The extinction coefficient of molecules that absorb light (e.g. ozone) at
    float3(0.000650f, 0.001881f, 0.000085f), 25.0f,

    // The average albedo of the ground.
    float3(0.100000f, 0.100000f, 0.100000f),
    // The cosine of the maximum Sun zenith angle for which atmospheric scattering
    -0.207912f,

    // precalced
    float3(1.f, 0.005802 / 0.013558, 0.005802 / 0.033100),
    1.f, // ms

    // The distance between the planet center and the bottom of the atmosphere.
    6360.000000f,
    // The distance between the planet center and the top of the atmosphere.
    6440.000000f,

    // The asymetry parameter for the Cornette-Shanks phase function forward
    0.8f,

    // The asymetry parameter for the Cornette-Shanks phase function backward
    -0.2f,
    // optimized mie phase coef
    float4(GetMiePhaseConsts(0.8f, 0.5f).x, GetMiePhaseConsts(0.8f, 0.5f).y, GetMiePhaseConsts(-0.2f, 0.5f).x,
      GetMiePhaseConsts(-0.2f, 0.5f).y),
    // The density profile of air molecules that absorb light (e.g. ozone), i.e.
    float2(1. / (25 - 10), -10. / (25 - 10)), float2(1. / (25 - 40), -40. / (25 - 40)),
    float3(0.5764705882352941f, 0.6274509803921569f, 1.f) * 0.25f, // 5500 kelvin + Kruithof effect results in
                                                                   // (0.5764705882352941f,0.6274509803921569f, 1f) *0.25
    12.5f};
  atmosphere = earthAtmosphere;
  atmosphere.solar_irradiance =
    earthAtmosphere.solar_irradiance * max(point3(current.solar_irradiance_scale), Point3(0.001, 0.001, 0.001));
  atmosphere.bottom_radius = earthAtmosphere.bottom_radius * current.planet_scale;
  const float totalAtmosphereScale = current.planet_scale * current.atmosphere_scale;
  atmosphere.top_radius =
    atmosphere.bottom_radius + (earthAtmosphere.top_radius - earthAtmosphere.bottom_radius) * totalAtmosphereScale;
  const float atmosphereAlt = (atmosphere.top_radius - atmosphere.bottom_radius);
  // ozone
  const float earthMaxOzoneAlt = 25.f, earthOzoneFalloffAlt = 15.f;
  const float ozone_max_alt = min(current.ozone_max_alt * totalAtmosphereScale * earthMaxOzoneAlt, atmosphereAlt - 0.1f);
  const float alt_dist = current.ozone_alt_dist * earthOzoneFalloffAlt * totalAtmosphereScale;
  const float ozone_start_alt = max(ozone_max_alt - alt_dist, 0.1f);
  const float ozone_end_alt = min(ozone_max_alt + alt_dist, atmosphereAlt);
  atmosphere.absorption_extinction = earthAtmosphere.absorption_extinction * current.ozone_scale;
  // atmosphere.mu_s_min = -0.99;

  atmosphere.absorption_density_max_alt = ozone_max_alt;
  atmosphere.absorption_density_linear_term0 = float2(1, -ozone_start_alt) / (ozone_max_alt - ozone_start_alt);
  atmosphere.absorption_density_linear_term1 = float2(1, -ozone_end_alt) / (ozone_max_alt - ozone_end_alt);
  // rayleigh
  atmosphere.rayleigh_scattering = current.rayleigh_scale * mul(point3(current.rayleigh_color), earthAtmosphere.rayleigh_scattering);
  atmosphere.rayleigh_density_altitude_exp_term =
    earthAtmosphere.rayleigh_density_altitude_exp_term * current.rayleigh_alt_scale / totalAtmosphereScale;
  // mie
  Point3 mie_color = max(current.mie_scale * point3(current.mie_scattering_color), Point3(0.00001, 0.00001, 0.00001));
  atmosphere.mie_scattering = earthAtmosphere.mie_scattering.x * mie_color;
  atmosphere.mie_extinction = atmosphere.mie_scattering + (earthAtmosphere.mie_extinction.x - earthAtmosphere.mie_scattering.x) *
                                                            current.mie_scale * current.mie_absorption_scale *
                                                            point3(current.mie_absorption_color);
  atmosphere.mie_extinction = max(atmosphere.mie_scattering * 1.000001, atmosphere.mie_extinction); // we can't extinct less than we
                                                                                                    // scatter

  atmosphere.mie_density_altitude_exp_term = -1. / max(current.mie_height * totalAtmosphereScale, 0.0001);
  atmosphere.mie2_strength = current.mie_scale > 0.00001f ? current.mie2_scale / current.mie_scale : 0.f;
  atmosphere.mie2_altitude = current.mie2_altitude / 1000.f; // not affected by planet/atmosphere scale, intentionally
  atmosphere.mie2_density_altitude_exp_term = -1.f / max(current.mie2_thickness / (3 * 1000.0f), 0.00001f); // not affected by
                                                                                                            // planet/atmosphere scale,
                                                                                                            // intentionally

  // atmosphere.sun_angular_radius = current.sun_size*earthAtmosphere.sun_angular_radius;

  atmosphere.multiple_scattering_factor = current.multiple_scattering_factor;
  Color3 ground_color = min(current.ground_color, Color3(1, 1, 1));
  atmosphere.ground_albedo = DimensionlessSpectrum(ground_color.r, ground_color.g, ground_color.b) * current.average_ground_albedo;

  // 5500 kelvin + Kruithof effect
  atmosphere.moon_color =
    mul(Point3(0.5764705882352941, 0.6274509803921569, 1) * 0.25f, point3(current.moon_color)) * current.moon_brightness;
  atmosphere.sunBrightness = 12.f * current.sun_brightness;

  // mie optimized precalc
  atmosphere.mie_forward_scattering_weight = clamp(current.mie_forward_weight, 0.001f, 0.999f);
  atmosphere.mie_phase_function_forward_g = current.mie_assymetry;
  atmosphere.mie_phase_function_backward_g = -current.mie_back_assymetry;
  float2 forward = GetMiePhaseConsts(atmosphere.mie_phase_function_forward_g, atmosphere.mie_forward_scattering_weight);
  float2 backward = GetMiePhaseConsts(atmosphere.mie_phase_function_backward_g, (1 - atmosphere.mie_forward_scattering_weight));
  atmosphere.mie_phase_consts = float4(forward.x, forward.y, backward.x, backward.y);
  atmosphere.mie_extrapolation_coef = GetExtrapolatedSingleMieScatteringCoefConst(atmosphere);
}

inline bool equal(const float2 &a, const float2 &b) { return is_relative_equal_float(a.x, b.x) && is_relative_equal_float(a.y, b.y); }
inline bool equal(const float3 &a, const float3 &b)
{
  return is_relative_equal_float(a.x, b.x) && is_relative_equal_float(a.y, b.y) && is_relative_equal_float(a.z, b.z);
}

bool DaScattering::setStatisticalCloudsInfo(float clouds_ms_contribution, float clouds_ms_attenuation, float clouds_start_altitude2,
  float clouds_end_altitude2, float clouds_shadow_coverage, float global_clouds_sigma)
{
  const float contributionNorm = 1. / (1 + clouds_ms_contribution + clouds_ms_contribution * clouds_ms_contribution);
  // 1.44269504089 - is log2(e)
  const float3 ms_sigma =
    float3(1, clouds_ms_attenuation, clouds_ms_attenuation * clouds_ms_attenuation) * global_clouds_sigma * 1.44269504089;
  const float3 new_shadow_contrib =
    float3(1, clouds_ms_contribution, clouds_ms_contribution * clouds_ms_contribution) * contributionNorm;
  const float3 new_stat_shadows_sigma = ms_sigma * clouds_shadow_coverage * 1000 * 0.5;
  const float2 new_stat_shadows_params =
    float2(sqr(getEarthRadius() + clouds_start_altitude2), sqr(getEarthRadius() + clouds_end_altitude2));
  bool ret = false;
  if (!equal(new_stat_shadows_sigma, clouds_stat_shadows_sigma) || !equal(new_shadow_contrib, clouds_stat_shadows_contribution) ||
      !equal(new_stat_shadows_params, clouds_stat_shadows_params))
    ret = (new_stat_shadows_sigma.x < 0);
  clouds_stat_shadows_contribution = new_shadow_contrib;
  clouds_stat_shadows_sigma = new_stat_shadows_sigma;
  clouds_stat_shadows_params = new_stat_shadows_params;
  return ret;
}
float DaScattering::getPreparedDistSq() { return SKIES_PREPARED_SHORT_PART_SQ; }
float DaScattering::getAtmosphereAltitudeKm() const { return atmosphere.top_radius - atmosphere.bottom_radius; }

Color3 DaScattering::getBaseMoonColor() { return color3(atmosphere.moon_color); }
Color3 DaScattering::getBaseSunColor() { return Color3(atmosphere.sunBrightness, atmosphere.sunBrightness, atmosphere.sunBrightness); }
