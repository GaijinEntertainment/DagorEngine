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
#include <EASTL/vector.h>


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


// With autoShrink and a retain budget > 0, a register whose use count reaches 0 keeps its texture (it is retained) and
// stays alive as a cached result for the next pass. Retained registers are evicted least recently released first when
// the size outside the pool would go above the budget. A budget of 0 disables retention. "tex:" input registers are never retained.
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
  // Evicts retained registers at once if the size outside the pool is above the new budget.
  void setRetainBudget(uint64_t bytes);
  uint64_t getRetainedMemSize() const { return retainedMemSize; }
  int getRegsRetained() const { return retainedCount; }
  int createTexture(const char *name, uint32_t fmt, int w, int h, int use_count);
  int createParticlesBuffer(const char *name, int max_count, int use_count);
  D3dResource *getResource(int regNo) const;

  Sbuffer *getParticlesBuffer(int regNo) const;
  Sbuffer *getParticlesBuffer(const char *name) const { return getParticlesBuffer(getRegNo(name)); }
  // The draw indirect args of the register whose particles buffer is particles; null for any other resource.
  Sbuffer *getParticlesArgs(const D3dResource *particles) const;

  BaseTexture *getTexture(int regNo) const;
  BaseTexture *getTexture(const char *name) const { return getTexture(getRegNo(name)); }
  TEXTUREID getTextureId(int regNo) const;
  TEXTUREID getTextureId(const char *name) const { return getTextureId(getRegNo(name)); }

  void setUseCount(int reg, unsigned cnt);
  // Sets the use count to 0 and, with autoShrink, closes the register even when retention would keep it.
  void discard(int reg);
  void textureUsed(int reg);
  bool keepAllOrNone(dag::ConstSpan<int> regs);
  int getRegNo(const char *name) const;
  int getRegUsageLeft(int reg) const;
  bool validateReg(int reg, bool log) const;
  void clearStat();
  uint64_t getMaxMemSize() const { return maxMemSize; }
  uint64_t getCurrentMemSize() const { return currentMemSize; }
  struct Stats
  {
    uint64_t resourcesCreated = 0, resourcesReleased = 0, bytesCreated = 0;
  };
  // Counts the resources that registers take (created or acquired) and release; a reuse in place counts nothing.
  // A particles buffer and its draw args count as one resource of the buffer size, also in the memory totals.
  // bytesCreated counts the resources registers own, pooled render targets too, not an acquired tex: file texture.
  // clearStat() resets them too, so it starts one window for these counters and for maxMemSize.
  Stats getStats() const { return stats; }
  const char *getRegName(int reg) const;
  bool texHasChanged(const char *name);
  void validateFilesData();
  // validateFile records a file only when its stat differs from the one the last load used;
  // markFileChanged records it always, for a poller whose own file time check saw the change.
  // Both take the file path as getInputFilesList() holds it, not a tex: register name; texHasChanged takes either.
  bool validateFile(const char *name);
  bool markFileChanged(const char *name);

  struct FileInfo
  {
    int64_t mtime = 0, size = 0; // last modification time
    FileInfo() = default;
    FileInfo(int64_t m, int64_t s) : mtime(m), size(s) {}
  };
  // Input files ("tex:" registers) can load ahead of createTexture. loadFile and releaseLoad touch no manager state;
  // the caller serializes all other calls to this manager.
  struct FileLoad
  { // -V802 PVS sizes EASTL members as one byte; at their real sizes no member order is smaller
    eastl::string regName;
    eastl::string texWithParams;
    FileInfo info;
    TEXTUREID texId = BAD_TEXTUREID;
    bool changed = true; // since last use: the resident copy is stale
  };
  // True for an input file register that has no live register and no load ahead, done or failed.
  bool isFileLoadPending(const char *reg_name) const;
  // Fills load for a pending reg_name with the file's time and size before the load; false when not pending or the file is missing.
  bool prepareFileLoad(const char *reg_name, FileLoad &load);
  // Loads load.texWithParams into load.texId holding one reference; BAD_TEXTUREID on failure or when someone else holds it.
  static void loadFile(FileLoad &load);
  static void releaseLoad(const FileLoad &load);
  // Takes over load.texId for createTexture(load.regName), a failed load too; releases it at once when not pending.
  void addLoadedFile(const FileLoad &load);
  void releaseLoadedFiles();

  // Render targets of released registers wait here for a new output or scratch target of the same size and format,
  // up to the limit in bytes; 0 (the default) pools nothing more, and releaseTexturePool frees what the pool holds.
  void setTexturePoolLimit(uint64_t bytes) { texturePoolLimit = bytes; }
  void releaseTexturePool();
  // Pooled targets that createTexture(name, fmt, w, h) can take for a register that has no texture.
  int pooledTargetCount(uint32_t fmt, int w, int h) const;
  // True when createTexture(reg_name) would create the texture of a gradient, curve, const or color input.
  bool isGeneratedTexturePending(const char *reg_name) const;

