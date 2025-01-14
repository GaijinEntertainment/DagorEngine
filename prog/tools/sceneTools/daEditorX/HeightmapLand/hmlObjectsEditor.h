// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_client.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_clipping.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <oldEditor/de_cm.h>
#include <de3_assetService.h>
#include <de3_pixelPerfectSelectionService.h>
#include <shaders/dag_overrideStateId.h>
#include <ioSys/dag_dataBlock.h>
#include <physMap/physMap.h>

namespace Outliner
{
class OutlinerWindow;
}

class StaticGeometryContainer;
class GeomObject;
class Node;
class LandscapeEntityLibWindowHelper;
class HeightmapLandOutlinerInterface;
class HmapLandHoleObject;
class SplineObject;
class SplinePointObject;
class CrossRoadData;
class LandscapeEntityObject;
class SnowSourceObject;
struct NearSnowSource;


class HmapLandObjectEditor : public ObjectEditor, public IAssetBaseViewClient, public IAssetUpdateNotify, public IDagorEdCustomCollider
{
public:
  HmapLandObjectEditor();
  virtual ~HmapLandObjectEditor();

  // ObjectEditor interface implementation
  virtual void fillToolBar(PropPanel::ContainerPropertyControl *toolbar);
  virtual void addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint,
    bool check = false) override;
  virtual void updateToolbarButtons();
  virtual bool pickObjects(IGenViewportWnd *wnd, int x, int y, Tab<RenderableEditableObject *> &objs) override;

  // virtual void handleCommand(int cmd);
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif);
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual void gizmoStarted();
  virtual void gizmoEnded(bool apply);

  virtual void beforeRender();
  virtual void render();
  virtual void renderTrans();

  virtual void update(real dt);

  virtual void setEditMode(int cm);
  virtual bool canSelectObj(RenderableEditableObject *o) override;
  virtual void createObjectBySample(RenderableEditableObject *sample);
  virtual void registerViewportAccelerators(IWndManager &wndManager) override;

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id) override;

  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  void autoAttachSplines();
  void makeBottomSplines();

  // IAssetBaseViewClient
  virtual void onAvClose() override;
  virtual void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override {}
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override {}


  // IAssetUpdateNotify interface
  virtual void onLandClassAssetChanged(landclass::AssetData *data);
  virtual void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  virtual void onSplineClassAssetChanged(splineclass::AssetData *data);

  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt);
  virtual const char *getColliderName() const { return "RoadGeom"; }
  virtual bool isColliderVisible() const;
  virtual void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types);
  virtual void getTypeNames(Tab<String> &names);
  virtual void onSelectedNames(const Tab<String> &names);

  virtual void getLayerNames(int type, Tab<String> &names);

  void setSelectMode(int cm);
  int getSelectMode() { return selectMode; }

  void setSelectionGizmoTranformMode(bool enable);

  void clearToDefaultState();
  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  void save(DataBlock &splBlk, DataBlock &polBlk, DataBlock &entBlk, DataBlock &ltBlk, int layer);
  void load(const DataBlock &splBlk, const DataBlock &polBlk, const DataBlock &entBlk, const DataBlock &ltBlk, int layer);

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int stage);
  void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys);

  void prepareCatmul(SplinePointObject *p1, SplinePointObject *p2);
  void recalcCatmul();

  float screenDistBetweenPoints(IGenViewportWnd *wnd, SplinePointObject *p1, SplinePointObject *p2);
  float screenDistBetweenCursorAndPoint(IGenViewportWnd *wnd, int x, int y, SplinePointObject *p);

  void removeSpline(SplineObject *s);

  void onLandRegionChanged(float x0, float z0, float x1, float z1, bool recalc_all = false);
  void clone(bool clone_seed);

  bool isCloneMode() { return cloneMode; }
  bool isEditingSpline(SplineObject *spline) { return spline == curSpline; }

  void exportAsComposit();
  void splitComposits();
  bool splitComposits(const PtrTab<RenderableEditableObject> &sel, PtrTab<LandscapeEntityObject> &compObj,
    PtrTab<LandscapeEntityObject> &decompObj, DataBlock &splitSplinesBlk, PtrTab<RenderableEditableObject> &otherObj);
  void instantiateGenToEntities();

  void exportToDag();
  void importFromDag();
  void importFromNode(const Node *node, bool &for_all, int &choosed, Tab<String> &imps);

  void renderGeometry(bool opaque);
  void renderGrassMask();

  void onBrushPaintEnd();

  virtual void moveObjects(PtrTab<RenderableEditableObject> &obj, const Point3 &delta, IEditorCoreEngine::BasisType basis) override;

  //! marks cross roads where p belongs as changed
  void markCrossRoadChanged(SplinePointObject *p);

  //! updates cross roads config when p is changed (if NULL, cross roads are globally rebuilt)
  void updateCrossRoads(SplinePointObject *p = NULL);

  //! updates cross roads config when point is added
  void updateCrossRoadsOnPointAdd(SplinePointObject *p);

  //! updates cross roads config when point is removed
  void updateCrossRoadsOnPointRemove(SplinePointObject *p);

  //! generates road geometry for cross roads that are marked changed
  void updateCrossRoadGeom();

  //! updates all splines/segments marked changed
  void updateSplinesGeom();

  static void unloadRoadBuilderDll();

  inline int splinesCount() const { return splines.size(); }
  inline SplineObject *getSpline(int idx) { return splines[idx]; }
  inline const SplineObject *getSpline(int idx) const { return splines[idx]; }

  void calcSnow(StaticGeometryContainer &container);
  void updateSnowSources();

  GeomObject &getClearedWaterGeom();
  void removeWaterGeom();

  PropPanel::PanelWindowPropertyControl *getCurrentPanelFor(RenderableEditableObject *obj);
  PropPanel::PanelWindowPropertyControl *getObjectPropertiesPanel();

  bool usesRendinstPlacement() const override;

  bool isSelectOnlyIfEntireObjectInRect() const { return selectOnlyIfEntireObjectInRect; }

  void setPhysMap(PhysMap *phys_map) { physMap = phys_map; }

  bool isSampleObject(const RenderableEditableObject &object) const { return &object == sample; }

  // Called when an object's name has changed. Only called for objects that are supported by the Outliner, and are added
  // (registered) into Object Editor.
  void onRegisteredObjectNameChanged(RenderableEditableObject &object);

  // When the name of the asset within the object (Props::entityName) changes.
  void onObjectEntityNameChanged(RenderableEditableObject &object);

  // When an object has been moved to a different edit layer.
  void onObjectEditLayerChanged(RenderableEditableObject &object);

  void loadOutlinerSettings(const DataBlock &settings);
  void saveOutlinerSettings(DataBlock &settings);

  virtual void updateImgui() override;

