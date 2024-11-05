//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tab.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>


//  float2 pos;
//  float2 scale;
//  float angle;
//  float brightness;
//  float depth;
//
//    ENTITY_PROPERTIES
//
//  float3 entityWorldPos;
//  float3 entityForward;
//  float3 entityUp;
//  int entityNameIndex;
//  int entityPlaceType;
//  int entitySeed;
//  float3 entityScale;
//  float reserved0;
//  float reserved1;

#define PARTICLE_SIZE (24 * 4)

class BaseTexture;
typedef BaseTexture Texture;
class D3dResource;
class BaseTexture;
class TextureGenLogger;
class ITextureGenEntitiesSaver;


class TextureRegManager
{
public:
  ~TextureRegManager() { close(); }
  TextureRegManager();

  void close();
  bool shrinkMem();
  TextureGenLogger *getLogger() const { return logger; }
  void setLogger(TextureGenLogger *logger_);
  void setEntitiesSaver(ITextureGenEntitiesSaver *entities_saver_) { entities_saver = entities_saver_; }
  ITextureGenEntitiesSaver *getEntitiesSaver() const { return entities_saver; }
  int getRegsUsed() const { return textureRegs.size(); }
  int getRegsAlive() const;
  eastl::hash_map<eastl::string, int> getRegsAliveMap() const;

  void setTextureRootDir(const char *tex_root_dir) { textureRootDir.setStr(tex_root_dir); }
  const eastl::hash_set<eastl::string> &getInputFilesList() { return inputFiles; }

  void setAutoShrink(bool on) { autoShrink = on; }
  bool getAutoShrink() const { return autoShrink; }
  int createTexture(const char *name, uint32_t fmt, int w, int h, int use_count);
  int createParticlesBuffer(const char *name, int max_count, int use_count);
  D3dResource *getResource(int regNo) const;

  Sbuffer *getParticlesBuffer(int regNo) const;
  Sbuffer *getParticlesBuffer(const char *name) const { return getParticlesBuffer(getRegNo(name)); }

  BaseTexture *getTexture(int regNo) const;
  BaseTexture *getTexture(const char *name) const { return getTexture(getRegNo(name)); }
  TEXTUREID getTextureId(int regNo) const;
  TEXTUREID getTextureId(const char *name) const { return getTextureId(getRegNo(name)); }

  void setUseCount(int reg, unsigned cnt);
  void textureUsed(int reg);
  int getRegNo(const char *name) const;
  int getRegUsageLeft(int reg) const;
  bool validateReg(int reg, bool log) const;
  void clearStat();
  uint64_t getMaxMemSize() const { return maxMemSize; }
  uint64_t getCurrentMemSize() const { return currentMemSize; }
  const char *getRegName(int reg) const;
  bool texHasChanged(const char *name);
  void validateFilesData();
  bool validateFile(const char *name);

protected:
  void getRelativeTexName(const char *name, String &texname, String &texWithParams);

  int findRegNo(const char *name) const;
  bool shrinkTex(int reg);
  struct TexureRegister
  {
    D3dResource *tex;
    eastl::string name;
    uint16_t useCount;
    TEXTUREID texId;
    bool owned;
    void close(TextureGenLogger *logger);
    void clear()
    {
      tex = 0;
      useCount = 0;
      texId = BAD_TEXTUREID;
      owned = false;
    }
    TexureRegister() : useCount(0), tex(0), texId(BAD_TEXTUREID), owned(false) {}
  };

  void fillCurveTexture(Texture *tex, const char *curve_str);
  void fillGradientTexture(Texture *tex, const char *gradient_str);
  void fillConstTexture(Texture *tex, const char *const_str);
  void fillSolidColorTexture(Texture *tex, const char *color_str, int width, int height);

  Tab<TexureRegister> textureRegs;
  Tab<int> freeRegs;
  struct FileInfo
  {
    int64_t mtime = 0, size = 0; // last modification time
    FileInfo() = default;
    FileInfo(int64_t m, int64_t s) : mtime(m), size(s) {}
  };
  eastl::hash_map<eastl::string, FileInfo> lastFileInfo;
  eastl::hash_map<eastl::string, FileInfo> changedFileInfo; // already changed
  eastl::hash_map<eastl::string, int> regNames;
  eastl::hash_set<eastl::string> inputFiles;
  uint64_t currentMemSize, maxMemSize;
  bool autoShrink;
  bool needReloadTextures;
  TextureGenLogger *logger;
  ITextureGenEntitiesSaver *entities_saver;
  String textureRootDir;
};
DAG_DECLARE_RELOCATABLE(TextureRegManager::TexureRegister);
