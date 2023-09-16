#pragma once


#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <ioSys/dag_dataBlock.h>
#include "decl.h"


class ssviewplugin::Plugin : public IGenEditorPlugin, public IRenderingService
{
public:
  static Plugin *self;

  static bool prepareRequiredServices();

public:
  Plugin();
  ~Plugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "ssview"; }
  virtual const char *getMenuCommandName() const { return "Streaming Scene View"; }
  virtual const char *getHelpUrl() const { return NULL; }

  virtual int getRenderOrder() const { return 101; }
  virtual int getBuildOrder() const { return 0; }

  virtual bool showInTabs() const { return false; }
  virtual bool showSelectAll() const { return true; }

  virtual bool acceptSaveLoad() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void beforeMainLoop() {}

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual void onNewProject() {}
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void setVisible(bool vis);
  virtual bool getVisible() const { return isVisible; }
  virtual bool getSelectionBox(BBox3 &box) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual void selectAll() {}
  virtual void deselectAll() {}

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects(IGenViewportWnd *vp);
  virtual void renderObjects();
  virtual void renderTransObjects();

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool onPluginMenuClick(unsigned id) { return false; }

  // IRenderingService
  virtual void renderGeometry(Stage stage);

private:
  bool isVisible;
  bool isDebugVisible;
  String streamingFolder, streamingBlkPath;
  DataBlock strmBlk;
  SceneHolder *streamingScene;

  bool loadScene(const char *streaming_folder);
};
