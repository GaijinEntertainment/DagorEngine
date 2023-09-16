//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <util/dag_string.h>

class HdrRender;
class DemonPostFx;
class PostFxRenderer;
struct RoDataBlock;


struct PostFxUserSettings
{
  bool shouldTakeStateFromBlk; // For DagorEd: ignore hdrMode.
  String hdrMode;
  bool hdrInstantAdaptation; // For DagorEd.

  PostFxUserSettings()
  {
    shouldTakeStateFromBlk = false;
    hdrMode = "none";
    hdrInstantAdaptation = false;
  }
};


// Takes care of HDR, motion blur, LDR glow, color curve settings.
// Applies motion blur, LDR glow, color curve.

class PostFx
{
public:
  PostFx(const DataBlock &level_settings, const DataBlock &game_settings, const PostFxUserSettings &user_settings);

  ~PostFx();

  // NULL should be passed to use previous settings.
  void restart(const DataBlock *level_settings, const DataBlock *game_settings, const PostFxUserSettings *user_settings);

  void restartEx(const RoDataBlock *level_settings, const DataBlock *game_settings, const PostFxUserSettings *user_settings)
  {
    if (level_settings)
      lastLevelSettings = *level_settings;
    restart(NULL, game_settings, user_settings);
  }

  //! updates settings that don't require restart(), or return false to indicated need of restart()
  bool updateSettings(const DataBlock *level_settings, const DataBlock *game_settings);

  void update(float timePassed);

  void apply(Texture *source, TEXTUREID sourceId, Texture *target, TEXTUREID targtexId, bool force_disable_motion_blur = false);

  bool getUseAdaptation();
  void setUseAdaptation(bool use);

  DemonPostFx *getDemonPostFx() { return demonPostFx; }
  PostFxRenderer *getGenPostFx() { return genPostFx; }


protected:
  DemonPostFx *demonPostFx;
  int targetW;
  int targetH;
  Texture *tempTex;
  TEXTUREID tempTexId;
  DataBlock lastLevelSettings;
  DataBlock lastGameSettings;
  PostFxUserSettings lastUserSettings;
  PostFxRenderer *genPostFx;


  void createTempTex();
};
