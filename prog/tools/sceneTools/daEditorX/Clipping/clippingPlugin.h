// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/iClipping.h>

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_clipping.h>
#include <de3_collisionPreview.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <scene/dag_frtdump.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include "clipping_builder.h"
#include <EditorCore/ec_workspace.h>

#include <sepGui/wndPublic.h>


namespace PropPanel
{
class ContainerPropertyControl;
}

class IClippingDumpBuilder;
class CollisionPropPanelClient;


//==============================================================================
class ClippingPlugin : public IGenEditorPlugin,
                       public IGenEventHandler,
                       public IClipping,
                       public IBinaryDataBuilder,
                       public IDagorEdCustomCollider,
                       public PropPanel::ControlEventHandler,
                       public IWndManagerWindowHandler
{
public:
  enum
  {
    PHYSENG_DagorFastRT,
    PHYSENG_Bullet,
  };

  ClippingPlugin();

  static const char *getPhysMatPath(ILogWriter *rep = NULL);
  static bool ClippingPlugin::isValidPhysMatBlk(const char *file, DataBlock &blk, ILogWriter *rep = NULL, DataBlock **mat_blk = NULL,
    DataBlock **def_blk = NULL);


  virtual const char *getInternalName() const { return "clipping"; }
  virtual const char *getMenuCommandName() const { return "Collision"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/Collision/index.htm"; }

  virtual int getRenderOrder() const { return 500; }
  virtual int getBuildOrder() const { return 200; }
  virtual bool showInTabs() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void beforeMainLoop() {}

  virtual void registerMenuAccelerators() override;
  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual IGenEventHandler *getEventHandler() { return this; }

  virtual void setVisible(bool vis);
  virtual bool getVisible() const { return isVisible; }
  virtual bool getSelectionBox(BBox3 &) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void onNewProject() {}
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual bool acceptSaveLoad() const { return true; }

  virtual void selectAll() {}
  virtual void deselectAll() {}

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects(IGenViewportWnd *vp) {}
  virtual void renderObjects();
  virtual void renderTransObjects() {}
  virtual void updateImgui() override;

  virtual void *queryInterfacePtr(unsigned huid);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  // command handlers
  // virtual void handleCommand(int cmd);
  // virtual void handleButtonClick(int btn_id, CtlBtnTemplate* btn, bool btn_pressed) { handleCommand(btn_id); }
  virtual bool onPluginMenuClick(unsigned id);
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif) {}
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) {}

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }
  virtual void handleViewportPaint(IGenViewportWnd *wnd);
  virtual void handleViewChange(IGenViewportWnd *wnd);
  // virtual void handleChildCreate ( CtlWndObject  *child ) {}
  // virtual void handleChildClose ( CtlWndObject  *child ) {}

  virtual bool catchEvent(unsigned ev_huid, void *userData);

  // IWndManagerWindowHandler

  virtual void *onWmCreateWindow(int type) override;
  virtual bool onWmDestroyWindow(void *window) override;

  // IClipping implemenatation
  virtual bool compileClippingWithDialog(bool for_game);

  virtual void manageCustomColliders();


  // IBinaryDataBuilder implemenatation
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params);
  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp);
  virtual bool useExportParameters() const { return true; }
  virtual void fillExportPanel(PropPanel::ContainerPropertyControl &params);
  virtual bool checkMetrics(const DataBlock &metrics_blk);

  bool recreatePanel();
  bool initClipping(bool for_game);
  FastRtDump *getFrt(bool game_frt);
  virtual void getFastRtDump(FastRtDump **p) { *p = getFrt(false); }

  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) { return false; }
  virtual const char *getColliderName() const { return "Collision plugin"; }
  virtual bool isColliderVisible() const { return getVisible(); }

public:
  bool showGcBox, showGcSph, showGcCap, showGcMesh;
  bool showVcm, showDags, showVcmWire;
  bool showGameFrt;

  DataBlock mainPanelState;

private:
  bool isVisible;
  float vcmRad;
  bool mPanelVisible;
  collisionpreview::Collision collision;
  bool collisionReady;
  FastRtDump *gameFrt;

  CollisionBuildSettings rtStg;
  int curPhysEngType;

  int toolBarId;
  CollisionPropPanelClient *panelClient;
  FastRtDump dagRtDump;
  bool dagRtDumpReady;

  Tab<String> clipDag;
  Tab<String> clipDagNew;

  FastNameMap disabledGamePlugins, editClipPlugins, disableCustomColliders;

  void importClipDag();

  void addClipNode(Node &n, IClippingDumpBuilder *rt);

  bool compileEditClip();
  bool compileGameClip(PropPanel::ContainerPropertyControl *panel, unsigned target_code);
  void prepareDAGcollision();

  bool compileClipping(bool for_game, Tab<int> &plugs, int phys_eng_type, unsigned target_code);

  inline void getDAGPath(String &path, const String &dag);
  inline void getEditorClipPath(String &path);

  void getGameClipPath(String &path, unsigned target_code) const;
  void clearDags();

  void getClippingFiles(Tab<String> &files, unsigned target_code) const;
  void makeDagPreviewCollision(bool force_remake);
  void makeGameFrtPreviewCollision(bool force_remake);

  bool onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel);
};


//==============================================================================
inline void ClippingPlugin::getEditorClipPath(String &path) { path = DAGORED2->getPluginFilePath(this, "editor_clip.frt.bin"); }


//==============================================================================
inline void ClippingPlugin::getDAGPath(String &path, const String &dag) { path = DAGORED2->getPluginFilePath(this, dag); }

extern bool check_collision_provider(IGenEditorPlugin *p);
extern int count_collision_provider();
