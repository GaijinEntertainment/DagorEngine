// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daSkies2/daSkies.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/unique_ptr.h>

class DaSkies;
class DataBlock;
class Point2;

struct DngSkies : public DaSkies
{
  DngSkies();
  ~DngSkies();

  bool updateSkyLightParams();
  float getCurrentSkyLightProgress() const { return skyLightProgress; }
  float getSkyLightSunAttenuation() const { return skyLightSunAttenuation; }
  float getSkyLightEnviAttenuation() const { return skyLightEnviAttenuation; }
  float getGiWeightAttenuation() const { return skyLightEnviAttenuation; }
  void setSkyLightParams(float progress, float sun_atten, float envi_atten, float gi_atten, float sky_atten, float base_sky_atten);
  void unsetSkyLightParams();
  void setAltitudeOffset(float alt_ofs) { altitudeOfs = alt_ofs; }
  void useFog(const Point3 &origin,
    SkiesData *data,
    const TMatrix &view_tm,
    const TMatrix4 &proj_tm,
    bool update_sky = true,
    float altitude_tolerance = SKY_PREPARE_THRESHOLD);
  void useFogNoScattering(const Point3 &origin,
    SkiesData *data,
    const TMatrix &view_tm,
    const TMatrix4 &proj_tm,
    bool update_sky = true,
    float altitude_tolerance = SKY_PREPARE_THRESHOLD);
  void prepare(const Point3 &dir_to_sun, bool force_update_cpu, float dt);

  // basically turns off sky influence completely (but doesn't spare computational cost)
  void enableBlackSkyRendering(bool black_sky_on);

  Point3 calcViewPos(const Point3 &view_pos) const { return view_pos + Point3(0, altitudeOfs, 0); }
  DPoint3 calcViewPos(const TMatrix &view_itm) const { return dpoint3(view_itm.getcol(3)) + DPoint3(0, altitudeOfs, 0); }
  TMatrix calcViewTm(const TMatrix &view_tm) const
  {
    if (altitudeOfs == 0.f)
      return view_tm;
    TMatrix itm = orthonormalized_inverse(view_tm);
    itm.m[3][1] += altitudeOfs;
    return orthonormalized_inverse(itm);
  }

protected:
  virtual bool canRenderStars() const override { return skyStars && !renderBlackSky; }

  float scatteringEffect = 1.0f;
  float lastScatteringEffect = 1.0f;

  float skyLightProgress = 0.0f;
  float skyLightSunAttenuation = 1.0f;
  float skyLightEnviAttenuation = 1.0f;
  float skyLightGiWeightAttenuation = 1.0f;

  float skyLightSkyAttenuation = 1.0f;
  float skyLightBaseSkyAttenuation = 1.0f;
  float altitudeOfs = 0;

  bool renderBlackSky = false;
};

void init_daskies();
void term_daskies();
DngSkies *get_daskies();
void load_daskies(const DataBlock &blk,
  float time_of_day = -1.f,
  const char *weather_blk = nullptr,
  int year = 1941,
  int month = 6,
  int day = 21,
  float lat = 55,
  float lon = 37);
void load_server_skies(
  const DataBlock &blk, float time_of_day = -1.f, int year = 1941, int month = 6, int day = 21, float lat = 55, float lon = 37);
void save_daskies(DataBlock &blk);
void update_delayed_weather_selection();
void before_render_daskies();
float get_daskies_time();
void set_daskies_time(float time_of_day);
void move_cumulus_clouds(const Point2 &amount);
void move_strata_clouds(const Point2 &amount);
void move_skies(const Point2 &amount);
int get_skies_seed();
void set_skies_seed(int seed);
Point3 calculate_server_sun_dir();