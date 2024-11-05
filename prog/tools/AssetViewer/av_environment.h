// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>


struct SunLightProps;
struct Color4;
class IObjEntity;
class BaseTexture;
class DagorAsset;


struct AssetLightData
{
  SimpleString textureName;
  SimpleString shaderVar;
  SimpleString envTextureName;
  SimpleString paintDetailsTexAsset; // per level painting texture name
  int envTextureStretch;
  SimpleString envBlkFn;
  SimpleString envLevelBlkFn;

  Color3 color;

  float specPower, specStrength;

  int rdbgType;
  bool rdbgUseNmap, rdbgUseDtex, rdbgUseLt;
  bool rdbgUseChrome;

  bool renderEnviEnt;
  SimpleString renderEnviEntAsset;
  Point3 renderEnviEntPos;
  IObjEntity *renderEnviEntity;

public:
  AssetLightData()
  {
    envTextureStretch = 0;

    rdbgType = 0;
    rdbgUseNmap = rdbgUseDtex = rdbgUseLt = true;
    rdbgUseChrome = true;

    renderEnviEnt = false;
    renderEnviEntPos.set(0, 0, 0);
    renderEnviEntity = NULL;
  }

  void setReflectionTexture();
  void setEnvironmentTexture(bool require_lighting_update = true);
  void setPaintDetailsTexture();
  void applyMicrodetailFromLevelBlk();

  void loadDefaultSettings(DataBlock &app_blk);

  SunLightProps *createSun();
};


namespace environment
{
void show_environment_settings(void *handle, AssetLightData *ald);
void load_settings(DataBlock &blk, AssetLightData *ald, const AssetLightData *ald_def, bool &rend_grid);
void save_settings(DataBlock &blk, AssetLightData *ald, const AssetLightData *ald_def, bool &rend_grid);

void load_skies_settings(const DataBlock &blk);
void save_skies_settings(DataBlock &blk);

void renderEnvironment(bool ortho);
void renderEnviEntity(AssetLightData &ald);
void clear();

const char *getEnviTitleStr(AssetLightData *ald);

void setUseSinglePaintColor(bool use);
bool isUsingSinglePaintColor();
void setSinglePaintColor(E3DCOLOR color);
E3DCOLOR getSinglePaintColor();
void updatePaintColorTexture();

void on_asset_changed(const DagorAsset &asset, AssetLightData &ald);
}; // namespace environment
