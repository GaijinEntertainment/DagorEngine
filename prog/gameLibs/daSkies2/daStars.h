#pragma once
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_resPtr.h>

class DataBlock;

class DaStars
{
public:
  void renderStars(const Driver3dPerspective &persp, float sun_brightness, float latitude = 55, float longtitude = 37,
    double julian_day = 2457388.5, float initial_azimuth_angle = 0.0f, float stars_intensity_mul = 1.0f); // 2016-01-01:00
  void renderMoon(const Point3 &origin, const Point3 &moon_dir, float moonAge, float sun_brightness);
  void renderCelestialObject(TEXTUREID tid, const Point3 &dir, float phase, float intensity, float size);
  bool setMoonVars(const Point3 &origin, const Point3 &moonDir, float moonAge, float sun_brightness); // if true moon should be
                                                                                                      // rendererd
  float getMoonIntenisty(const Point3 &origin, const Point3 &moonDir, float moonAgeDays, float sun_brightness, float &moonAgePercent);
  bool setMoonResourceName(const char *res_name);
  void setMoonSize(float size);
  TEXTUREID getMoonTexId() const { return moonTex.getTexId(); }

  void init(const char *star_texture, const char *moon_texture);
  void close();
  DaStars() = default;
  ~DaStars();

protected:
  ShaderMaterial *moonMaterial = nullptr;
  ShaderElement *moonElem = nullptr;
  ShaderMaterial *starsMaterial = nullptr;
  dynrender::RElem starsRendElem;
  UniqueBuf starsVb;
  UniqueBuf starsIb;
  SharedTexHolder starsTex, moonTex;
  float moonSize = 0.f;

  void generateStars();
  void resetMoonResourceName();
};