// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assetsGui/av_client.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_collision.h>
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
  ~HmapLandObjectEditor() override;

  // ObjectEditor interface implementation
  void fillToolBar(PropPanel::ContainerPropertyControl *toolbar) override;
  void addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint, bool check = false) override;
  void updateToolbarButtons() override;
  bool pickObjects(IGenViewportWnd *wnd, int x, int y, Tab<RenderableEditableObject *> &objs) override;

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;

  void beforeRender() override;
  void render() override;
  void renderTrans() override;

  void update(real dt) override;

  void setEditMode(int cm) override;
  bool canSelectObj(RenderableEditableObject *o) override;
  void createObjectBySample(RenderableEditableObject *sample) override;
  void registerViewportAccelerators(IWndManager &wndManager) override;

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void autoAttachSplines();
  void makeBottomSplines();

  // IAssetBaseViewClient
  void onAvClose() override;
  void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override {}
  void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override {}


  // IAssetUpdateNotify interface
  void onLandClassAssetChanged(landclass::AssetData *data) override;
  void onLandClassAssetTexturesChanged(landclass::AssetData *data) override {}
  void onSplineClassAssetChanged(splineclass::AssetData *data) override;

  // IDagorEdCustomCollider
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override;
  const char *getColliderName() const override { return "RoadGeom"; }
  bool isColliderVisible() const override;
  void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types) override;
  void getTypeNames(Tab<String> &names) override;
  void onSelectedNames(const Tab<String> &names) override;

  void getLayerNames(int type, Tab<String> &names) override;

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

  void moveObjects(PtrTab<RenderableEditableObject> &obj, const Point3 &delta, IEditorCoreEngine::BasisType basis) override;

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

  bool supportsRealtimeUpdate() const;

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
  void saveOutlinerState();

  void updateImgui() override;

public:
  class LoftAndGeomCollider : public IDagorEdCustomCollider
  {
  public:
    LoftAndGeomCollider(bool _loft, HmapLandObjectEditor &oe) : loft(_loft), objEd(oe), loftLayerOrder(-1) {}
    bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
    bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override;
    const char *getColliderName() const override { return loft ? "LoftGeom" : "PolyGeom"; }
    bool isColliderVisible() const override;

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
  void onRemoveObject(RenderableEditableObject &obj) override;
  void _removeObjects(RenderableEditableObject **obj, int num, bool use_undo) override;
  void onAddObject(RenderableEditableObject &obj) override;
  void _addObjects(RenderableEditableObject **obj, int num, bool use_undo) override;
  void onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags) override;
  void updateSelection() override;
  void fillSelectionMenu(IGenViewportWnd *wnd, PropPanel::IMenu *menu) override;

  bool findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out, bool place_on_ri_collision = false);
  void selectNewObjEntity(const char *name);
  SplineObject *findSplineAndDirection(IGenViewportWnd *wnd, SplinePointObject *p, bool &dirs_equal, SplineObject *excl = NULL);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &selection_service,
    LandscapeEntityObject &landscape_entity, IObjEntity &entity, const Point3 &ray_origin, const Point3 &ray_direction,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  void getPixelPerfectHits(IPixelPerfectSelectionService &selection_service, IGenViewportWnd &wnd, int x, int y,
    Tab<RenderableEditableObject *> &hits);

  // Returns true if it was in progress and got canceled.
  bool stopSplineCreation();

  bool isOutlinerWindowOpen() const { return outlinerWindowOpen; }
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
  bool outlinerWindowOpen = false;
};
