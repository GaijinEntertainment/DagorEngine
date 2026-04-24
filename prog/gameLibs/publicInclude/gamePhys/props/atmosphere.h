//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_math.h>
#include <math/dag_Point3.h>

namespace gamephys
{
namespace atmosphere
{
extern float stdP0;  // Standard pressure at sea level, Pa
extern float stdT0;  // Standard temperature at sea level, K
extern float stdRo0; // Standard density [kg/m3] t=15`C, p=760 mm/1013 gPa

extern float _g;    // Earth gravity
extern float _P0;   // Pressure at sea level, Pa
extern float _T0;   // Temperature at sea level, K
extern float _ro0;  // Density [kg/m3] t=15`C, p=760 mm/1013 gPa
extern float _Mu0;  // Viscosity [Pa*sec]
extern float _hMax; // Maximal altitude
extern float _water_density;

inline constexpr float DENSITY_COEFFS[5] = {1.f, -9.59387e-05f, 3.53118e-09f, -5.83556e-14f, 2.28719e-19f};
inline constexpr float PRESSURE_COEFFS[5] = {1.f, -0.000118441f, 5.6763e-09f, -1.3738e-13f, 1.60373e-18f};
inline constexpr float TEMPERATURE_COEFFS[5] = {1.f, -2.27712e-05f, 2.18069e-10f, -5.71104e-14f, 3.97306e-18f};

/** Earth gravity */
inline float g() { return _g; }
inline void set_g(float value) { _g = value; };
inline void reset_g() { _g = 9.80665f; };

/** Pressure at sea level, Pa */
inline float P0() { return _P0; }

/** Temperature at sea level, K */
inline float T0() { return _T0; }

/** Density of air [kg/m3] (t=15`C, p=760 mm/1013 gPa ) */
inline float ro0() { return _ro0; }

inline float water_density() { return _water_density; }

/** Viscosity [Pa*sec] */
inline float Mu0() { return _Mu0; }

inline float hMax() { return _hMax; }


/** Set atmosphere conditions at sea level.
  @param pressure    - mm of Mercury column,
  @param temperature - degrees on C scale  */
void set(float pressure, float temperature);

/** Recalculates air density at sea level*/
void calcDensity0();

/** Reset atmosphere conditions at sea level to standard values*/
void reset();

/** Get Pressure( H [meters] ) , [ Pa ] */
float pressure(float h);

/** Get Temperature( H [meters] ) , [ K ] */
float temperature(float h);

/** Get Sonic Speed( H [meters] ) , [ m/s ] */
float sonicSpeed(float h);

/** Get Density( H [meters] ) [kg*sec2/m4]  */
float density(float h);

/** Get {Dynamic Turbulent} viscosity( H [meters] ) , [ Pa*sec ] */
float viscosity(float h);

/** Get Kinetic(kinematic turbulent) Viscosity( T [K] ) , [ m2/sec ] */
float kineticViscosity(float h);

void set_wind(const Point3 &wind_vel);
void set_wind(const Point3 &wind_dir, float beaufort);

Point3 get_wind();

// Helper functions
inline float calc_ias_coeff(float alt) { return sqrtf(density(alt) / stdRo0); }

inline float calc_tas_coeff(float alt) { return sqrtf(stdRo0 / density(alt)); }

inline float pitot_indicator(float h, float v) { return v * calc_ias_coeff(h); }

float getAltitudeByAirDensity(float air_density, float tolerance, float start_altitude = 0.0f);
float getAltitudeByAirPressure(float air_pressure, float tolerance, float start_altitude = 0.0f);
} // namespace atmosphere
} // namespace gamephys
