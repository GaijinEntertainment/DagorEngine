#ifndef ATMOSPHERE_PARAMETERS
#define ATMOSPHERE_PARAMETERS 1

/*
<h3>Atmosphere parameters</h3>

<p>Using the above types, we can now define the parameters of our atmosphere
model. We start with the definition of density profiles, which are needed for
parameters that depend on the altitude:
*/

/*
The atmosphere parameters are then defined by the following struct:
*/

//this is const buffer <> CPU. so it must be aligned to float4!
struct AtmosphereParameters {
  // The solar irradiance at the top of the atmosphere.
  IrradianceSpectrum solar_irradiance;
  // The sun's angular radius. Warning: the implementation uses approximations
  // that are valid only if this angle is smaller than 0.1 radians.
  Angle sun_angular_radius;

  // The scattering coefficient of air molecules at the altitude where their
  // density is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The scattering coefficient at altitude h is equal to
  // 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
  ScatteringSpectrum rayleigh_scattering;
  // The density profile of air molecules, i.e. a function from altitude to
  // dimensionless values between 0 (null density) and 1 (maximum density).
  //DensityProfile rayleigh_density;
  Number rayleigh_density_altitude_exp_term;

  // The scattering coefficient of aerosols at the altitude where their density
  // is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The scattering coefficient at altitude h is equal to
  // 'mie_scattering' times 'mie_density' at this altitude.
  ScatteringSpectrum mie_scattering;
  // The density profile of aerosols, i.e. a function from altitude to
  // dimensionless values between 0 (null density) and 1 (maximum density).
  //DensityProfile mie_density;
  Number mie_density_altitude_exp_term;//todo: make more sophisticated

  // The extinction coefficient of aerosols at the altitude where their density
  // is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The extinction coefficient at altitude h is equal to
  // 'mie_extinction' times 'mie_density' at this altitude.
  ScatteringSpectrum mie_extinction;
  // The asymetry parameter for the Cornette-Shanks phase function for the
  // aerosols forward weight.
  Number mie_forward_scattering_weight;

  //second mie layer
  Number mie2_strength,//can be bigger than 1. making it relative to 'common mie'
         mie2_altitude,//where (and below) strength is 1
         mie2_density_altitude_exp_term;//
  Number padding;
  // The extinction coefficient of molecules that absorb light (e.g. ozone) at
  // the altitude where their density is maximum, as a function of wavelength.
  // The extinction coefficient at altitude h is equal to
  // 'absorption_extinction' times 'absorption_density' at this altitude.
  ScatteringSpectrum absorption_extinction;
  Length absorption_density_max_alt;//absorption profile part

  // The average albedo of the ground.
  DimensionlessSpectrum ground_albedo;
  // The cosine of the maximum Sun zenith angle for which atmospheric scattering
  // must be precomputed (for maximum precision, use the smallest Sun zenith
  // angle yielding negligible sky light radiance values. For instance, for the
  // Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
  Number mu_s_min;//to be removed

  // precalced
  DimensionlessSpectrum mie_extrapolation_coef;
  Number multiple_scattering_factor;

  // The distance between the planet center and the bottom of the atmosphere.
  Length bottom_radius;
  // The distance between the planet center and the top of the atmosphere.
  Length top_radius;

  // The asymetry parameter for the Cornette-Shanks phase function for the
  // aerosols.
  Number mie_phase_function_forward_g;
  Number mie_phase_function_backward_g;

  //float2(1.0 + g * g, - 2.0 * g)*pow(2.0 * (1.0 - g * g) / (2.0 + g * g), 1./-1.5);
  //optimized mie phase coef
  float4 mie_phase_consts;
  // The density profile of air molecules that absorb light (e.g. ozone), i.e.
  // a function from altitude to dimensionless values between 0 (null density)
  // and 1 (maximum density).
  //DensityProfile absorption_density;
  float2 absorption_density_linear_term0;
  float2 absorption_density_linear_term1;

  //https://en.wikipedia.org/wiki/Kruithof_curve
  //5500 kelvin + Kruithof effect results in (0.5764705882352941,0.6274509803921569, 1)
  // full moon lux brighntess is way darker
  DimensionlessSpectrum moon_color;//(0.5764705882352941,0.6274509803921569, 1)*.25 by default. That affects sky and moon
  Number sunBrightness;//this is 10 by default. that affects everything, sky and sun
};

#endif