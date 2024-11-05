// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_WindService.h>
#include <render/wind/ambientWind.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <osApiWrappers/dag_direct.h>

// Keep these values in-sync with skyquake/prog/render/weather.cpp
const float MIN_BEAUFORT_SCALE = 0.0f;
const float MAX_BEAUFORT_SCALE = 12.0f;

static const IWindService::WindSettings defaultWindSettings;

static void decode_rel_fname(String &dest, const char *src, const char *appDir)
{
  if (src[0] && src[1] != ':')
    dest.printf(512, "%s%s", appDir, src);
  else
    dest = src;
}

static void writeAmbientSettings(DataBlock &outBlk, const IWindService::WindSettings::Ambient &ambient)
{
  outBlk.setPoint4("windMapBorders", ambient.windMapBorders);
  outBlk.getStr("windFlowTextureName", ambient.windFlowTextureName);
  outBlk.getStr("windNoiseTextureName", ambient.windNoiseTextureName);
  outBlk.setReal("windNoiseStrength", ambient.windNoiseStrength);
  outBlk.setReal("windNoiseSpeed", ambient.windNoiseSpeed);
  outBlk.setReal("windNoiseScale", ambient.windNoiseScale);
  outBlk.setReal("windNoisePerpendicular", ambient.windNoisePerpendicular);
}

static void readAmbientSettings(IWindService::WindSettings::Ambient &inout, const DataBlock &inBlk)
{
  inout.windMapBorders = inBlk.getPoint4("windMapBorders", inout.windMapBorders);
  inout.windFlowTextureName = inBlk.getStr("windFlowTextureName", inout.windFlowTextureName);
  inout.windNoiseTextureName = inBlk.getStr("windNoiseTextureName", inout.windNoiseTextureName);
  inout.windNoiseStrength = inBlk.getReal("windNoiseStrength", inout.windNoiseStrength);
  inout.windNoiseSpeed = inBlk.getReal("windNoiseSpeed", inout.windNoiseSpeed);
  inout.windNoiseScale = inBlk.getReal("windNoiseScale", inout.windNoiseScale);
  inout.windNoisePerpendicular = inBlk.getReal("windNoisePerpendicular", inout.windNoisePerpendicular);
}

void IWindService::writeSettingsBlk(DataBlock &outBlk, const PreviewSettings &inSettings)
{
  outBlk.setBool("enabled", inSettings.enabled);
  outBlk.setReal("windAzimuth", inSettings.windAzimuth);
  outBlk.setReal("windStrength", inSettings.windStrength);
  outBlk.setStr("windLevelPath", inSettings.levelPath);

  writeAmbientSettings(*outBlk.addBlock("ambientWind"), inSettings.ambient);
}

void IWindService::readSettingsBlk(PreviewSettings &outSettings, const DataBlock &inBlk)
{
  outSettings = PreviewSettings();
  outSettings.enabled = inBlk.getBool("enabled", outSettings.enabled);
  outSettings.windAzimuth = inBlk.getReal("windAzimuth", outSettings.windAzimuth);
  outSettings.windStrength = inBlk.getReal("windStrength", outSettings.windStrength);
  outSettings.levelPath = inBlk.getStr("windLevelPath", outSettings.levelPath);

  readAmbientSettings(outSettings.ambient, *inBlk.getBlockByNameEx("ambientWind"));
}

