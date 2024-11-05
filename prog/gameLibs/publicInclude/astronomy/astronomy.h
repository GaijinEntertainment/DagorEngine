//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "starsCatalog.h"
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <util/dag_globDef.h>

struct Polar
{
  Polar() : phi(0.f), theta(0.f), r(0.f) {}

  Polar(double az, double elev, double r_ = 1.f) : phi(az), theta(elev), r(r_) {}

  double phi;   // azimuth of vector
  double theta; // altitude of vector
  double r;     // norm of vector
};


inline Point3 &polar_to_vec(const Polar &pol, Point3 &out)
{
  const double cosEl = cos(pol.theta);

  out.x = pol.r * cos(pol.phi) * cosEl;
  out.y = pol.r * sin(pol.phi) * cosEl;
  out.z = pol.r * sin(pol.theta);

  return out;
}


inline Polar &vec_to_polar(const Point3 &vec, Polar &out)
{
  // Length of projection in x-y-plane:
  const double rhoSqr = vec.x * vec.x + vec.y * vec.y;

  // Norm of vector
  out.r = safe_sqrt(rhoSqr + vec.z * vec.z);

  // Azimuth of vector
  if ((vec.x == 0.f) && (vec.y == 0.f))
    out.phi = 0.0;
  else
    out.phi = atan2(vec.y, vec.x);

  if (out.phi < 0.f)
    out.phi += 2.f * PI;

  // Altitude of vector
  const double rho = safe_sqrt(rhoSqr);
  if ((vec.z == 0.f) && (rho == 0.f))
    out.theta = 0.f;
  else
    out.theta = atan2((double)vec.z, rho);

  return out;
}


inline double julian_day(unsigned int year, unsigned int month, unsigned int day, double time)
{
  if (month <= 2)
  {
    month = month + 12;
    year = year - 1;
  }
  month = month - 3;
  year += 4800;
  double jdn =
    day + floor((153. * month + 2.) / 5.) + 365. * year + floor(year / 4.) - floor(year / 100.) + floor(year / 400.) - 32045;
  return jdn + (time - 12.) / 24;
  /*
    if ( month <= 2 )
    {
      month = month + 12-3;
      year = year - 1;
    }
    return (int)(365.25*year) + (int)(30.6001*(month+1)) - 15 + 1720996.5 + day + time/24.0;*/
}

inline double julian_century(double jd) { return (jd - 2451545) / 36525; }

inline double julian_get_time(double jd, double time) { return floor(jd) + (time - 12.0) / 24.0; }


inline double calc_lst_degrees(double jd, real longitude) // lontitude in degrees
{
  jd -= 2451545.0;          /* set relative to 2000.0 */
  double jdm = jd / 36525.; /* convert jd to julian centuries */
  double intPart = floor(jd);
  jd -= intPart;
  double greenwichSiderialTime =
    280.46061837 + 360.98564736629 * jd + .98564736629 * intPart + jdm * jdm * (3.87933e-4 - jdm / 38710000.);

  double localSiderialTime = greenwichSiderialTime + longitude;
  localSiderialTime -= static_cast<int>(localSiderialTime / 360) * 360.0;
  if (localSiderialTime < 0)
    localSiderialTime += 360.0;

  return localSiderialTime;
}


inline double calc_lst_hours(double jd, float longitude) // longitude in degrees
{
  return calc_lst_degrees(jd, longitude) / 15.0f;
}


inline void altaz_2_xyz(real alt, real az, real &up, real &west, real &thirdAxis)
{
  Point3 v = polar_to_vec(Polar(az, alt), v);
  thirdAxis = v.x;
  west = v.z;
  up = v.y;
}


inline void radec_2_altaz_old(real ra, real dec, real lst, real latitude, real &alt, real &az)
{
  double h = (lst - ra) * 15.0f;

  h = DegToRad(h);
  double declRad = DegToRad(dec);
  double latRad = DegToRad(latitude);

  Point3 e_equ = polar_to_vec(Polar(h, declRad), e_equ); // unit vector in horizontal system
  Point3 e_hor = rotyTM(HALFPI - latRad) * e_equ;        // unit vector in equatorial system

  Polar e_hor_pol = vec_to_polar(e_hor, e_hor_pol);

  alt = static_cast<real>(e_hor_pol.theta);
  az = -static_cast<real>(e_hor_pol.phi) - HALFPI;
}

inline void radec_2_altaz(real ra, real dec, real lst, real latitude, real &alt, real &az)
{
  double h = (lst - ra) * 15.0f;

  h = DegToRad(h);
  double declRad = DegToRad(dec);
  double latRad = DegToRad(latitude);

  double sinDecl = sin(declRad), cosDecl = cos(declRad);
  double sinLat = sin(latRad), cosLat = cos(latRad);
  double sinALT = sinDecl * sinLat + cosDecl * cosLat * cos(h);

  double cosA = (sinDecl - sinALT * sinLat) / (safe_sqrt(1 - sinALT * sinALT) * cosLat);

  alt = asin(sinALT);
  az = acos(cosA);
  if (sin(h) < 0)
    az = TWOPI - az;
}


