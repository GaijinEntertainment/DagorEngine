//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "de_decl.h"

#include <generic/dag_tab.h>

#include <EditorCore/ec_object.h>
#include <EditorCore/ec_interface.h>


class IDagorEdCustomCollider;
class DeWorkspace;

typedef class PropertyContainerControlBase PropPanel2;

struct LightingSettings;


namespace StaticSceneBuilder
{
class StdTonemapper;
}


// generic DagorEditor plugin interface
class IGenEditorPlugin : public IGenEditorPluginBase
{
public:
  enum
  {
    VERSION_1_1 = 0x3E15,

    CURRENT_VERSION = VERSION_1_1,
  };

  virtual ~IGenEditorPlugin() {}

  // internal name (C idenitifier name restrictons); used save/load data and logging
  virtual const char *getInternalName() const = 0;
  // user-friendly plugin name to be displayed in menu
  virtual const char *getMenuCommandName() const = 0;
  // returns desired order number; by default plugins are sorted by this number (increasing order)
  virtual int getRenderOrder() const = 0;
  // returns desired order number for binary data build process;
  // data is build and saved to dump in increasion order
  virtual int getBuildOrder() const = 0;

  // non-pure virtual functions
  //  help link in CHM file
  virtual const char *getHelpUrl() const = 0;
  // show this plugin in menu
  virtual bool showInMenu() const { return true; }
  // show this plugin in Tab Bar
  virtual bool showInTabs() const { return false; }
  // show in menu "Select all" command for this plugin
  virtual bool showSelectAll() const { return false; }


  // notification about register/unregister
  virtual void registered() = 0;
  virtual void unregistered() = 0;

  // called before enter in main loop when all plugins loaded and initialized
  virtual void beforeMainLoop() = 0;

  // called when user requests switch to this plugin (or for current plugin when returning from excl. camera mode)
  virtual void registerMenuAccelerators() {}
  // called when user requests switch to this plugin; returns true on success
  virtual bool begin(int toolbar_id, unsigned menu_id) = 0;
  // called when user requests switch from this plugin; returns true on success
  virtual bool end() = 0;
  // called by editor AFTER begin() returns true; can return NULL when no event handling is needed
  virtual IGenEventHandler *getEventHandler() = 0;

  // used by editor to set/get visibility flag
  virtual void setVisible(bool vis) = 0;
  virtual bool getVisible() const = 0;

  virtual bool getSelectionBox(BBox3 &box) const = 0;
  virtual bool getStatusBarPos(Point3 &pos) const = 0;

  // clears all objects contained in this plugin (on NEW or before LOAD command)
  virtual void clearObjects() = 0;
  // notify plugin about new project creation
  virtual void onNewProject() = 0;
  // saves all objects contained in this plugin to data block
  // and/or to base_path folder (with trailing slash)
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) = 0;
  // loads all objects for this plugin from data block
  // and/or from base_path folder (with trailing slash)
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) = 0;
  virtual bool acceptSaveLoad() const = 0;

  // selects all objects
  virtual void selectAll() = 0;
  // deselects all objects
  virtual void deselectAll() = 0;

  // render/acting interface
  //! update plugin objects regularly between frames
  virtual void actObjects(float dt) = 0;
  //! prepare for render; called with NULL before drawing, then with vp!=NULL for each viewport during frame drawing
  virtual void beforeRenderObjects(IGenViewportWnd *vp) = 0;
  //! render plugin objects, opaque pass (called for each viewport)
  virtual void renderObjects() = 0;
  //! render plugin objects, transparent pass (called for each viewport)
  virtual void renderTransObjects() = 0;

  virtual void onBeforeReset3dDevice() {}

  // COM-like facilities
  virtual void *queryInterfacePtr(unsigned huid) = 0;
  template <class T>
  inline T *queryInterface()
  {
    return (T *)queryInterfacePtr(T::HUID);
  }

  // return true if stop catching
  virtual bool catchEvent(unsigned event_huid, void *userData) { return false; }
  virtual bool onPluginMenuClick(unsigned id) = 0;
};


// Dagor Editor interface to be used by plugins
class IDagorEd2Engine : public IEditorCoreEngine
{
public:
  static const int DAGORED_VERSION = 0x10C;
  static const int DE_USER_TB = 2;

  // generic version compatibility check
  inline IDagorEd2Engine() { dagorEdInterfaceVer = DAGORED_VERSION; }
  inline bool checkVersion() { return IEditorCoreEngine::checkVersion() && (dagorEdInterfaceVer == DAGORED_VERSION); }

  // workspace usage functions
  virtual const char *getSdkDir() const = 0;
  virtual const char *getGameDir() const = 0;
  virtual const char *getLibDir() const = 0;
  virtual const char *getSceneFileName() const = 0;
  virtual const char *getSoundFileName() const = 0;
  virtual const char *getSoundFxFileName() const = 0;
  virtual const char *getScriptLibrary() const = 0;
  virtual const char *getExportPath(int platf_id) const = 0;
  virtual float getMaxTraceDistance() const = 0;
  virtual dag::ConstSpan<unsigned> getAdditionalPlatforms() const = 0;

  virtual void addExportPath(int platf_id) = 0;

  // get workspace for know plugin-specific features
  virtual const DeWorkspace &getWorkspace() const = 0;

  virtual bool isInBatchOp() = 0;

  virtual void disablePluginsRender() = 0;
  virtual void enablePluginsRender() = 0;