protected:
  void getRelativeTexName(const char *name, String &texname, String &texWithParams);
  TEXTUREID takeLoadedFile(const char *reg_name, const char *tex_name);
  bool poolTexture(int reg);
  int findPooledTarget(uint32_t flags, int w, int h) const;
  BaseTexture *takePooledTexture(uint32_t flags, int w, int h);

  int findRegNo(const char *name) const;
  bool shrinkTex(int reg);
  void releaseUnused(int reg);
  void unretain(int reg);
  void evictRetained(uint64_t bytes_to_add);
  struct TexureRegister
  {
    D3dResource *tex;
    Sbuffer *particlesArgs = nullptr;
    eastl::string name;
    uint16_t useCount;
    TEXTUREID texId;
    bool owned;
    bool retained;
    int retainPrev, retainNext;
    // Creation flags of an output render target, which the pool can take; 0 for other resources.
    uint32_t targetFlags;
    void close(TextureGenLogger *logger);
    void clear()
    {
      tex = 0;
      useCount = 0;
      texId = BAD_TEXTUREID;
      owned = false;
      targetFlags = 0;
    }
    bool isAlive() const { return useCount != 0 || retained; }
    TexureRegister() :
      useCount(0), tex(0), texId(BAD_TEXTUREID), owned(false), retained(false), retainPrev(-1), retainNext(-1), targetFlags(0)
    {}
  };

  void fillCurveTexture(Texture *tex, const char *curve_str);
  void fillGradientTexture(Texture *tex, const char *gradient_str);
  void fillConstTexture(Texture *tex, const char *const_str);
  void fillSolidColorTexture(Texture *tex, const char *color_str, int width, int height);

  Tab<TexureRegister> textureRegs;
  Tab<int> freeRegs;
  eastl::hash_map<eastl::string, FileInfo> lastFileInfo;
  eastl::hash_map<eastl::string, FileInfo> changedFileInfo; // already changed
  eastl::hash_map<eastl::string, int> regNames;
  struct LoadedFile
  {
    TEXTUREID texId;
    FileInfo info;
    uint64_t memSize = 0;
  };
  eastl::hash_map<eastl::string, LoadedFile> loadedFiles;
  struct PooledTexture
  {
    BaseTexture *tex;
    uint32_t flags;
    int w;
    int h;
  };
  eastl::vector<PooledTexture> texturePool; // oldest first
  uint64_t texturePoolSize = 0;
  uint64_t texturePoolLimit = 0;
  eastl::hash_set<eastl::string> inputFiles;
  uint64_t currentMemSize, maxMemSize;
  uint64_t retainBudget = 0, retainedMemSize = 0;
  int retainedCount = 0;
  // least recently released first
  int retainHead = -1, retainTail = -1;
  Stats stats;
  bool autoShrink;
  bool needReloadTextures;
  TextureGenLogger *logger;
  ITextureGenEntitiesSaver *entities_saver;
  String textureRootDir;
};
DAG_DECLARE_RELOCATABLE(TextureRegManager::TexureRegister);
