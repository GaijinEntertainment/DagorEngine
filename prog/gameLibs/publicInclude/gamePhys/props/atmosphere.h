//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