inline void radec_2_xyz(real ra, real dec, real lst, real latitude, real &x, real &y, real &z)
{
  double h = (lst - ra) * 15.0f;

  h = DegToRad(h);
  double declRad = DegToRad(dec);
  double latRad = DegToRad(latitude);

  double sinDecl = sin(declRad), cosDecl = cos(declRad);
  double sinLat = sin(latRad), cosLat = cos(latRad);
  double sinALT = sinDecl * sinLat + cosDecl * cosLat * cos(h);

  double cosA = (sinDecl - sinALT * sinLat) / (safe_sqrt(1 - sinALT * sinALT) * cosLat);

  double sinA = safe_sqrt(1 - cosA * cosA);

  if (sin(h) > 0)
    sinA = -sinA;

  double cosAlt = safe_sqrt(1 - sinALT * sinALT);
  x = sinA * cosAlt;
  y = sinALT;
  z = cosA * cosAlt;
}


inline double Frac(double x) { return x - floor(x); }


inline star_catalog::StarDesc &get_sun(double jd, star_catalog::StarDesc &out)
{

  double t = julian_century(jd);

  double l, m;

  // Mean anomaly and ecliptic longitude
  m = TWOPI * Frac(0.993133 + 99.997361 * t);
  l = TWOPI * Frac(0.7859453 + m / TWOPI + (6893.0 * sin(m) + 72.0 * sin(2.0 * m) + 6191.2 * t) / 1296.0e3);

  const double eps = 23.43929111 * DEG_TO_RAD;
  // Equatorial coordinates
  Point3 e_Sun = rotxTM(-eps) * polar_to_vec(Polar(l, 0.f), e_Sun);
  Polar e_Sun_pol = vec_to_polar(e_Sun, e_Sun_pol);

  out.rightAscension = static_cast<real>(RadToDeg(e_Sun_pol.phi) / 15);
  out.declination = static_cast<real>(RadToDeg(e_Sun_pol.theta));

  return out;
}
#define MOON_PERIOD 29.530588853
inline float get_moon_age(double jd)
{
  float age = fmod(jd - 2451550.1, MOON_PERIOD);
  if (age < 0)
    age += MOON_PERIOD;
  return age;
}
inline star_catalog::StarDesc &get_moon(double jd, star_catalog::StarDesc &out)
{
  // Constants
  const double T = julian_century(jd);
  const double eps = 23.43929111 * DEG_TO_RAD;
  const double Arcs = 3600.0 * 180.0 / PI;
  double L_0, l, ls, F, D, dL, S, h, N;
  double l_Moon, b_Moon;

  // Mean elements of lunar orbit
  L_0 = Frac(0.606433 + 1336.855225 * T); // mean longitude [rev]

  l = TWOPI * Frac(0.374897 + 1325.552410 * T); // Moon's mean anomaly
  ls = TWOPI * Frac(0.993133 + 99.997361 * T);  // Sun's mean anomaly
  D = TWOPI * Frac(0.827361 + 1236.853086 * T); // Diff. long. Moon-Sun
  F = TWOPI * Frac(0.259086 + 1342.227825 * T); // Dist. from ascending node


  // Perturbations in longitude and latitude
  dL = +22640 * sin(l) - 4586 * sin(l - 2 * D) + 2370 * sin(2 * D) + 769 * sin(2 * l) - 668 * sin(ls) - 412 * sin(2 * F) -
       212 * sin(2 * l - 2 * D) - 206 * sin(l + ls - 2 * D) + 192 * sin(l + 2 * D) - 165 * sin(ls - 2 * D) - 125 * sin(D) -
       110 * sin(l + ls) + 148 * sin(l - ls) - 55 * sin(2 * F - 2 * D);
  S = F + (dL + 412 * sin(2 * F) + 541 * sin(ls)) / Arcs;
  h = F - 2 * D;
  N = -526 * sin(h) + 44 * sin(l + h) - 31 * sin(-l + h) - 23 * sin(ls + h) + 11 * sin(-ls + h) - 25 * sin(-2 * l + F) +
      21 * sin(-l + F);

  // Ecliptic longitude and latitude
  l_Moon = TWOPI * Frac(L_0 + dL / 1296.0e3); // [rad]
  b_Moon = (18520.0 * sin(S) + N) / Arcs;     // [rad]

  // Equatorial coordinates
  Point3 e_Moon = rotxTM(-eps) * polar_to_vec(Polar(l_Moon, b_Moon), e_Moon);
  Polar e_Moon_pol = vec_to_polar(e_Moon, e_Moon_pol);

  out.rightAscension = static_cast<real>(RadToDeg(e_Moon_pol.phi) / 15.0);
  out.declination = static_cast<real>(RadToDeg(e_Moon_pol.theta));

  return out;
}