  virtual void preparePluginsListmenu() = 0;
  virtual void startWithWorkspace(const char *def_workspace_name) = 0;

  virtual DagorEdPluginData *getPluginData(int idx) = 0;

  virtual IGenEditorPlugin *getPluginByName(const char *name) const = 0;
  virtual IGenEditorPlugin *getPluginByMenuName(const char *name) const = 0;

  virtual IGenEditorPluginBase *getPluginBase(int idx) { return getPlugin(idx); }
  virtual IGenEditorPluginBase *curPluginBase() { return curPlugin(); }

  // project files management
  virtual void getPluginProjectPath(const IGenEditorPlugin *plugin, String &base_path) const = 0;
  virtual String getPluginFilePath(const IGenEditorPlugin *plugin, const char *fname) const = 0;
  virtual void getProjectFolderPath(String &base_path) = 0;
  virtual String getProjectFileName() = 0;

  virtual bool copyFileToProject(const char *from_filename, const char *to_filename, bool overwrite, bool add_to_cvs) = 0;

  virtual bool addFileToProject(const char *filename) = 0; // add to cvs
  virtual bool removeFileFromProject(const char *filename, bool remove_from_cvs) = 0;

  // addition viewport camera interface
  virtual void setViewportCameraMode(unsigned int viewport_no, bool camera_mode) = 0;
  virtual void setViewportCameraViewProjection(unsigned int viewport_no, TMatrix &view, float fov) = 0;
  virtual void switchCamera(const unsigned int from, const unsigned int to) = 0;
  virtual void setViewportCustomCameras(ICustomCameras *customCameras) = 0;

  virtual void setAnimateViewports(bool animate) = 0;

  // Static geometry lighting routine
  virtual bool isUseDirectLight() const = 0;
  virtual void setUseDirectLight(bool use) = 0;

  // custom colliders
  virtual void registerCustomCollider(IDagorEdCustomCollider *coll) const = 0;
  virtual void unregisterCustomCollider(IDagorEdCustomCollider *coll) const = 0;
  virtual void enableCustomShadow(const char *name) const = 0;
  virtual void disableCustomShadow(const char *name) const = 0;
  virtual void enableCustomCollider(const char *name) const = 0;
  virtual void disableCustomCollider(const char *name) const = 0;
  virtual bool isCustomShadowEnabled(const IDagorEdCustomCollider *collider) const = 0;
  virtual int getCustomCollidersCount() const = 0;
  virtual bool fillCustomCollidersList(PropPanel2 &params, const char *grp_caption, int grp_pid, int collider_pid, bool shadow,
    bool open_grp = false) const = 0;
  virtual bool getUseOnlyVisibleColliders() const = 0;
  virtual void setUseOnlyVisibleColliders(bool use) = 0;

  virtual bool onPPColliderCheck(int pid, const PropPanel2 &panel, int collider_pid, bool shadow) const = 0;
  virtual IDagorEdCustomCollider *getCustomCollider(int idx) const = 0;

  //! returns highly temporary slice of colliders (allocated in static array), to be copied immediately on receive
  virtual dag::ConstSpan<IDagorEdCustomCollider *> loadColliders(const DataBlock &blk, unsigned &out_filter_mask,
    const char *blk_name = "colliders") const = 0;

  virtual void setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filter_mask) const = 0;
  virtual void restoreEditorColliders() const = 0;

  //! returns highly temporary slice of colliders (allocated in static array), to be copied immediately on receive
  virtual dag::ConstSpan<IDagorEdCustomCollider *> getCurColliders(unsigned &out_filter_mask) const = 0;

  //! writes colliders list to BLK in generic format
  virtual void saveColliders(DataBlock &blk, dag::ConstSpan<IDagorEdCustomCollider *> coll, unsigned filter_mask,
    bool save_filters = true) const = 0;

  // updates vieports
  virtual void repaint() = 0;


  static const char *getBuildVersion();

  // single global engine object
  static inline IDagorEd2Engine *get() { return __dagored_global_instance; }
  static inline void set(IDagorEd2Engine *eng)
  {
    __dagored_global_instance = eng;
    IEditorCoreEngine::set(eng);
  }

  virtual void correctCursorInSurfMove(const Point3 &delta) = 0;

  virtual bool shadowRayHitTest(const Point3 &src, const Point3 &dir, real dist) = 0;

  // unique Id per project
  virtual int getNextUniqueId() = 0;

  virtual bool spawnEvent(unsigned event_huid, void *userData) = 0;

protected:
  int dagorEdInterfaceVer;

  virtual class LibCache *getLibCachePtr() { return NULL; }
  virtual Tab<struct WspLibData> *getLibData() { return NULL; }

private:
  static IDagorEd2Engine *__dagored_global_instance;
};


//! to be called by DLL plugins on startup
void daeditor3_init_globals(IDagorEd2Engine &editor);


#define DAGORED2 IDagorEd2Engine::get()

inline const char *de_get_lib_dir()
{
  if (IDagorEd2Engine::get())
    return DAGORED2->getLibDir();
  return NULL;
}

inline const char *de_get_sdk_dir()
{
  if (IDagorEd2Engine::get())
    return DAGORED2->getSdkDir();
  return NULL;
}

inline const char *de_get_game_dir()
{
  if (IDagorEd2Engine::get())
    return DAGORED2->getGameDir();
  return NULL;
}
