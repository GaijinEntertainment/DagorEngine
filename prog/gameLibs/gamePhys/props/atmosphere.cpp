// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamePhys/props/atmosphere.h"
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <util/dag_globDef.h>

using namespace gamephys;

/** Polynomials */
static float Density[5] = {1.f, -9.59387e-05f, 3.53118e-09f, -5.83556e-14f, 2.28719e-19f};
static float Pressure[5] = {1.f, -0.000118441f, 5.6763e-09f, -1.3738e-13f, 1.60373e-18f};
static float Temperature[5] = {1.f, -2.27712e-05f, 2.18069e-10f, -5.71104e-14f, 3.97306e-18f};

static Point3 wind_vel = Point3(0.f, 0.f, 0.f);

static inline float poly(float tab[], float v) { return (((tab[4] * v + tab[3]) * v + tab[2]) * v + tab[1]) * v + tab[0]; }

float atmosphere::stdP0 = 101300.f; // Standard pressure at sea level, Pa
float atmosphere::stdT0 = 288.16f;  // Standard temperature at sea level, K
float atmosphere::stdRo0 = 1.225f;  // Standard density [kg/m3] t=15`C, p=760 mm/1013 gPa

float atmosphere::_g = 9.81f;       // Earth gravity
float atmosphere::_P0 = 101300.f;   // Pressure at sea level, Pa
float atmosphere::_T0 = 288.16f;    // Temperature at sea level, K
float atmosphere::_ro0 = 1.225f;    // Density [kg/m3] t=15`C, p=760 mm/1013 gPa
float atmosphere::_Mu0 = 1.825e-6f; // Viscosity [Pa*sec]
float atmosphere::_hMax = 18300.0f; // Maximal altitude
float atmosphere::_water_density = 1000.0f;

/** Set atmosphere conditions at sea level.
  @param pressure    - mm of Mercury column,
  @param temperature - degrees on C scale  */
void atmosphere::set(float pressure, float temperature)
{
  pressure *= 101300.f / 760.f; // [Pa] , [N/m2]
  temperature += 273.16f;       // scale of Kelvin

  _P0 = pressure;
  _T0 = temperature;
  calcDensity0();
}

void atmosphere::calcDensity0() { _ro0 = 1.225f * (_P0 / 101300.f) * (288.16f / _T0); }

/** Reset atmosphere conditions at sea level to standard values*/
void atmosphere::reset()
{
  _P0 = stdP0;
  _T0 = stdT0;
  _ro0 = stdRo0;
}

/** Get Pressure( H [meters] ) , [ Pa ] */
float atmosphere::pressure(float h) { return P0() * poly(Pressure, min(h, hMax())) * (hMax() / max(hMax(), h)); }

/** Get Temperature( H [meters] ) , [ K ] */
float atmosphere::temperature(float h) { return T0() * poly(Temperature, min(h, hMax())); }

/** Get Sonic Speed( H [meters] ) , [ m/s ] */
float atmosphere::sonicSpeed(float h) { return 20.1f * sqrtf(temperature(h)); }

/** Get Density( H [meters] ) [kg*sec2/m4]  */
float atmosphere::density(float h) { return ro0() * poly(Density, min(h, hMax())) * (hMax() / max(hMax(), h)); }

/** Get {Dynamic Turbulent} viscosity( H [meters] ) , [ Pa*sec ] */
float atmosphere::viscosity(float h) { return Mu0() * powf(temperature(h) / T0(), 0.76); }

/** Get Kinetic(kinematic turbulent) Viscosity( T [K] ) , [ m2/sec ] */
float atmosphere::kineticViscosity(float h) { return viscosity(h) / density(h); }

void atmosphere::set_wind(const Point3 &in_wind_vel) { wind_vel = in_wind_vel; }

void atmosphere::set_wind(const Point3 &wind_dir, float beaufort) { wind_vel = wind_dir * (powf(beaufort, 1.5f) * 0.836f); }

Point3 atmosphere::get_wind() { return wind_vel; }

struct CalcAirDensityByAltitude
{
  CalcAirDensityByAltitude(float air_density) : airDensity(air_density) { ; }
  float airDensity;
  inline float operator()(float alt) const { return gamephys::atmosphere::ro0() * poly(Density, alt) - airDensity; }
};

static const float densityAtHmax = gamephys::atmosphere::ro0() * poly(Density, gamephys::atmosphere::hMax());

float atmosphere::getAltitudeByAirDensity(float air_density, float tolerance, float start_altitude)
{
  if (air_density < densityAtHmax)
    return densityAtHmax * hMax() / max(air_density, 1.0e-5f);
  else
  {
    CalcAirDensityByAltitude calcAirDensityByAltitude(air_density);
    float result = start_altitude;
    solve_newton(calcAirDensityByAltitude, start_altitude, tolerance, tolerance, 0.0f, 10, result);
    return result;
  }
}

struct CalcAirPressureByAltitude
{
  CalcAirPressureByAltitude(float air_pressure) : airPressure(air_pressure) { ; }
  float airPressure;
  inline float operator()(float alt) const { return gamephys::atmosphere::P0() * poly(Pressure, alt) - airPressure; }
};

static const float pressureAtHMax = gamephys::atmosphere::P0() * poly(Pressure, gamephys::atmosphere::hMax());

float atmosphere::getAltitudeByAirPressure(float air_pressure, float tolerance, float start_altitude)
{
  if (air_pressure < pressureAtHMax)
    return pressureAtHMax * hMax() / max(air_pressure, 1.0e-5f);
  else
  {
    CalcAirPressureByAltitude calcAirPressureByAltitude(air_pressure);
    float result = start_altitude;
    solve_newton(calcAirPressureByAltitude, start_altitude, tolerance, tolerance, 0.0f, 10, result);
    return result;
  }
}