class GenericWindService : public IWindService
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
  GenericWindService() :
    weatherWindStrength(defaultWindSettings.windStrength),
    weatherWindDir(cosf(DegToRad(defaultWindSettings.windAzimuth)), sinf(DegToRad(defaultWindSettings.windAzimuth)))
  {}

  virtual void init(const char *inAppDdir, const DataBlock &envBlk) override
  {
    appDir = inAppDdir;
    windTemplateBlk = DataBlock::emptyBlock;
    gameParamsAmbientWindBlk = DataBlock::emptyBlock;

    const char *windTemplatePath = envBlk.getBlockByNameEx("ambientWind")->getStr("templatePath", NULL);

    if (windTemplatePath)
    {
      useTemplate = true;
      String absWindTemplatePath;
      decode_rel_fname(absWindTemplatePath, windTemplatePath, appDir);
      if (dd_file_exists(absWindTemplatePath))
      {
        DataBlock windBlk;
        if (windBlk.load(absWindTemplatePath))
          windTemplateBlk = *(windBlk.getBlockByNameEx("wind")->getBlockByNameEx("_group"));
      }
    }
    else
    {
      useTemplate = false;
      gameParamsAmbientWindBlk = *::dgs_get_game_params()->getBlockByNameEx("ambientWindParams");
    }

    updateParams();
  }

  virtual void term() override { close_ambient_wind(); }

  virtual void update() override { update_ambient_wind(); }

  virtual void setWeather(const DataBlock &inWeatherTypeBlk, const Point2 &inWeatherWindDir) override
  {
    weatherWindStrength = inWeatherTypeBlk.getReal("wind_strength", defaultWindSettings.windStrength);
    weatherAmbientWindBlk = *inWeatherTypeBlk.getBlockByNameEx("ambientWindParams");
    weatherWindDir = inWeatherWindDir;
    updateParams();
  }

  virtual void setPreview(const PreviewSettings &inPreviewSettings) override
  {
    previewSettings = inPreviewSettings;
    updateParams();
  }

  virtual bool isLevelEcsSupported() const override { return useTemplate; }

