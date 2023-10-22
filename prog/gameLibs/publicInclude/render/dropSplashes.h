//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resId.h>

class DataBlock;
class Point3;
class ShaderMaterial;
struct Color4;
namespace dynrender
{
struct RElem;
};

class DropSplashes
{
  eastl::unique_ptr<ShaderMaterial> splashMaterial;
  eastl::unique_ptr<ShaderMaterial> spriteMaterial;
  eastl::unique_ptr<dynrender::RElem> splashRendElem;
  eastl::unique_ptr<dynrender::RElem> spriteRendElem;
  BufPtr splashVb;
  BufPtr splashIb;
  float currentTime;
  float distance;
  float splashTimeToLive;
  float iterationTime;
  float partOfSprites;
  float volumetricSplashScale;
  float spriteSplashScale;
  uint32_t splashesCount;
  TEXTUREID volumetricSplashTextureId;
  TEXTUREID spriteSplashTextureId;

  TEXTUREID initSplashTexture(const DataBlock &blk, const char *tex_name, const char *res_default_name);
  void initSplashTextures(const DataBlock &blk);
  void initSplashShader();
  void initSpriteShader();
  uint32_t getSpritesCount() const;
  void fillBuffers();

public:
  explicit DropSplashes(const DataBlock &blk);
  ~DropSplashes();
  void render();
  void update(float dt, const Point3 &view_pos);

  void setSplashesCount(const uint32_t splashes_count) { splashesCount = splashes_count; }
  void setDistance(float dist);
  void setTimeToLive(float time_to_live);
  void setIterationTime(float iteration_time);
  void setVolumetricSplashScale(const float scale) { volumetricSplashScale = scale; }
  void setSpriteSplashScale(const float scale) { spriteSplashScale = scale; }
  void setPartOfSprites(float part);
  void setSpriteYPos(float pos);

  float getDistance() const { return distance; }
  float getTimeToLive() const { return splashTimeToLive; }
  float getIterationTime() const { return iterationTime; }
  float getVolumetricSplashScale() const { return volumetricSplashScale; }
  float getSpriteSplashScale() const { return spriteSplashScale; }
  float getPartOfSprites() const { return partOfSprites; }
  float getSpriteYPos() const;
  uint32_t getCount() const { return splashesCount; }
  void afterReset() { fillBuffers(); }
};