public:
  class LoftAndGeomCollider : public IDagorEdCustomCollider
  {
  public:
    LoftAndGeomCollider(bool _loft, HmapLandObjectEditor &oe) : loft(_loft), objEd(oe), loftLayerOrder(-1) {}
    virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
    virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt);
    virtual const char *getColliderName() const { return loft ? "LoftGeom" : "PolyGeom"; }
    virtual bool isColliderVisible() const;

    void setLoftLayer(int layer) { loftLayerOrder = layer; }

  protected:
    bool loft;
    HmapLandObjectEditor &objEd;
    int loftLayerOrder;
  };
  LoftAndGeomCollider loftGeomCollider, polyGeomCollider;

  Tab<CrossRoadData *> crossRoads;
  bool requireTileTex, hasColorTex, hasLightmapTex, useMeshSurface;
  String lastExportPath, lastImportPath, lastAsCompositExportPath;
  bool crossRoadsChanged;
  bool splinesChanged;
  bool autoUpdateSpline;
  float maxPointVisDist;
  int generationCount;

  static int geomBuildCntLoft, geomBuildCntPoly, geomBuildCntRoad;
  static int waterSurfSubtypeMask;

  bool shouldShowPhysMatColors() const { return showPhysMatColors; }

protected:
  PlacementRotation buttonIdToPlacementRotation(int id);
  virtual void onRemoveObject(RenderableEditableObject &obj) override;
  virtual void _removeObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual void onAddObject(RenderableEditableObject &obj) override;
  virtual void _addObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual void onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags) override;
  virtual void updateSelection() override;
  virtual void fillSelectionMenu(IGenViewportWnd *wnd, PropPanel::IMenu *menu) override;

  bool findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out, bool place_on_ri_collision = false);
  void selectNewObjEntity(const char *name);
  SplineObject *findSplineAndDirection(IGenViewportWnd *wnd, SplinePointObject *p, bool &dirs_equal, SplineObject *excl = NULL);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &selection_service,
    LandscapeEntityObject &landscape_entity, IObjEntity &entity, const Point3 &ray_origin, const Point3 &ray_direction,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  void getPixelPerfectHits(IPixelPerfectSelectionService &selection_service, IGenViewportWnd &wnd, int x, int y,
    Tab<RenderableEditableObject *> &hits);

  bool isOutlinerWindowOpen() const { return outlinerWindow.get() != nullptr; }
  void showOutlinerWindow(bool show);

  static RenderableEditableObject &getMainObjectForOutliner(RenderableEditableObject &object);

  Point3 *debugP1, *debugP2;

  SplinePointObject *catmul[4];

  int selectMode;
  int currentMode;

  Tab<SplineObject *> splines;
  GeomObject *waterGeom;

  bool cloneMode;
  Point3 cloneDelta;
  String cloneName;
  PtrTab<RenderableEditableObject> cloneObjs;

  bool inGizmo;
  DynRenderBuffer *ptDynBuf;
  Point3 locAx[3];

  Ptr<SplinePointObject> curPt;
  SplineObject *curSpline;
  String curSplineAsset;

  Ptr<LandscapeEntityObject> newObj;

  IObjectCreator *objCreator;
  bool areLandHoleBoxesVisible;
  bool hideSplines;
  bool usePixelPerfectSelection;
  bool selectOnlyIfEntireObjectInRect;
  bool showPhysMat, showPhysMatColors;

  dag::Vector<IPixelPerfectSelectionService::Hit> pixelPerfectSelectionHitsCache;
  Tab<NearSnowSource> nearSources;

  shaders::OverrideStateId zFuncLessStateId;

  PhysMap *physMap = nullptr;

  eastl::unique_ptr<Outliner::OutlinerWindow> outlinerWindow;
  eastl::unique_ptr<HeightmapLandOutlinerInterface> outlinerInterface;
  DataBlock outlinerSettings;
};