private:
  static void readFromTemplate(WindSettings &inout, const DataBlock &windTemplateBlk)
  {
    inout.windAzimuth = windTemplateBlk.getReal("wind__dir", inout.windAzimuth);
    inout.windStrength = windTemplateBlk.getReal("wind__strength", inout.windStrength);
    inout.ambient.windNoiseStrength = windTemplateBlk.getReal("wind__noiseStrength", inout.ambient.windNoiseStrength);
    inout.ambient.windNoiseSpeed = windTemplateBlk.getReal("wind__noiseSpeed", inout.ambient.windNoiseSpeed);
    inout.ambient.windNoiseScale = windTemplateBlk.getReal("wind__noiseScale", inout.ambient.windNoiseScale);
    inout.ambient.windNoisePerpendicular = windTemplateBlk.getReal("wind__noisePerpendicular", inout.ambient.windNoisePerpendicular);
    inout.ambient.windMapBorders = windTemplateBlk.getPoint4("wind__left_top_right_bottom", inout.ambient.windMapBorders);
    inout.ambient.windFlowTextureName = windTemplateBlk.getStr("wind__flowMap", inout.ambient.windFlowTextureName);
  }

  void updateParams()
  {
    currentSettings = WindSettings();
    currentSettings.enabled = false;

    WindSettings::Ambient &currentAmbient = currentSettings.ambient;

    if (useTemplate)
    {
      // Template version uses the default values from the template definied in the application blk
      //   Since customizing the settings per level is done through ecs in-game, it cannot be read here.
      if (!windTemplateBlk.isEmpty())
      {
        currentSettings.enabled = true;

        readFromTemplate(currentSettings, windTemplateBlk);
      }
    }
    else
    {
      // There is no template, so use the gameParams.blk as a default values, which can be override by the weather's settings
      if (!gameParamsAmbientWindBlk.isEmpty())
      {
        currentSettings.enabled = true;

        float minWindNoiseSpeed = gameParamsAmbientWindBlk.getReal("minWindNoiseSpeed", 1.0f);
        float maxWindNoiseSpeed = gameParamsAmbientWindBlk.getReal("maxWindNoiseSpeed", 6.0f);
        float minWindNoiseScale = gameParamsAmbientWindBlk.getReal("minWindNoiseScale", 1.0f);
        float maxWindNoiseScale = gameParamsAmbientWindBlk.getReal("maxWindNoiseScale", 8.0f);
        float minWindNoiseStrength = gameParamsAmbientWindBlk.getReal("minWindNoiseStrength", 0.5f);
        float maxWindNoiseStrength = gameParamsAmbientWindBlk.getReal("maxWindNoiseStrength", 3.0f);
        float maxWindNoisePerpendicularStrength = gameParamsAmbientWindBlk.getReal("maxWindNoisePerpendicularStrength", 0.5f);
        float minWindNoisePerpendicularStrength = gameParamsAmbientWindBlk.getReal("minWindNoisePerpendicularStrength", 0.2f);

        currentSettings.windStrength = weatherWindStrength;
        currentSettings.windDir = weatherWindDir;
        currentSettings.useWindDir = true;

        currentAmbient.windNoiseStrength =
          cvt(currentSettings.windStrength, MIN_BEAUFORT_SCALE, MAX_BEAUFORT_SCALE, minWindNoiseStrength, maxWindNoiseStrength);
        currentAmbient.windNoiseSpeed =
          cvt(currentSettings.windStrength, MIN_BEAUFORT_SCALE, MAX_BEAUFORT_SCALE, minWindNoiseSpeed, maxWindNoiseSpeed);
        currentAmbient.windNoiseScale =
          cvt(currentSettings.windStrength, MIN_BEAUFORT_SCALE, MAX_BEAUFORT_SCALE, minWindNoiseScale, maxWindNoiseScale);
        currentAmbient.windNoisePerpendicular = cvt(currentSettings.windStrength, MIN_BEAUFORT_SCALE, MAX_BEAUFORT_SCALE,
          minWindNoisePerpendicularStrength, maxWindNoisePerpendicularStrength);

        currentAmbient.windMapBorders = gameParamsAmbientWindBlk.getPoint4("windMapBorders", currentAmbient.windMapBorders);
        currentAmbient.windNoiseTextureName =
          gameParamsAmbientWindBlk.getStr("windNoiseTextureName", currentAmbient.windNoiseTextureName);
        currentAmbient.windFlowTextureName =
          gameParamsAmbientWindBlk.getStr("windFlowTextureName", currentAmbient.windFlowTextureName);
      }

      if (!weatherAmbientWindBlk.isEmpty())
      {
        currentSettings.enabled = true;

        currentSettings.windStrength = weatherWindStrength;
        currentSettings.windDir = weatherWindDir;
        currentSettings.useWindDir = true;

        readAmbientSettings(currentAmbient, weatherAmbientWindBlk);
      }
    }

    if (previewSettings.enabled)
    {
      currentSettings = previewSettings;

      String absLevelPath;
      decode_rel_fname(absLevelPath, previewSettings.levelPath, appDir);
      if (!dd_file_exists(absLevelPath) && dd_file_exists(previewSettings.levelPath))
        absLevelPath = previewSettings.levelPath;

      if (!absLevelPath.empty())
      {
        DataBlock levelBlk;
        if (levelBlk.load(absLevelPath))
        {
          for (int i = 0; i < levelBlk.blockCount(); i++)
          {
            DataBlock *entityBlock = levelBlk.getBlock(i);
            String templateName = String(entityBlock->getStr("_template", ""));
            if (templateName == "wind")
            {
              readFromTemplate(currentSettings, *entityBlock);
              break;
            }
          }
        }
      }
    }

    if (!currentSettings.useWindDir)
    {
      float rad = DegToRad(currentSettings.windAzimuth);
      currentSettings.windDir.x = cosf(rad);
      currentSettings.windDir.y = sinf(rad);
      currentSettings.useWindDir = true;
    }

    init_ambient_wind(currentAmbient.windFlowTextureName, currentAmbient.windMapBorders, currentSettings.windDir,
      currentSettings.windStrength, currentAmbient.windNoiseStrength, currentAmbient.windNoiseSpeed, currentAmbient.windNoiseScale,
      currentAmbient.windNoisePerpendicular, currentAmbient.windNoiseTextureName);

    if (currentSettings.enabled)
      enable_ambient_wind();
    else
      disable_ambient_wind();
  }
};

static GenericWindService srv;

void *get_generic_wind_service() { return &srv; }
