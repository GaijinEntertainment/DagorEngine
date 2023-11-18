// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _DE2_PLUGIN_ROADS_ROADOBJECTSEDITOR_H_
#define _DE2_PLUGIN_ROADS_ROADOBJECTSEDITOR_H_
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

class StaticGeometryContainer;
class GeomObject;
class Node;
class LandscapeEntityLibWindowHelper;
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
  virtual void fillToolBar(PropertyContainerControlBase *toolbar);
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
  virtual void createObjectBySample(RenderableEditableObject *sample);

  virtual void onClick(int pcb_id, PropPanel2 *panel);
  void autoAttachSplines();
  void makeBottomSplines();

  // IAssetBaseViewClient
  virtual void onAvClose();
  virtual void onAvAssetDblClick(const char *asset_name) {}
  virtual void onAvSelectAsset(const char *asset_name);
  virtual void onAvSelectFolder(const char *asset_folder_name) {}


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

  CPanelWindow *getCurrentPanelFor(RenderableEditableObject *obj);

  bool usesRendinstPlacement() const override;

  bool isSelectOnlyIfEntireObjectInRect() const { return selectOnlyIfEntireObjectInRect; }

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

  struct LayersDlg : public ControlEventHandler
  {
  public:
    LayersDlg();
    ~LayersDlg();

    bool isVisible() const;
    void show();
    void hide();
    void refillPanel();

    virtual void onChange(int pid, PropertyContainerControlBase *panel);
    virtual void onClick(int pid, PropertyContainerControlBase *panel);
    virtual void onDoubleClick(int pid, PropertyContainerControlBase *panel);
    virtual void onPostEvent(int pid, PropPanel2 *panel)
    {
      if (pid == 1)
        refillPanel();
    }

    CDialogWindow *dlg;
    int wr[4];
    DataBlock panelState;
  };
  LayersDlg layersDlg;

protected:
  PlacementRotation buttonIdToPlacementRotation(int id);
  virtual void _removeObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual void _addObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual bool canSelectObj(RenderableEditableObject *o);

  bool findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out, bool place_on_ri_collision = false);
  void selectNewObjEntity(const char *name);
  SplineObject *findSplineAndDirection(IGenViewportWnd *wnd, SplinePointObject *p, bool &dirs_equal, SplineObject *excl = NULL);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &selection_service,
    LandscapeEntityObject &landscape_entity, IObjEntity &entity, const Point3 &ray_origin, const Point3 &ray_direction,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  void getPixelPerfectHits(IPixelPerfectSelectionService &selection_service, IGenViewportWnd &wnd, int x, int y,
    Tab<RenderableEditableObject *> &hits);

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

  dag::Vector<IPixelPerfectSelectionService::Hit> pixelPerfectSelectionHitsCache;
  Tab<NearSnowSource> nearSources;

  shaders::OverrideStateId zFuncLessStateId;
};

#endif