inline Point3 &calculate_sun_dir(unsigned int year, unsigned int month, unsigned int day, double time, real latitude, real longitude,
  Point3 &out)
{
  double jd = julian_day(year, month, day, time);
  double lst = calc_lst_hours(jd, longitude);

  // Get brightness of brightest star
  star_catalog::StarDesc s = get_sun(jd, s);
  radec_2_xyz(s.rightAscension, s.declination, lst, latitude, out.z, out.y, out.x);

  out.normalize();
  out = -out;

  return out;
}

inline Point3 &calculate_moon_dir(unsigned int year, unsigned int month, unsigned int day, double time, real latitude, real longitude,
  Point3 &out)
{
  double jd = julian_day(year, month, day, time);
  double lst = calc_lst_hours(jd, longitude);

  // Get brightness of brightest star
  star_catalog::StarDesc s = get_moon(jd, s);
  radec_2_xyz(s.altitude, s.azimuth, lst, latitude, out.z, out.y, out.x);

  out.normalize();
  out = -out;

  return out;
}

struct StarHighFn
{
  StarHighFn(float in_longitude, float in_latitude, int in_year, int in_month, int in_day, float in_target, bool in_moon) :
    longitude(in_longitude), latitude(in_latitude), year(in_year), month(in_month), day(in_day), target(in_target), moon(in_moon)
  {}

  Point3 calcDir(float gtm)
  {
    double starsJulianDay = julian_day(year, month, day, gtm);
    double lst = calc_lst_hours(starsJulianDay, longitude);
    star_catalog::StarDesc starDesc;
    moon ? get_moon(starsJulianDay, starDesc) : get_sun(starsJulianDay, starDesc);
    Point3 dir;
    radec_2_xyz(starDesc.rightAscension, starDesc.declination, lst, latitude, dir.z, dir.y, dir.x);
    return dir;
  }

  float operator()(float gtm) { return -target + calcDir(gtm).y; }

  float longitude;
  float latitude;
  int year;
  int month;
  int day;
  float target;
  bool moon;
};

inline float calculate_local_time_of_day(float latitude, int year, int month, int day, float time_of_day)
{
#define TIME_EPS       0.001
#define NIGHT_MIN_SUN  -0.15
#define NIGHT_MAX_MOON 0.15

  StarHighFn starFn = StarHighFn(0.0f, latitude, year, month, day, 0.0f, false);
  double midday = golden_section_calculate(0.0, 24.0, TIME_EPS, 1, starFn);
  double midnight = golden_section_calculate(0.0, 24.0, TIME_EPS, -1, starFn);
  bool night = time_of_day < 0.0f;
  time_of_day = fabsf(time_of_day);

  double sundawn = (midnight + midday) * 0.5f;
  starFn.target = night ? NIGHT_MIN_SUN : 0.0f;
  if (sign(starFn(midday)) != sign(starFn(midnight)))
    sundawn = secant_calculate(midday, midnight, TIME_EPS, starFn);
  starFn.target = 0.0f;
  double dayLen = fabs(sundawn - midday) * 2.0;
  sundawn = midday - dayLen * 0.5f;

  double result = sundawn;
  if (night)
  {
    starFn.moon = true;
    double nightLen = 24.0 - dayLen;
    double moonMax = golden_section_calculate(sundawn - nightLen, sundawn, TIME_EPS, 1, starFn);
    moonMax =
      starFn(moonMax) > starFn(sundawn) ? (starFn(moonMax) > starFn(sundawn - nightLen) ? moonMax : sundawn - nightLen) : sundawn;
    double moonMin = sundawn;
    starFn.target = NIGHT_MAX_MOON;
    if (time_of_day < 0.5f)
    {
      if (sign(starFn(sundawn)) != sign(starFn(moonMax)))
        moonMin = secant_calculate(moonMax, sundawn, TIME_EPS, starFn);
      else
        moonMin = sundawn;
      time_of_day = time_of_day * 2.0f;
    }
    else
    {
      if (sign(starFn(sundawn - nightLen)) != sign(starFn(moonMax)))
        moonMin = secant_calculate(sundawn - nightLen, moonMax, TIME_EPS, starFn);
      else
        moonMin = sundawn - nightLen;
      time_of_day = 1.0f - (time_of_day - 0.5f) * 2.0f;
    }
    starFn.target = 0.0f;
    result = lerp(moonMin, moonMax, time_of_day);
  }
  else
  {
    result = lerp(sundawn, sundawn + dayLen, time_of_day);
  }

  return result;
}
