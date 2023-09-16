#include <daSkies2/daSkies.h>
#include <astronomy/astronomy.h>

void DaSkies::setAstronomicalSunMoon()
{
  double lst = calc_lst_hours(starsJulianDay, starsLongtitude);
  star_catalog::StarDesc moon, sun;
  get_moon(starsJulianDay, moon);
  get_sun(starsJulianDay, sun);
  Point3 currentMoonDir, sunDir;
  radec_2_xyz(moon.rightAscension, moon.declination, lst, starsLatitude, currentMoonDir.x, currentMoonDir.y, currentMoonDir.z);
  radec_2_xyz(sun.rightAscension, sun.declination, lst, starsLatitude, sunDir.x, sunDir.y, sunDir.z);
  if (initialAzimuthAngle > 0.0f)
  {
    Quat rotY(Point3(0.0f, 1.0f, 0.0), initialAzimuthAngle * DEG_TO_RAD);
    sunDir = rotY * sunDir;
    currentMoonDir = rotY * currentMoonDir;
  }
  real_skies_sun_light_dir = sunDir;
  setMoonDir(currentMoonDir);
  setMoonAge(get_moon_age(starsJulianDay));

  const float startAppearSunAsMoon = -0.1f, sunFullyMoon = -0.15f;
  moonEffect = clamp((startAppearSunAsMoon - real_skies_sun_light_dir.y) / (startAppearSunAsMoon - sunFullyMoon), 0.f, 1.f);
}

void DaSkies::setMoonAge(float age)
{
  moonAge = age;
  moonAge = fmodf(moonAge, MOON_PERIOD);
  if (moonAge < 0)
    moonAge += MOON_PERIOD;
}

void DaSkies::calcSunMoonDir(float forward_time, Point3 &sun_dir, Point3 &moon_dir) const
{
  double julianDay = starsJulianDay + forward_time / 24.0 / 60.0 / 60.0;
  double lst = calc_lst_hours(julianDay, starsLongtitude);

  star_catalog::StarDesc moon, sun;
  get_moon(julianDay, moon);
  get_sun(julianDay, sun);
  radec_2_xyz(moon.rightAscension, moon.declination, lst, starsLatitude, moon_dir.x, moon_dir.y, moon_dir.z);
  radec_2_xyz(sun.rightAscension, sun.declination, lst, starsLatitude, sun_dir.x, sun_dir.y, sun_dir.z);
  if (initialAzimuthAngle > 0.0f)
  {
    Quat rotY(Point3(0.0f, 1.0f, 0.0), initialAzimuthAngle * DEG_TO_RAD);
    sun_dir = rotY * sun_dir;
    moon_dir = rotY * moon_dir;
  }
}

void DaSkies::setSunAndMoonDir(const Point3 &sun_dir, const Point3 &moon_dir)
{
  real_skies_sun_light_dir = normalize(sun_dir);
  moonDir = normalize(moon_dir);
}
