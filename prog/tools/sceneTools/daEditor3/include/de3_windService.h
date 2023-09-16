//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
class DataBlock;

class IWindService
{
public:
  static constexpr unsigned HUID = 0xE1F67B0Bu; // IWindService

  struct WindSettings
  {
    struct Ambient
    {
      float windNoiseStrength = 2.0f;
      float windNoiseSpeed = 2.0f;
      float windNoiseScale = 70.0f;
      float windNoisePerpendicular = 0.5f;
      SimpleString windNoiseTextureName;
      SimpleString windFlowTextureName;
      Point4 windMapBorders = Point4(-2048, -2048, 2048, 2048);
    };

    bool enabled = false;
    bool useWindDir = false;
    float windAzimuth = 40.0f;
    float windStrength = 1.0f;
    Point2 windDir = Point2(0.6, 0.8);
    Ambient ambient;
  };

  struct PreviewSettings : WindSettings
  {
    SimpleString levelPath;
  };

  static void writeSettingsBlk(DataBlock &, const PreviewSettings &);
  static void readSettingsBlk(PreviewSettings &, const DataBlock &);

  virtual void term() = 0;
  virtual void init(const char *inAppDdir, const DataBlock &) = 0;

  virtual void update() = 0;
  virtual void setWeather(const DataBlock &, const Point2 &) = 0;
  virtual void setPreview(const PreviewSettings &) = 0;

  virtual bool isLevelEcsSupported() const = 0;
};
