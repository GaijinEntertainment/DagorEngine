//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <image/dag_texPixel.h>
#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <EASTL/string.h>

struct AmbientWindParameters
{
  Point4 windMapBorders;
  Point2 windDir;
  float windStrength; // beaufort scale
  float windNoiseStrength;
  float windNoiseSpeed;
  float windNoiseScale;
  float windNoisePerpendicular;
  float windGrassNoiseStrength;
  float windGrassNoiseSpeed;
  float windGrassNoiseScale;
  float windGrassNoisePerpendicular;
};

class AmbientWind
{
public:
  static float beaufort_to_meter_per_second(float beaufort);
  static float meter_per_second_to_beaufort(float meter_per_second);

  void setWindParameters(const DataBlock &blk);
  void setWindTextures(const DataBlock &blk);

  void setWindParameters(const Point4 &wind_map_borders, const Point2 &wind_dir, float wind_strength_beaufort,
    float wind_noise_strength_multiplier, float wind_noise_speed_beaufort, float wind_noise_scale, float wind_noise_perpendicular);
  void setWindParametersWithGrass(const AmbientWindParameters &params);
  void setWindTextures(const char *wind_flowmap_name, const char *wind_noisemap_name = "");
  void init();
  void update();
  void close();
  void enable();
  void disable();

  AmbientWind() { init(); };
  ~AmbientWind() { close(); };


  float getWindSpeed(); // return speed in mps. Use meter_per_second_to_beaufort after to get beaufort.
  const Point2 &getWindDir();

private:
  static AmbientWind *instance;

  void fillFlowmapTexFallback(const Point2 &wind_dir);
  void setWindParametersToShader(float wind_noise_strength_multiplier, float wind_noise_speed_beaufort, float wind_noise_scale,
    float wind_noise_perpendicular, const Point4 &wind_map_borders);
  void setWindParametersToShader(const AmbientWindParameters &params);

  bool parametersInitialized = false;

  bool enabled = false;
  float windSpeed = 0.0f;
  Point2 windDir = Point2(1, 0);
  float noiseStrength = 0.0f;
  Color4 windParams = {};

  eastl::string flowmapName;
  eastl::string noisemapName;
  UniqueTex flowmapTexFallback;
  SharedTex flowmapGameresTex;
  SharedTex noisemapGameresTex;
};

// This functions are for use in non-ECS projects only
AmbientWind *get_ambient_wind();
void init_ambient_wind(const DataBlock &blk);
void init_ambient_wind(const char *wind_flowmap_name, const Point4 &wind_map_borders, const Point2 &wind_dir,
  float wind_strength_beaufort, float wind_noise_strength_multiplier, float wind_noise_speed_beaufort, float wind_noise_scale,
  float wind_noise_perpendicular, const char *wind_noisemap_name = "");
void init_ambient_wind_with_grass(const AmbientWindParameters &params, const char *wind_flowmap_name,
  const char *wind_noisemap_name = "");
void enable_ambient_wind();
void disable_ambient_wind();
void close_ambient_wind();
void update_ambient_wind();
