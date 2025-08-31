// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/iCollision.h>

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_collision.h>
#include <de3_collisionPreview.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <scene/dag_frtdump.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include "collision_builder.h"
#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_workspace.h>


namespace PropPanel
{
class ContainerPropertyControl;
}

class ICollisionDumpBuilder;
class CollisionPropPanelClient;


//==============================================================================
class CollisionPlugin : public IGenEditorPlugin,
                        public IGenEventHandler,
                        public ICollision,
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

  CollisionPlugin();

  static const char *getPhysMatPath(ILogWriter *rep = NULL);
  static bool isValidPhysMatBlk(const char *file, DataBlock &blk, ILogWriter *rep = NULL, DataBlock **mat_blk = NULL,
    DataBlock **def_blk = NULL);


  const char *getInternalName() const override { return "clipping"; }
  const char *getMenuCommandName() const override { return "Collision"; }
  const char *getHelpUrl() const override { return "/html/Plugins/Collision/index.htm"; }

  int getRenderOrder() const override { return 500; }
  int getBuildOrder() const override { return 200; }
  bool showInTabs() const override { return true; }

  void registered() override;
  void unregistered() override;
  void beforeMainLoop() override {}

  void registerMenuAccelerators() override;
  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override;
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &) const override { return false; }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void onNewProject() override {}
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  bool acceptSaveLoad() const override { return true; }

  void selectAll() override {}
  void deselectAll() override {}

  void actObjects(float dt) override {}
  void beforeRenderObjects(IGenViewportWnd *vp) override {}
  void renderObjects() override;
  void renderTransObjects() override {}
  void updateImgui() override;

  void *queryInterfacePtr(unsigned huid) override;

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  // command handlers
  bool onPluginMenuClick(unsigned id) override;

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override { return false; }
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override { return false; }
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override { return false; }
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override { return false; }
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  void handleViewChange(IGenViewportWnd *wnd) override;

  bool catchEvent(unsigned ev_huid, void *userData) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  // ICollision implemenatation
  bool compileCollisionWithDialog(bool for_game) override;

  void manageCustomColliders() override;


  // IBinaryDataBuilder implemenatation
  bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params) override;
  bool addUsedTextures(ITextureNumerator &tn) override { return true; }
  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp) override;
  bool useExportParameters() const override { return true; }
  void fillExportPanel(PropPanel::ContainerPropertyControl &params) override;
  bool checkMetrics(const DataBlock &metrics_blk) override;

  bool recreatePanel();
  bool initCollision(bool for_game) override;
  FastRtDump *getFrt(bool game_frt);
  void getFastRtDump(FastRtDump **p) override { *p = getFrt(false); }

  // IDagorEdCustomCollider
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override { return false; }
  const char *getColliderName() const override { return "Collision plugin"; }
  bool isColliderVisible() const override { return getVisible(); }

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

  void addClipNode(Node &n, ICollisionDumpBuilder *rt, const char *dag_fn = nullptr, bool report_stats = false);

  bool compileEditClip();
  bool compileGameClip(PropPanel::ContainerPropertyControl *panel, unsigned target_code);
  void prepareDAGcollision();

  bool compileCollision(bool for_game, Tab<int> &plugs, int phys_eng_type, unsigned target_code);

  inline void getDAGPath(String &path, const String &dag);
  inline void getEditorClipPath(String &path);

  void getGameClipPath(String &path, unsigned target_code) const;
  void clearDags();

  void getCollisionFiles(Tab<String> &files, unsigned target_code) const;
  void makeDagPreviewCollision(bool force_remake);
  void makeGameFrtPreviewCollision(bool force_remake);

  bool onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel);
};


//==============================================================================
inline void CollisionPlugin::getEditorClipPath(String &path) { path = DAGORED2->getPluginFilePath(this, "editor_clip.frt.bin"); }


//==============================================================================
inline void CollisionPlugin::getDAGPath(String &path, const String &dag) { path = DAGORED2->getPluginFilePath(this, dag); }

extern bool check_collision_provider(IGenEditorPlugin *p);
extern int count_collision_provider();
