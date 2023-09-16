#ifndef __GAIJIN_EDITORCORE_WORKSPACE__
#define __GAIJIN_EDITORCORE_WORKSPACE__
#pragma once


#include <generic/dag_tab.h>
#include <libTools/containers/dag_StrMap.h>

#include <ioSys/dag_dataBlock.h>

struct WspLibData
{
  String name;
  String fname;
};


class EditorWorkspace
{
public:
  EditorWorkspace();
  virtual ~EditorWorkspace();

  // Initialize workspace.blk file to get list of available workspaces
  bool initWorkspaceBlk(const char *path);

  // Delete data loaded in initWorkspaceBlk(). Must be called after workspace init
  //(usually after loadFromBlk() method
  void freeWorkspaceBlk();

  // Returns worksapaces names list
  void getWspNames(Tab<String> &list) const;

  // Loads workspace from BLK. Must be called after initWorkspaceBlk()
  bool load(const char *workspace_name, bool *app_path_set = NULL);
  // Loads workspace directly from "application.blk" file
  bool loadIndirect(const char *app_blk_path);

  // Saves workspace in BLK
  bool save();

  // Remove current workspace from BLK
  bool remove();

  inline void setName(const char *new_name) { name = new_name; }
  void setAppPath(const char *new_path);

  inline const char *getName() const { return name; }
  inline const char *getAppDir() const { return appDir; }
  inline const char *getAppPath() const { return appPath; }
  inline const char *getSdkDir() const { return sdkDir; }
  inline const char *getLibDir() const { return libDir; }
  inline const char *getLevelsDir() const { return levelsDir; }
  inline const char *getResDir() const { return resDir; }
  inline const char *getScriptDir() const { return scriptDir; }
  inline const char *getAssetScriptDir() const { return assetScriptsDir; }
  inline const char *getGameDir() const { return gameDir; }
  inline const char *getLevelsBinDir() const { return levelsBinDir; }
  inline const char *getPhysmatPath() const { return physmatPath; }
  inline const char *getSoundFileName() const { return soundFileName; }
  inline const char *getSoundFxFileName() const { return soundFxFileName; }
  inline const char *getScriptLibrary() const { return scriptLibrary; }

  inline const char *getCollisionName() const { return collisionName; }

  inline const Tab<String> &getDagorEdDisabled() const { return deDisabled; }
  inline const Tab<String> &getResourceEdDisabled() const { return reDisabled; }

  inline dag::ConstSpan<unsigned> getAdditionalPlatforms() const { return platforms; }
  static unsigned getPlatformFromStr(const char *platf);
  static const char *getPlatformNameFromId(unsigned plt);

  inline float getMaxTraceDistance() const { return maxTraceDistance; }

protected:
  String blkPath;

  struct Workspaces
  {
    DataBlock blk;
    StriMap<DataBlock *> names;

    Workspaces() : names(tmpmem) {}
  };

  Workspaces *wspData;

  // Load application-specific data from BLK
  virtual bool loadSpecific(const DataBlock &blk) { return true; }
  // Load application-specific data from application.blk
  virtual bool loadAppSpecific(const DataBlock &blk) { return true; }

  // Save application-specific data in BLK
  // Must use only DataBlock::set... methods and DataBlock::addBlock to avoid rewrite other
  // application data
  virtual bool saveSpecific(DataBlock &blk) { return true; }

  virtual bool createApplicationBlk(const char *path) const;

private:
  String name;
  String appDir;
  String appPath;

  String sdkDir;
  String libDir;
  String levelsDir;
  String resDir;
  String scriptDir;
  String assetScriptsDir;
  String gameDir;
  String levelsBinDir;
  String physmatPath;
  String soundFileName;
  String scriptLibrary;
  String soundFxFileName;

  String collisionName;

  Tab<String> deDisabled;
  Tab<String> reDisabled;

  Tab<unsigned> platforms;

  float maxTraceDistance;

  DataBlock *findWspBlk(DataBlock &blk, const char *wsp_name, bool create_new);

  bool loadFromBlk(DataBlock &blk, bool *app_path_set = NULL);
};


#endif //__GAIJIN_EDITORCORE_WORKSPACE__
