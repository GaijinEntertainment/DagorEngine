// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_windService.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

class WindBaseService : public IWindService
{
  DataBlock windTemplateBlk;
  DataBlock gameParamsAmbientWindBlk;
  DataBlock weatherAmbientWindBlk;
  float weatherWindStrength;
  Point2 weatherWindDir;
  WindSettings currentSettings;
  PreviewSettings previewSettings;
  bool useTemplate = false;
  String appDir;

public:
  WindBaseService();

  void init(const char *inAppDir, const DataBlock &envBlk) override;
  void setWeather(const DataBlock &inWeatherTypeBlk, const Point2 &inWeatherWindDir) override;
  void setPreview(const PreviewSettings &inPreviewSettings) override;
  bool isLevelEcsSupported() const override { return useTemplate; }

private:
  static void readFromTemplate(WindSettings &inout, const DataBlock &blk);
  void updateParams();
  virtual void applyWind(const WindSettings &settings, WindSettings::Ambient &currentAmbient) const = 0;
};
