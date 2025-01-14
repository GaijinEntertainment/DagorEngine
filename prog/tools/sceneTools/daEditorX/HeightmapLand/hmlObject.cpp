// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlEntity.h"
#include "hmlLight.h"
#include "hmlOutliner.h"
#include "hmlSnow.h"
#include "entityCopyDlg.h"

#include "hmlObjectsEditor.h"
#include "hmlHoleObject.h"
#include "roadUtil.h"

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <de3_splineClassData.h>
#include <de3_genObjUtil.h>
#include <de3_genObjData.h>
#include <de3_hmapService.h>

#include <EditorCore/ec_IEditorCore.h>

#include <coolConsole/coolConsole.h>

#include <libTools/staticGeom/geomObject.h>
#include <libTools/util/strUtil.h>

#include <libTools/dagFileRW/splineShape.h>
#include <libTools/dagFileRW/dagFileShapeObj.h>
#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_outliner.h>
#include <propPanel/commonWindow/dialogWindow.h>

#include <math/dag_math2d.h>
#include <math/dag_mathBase.h>
#include <math/dag_rayIntersectBox.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_fastIntList.h>

#include "crossRoad.h"
#include "hmlCm.h"
#include "common.h"

#include <debug/dag_debug.h>
#include <heightMapLand/dag_hmlGetHeight.h>

#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;
using editorcore_extapi::make_full_start_path;

using heightmapland::CopyDlg;

#define OBJECT_SPLINE  "Splines"
#define OBJECT_POLYGON "Polygons"
#define OBJECT_ENTITY  "Entities"
#define OBJECT_LIGHT   "Sphere lights"
#define OBJECT_SNOW    "Snow sources"

int HmapLandObjectEditor::geomBuildCntLoft = 0;
int HmapLandObjectEditor::geomBuildCntPoly = 0;
int HmapLandObjectEditor::geomBuildCntRoad = 0;
int HmapLandObjectEditor::waterSurfSubtypeMask = -1;


extern objgenerator::WorldHugeBitmask bm_loft_mask[LAYER_ORDER_MAX], bm_poly_mask;

static TEXTUREID signTexPt = BAD_TEXTUREID;


#define CROSSROADS_TRACE_GEOM 0

#define TRACE_CACHE_CELL_SIZE     256
#define TRACE_CACHE_CELL_SIZE_INV (1.f / TRACE_CACHE_CELL_SIZE)
#define TRACE_CACHE_EXTENSION     (TRACE_CACHE_CELL_SIZE / 4)

struct TraceSplineCache
{
  void *arrayPtr;
  int arrayCount;
  Tab<SplineObject *> cached;
  int ix, iz;
  float dist;

  TraceSplineCache() { reset(); }

  void reset()
  {
    arrayPtr = nullptr;
    arrayCount = VERY_BIG_NUMBER;
    cached.clear();
    ix = VERY_BIG_NUMBER;
    iz = VERY_BIG_NUMBER;
  }

  bool build(const Tab<SplineObject *> &splines, const Point3 &p, const Point3 &dir)
  {
    if (splines.size() < 50)
      return false;

    bool valid = SplineObject::isSplineCacheValid && ((void *)splines.data() == arrayPtr && splines.size() == arrayCount);
    int x = 0;
    int z = 0;
    if (valid)
    {
      x = int(floorf(p.x * TRACE_CACHE_CELL_SIZE_INV));
      z = int(floorf(p.z * TRACE_CACHE_CELL_SIZE_INV));
      valid = (x == ix && z == iz);
    }

    if (valid)
      return true;

    reset();

    BBox3 cellBox(
      Point3(x * TRACE_CACHE_CELL_SIZE - TRACE_CACHE_EXTENSION, -VERY_BIG_NUMBER, z * TRACE_CACHE_CELL_SIZE - TRACE_CACHE_EXTENSION),
      Point3((x + 1) * TRACE_CACHE_CELL_SIZE + TRACE_CACHE_EXTENSION, VERY_BIG_NUMBER,
        (z + 1) * TRACE_CACHE_CELL_SIZE + TRACE_CACHE_EXTENSION));

    ix = x;
    iz = z;
    arrayPtr = (void *)splines.data();
    arrayCount = splines.size();

    for (int i = 0; i < splines.size(); i++)
      if (splines[i]->isCreated())
      {
        if (splines[i]->getSplineBox() & cellBox)
          cached.push_back(splines[i]);
      }

    SplineObject::isSplineCacheValid = true;
    return true;
  }
} spline_cache;


HmapLandObjectEditor::HmapLandObjectEditor() :
  inGizmo(false),
  splines(midmem_ptr()),
  cloneMode(false),
  crossRoads(midmem),
  loftGeomCollider(true, *this),
  polyGeomCollider(false, *this),
  nearSources(tmpmem),
  cloneObjs(midmem)
{
  waterGeom = NULL;
  objCreator = NULL;
  areLandHoleBoxesVisible = true;
  hideSplines = false;
  showPhysMat = showPhysMatColors = false;
  autoUpdateSpline = true;
  usePixelPerfectSelection = false;
  selectOnlyIfEntireObjectInRect = false;
  maxPointVisDist = 5000.0;

  memset(catmul, 0, sizeof(catmul));
  clear_and_shrink(splines);
  ptDynBuf = dagRender->newDynRenderBuffer("editor_rhv_tex", midmem);

  SplinePointObject::initStatics();
  if (signTexPt == BAD_TEXTUREID)
  {
    signTexPt = dagRender->addManagedTexture(::make_full_start_path("../commonData/tex/sign.tga"));
    dagRender->acquireManagedTex(signTexPt);
  }

  curPt = NULL;
  curSpline = NULL;
  generationCount = 0;

  selectMode = -1;
  ObjectEditor::setEditMode(CM_OBJED_MODE_SELECT);

  debugP1 = debugP2 = NULL;

  crossRoadsChanged = true;

  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (assetSrv)
    assetSrv->subscribeUpdateNotify(this, true, true);

  setSuffixDigitsCount(3);
  nearSources.reserve(SnowSourceObject::NEAR_SRC_COUNT);

  if (waterSurfSubtypeMask == -1)
    waterSurfSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("water_surf");

  shaders::OverrideState zFuncLessState;
  zFuncLessState.set(shaders::OverrideState::Z_FUNC);
  zFuncLessState.zFunc = CMPF_LESS;
  zFuncLessStateId = dagGeom->create(zFuncLessState);
}


HmapLandObjectEditor::~HmapLandObjectEditor()
{
  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (assetSrv)
    assetSrv->unsubscribeUpdateNotify(this, true, true);

  dagGeom->deleteGeomObject(waterGeom);
  dagRender->deleteDynRenderBuffer(ptDynBuf);
  setCreateBySampleMode(NULL);
  selectNewObjEntity(NULL);

  dagGeom->destroy(zFuncLessStateId);
}

void HmapLandObjectEditor::clearToDefaultState()
{
  setCreateBySampleMode(NULL);
  selectNewObjEntity(NULL);
  del_it(objCreator);

  inGizmo = false;
  areLandHoleBoxesVisible = true;

  memset(catmul, 0, sizeof(catmul));

  SplinePointObject::initStatics();
  SphereLightObject::clearMats();
  SnowSourceObject::clearMats();

  curPt = NULL;
  curSpline = NULL;

  selectMode = -1;
}

PropPanel::PanelWindowPropertyControl *HmapLandObjectEditor::getCurrentPanelFor(RenderableEditableObject *obj)
{
  if (selectedCount() != 1 || getSelected(0) != obj || !objectPropBar)
    return NULL;
  return objectPropBar->getPanel();
}

PropPanel::PanelWindowPropertyControl *HmapLandObjectEditor::getObjectPropertiesPanel()
{
  return objectPropBar ? objectPropBar->getPanel() : nullptr;
}

void HmapLandObjectEditor::addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint,
  bool check)
{
  // In the Landscape editor instead of the "Select objects by name" dialog we show the Outliner.
  if (id == CM_OBJED_SELECT_BY_NAME)
  {
    hint = "Outliner (H)";
    check = true;
  }

  ObjectEditor::addButton(tb, id, bmp_name, hint, check);
}

void HmapLandObjectEditor::fillToolBar(PropPanel::ContainerPropertyControl *toolbar)
{
  PropPanel::ContainerPropertyControl *tb1 = toolbar->createToolbarPanel(0, "");

  addButton(tb1, CM_SHOW_PANEL, "show_hml_panel", "Show properties panel (P)", true);
  //  addButton(tb1, CM_SHOW_LAND_OBJECTS, "show_bounds", "Show land hole boxes", true);
  tb1->createSeparator();

  addButton(tb1, CM_HILL_UP, "terr_up_vertex", "Hill up mode (1)", true);
  addButton(tb1, CM_HILL_DOWN, "terr_down_vertex", "Hill down mode (2)", true);
  addButton(tb1, CM_ALIGN, "terr_align", "Align mode (3)", true);
  addButton(tb1, CM_SMOOTH, "terr_smooth", "Smooth mode (4)", true);
  if (hasLightmapTex)
    addButton(tb1, CM_SHADOWS, "terr_light", "Calculate lighting mode (5)", true);
  addButton(tb1, CM_SCRIPT, "terr_script", "Apply script (6)", true);
  tb1->createSeparator();

  ObjectEditor::fillToolBar(toolbar);

  PropPanel::ContainerPropertyControl *tb2 = toolbar->createToolbarPanel(0, "");

  tb2->createSeparator();
  addButton(tb2, CM_HIDE_SPLINES, "hide_splines", "Hide splines (Ctrl+0)", true);
  addButton(tb2, CM_SHOW_PHYSMAT, "show_physmat", "Show physmat", true);
  addButton(tb2, CM_SHOW_PHYSMAT_COLORS, "show_physmat_colors", "Show physmat color", true);
  tb2->createSeparator();
  addButton(tb2, CM_SELECT_PT, "select_vertex", "Select only points (Ctrl+1)", true);
  addButton(tb2, CM_SELECT_SPLINES, "select_spline", "Select only splines (Ctrl+2)", true);
  addButton(tb2, CM_SELECT_ENT, "select_entity", "Select only entities (Ctrl+3)", true);
  addButton(tb2, CM_SELECT_SPL_ENT, "select_spl_ent", "Select only splines and entities (Ctrl+7)", true);
  //  addButton(tb2, CM_SELECT_LT, "select_lt", "Select only light spheres (Ctrl+4)", true);
  if (HmapLandPlugin::self->isSnowAvailable())
    addButton(tb2, CM_SELECT_SNOW, "hide_multi_objects", "Select only snow sources (Ctrl+5)", true);
  addButton(tb2, CM_USE_PIXEL_PERFECT_SELECTION, "ctrl_eff_from_att", "Use pixel perfect selection (Ctrl+Q)", true);
  addButton(tb2, CM_SELECT_ONLY_IF_ENTIRE_OBJECT_IN_RECT, "select_only_if_entire_object_in_rect",
    "Select only if the entire object is in the selection rectangle", true);
  tb2->createSeparator();

  //  addButton(tb2, CM_CREATE_HOLEBOX_MODE, "create_holebox", "Create land hole box", true);
  addButton(tb2, CM_CREATE_ENTITY, "create_lib_ent", "Create entity", true);
  addButton(tb2, CM_ROTATION_NONE, "rotation_none", "No rotation", true);
  addButton(tb2, CM_ROTATION_X, "rotation_x", "X to normal", true);
  addButton(tb2, CM_ROTATION_Y, "rotation_y", "Y to normal", true);
  addButton(tb2, CM_ROTATION_Z, "rotation_z", "Z to normal", true);
  addButton(tb2, CM_CREATE_SPLINE, "create_spline", "Create spline (hold shift to place on RI)", true);
  addButton(tb2, CM_CREATE_POLYGON, "create_poly", "Create polygon", true);
  //  addButton(tb2, CM_CREATE_LT, "create_lt", "Create light sphere", true);
  if (HmapLandPlugin::self->isSnowAvailable())
    addButton(tb2, CM_CREATE_SNOW_SOURCE, "create_multi_object", "Create snow source", true);
  tb2->createSeparator();

  addButton(tb2, CM_REFINE_SPLINE, "spline_refine", "Refine spline", true);
  addButton(tb2, CM_SPLIT_SPLINE, "split_spline", "Split spline", true);
  addButton(tb2, CM_SPLIT_POLY, "split_poly", "Split poly", true);

  addButton(tb2, CM_REVERSE_SPLINE, "spline_reverse", "Reverse spline(s)");
  addButton(tb2, CM_CLOSE_SPLINE, "spline_close", "Close selected spline(s)");
  addButton(tb2, CM_OPEN_SPLINE, "spline_break", "Un-close spline at selected point");
  tb2->createSeparator();

  addButton(tb2, CM_MANUAL_SPLINE_REGEN_MODE, "offline", "Use manual obj/geom update for spline/poly", true);
  addButton(tb2, CM_SPLINE_REGEN, "compile", "Manual spline/poly obj/geom update (F1)");

  setButton(CM_MANUAL_SPLINE_REGEN_MODE, !autoUpdateSpline);
  enableButton(CM_SPLINE_REGEN, !autoUpdateSpline);

  addButton(tb2, CM_REBUILD_SPLINES_BITMASK, "splinemask", "Rebuild splines bitmask (F2)");
}

void HmapLandObjectEditor::updateToolbarButtons()
{
  ObjectEditor::updateToolbarButtons();

  setRadioButton(CM_SMOOTH, getEditMode());
  setRadioButton(CM_ALIGN, getEditMode());
  setRadioButton(CM_HILL_UP, getEditMode());
  setRadioButton(CM_HILL_DOWN, getEditMode());
  setRadioButton(CM_SHADOWS, getEditMode());
  setRadioButton(CM_SCRIPT, getEditMode());
  setRadioButton(CM_CREATE_SPLINE, getEditMode());
  setRadioButton(CM_REFINE_SPLINE, getEditMode());
  setRadioButton(CM_SPLIT_SPLINE, getEditMode());
  setRadioButton(CM_SPLIT_POLY, getEditMode());
  setRadioButton(CM_CREATE_POLYGON, getEditMode());
  setRadioButton(CM_CREATE_ENTITY, getEditMode());
  setButton(CM_ROTATION_NONE, buttonIdToPlacementRotation(CM_ROTATION_NONE) == selectedPlacementRotation);
  setButton(CM_ROTATION_X, buttonIdToPlacementRotation(CM_ROTATION_X) == selectedPlacementRotation);
  setButton(CM_ROTATION_Y, buttonIdToPlacementRotation(CM_ROTATION_Y) == selectedPlacementRotation);
  setButton(CM_ROTATION_Z, buttonIdToPlacementRotation(CM_ROTATION_Z) == selectedPlacementRotation);
  setRadioButton(CM_CREATE_HOLEBOX_MODE, getEditMode());
  setRadioButton(CM_CREATE_LT, getEditMode());
  setRadioButton(CM_CREATE_SNOW_SOURCE, getEditMode());
  setButton(CM_SHOW_LAND_OBJECTS, areLandHoleBoxesVisible);
  setButton(CM_HIDE_SPLINES, hideSplines);
  setButton(CM_SHOW_PHYSMAT, showPhysMat);
  setButton(CM_SHOW_PHYSMAT_COLORS, showPhysMatColors);
  setButton(CM_MANUAL_SPLINE_REGEN_MODE, !autoUpdateSpline);
  setButton(CM_OBJED_SELECT_BY_NAME, isOutlinerWindowOpen());
  setButton(CM_USE_PIXEL_PERFECT_SELECTION, usePixelPerfectSelection);
  setButton(CM_SELECT_ONLY_IF_ENTIRE_OBJECT_IN_RECT, selectOnlyIfEntireObjectInRect);

  if (HmapLandPlugin::self)
    enableButton(CM_SPLINE_REGEN, !autoUpdateSpline);
}


void HmapLandObjectEditor::beforeRender()
{
  ObjectEditor::beforeRender();

  if (autoUpdateSpline)
    updateSplinesGeom();
  else if (splinesChanged)
  {
    bool changed = false;
    for (int i = 0; i < splines.size(); ++i)
      if (splines[i]->modifChanged)
      {
        splines[i]->reApplyModifiers(false);
        changed = true;
      }
    if (changed)
    {
      SplineObject::isSplineCacheValid = false;
      HmapLandPlugin::self->applyHmModifiers(false, true, false);
      HmapLandPlugin::hmlService->invalidateClipmap(false);
      DAGORED2->invalidateViewportCache();
    }
  }
}

void HmapLandObjectEditor::updateSplinesGeom()
{
  static Tab<SplineObject *> chSpl(tmpmem);

  if (!splinesChanged && !crossRoadsChanged)
    return;

  SplineObject::isSplineCacheValid = false;
  HmapLandPlugin::hmlService->invalidateClipmap(false, false);
  if (HmapLandPlugin::gpuGrassService)
    HmapLandPlugin::gpuGrassService->invalidate();
  if (HmapLandPlugin::grassService)
    HmapLandPlugin::grassService->forceUpdate();
  generationCount++;
  geomBuildCntLoft = geomBuildCntPoly = geomBuildCntRoad = 0;
  // DAEDITOR3.conNote("update %d", generationCount);

  if (crossRoadsChanged)
    for (int i = 0; i < crossRoads.size(); ++i)
      if (crossRoads[i]->checkCrossRoad())
        splinesChanged = true;

  if (splinesChanged)
  {
    for (int i = 0; i < splines.size(); ++i)
      if (splines[i]->modifChanged)
        splines[i]->reApplyModifiers(false);
    HmapLandPlugin::self->applyHmModifiers(true, true, false);

    // first spline update stage: generate geometry
    for (int i = 0; i < splines.size(); ++i)
      if (splines[i]->splineChanged)
      {
        // DAEDITOR3.conNote("update spline %s, first pass", splines[i]->getName());
        chSpl.push_back(splines[i]);
        splines[i]->updateSpline(SplineObject::STAGE_START);
      }

    splinesChanged = false;
  }

  if (crossRoadsChanged)
  {
    updateCrossRoadGeom();
    crossRoadsChanged = false;
  }

  // last spline update stage: generate entities
  for (int j = SplineObject::STAGE_START + 1; j <= SplineObject::STAGE_FINISH; j++)
  {
    if (j == SplineObject::STAGE_GEN_LOFT)
    {
      for (int layer = 0; layer < LAYER_ORDER_MAX; ++layer)
      {
        loftGeomCollider.setLoftLayer(layer);
        for (int i = 0; i < chSpl.size(); ++i)
          if (chSpl[i]->getLayer() == layer)
            chSpl[i]->updateSpline(j);
      }
      loftGeomCollider.setLoftLayer(-1);
    }
    else
      for (int i = 0; i < chSpl.size(); ++i)
        chSpl[i]->updateSpline(j);
  }

  BBox3 changedRegion;

  for (int i = 0; i < chSpl.size(); ++i)
    changedRegion += chSpl[i]->getGeomBoxChanges();

  chSpl.clear();

  LandscapeEntityObject::rePlaceAllEntitiesOnCollision(*this, geomBuildCntLoft > 0, geomBuildCntPoly > 0, geomBuildCntRoad > 0,
    changedRegion);
}

void HmapLandObjectEditor::render()
{
  bool curPlugin = DAGORED2->curPlugin() == HmapLandPlugin::self;
  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

  TMatrix4 gtm;
  d3d::getglobtm(gtm);
  Frustum frustum;
  frustum.construct(gtm);
  int w, h;
  d3d::get_target_size(w, h);
  Point2 scale;
  scale.x = 2.0 / w;
  scale.y = 2.0 / h;

  if (curPlugin)
  {
    bool all = !hideSplines;
    SplineObject *spl_of_sel_pt = NULL;
    if (hideSplines)
      for (int i = 0; i < selection.size(); ++i)
        if (SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[i]))
        {
          spl_of_sel_pt = p->spline;
          break;
        }

    if ((st_mask & SphereLightObject::lightGeomSubtype) && all)
      for (int i = 0; i < objects.size(); ++i)
      {
        SphereLightObject *l = RTTI_cast<SphereLightObject>(objects[i]);
        if (l)
          l->render();
      }

    dagRender->startLinesRender(true, false, true);
    dagRender->setLinesTm(TMatrix::IDENT);
    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected() || splines[i] == spl_of_sel_pt)
        splines[i]->renderLines(false, frustum);
    dagRender->endLinesRender();

    dagRender->startLinesRender();
    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected() || splines[i] == spl_of_sel_pt)
        splines[i]->renderLines(true, frustum);

    if (debugP1 && debugP2)
      dagRender->renderLine(*debugP1, *debugP2, E3DCOLOR(255, 0, 0));
    dagRender->endLinesRender();

    dagRender->dynRenderBufferClearBuf(*ptDynBuf);


    int cnt = 0;
    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected() || splines[i] == spl_of_sel_pt)
        splines[i]->render(ptDynBuf, gtm, scale, frustum, cnt);

    if (curPt && curPt->visible)
      SplinePointObject::renderPoint(*ptDynBuf, toPoint4(curPt->getPt(), 1) * gtm, scale * SplinePointObject::ptScreenRad,
        E3DCOLOR(200, 200, 10));

    dagGeom->set(zFuncLessStateId);
    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 0);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, SplinePointObject::texPt);
    dagGeom->reset_override();

    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 1);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, SplinePointObject::texPt);
    dagRender->dynRenderBufferFlush(*ptDynBuf);

    // render note signs
    dagRender->dynRenderBufferClearBuf(*ptDynBuf);
    if (!all)
      goto end;
    for (int i = 0; i < objects.size(); ++i)
    {
      LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]);
      if (p && !p->getProps().notes.empty())
        SplinePointObject::renderPoint(*ptDynBuf, toPoint4(p->getPos(), 1) * gtm, 3 * scale * SplinePointObject::ptScreenRad,
          E3DCOLOR(50, 60, 255));
    }
    for (int i = 0; i < splines.size(); ++i)
      if (!splines[i]->getProps().notes.empty())
        SplinePointObject::renderPoint(*ptDynBuf, toPoint4(splines[i]->points[0]->getPos(), 1) * gtm,
          3 * scale * SplinePointObject::ptScreenRad, E3DCOLOR(50, 160, 255));

    dagGeom->set(zFuncLessStateId);
    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 0);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, signTexPt);
    dagGeom->reset_override();

    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 1);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, signTexPt);
    dagRender->dynRenderBufferFlush(*ptDynBuf);
  }
  else if ((HmapLandPlugin::self->renderAllSplinesAlways || HmapLandPlugin::self->renderSelSplinesAlways))
  {
    bool all = HmapLandPlugin::self->renderAllSplinesAlways && !hideSplines;

    dagRender->startLinesRender(true, false, true);
    dagRender->setLinesTm(TMatrix::IDENT);
    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected())
        splines[i]->renderLines(false, frustum);
    dagRender->endLinesRender();

    dagRender->startLinesRender();

    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected())
        splines[i]->renderLines(true, frustum);

    if (debugP1 && debugP2)
      dagRender->renderLine(*debugP1, *debugP2, E3DCOLOR(255, 0, 0));

    dagRender->endLinesRender();

    dagRender->dynRenderBufferClearBuf(*ptDynBuf);


    int cnt = 0;
    for (int i = 0; i < splines.size(); ++i)
      if (all || splines[i]->isSelected())
        splines[i]->render(ptDynBuf, gtm, scale, frustum, cnt);

    dagGeom->set(zFuncLessStateId);
    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 0);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, SplinePointObject::texPt);
    dagGeom->reset_override();

    dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 1);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, SplinePointObject::texPt);
    dagRender->dynRenderBufferFlush(*ptDynBuf);
  }

end:
  if (objCreator)
    objCreator->render();
}


void HmapLandObjectEditor::renderTrans()
{
  bool curPlugin = DAGORED2->curPlugin() == HmapLandPlugin::self;
  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

  if (!curPlugin)
    return;

  // render object boxes
  if (1)
  {
    if (st_mask & SphereLightObject::lightGeomSubtype)
      for (int i = 0; i < objects.size(); ++i)
      {
        SphereLightObject *l = RTTI_cast<SphereLightObject>(objects[i]);
        if (l)
          l->renderTrans();
      }

    dagRender->startLinesRender();
    dagRender->setLinesTm(TMatrix::IDENT);
    for (int i = 0; i < objects.size(); ++i)
    {
      if (LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]))
      {
        p->renderBox();
        continue;
      }

      if (areLandHoleBoxesVisible)
      {
        if (HmapLandHoleObject *o = RTTI_cast<HmapLandHoleObject>(objects[i]))
          o->render();
      }

      if (st_mask & SphereLightObject::lightGeomSubtype)
      {
        if (SphereLightObject *l = RTTI_cast<SphereLightObject>(objects[i]))
          l->renderObject();
      }
      if (st_mask & SnowSourceObject::showSubtypeId)
      {
        if (SnowSourceObject *sn = RTTI_cast<SnowSourceObject>(objects[i]))
          sn->renderObject();
      }
    }

    dagRender->endLinesRender();
  }
}

void HmapLandObjectEditor::renderGrassMask()
{
  d3d::settm(TM_WORLD, TMatrix::IDENT);
  TMatrix4 globtm;
  d3d::getglobtm(globtm);
  Frustum frustum;
  frustum.construct(globtm);

  ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  if (splSrv)
  {
    FastIntList loft_layers;
    loft_layers.addInt(0);
    splSrv->gatherLoftLayers(loft_layers, true);
    bool opaque = true;
    for (int lli = 0; lli < loft_layers.size(); lli++)
    {
      int ll = loft_layers.getList()[lli];
      splSrv->renderLoftGeom(ll, opaque, frustum, 0);
    }
  }

  IRenderingService *prefabMgr = nullptr;
  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
    IRenderingService *iface = p->queryInterface<IRenderingService>();
    if (stricmp(p->getInternalName(), "_prefabEntMgr") == 0)
      prefabMgr = iface;
  }
  if (prefabMgr)
  {
    prefabMgr->renderGeometry(IRenderingService::STG_RENDER_GRASS_MASK);
    // prefabs rendering can modify TM_WORLD, so we restore default
    d3d::settm(TM_WORLD, TMatrix::IDENT);
  }
}

void HmapLandObjectEditor::renderGeometry(bool opaque)
{
  static int landCellShortDecodeXZ = dagGeom->getShaderVariableId("landCellShortDecodeXZ");
  static int landCellShortDecodeY = dagGeom->getShaderVariableId("landCellShortDecodeY");
  static Color4 dec_xz_0(0, 0, 0, 0), dec_y_0(0, 0, 0, 0);
  if (landCellShortDecodeXZ != -1)
  {
    dec_xz_0 = dagGeom->shaderGlobalGetColor4(landCellShortDecodeXZ);
    dagGeom->shaderGlobalSetColor4(landCellShortDecodeXZ, Color4(1.0f / 32767.f, 0, 0, 0));
  }
  if (landCellShortDecodeY != -1)
  {
    dec_y_0 = dagGeom->shaderGlobalGetColor4(landCellShortDecodeY);
    dagGeom->shaderGlobalSetColor4(landCellShortDecodeY, Color4(1.0f / 32767.f, 0, 0, 0));
  }

  ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  d3d::settm(TM_WORLD, TMatrix::IDENT);
  TMatrix4 globtm;
  d3d::getglobtm(globtm);
  Frustum frustum;
  frustum.construct(globtm);
  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (waterGeom && (st_mask & waterSurfSubtypeMask))
  {
    if (opaque)
      dagGeom->geomObjectRender(*waterGeom);
    else
      dagGeom->geomObjectRenderTrans(*waterGeom);
  }

  FastIntList loft_layers;
  loft_layers.addInt(0);
  Tab<SplineObject *> renderSplines;
  renderSplines.reserve(splines.size());
  for (int i = 0; i < splines.size(); ++i)
  {
    if (frustum.testBoxB(splines[i]->getGeomBox()))
      renderSplines.push_back(splines[i]);
  }
  if (splSrv && (st_mask & SplineObject::splineSubtypeMask))
    splSrv->gatherLoftLayers(loft_layers, true);

  for (int lli = 0; lli < loft_layers.size(); lli++)
  {
    int max_layer = 0, ll = loft_layers.getList()[lli];
    for (int i = 0; i < renderSplines.size(); ++i)
    {
      int l = renderSplines[i]->getLayer();
      if (!l)
        renderSplines[i]->renderGeom(opaque, ll, frustum);
      else if (max_layer < l)
        max_layer = l;
    }
    if (splSrv)
      splSrv->renderLoftGeom(ll, opaque, frustum, 0);
    for (int l = 1; l <= max_layer; l++)
    {
      for (int i = 0; i < renderSplines.size(); ++i)
        if (renderSplines[i]->getLayer() == l)
          renderSplines[i]->renderGeom(opaque, ll, frustum);
      if (splSrv)
        splSrv->renderLoftGeom(ll, opaque, frustum, l);
    }
  }

  if (st_mask & SplineObject::roadsSubtypeMask)
    for (int i = 0; i < crossRoads.size(); i++)
    {
      GeomObject *geom = crossRoads[i]->getRoadGeom();
      if (!geom)
        continue;
      if (!frustum.testBoxB(dagGeom->geomObjectGetBoundBox(*geom)))
        continue;
      if (opaque)
        dagGeom->geomObjectRender(*geom);
      else
        dagGeom->geomObjectRenderTrans(*geom);
    }

  if (landCellShortDecodeXZ != -1)
    dagGeom->shaderGlobalSetColor4(landCellShortDecodeXZ, dec_xz_0);
  if (landCellShortDecodeY != -1)
    dagGeom->shaderGlobalSetColor4(landCellShortDecodeY, dec_y_0);
}

void HmapLandObjectEditor::onBrushPaintEnd()
{
  /*== too brute force; these should be updated locally in onLandRegionChanged()
  for (int i = 0; i < splines.size(); i++)
    if (!splines[i]->isPoly())
      splines[i]->updateFullLoft();
  */
}

void HmapLandObjectEditor::setSelectionGizmoTranformMode(bool enable)
{
  for (int i = 0; i < selection.size(); ++i)
    if (auto *obj = RTTI_cast<LandscapeEntityObject>(selection[i]))
      obj->setGizmoTranformMode(enable);
}

void HmapLandObjectEditor::gizmoStarted()
{
  ObjectEditor::gizmoStarted();
  inGizmo = false;
  getAxes(locAx[0], locAx[1], locAx[2]);
  inGizmo = true;

  setSelectionGizmoTranformMode(true);

  for (int i = 0; i < selection.size(); ++i)
    if (!RTTI_cast<LandscapeEntityObject>(selection[i]) && !RTTI_cast<SplineObject>(selection[i]) &&
        !RTTI_cast<SphereLightObject>(selection[i]) && !RTTI_cast<SnowSourceObject>(selection[i]))
      return;

  if ((wingw::is_key_pressed(wingw::V_SHIFT)) && getEditMode() == CM_OBJED_MODE_MOVE)
  {
    if (!cloneMode)
      cloneMode = true;

    cloneDelta = getPt();
  }

  if (cloneMode)
  {
    clear_and_shrink(cloneObjs);
    clone(true);
  }
}

static void renameObjects(PtrTab<RenderableEditableObject> &objs, const char *new_prefix, HmapLandObjectEditor &ed)
{
  String fn;

  for (int i = 0; i < objs.size(); ++i)
  {
    if (new_prefix && *new_prefix)
      fn = new_prefix;
    else
      fn = objs[i]->getName();

    ed.setUniqName(objs[i], fn);
  }
}

void HmapLandObjectEditor::gizmoEnded(bool apply)
{
  setSelectionGizmoTranformMode(false);

  HmapLandPlugin::hmlService->invalidateClipmap(false);
  if (cloneMode && selection.size())
  {
    int cloneCount;
    bool cloneSeed = false;

    CopyDlg copyDlg(cloneName, cloneCount, cloneSeed);

    if (copyDlg.execute())
    {
      const Point3 delta = getPt() - cloneDelta;

      if (!cloneSeed)
        for (int i = 0; i < selection.size(); ++i)
        {
          if (LandscapeEntityObject *l = RTTI_cast<LandscapeEntityObject>(selection[i]))
            l->setRndSeed(grnd());
          else if (SplineObject *s = RTTI_cast<SplineObject>(selection[i]))
            s->setRandomSeed(grnd());
        }

      for (int cloneId = 2; cloneId <= cloneCount; ++cloneId)
      {
        renameObjects(selection, cloneName, *this);

        cloneDelta = getPt();
        clone(cloneSeed);
        for (int i = 0; i < selection.size(); ++i)
          selection[i]->moveObject(delta, IEditorCoreEngine::BASIS_World);
      }

      renameObjects(selection, cloneName, *this);
      getUndoSystem()->accept("Clone object(s)");
      updateSelection();
    }
    else
    {
      for (int i = 0; i < cloneObjs.size(); ++i)
        removeObject(cloneObjs[i]);

      getUndoSystem()->cancel();
    }

    cloneMode = false;

    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
    isGizmoStarted = false;
  }
  else
    ObjectEditor::gizmoEnded(apply);
  inGizmo = false;
}

void HmapLandObjectEditor::_addObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  Tab<RenderableEditableObject *> objs(tmpmem);
  objs.resize(num);
  for (int i = 0; i < num; i++)
    objs[num - i - 1] = obj[i];

  ObjectEditor::_addObjects(objs.data(), num, use_undo);

  for (int i = 0; i < num; ++i)
    if (RTTI_cast<SnowSourceObject>(objs[i]))
    {
      HmapLandPlugin::self->updateSnowSources();
      break;
    }

  clear_and_shrink(splines);
  for (int i = 0; i < objects.size(); i++)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
    if (o && !o->splineInactive)
      splines.push_back(o);
  }
}

void HmapLandObjectEditor::_removeObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  Tab<SplineObject *> remSplines(tmpmem);
  Tab<SplinePointObject *> remPt0(tmpmem);
  PtrTab<SplinePointObject> remPoints(tmpmem);
  Tab<RenderableEditableObject *> remObjs(tmpmem);
  bool rebuild_hmap_modif = false;

  // splitting objects on groops
  for (int i = 0; i < num; i++)
  {
    if (SplineObject *o = RTTI_cast<SplineObject>(obj[i]))
      remSplines.push_back(o);
    else if (SplinePointObject *p = RTTI_cast<SplinePointObject>(obj[i]))
      remPt0.push_back(p);
    else
      remObjs.push_back(obj[i]);
  }

  // add points of rem-splines to remove-list
  for (int i = 0; i < remSplines.size(); i++)
  {
    for (int j = remSplines[i]->points.size() - (remSplines[i]->isClosed() ? 2 : 1); j >= 0; j--)
    {
      remPoints.push_back(remSplines[i]->points[j]);
      erase_item_by_value(remPt0, remSplines[i]->points[j]);
    }
    if (remSplines[i]->isAffectingHmap())
    {
      remSplines[i]->reApplyModifiers(false);
      rebuild_hmap_modif = true;
    }
    if (remSplines[i]->isClosed())
      getUndoSystem()->put(remSplines[i]->makePointListUndo());
    remSplines[i]->points.clear();
  }

  if (use_undo)
  {
    Tab<SplineObject *> undoPointsListSplines(tmpmem);
    for (int i = 0; i < remPoints.size(); i++)
      if (remPoints[i]->arrId == 0 && remPoints[i]->spline->isClosed())
        if (find_value_idx(undoPointsListSplines, remPoints[i]->spline) == -1)
          undoPointsListSplines.push_back(remPoints[i]->spline);

    for (int i = 0; i < undoPointsListSplines.size(); i++)
      getUndoSystem()->put(undoPointsListSplines[i]->makePointListUndo());
  }

  ObjectEditor::_removeObjects((RenderableEditableObject **)remPoints.data(), remPoints.size(), use_undo);
  ObjectEditor::_removeObjects((RenderableEditableObject **)remSplines.data(), remSplines.size(), use_undo);
  ObjectEditor::_removeObjects((RenderableEditableObject **)remPt0.data(), remPt0.size(), use_undo);
  ObjectEditor::_removeObjects(remObjs.data(), remObjs.size(), use_undo);


  // synchronize our splines array with real splines array
  clear_and_shrink(splines);
  for (int i = 0; i < objects.size(); i++)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
    if (o)
    {
      if ((o->isPoly() && o->points.size() < 3) || (!o->isPoly() && o->points.size() < 2))
        _removeObjects((RenderableEditableObject **)&o, 1, use_undo);
      else if (!o->splineInactive)
        splines.push_back(o);
    }
  }
  if (rebuild_hmap_modif)
    HmapLandPlugin::self->applyHmModifiers();

  HmapLandPlugin::self->onObjectsRemove();
}


bool HmapLandObjectEditor::canSelectObj(RenderableEditableObject *o)
{
  if (!ObjectEditor::canSelectObj(o))
    return false;

  if (o == curPt)
    return false;

  if (LandscapeEntityObject *e = RTTI_cast<LandscapeEntityObject>(o))
  {
    if (EditLayerProps::layerProps[e->getEditLayerIdx()].lock)
      return false;
  }
  else if (SplineObject *s = RTTI_cast<SplineObject>(o))
  {
    if (EditLayerProps::layerProps[s->getEditLayerIdx()].lock)
      return false;
  }
  else if (SplinePointObject *p = RTTI_cast<SplinePointObject>(o))
  {
    if (SplineObject *s = p->spline)
      if (EditLayerProps::layerProps[s->getEditLayerIdx()].lock)
        return false;
  }

  switch (selectMode)
  {
    case CM_SELECT_PT: return RTTI_cast<SplinePointObject>(o) != NULL;
    case CM_SELECT_SPLINES: return RTTI_cast<SplineObject>(o) != NULL;
    case CM_SELECT_ENT: return RTTI_cast<LandscapeEntityObject>(o) != NULL;
    case CM_SELECT_SPL_ENT: return RTTI_cast<LandscapeEntityObject>(o) || RTTI_cast<SplineObject>(o);
    case CM_SELECT_LT: return RTTI_cast<SphereLightObject>(o) != NULL;
    case CM_SELECT_SNOW: return RTTI_cast<SnowSourceObject>(o) != NULL;

    default: return true;
  }

  return false;
}


void HmapLandObjectEditor::onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags)
{
  ObjectEditor::onObjectFlagsChange(obj, changed_flags);

  if ((changed_flags & RenderableEditableObject::FLG_SELECTED) != 0)
  {
    if (outlinerWindow)
      outlinerWindow->onObjectSelectionChanged(getMainObjectForOutliner(*obj));
    HmapLandPlugin::self->onObjectSelectionChanged(obj);
  }
}


void HmapLandObjectEditor::updateSelection()
{
  ObjectEditor::updateSelection();

  if (outlinerWindow)
    for (int i = 0; i < objects.size(); ++i)
      outlinerWindow->onObjectSelectionChanged(getMainObjectForOutliner(*objects[i]));
}


void HmapLandObjectEditor::fillSelectionMenu(IGenViewportWnd *wnd, PropPanel::IMenu *menu)
{
  wnd->setMenuEventHandler(this);

  int type = -1;
  int oneLayer = -1;
  int perTypeLayerCount = 0;
  bool enabledMoveToLayer = selection.size() > 0;
  HeightmapLandOutlinerInterface outliner(*this);
  for (auto obj : selection)
  {
    RenderableEditableObject *object = obj;
    if (SplinePointObject *p = RTTI_cast<SplinePointObject>(object))
      if (p->spline)
        object = p->spline;

    int objType;
    int perTypeLayerIndex;
    const bool supported = outliner.getObjectTypeAndPerTypeLayerIndex(*object, objType, perTypeLayerIndex);
    if (supported && (type == objType || type == -1))
    {
      type = objType;
      if (oneLayer == perTypeLayerIndex || oneLayer == -1)
        oneLayer = perTypeLayerIndex;
      else
        oneLayer = -2;
    }
    else
    {
      type = -2;
      oneLayer = -2;
      enabledMoveToLayer = false;
    }

    if (type > -1)
    {
      perTypeLayerCount = outliner.getLayerCount(type);
      enabledMoveToLayer = perTypeLayerCount > 1;
    }
  }

  menu->addSubMenu(ROOT_MENU_ITEM, CM_MOVE_TO_LAYER, "Move to layer");
  menu->setEnabledById(CM_MOVE_TO_LAYER, enabledMoveToLayer);
  if (enabledMoveToLayer)
  {
    for (int layerIdx = 0; layerIdx < perTypeLayerCount; ++layerIdx)
    {
      const char *layerName = outliner.getLayerName(type, layerIdx);
      const int id = CM_MOVE_TO_LAYER_FIRST + layerIdx;
      menu->addItem(CM_MOVE_TO_LAYER, id, layerName);
      if (layerIdx == oneLayer)
        menu->setEnabledById(id, false);
    }
  }

  menu->addItem(ROOT_MENU_ITEM, CM_EXPORT_AS_COMPOSIT, "Export as composit");
  menu->addItem(ROOT_MENU_ITEM, CM_SPLIT_COMPOSIT, "Split composites");
}


int HmapLandObjectEditor::onMenuItemClick(unsigned id)
{
  if (CM_MOVE_TO_LAYER_FIRST <= id && id <= CM_MOVE_TO_LAYER_LAST)
  {
    HeightmapLandOutlinerInterface outliner(*this);

    int type = -1;
    const int destinationLayerIndex = id - CM_MOVE_TO_LAYER_FIRST;
    dag::Vector<RenderableEditableObject *> objectsToMove;
    for (auto obj : selection)
    {
      RenderableEditableObject *object = obj;
      if (SplinePointObject *p = RTTI_cast<SplinePointObject>(object))
        if (p->spline)
        {
          object = p->spline;
        }
        else
        {
          objectsToMove.clear();
          break;
        }

      int objType, objLayerIndex;
      if (outliner.getObjectTypeAndPerTypeLayerIndex(*object, objType, objLayerIndex))
      {
        if (type == objType || type == -1)
        {
          type = objType;
          if (objLayerIndex != destinationLayerIndex && find_value_idx(objectsToMove, object) < 0)
            objectsToMove.push_back(object);
        }
        else
        {
          objectsToMove.clear();
          break;
        }
      }
    }

    const int amount = objectsToMove.size();
    if (amount > 0)
    {
      outliner.moveObjectsToLayer(make_span(objectsToMove), type, destinationLayerIndex);

      eastl::unique_ptr<PropPanel::DialogWindow> dialog(
        DAGORED2->createDialog(hdpi::_pxScaled(360), hdpi::_pxScaled(100), "Move to layer"));
      dialog->removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
      PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
      const char *layerName = outliner.getLayerName(type, destinationLayerIndex);
      panel->createStatic(0, String(0, "%d node(s) moved to layer \"%s\"", amount, layerName));
      dialog->showDialog();

      return 1;
    }
  }
  else if (id == CM_EXPORT_AS_COMPOSIT)
  {
    exportAsComposit();
  }
  else if (id == CM_SPLIT_COMPOSIT)
  {
    splitComposits();
  }

  return 0;
}


void HmapLandObjectEditor::removeSpline(SplineObject *s)
{
  for (int i = 0; i < splines.size(); i++)
    if (splines[i] == s)
      erase_items(splines, i, 1);
}


void HmapLandObjectEditor::save(DataBlock &blk)
{
  blk.setBool("areLandHoleBoxesVisible", areLandHoleBoxesVisible);

  for (int i = 0; i < objectCount(); ++i)
    if (HmapLandHoleObject *obj = RTTI_cast<HmapLandHoleObject>(getObject(i)))
      obj->save(*blk.addNewBlock("hole"));
}

void HmapLandObjectEditor::save(DataBlock &splBlk, DataBlock &polBlk, DataBlock &entBlk, DataBlock &ltBlk, int layer)
{
  for (int i = 0; i < objects.size(); ++i)
    if (SplineObject *s = RTTI_cast<SplineObject>(objects[i]))
    {
      if (s && s->getEditLayerIdx() == (layer < 0 ? s->lpIndex() : layer))
      {
        if (s->isPoly())
          s->save(polBlk);
        else
          s->save(splBlk);
      }
    }
    else
    {
      LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]);
      if (p && (p != newObj) && p->getEditLayerIdx() == (layer < 0 ? p->lpIndex() : layer))
        p->save(*entBlk.addNewBlock("entity"));

      if (layer >= 0)
        continue;

      if (SphereLightObject *l = RTTI_cast<SphereLightObject>(objects[i]))
        l->save(*ltBlk.addNewBlock("SphereLightSource"));
      else if (SnowSourceObject *s = RTTI_cast<SnowSourceObject>(objects[i]))
        s->save(*ltBlk.addNewBlock("SnowSource"));
    }

  if (layer < 0)
    LandscapeEntityObject::saveColliders(entBlk);
}


void HmapLandObjectEditor::load(const DataBlock &main_blk)
{
  areLandHoleBoxesVisible = main_blk.getBool("areLandHoleBoxesVisible", areLandHoleBoxesVisible);

  int holeNameId = main_blk.getNameId("hole");

  for (int bi = 0; bi < main_blk.blockCount(); ++bi)
  {
    const DataBlock &blk = *main_blk.getBlock(bi);

    if (blk.getBlockNameId() == holeNameId)
    {
      HmapLandHoleObject *obj = new HmapLandHoleObject;
      addObject(obj, false);
      obj->load(blk);
    }
  }
}

void HmapLandObjectEditor::load(const DataBlock &splBlk, const DataBlock &polBlk, const DataBlock &entBlk, const DataBlock &ltBlk,
  int layer)
{
  if (layer < 0)
    splines.clear();

  int nid = splBlk.getNameId("spline");
  for (int i = 0; i < splBlk.blockCount(); i++)
    if (splBlk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *splBlk.getBlock(i);

      SplineObject *s = new SplineObject(false);
      s->setEditLayerIdx(layer < 0 ? s->lpIndex() : layer);
      addObject(s, false);
      s->load(cb, false);
      s->onCreated(false);
    }

  nid = polBlk.getNameId("polygon");
  for (int i = 0; i < polBlk.blockCount(); i++)
    if (polBlk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *polBlk.getBlock(i);

      SplineObject *s = new SplineObject(true);
      s->setEditLayerIdx(layer < 0 ? s->lpIndex() : layer);
      addObject(s, false);
      s->load(cb, false);
      s->onCreated(false);
    }

  nid = entBlk.getNameId("entity");
  if (layer < 0)
    LandscapeEntityObject::loadColliders(entBlk);

  for (int i = 0; i < entBlk.blockCount(); i++)
    if (entBlk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *entBlk.getBlock(i);

      LandscapeEntityObject *o = new LandscapeEntityObject(NULL);
      o->setEditLayerIdx(layer < 0 ? o->lpIndex() : layer);
      addObject(o, false);
      o->load(cb);
    }

  if (layer >= 0)
    return;

  nid = ltBlk.getNameId("SphereLightSource");
  int nids = ltBlk.getNameId("SnowSource");

  for (int i = 0; i < ltBlk.blockCount(); i++)
    if (ltBlk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *ltBlk.getBlock(i);

      SphereLightObject *o = new SphereLightObject;
      addObject(o, false);
      o->load(cb);
    }
    else if (ltBlk.getBlock(i)->getBlockNameId() == nids)
    {
      const DataBlock &cb = *ltBlk.getBlock(i);

      SnowSourceObject *o = new SnowSourceObject;
      addObject(o, false);
      o->load(cb);
    }
}


void HmapLandObjectEditor::getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types)
{
  names.clear();

  if (types.empty())
    return;

  bool showSplines = false;
  bool showPolys = false;
  bool showEnt = false;
  bool showLt = false;
  bool showSnow = false;

  for (int ti = 0; ti < types.size(); ++ti)
  {
    if (types[ti] == 0)
      showSplines = true;

    if (types[ti] == 1)
      showPolys = true;

    if (types[ti] == 2)
      showEnt = true;

    if (types[ti] == 3)
      showLt = true;

    if (types[ti] == 4)
      showSnow = true;
  }

  for (int i = 0; i < splines.size(); ++i)
  {
    SplineObject *so = splines[i];
    if (so && so->isCreated())
    {
      if ((so->isPoly() && showPolys) || (!so->isPoly() && showSplines))
      {
        names.push_back() = so->getName();

        if (so->isSelected())
          sel_names.push_back() = so->getName();
      }
    }
  }

  if (showEnt || showLt || showSnow)
    for (int i = 0; i < objects.size(); i++)
    {
      RenderableEditableObject *o;

      o = showEnt ? RTTI_cast<LandscapeEntityObject>(objects[i]) : NULL;
      if (o)
      {
        names.push_back() = o->getName();

        if (o->isSelected())
          sel_names.push_back() = o->getName();
      }

      o = showLt ? RTTI_cast<SphereLightObject>(objects[i]) : NULL;
      if (o)
      {
        names.push_back() = o->getName();

        if (o->isSelected())
          sel_names.push_back() = o->getName();
      }

      o = showSnow ? RTTI_cast<SnowSourceObject>(objects[i]) : NULL;
      if (o)
      {
        names.push_back() = o->getName();

        if (o->isSelected())
          sel_names.push_back() = o->getName();
      }
    }
}


void HmapLandObjectEditor::getTypeNames(Tab<String> &names)
{
  bool has_snow = HmapLandPlugin::self->isSnowAvailable();

  names.resize(has_snow ? 5 : 4);
  names[0] = OBJECT_SPLINE;
  names[1] = OBJECT_POLYGON;
  names[2] = OBJECT_ENTITY;
  names[3] = OBJECT_LIGHT;
  if (has_snow)
    names[4] = OBJECT_SNOW;
}


void HmapLandObjectEditor::onSelectedNames(const Tab<String> &names)
{
  getUndoSystem()->begin();
  unselectAll();

  Tab<SplinePointObject *> points(tmpmem);

  for (int ni = 0; ni < names.size(); ++ni)
  {
    RenderableEditableObject *o = getObjectByName(names[ni]);
    if (canSelectObj(o))
      o->selectObject();
  }

  getUndoSystem()->accept("Select");
  updateGizmo();
}


void HmapLandObjectEditor::getLayerNames(int type, Tab<String> &names)
{
  const int layerCount = EditLayerProps::layerProps.size();
  for (int i = 0; i < layerCount; i++)
  {
    auto &layerProps = EditLayerProps::layerProps[i];
    if (layerProps.type == type)
      names.push_back(String(layerProps.name()));
  }
}


bool HmapLandObjectEditor::findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out, bool place_on_ri_collision)
{
  Point3 dir, world;
  real dist = DAGORED2->getMaxTraceDistance();

  wnd->clientToWorld(Point2(x, y), world, dir);

  if (place_on_ri_collision)
    EDITORCORE->setupColliderParams(1, BBox3());

  bool hasTarget = IEditorCoreEngine::get()->traceRay(world, dir, dist, NULL);

  if (place_on_ri_collision)
    EDITORCORE->setupColliderParams(0, BBox3());

  if (hasTarget)
  {
    out = world + dir * dist;
    // out.y += 0.2;
    return true;
  }

  return false;
}


bool HmapLandPlugin::getSelectionBox(BBox3 &box) const { return objEd.getSelectionBox(box); }


void HmapLandObjectEditor::prepareCatmul(SplinePointObject *p1, SplinePointObject *p2)
{
  if (p1->linkCount() == 2)
    catmul[0] = p1->getLinkedPt(0) == p2 ? p1->getLinkedPt(1) : p1->getLinkedPt(0);
  else
    catmul[0] = p1;

  catmul[1] = p1;
  catmul[2] = p2;

  if (p2->linkCount() == 2)
    catmul[3] = p2->getLinkedPt(0) == p1 ? p2->getLinkedPt(1) : p2->getLinkedPt(0);
  else
    catmul[3] = p2;

  recalcCatmul();
}


void HmapLandObjectEditor::recalcCatmul()
{
  if (catmul[1])
  {
    BezierSplineInt<Point3> sp;
    Point3 v[4];

    for (int i = 0; i < 4; i++)
      v[i] = catmul[i]->getPt();
    sp.calculateCatmullRom(v);
    sp.calculateBack(v);

    catmul[1]->setRelBezierIn(v[0] - v[1]);
    catmul[1]->setRelBezierOut(v[1] - v[0]);
    catmul[1]->markChanged();
    catmul[2]->setRelBezierIn(v[2] - v[3]);
    catmul[2]->setRelBezierOut(v[3] - v[2]);
    catmul[2]->markChanged();
  }
}

void HmapLandObjectEditor::setEditMode(int cm)
{
  memset(catmul, 0, sizeof(catmul));

  DAGORED2->endBrushPaint();

  if (getUndoSystem()->is_holding())
    DAGORED2->setGizmo(NULL, IDagorEd2Engine::MODE_None);

  if (curPt)
  {
    curPt->visible = false;
    curPt->spline = NULL;
  }
  curPt = NULL;
  curSpline = NULL;

  setCreateBySampleMode(NULL);
  del_it(objCreator);
  selectNewObjEntity(NULL);

  if (cm == CM_CREATE_HOLEBOX_MODE)
  {
    objCreator = dagGeom->newBoxCreator();

    areLandHoleBoxesVisible = true;
    updateToolbarButtons();
    DAGORED2->repaint();
  }
  else if (cm == CM_CREATE_LT)
  {
    objCreator = dagGeom->newSphereCreator();

    updateToolbarButtons();
    DAGORED2->repaint();
  }
  else if (cm == CM_CREATE_SNOW_SOURCE)
  {
    objCreator = dagGeom->newSphereCreator();

    updateToolbarButtons();
    DAGORED2->repaint();
  }

  ObjectEditor::setEditMode(cm);
  if (getEditMode() != CM_CREATE_ENTITY)
    DAEDITOR3.hideAssetWindow();
  DAGORED2->repaint();
}

void HmapLandObjectEditor::gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int stage)
{
  ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  int st_mask =
    DAEDITOR3.getEntitySubTypeMask(collision ? IObjEntityFilter::STMASK_TYPE_COLLISION : IObjEntityFilter::STMASK_TYPE_EXPORT);

  updateSplinesGeom();

  if (collision)
    flags |= StaticGeometryNode::FLG_COLLIDABLE;

  FastIntList loft_layers;
  loft_layers.addInt(0);
  if (splSrv && (st_mask & SplineObject::splineSubtypeMask))
    splSrv->gatherLoftLayers(loft_layers, false);

  for (int lli = 0; lli < loft_layers.size(); lli++)
  {
    int max_layer = 0, ll = loft_layers.getList()[lli];
    for (int i = 0; i < splines.size(); ++i)
    {
      int l = splines[i]->getLayer();
      if (max_layer < l)
        max_layer = l;
    }
    for (int l = 0; l <= max_layer; l++)
    {
      if (splSrv)
        splSrv->gatherStaticGeometry(cont, flags, collision, ll, stage, l);
      for (int i = 0; i < splines.size(); ++i)
        if (splines[i]->getLayer() == l)
        {
          StaticGeometryContainer splineNodes;
          splines[i]->gatherStaticGeometry(splineNodes, flags, collision, ll, stage);
          for (int nodeNo = 0; nodeNo < splineNodes.nodes.size(); nodeNo++)
          {
            splineNodes.nodes[nodeNo]->script.setInt("splineLayer", l); // Use this and layer:i from loft to split materials by layers.
            cont.addNode(splineNodes.nodes[nodeNo]);
          }
          clear_and_shrink(splineNodes.nodes);
        }
    }
  }

  if (st_mask & SplineObject::roadsSubtypeMask)
    for (int i = 0; i < crossRoads.size(); i++)
    {
      GeomObject *geom = crossRoads[i]->getRoadGeom();
      if (geom)
        SplineObject::gatherStaticGeom(cont, *geom->getGeometryContainer(), flags, 0, i, stage);
    }

  if (waterGeom && (st_mask & waterSurfSubtypeMask))
    SplineObject::gatherStaticGeom(cont, *waterGeom->getGeometryContainer(), flags, 7, 0, stage);

  if (st_mask & SphereLightObject::lightGeomSubtype)
    for (int i = 0; i < objects.size(); ++i)
    {
      SphereLightObject *l = RTTI_cast<SphereLightObject>(objects[i]);
      if (l)
        l->gatherGeom(cont);
    }
}
void HmapLandObjectEditor::gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, Tab<Point3> &water_border_polys,
  Tab<Point2> &hmap_sweep_polys)
{
  for (int i = 0; i < splines.size(); i++)
    splines[i]->gatherLoftLandPts(loft_pt_cloud, water_border_polys, hmap_sweep_polys);
}

void HmapLandObjectEditor::setSelectMode(int cm)
{
  if (selectMode == cm)
    return;
  int old_mode = selectMode;
  selectMode = cm;
  setRadioButton(CM_SELECT_PT, selectMode);
  setRadioButton(CM_SELECT_SPLINES, selectMode);
  setRadioButton(CM_SELECT_ENT, selectMode);
  setRadioButton(CM_SELECT_SPL_ENT, selectMode);
  setRadioButton(CM_SELECT_LT, selectMode);
  setRadioButton(CM_SELECT_SNOW, selectMode);

  if (selectMode != -1)
  {
    bool desel = false;

    getUndoSystem()->begin();
    for (int i = selection.size() - 1; i >= 0; i--)
      if (!canSelectObj(selection[i]))
      {
        selection[i]->selectObject(false);
        desel = true;
      }
    updateSelection();

    class UndoSelModeChange : public UndoRedoObject
    {
      HmapLandObjectEditor *ed;
      int oldMode, redoMode;

    public:
      UndoSelModeChange(HmapLandObjectEditor *e, int mode) : ed(e)
      {
        oldMode = mode;
        redoMode = ed->selectMode;
      }

      virtual void restore(bool save_redo)
      {
        if (save_redo)
          redoMode = ed->selectMode;
        ed->setSelectMode(oldMode);
      }

      virtual void redo() { ed->setSelectMode(redoMode); }

      virtual size_t size() { return sizeof(*this); }
      virtual void accepted() {}
      virtual void get_description(String &s) { s = "UndoEdSelectModeChange"; }
    };

    if (desel)
    {
      getUndoSystem()->put(new UndoSelModeChange(this, old_mode));
      getUndoSystem()->accept("Change select mode");
    }
    else
      getUndoSystem()->cancel();

    invalidateObjectProps();
  }
}


float HmapLandObjectEditor::screenDistBetweenPoints(IGenViewportWnd *wnd, SplinePointObject *p1, SplinePointObject *p2)
{
  if (!wnd || !p1 || !p2)
    return -1;

  Point2 pt1, pt2;

  if (!wnd->worldToClient(p1->getPt(), pt1))
    return -1;

  if (!wnd->worldToClient(p2->getPt(), pt2))
    return -1;

  float dx = pt2.x - pt1.x;
  float dy = pt2.y - pt1.y;

  return sqrt(dx * dx + dy * dy);
}


float HmapLandObjectEditor::screenDistBetweenCursorAndPoint(IGenViewportWnd *wnd, int x, int y, SplinePointObject *p)
{
  if (!wnd || !p)
    return -1;

  Point2 pt;

  if (!wnd->worldToClient(p->getPt(), pt))
    return -1;

  float dx = pt.x - x;
  float dy = pt.y - y;

  return sqrt(dx * dx + dy * dy);
}


void HmapLandObjectEditor::update(real dt)
{
  if (objCreator && objCreator->isFinished())
  {
    if (objCreator->isOk() && getEditMode() == CM_CREATE_HOLEBOX_MODE)
    {
      // create object
      HmapLandHoleObject *obj = new HmapLandHoleObject;

      Point3 size = objCreator->matrix.getcol(0) + objCreator->matrix.getcol(1) + objCreator->matrix.getcol(2);

      Point3 center = objCreator->matrix.getcol(3);

      center.y += size.y * 0.5f;

      obj->setPos(center);

      Point3 boxSize;

      boxSize.x = fabsf(size.x);
      boxSize.y = fabsf(size.y);
      boxSize.z = fabsf(size.z);

      obj->setBoxSize(boxSize);

      String name("HoleBox001");
      setUniqName(obj, name);

      getUndoSystem()->begin();
      addObject(obj);
      getUndoSystem()->accept("create land hole box");

      HmapLandPlugin::self->resetRenderer();
      del_it(objCreator);
    }
    else if (objCreator->isOk() && getEditMode() == CM_CREATE_LT)
    {
      SphereLightObject *obj = new SphereLightObject;
      String name("Sph_Light_Source_001");
      setUniqName(obj, name);
      obj->setWtm(objCreator->matrix);

      getUndoSystem()->begin();
      addObject(obj);
      obj->selectObject();
      getUndoSystem()->accept("create sphere light source");

      del_it(objCreator);
      // objCreator = dagGeom->newSphereCreator();
    }
    else if (objCreator->isOk() && getEditMode() == CM_CREATE_SNOW_SOURCE)
    {
      SnowSourceObject *obj = new SnowSourceObject;
      String name("Snow_Source_001");
      setUniqName(obj, name);
      obj->setWtm(objCreator->matrix);

      getUndoSystem()->begin();
      addObject(obj);
      obj->selectObject();
      getUndoSystem()->accept("create snow source");

      del_it(objCreator);
      // objCreator = dagGeom->newSphereCreator();
    }
    else
      del_it(objCreator);

    if (!objCreator)
      setEditMode(CM_OBJED_MODE_SELECT);
  }

  ObjectEditor::update(dt);
}

void HmapLandObjectEditor::onAvClose()
{
  if (getEditMode() == CM_CREATE_ENTITY)
  {
    setCreateBySampleMode(NULL);
    selectNewObjEntity(NULL);
    ObjectEditor::setEditMode(CM_OBJED_MODE_SELECT);
  }
}
void HmapLandObjectEditor::onAvSelectAsset(DagorAsset *asset, const char *asset_name)
{
  if (getEditMode() == CM_CREATE_ENTITY)
    selectNewObjEntity(asset_name);
  else if (getEditMode() == CM_CREATE_SPLINE)
  {
    curSplineAsset = asset_name;
    if (curSpline)
      curSpline->changeAsset(curSplineAsset, false);
  }
  else if (getEditMode() == CM_CREATE_POLYGON)
  {
    curSplineAsset = asset_name;
    if (curSpline)
      curSpline->changeAsset(curSplineAsset, false);
  }
}

void HmapLandObjectEditor::onLandRegionChanged(float x0, float z0, float x1, float z1, bool recalc_all /*= false*/)
{
  BBox2 upd(Point2(x0, z0), Point2(x1, z1));
  for (int i = 0; i < splines.size(); ++i)
    if (splines[i] && splines[i]->isCreated())
      if (recalc_all || splines[i]->intersects(upd, true))
      {
        splines[i]->splineChanged = true;
        splinesChanged = true;
      }

  for (int i = 0; i < objects.size(); i++)
  {
    LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]);
    if (o && o->getProps().placeType)
    {
      Point3 pp = o->getPos();
      if (recalc_all || (upd & Point2(pp.x, pp.z)))
        o->setPos(pp);
    }
  }
}
void HmapLandObjectEditor::onLandClassAssetChanged(landclass::AssetData *a)
{
  bool changed = false;
  for (int i = 0; i < splines.size(); ++i)
    changed |= splines[i]->onAssetChanged(a);

  if (changed)
    DAGORED2->invalidateViewportCache();
}
void HmapLandObjectEditor::onSplineClassAssetChanged(splineclass::AssetData *a)
{
  bool changed = false;
  for (int i = 0; i < splines.size(); ++i)
    changed |= splines[i]->onAssetChanged(a);

  if (changed)
    DAGORED2->invalidateViewportCache();
}

void HmapLandObjectEditor::clone(bool clone_seed)
{
  PtrTab<RenderableEditableObject> oldSel(selection);

  unselectAll();

  for (int i = 0; i < oldSel.size(); ++i)
  {
    LandscapeEntityObject *l = RTTI_cast<LandscapeEntityObject>(oldSel[i]);
    if (l)
    {
      LandscapeEntityObject *nl = l->clone();
      // cloneName = nl->getName();
      addObject(nl);
      cloneObjs.push_back(nl);
      nl->setRndSeed(clone_seed ? l->getRndSeed() : grnd());
      nl->selectObject();
      continue;
    }

    SplineObject *s = RTTI_cast<SplineObject>(oldSel[i]);
    if (s)
    {
      SplineObject *ns = s->clone();
      // cloneName = ns->getName();
      addObject(ns);
      cloneObjs.push_back(ns);
      ns->prepareSplineClassInPoints();
      ns->getSpline();
      ns->onCreated();
      ns->setRandomSeed(clone_seed ? s->getProps().rndSeed : grnd());
      ns->selectObject();
      continue;
    }

    SphereLightObject *o = RTTI_cast<SphereLightObject>(oldSel[i]);
    if (o)
    {
      SphereLightObject *nl = o->clone();
      // cloneName = nl->getName();
      addObject(nl);
      cloneObjs.push_back(nl);
      nl->selectObject();
      continue;
    }

    SnowSourceObject *sn = RTTI_cast<SnowSourceObject>(oldSel[i]);
    if (sn)
    {
      SnowSourceObject *nl = sn->clone();
      // cloneName = nl->getName();
      addObject(nl);
      cloneObjs.push_back(nl);
      nl->selectObject();
      continue;
    }
  }
}


static CrossRoadData *addCross(dag::ConstSpan<CrossRoadData *> cr, SplinePointObject *old_pt, SplinePointObject *new_pt)
{
  for (int i = 0; i < cr.size(); i++)
    if (cr[i]->find(old_pt) != -1)
    {
      if (cr[i]->find(old_pt) == -1)
      {
        cr[i]->points.push_back(new_pt);
        new_pt->markChanged();
      }
      new_pt->isCross = true;
      return cr[i];
    }
  return NULL;
}
static void markCrossChanged(dag::ConstSpan<CrossRoadData *> cr, SplinePointObject *pt)
{
  for (int i = 0; i < cr.size(); i++)
    if (cr[i]->find(pt) != -1)
    {
      cr[i]->changed = true;
      return;
    }
}

void HmapLandObjectEditor::markCrossRoadChanged(SplinePointObject *p)
{
  for (int i = 0; i < crossRoads.size(); i++)
    if (crossRoads[i]->find(p) != -1)
    {
      crossRoads[i]->changed = true;
      crossRoadsChanged = true;
      return;
    }
}

void HmapLandObjectEditor::updateCrossRoads(SplinePointObject *p)
{
  if (!p)
  {
    for (int i = 0; i < crossRoads.size(); i++)
      for (int j = crossRoads[i]->points.size() - 1; j >= 0; j--)
        crossRoads[i]->points[j]->isCross = false;

    clear_all_ptr_items(crossRoads);
    for (int i = 0; i < splines.size(); i++)
      for (int j = splines[i]->points.size() - 1; j >= 0; j--)
        updateCrossRoadsOnPointAdd(splines[i]->points[j]);
  }
  else
  {
    for (int i = 0; i < crossRoads.size(); i++)
      if (crossRoads[i]->find(p) != -1)
      {
        Point3 p3 = p->getProps().pt;
        for (int j = crossRoads[i]->points.size() - 1; j >= 0; j--)
          if (crossRoads[i]->points[j] != p && lengthSq(crossRoads[i]->points[j]->getProps().pt - p3) > 1e-4)
          {
            updateCrossRoadsOnPointRemove(p);
            updateCrossRoadsOnPointAdd(p);
            break;
          }
        return;
      }

    updateCrossRoadsOnPointAdd(p);
    if (p->arrId > 0 && p->spline->points[p->arrId - 1]->isCross)
    {
      markCrossChanged(crossRoads, p->spline->points[p->arrId - 1]);
      crossRoadsChanged = true;
    }
    if (p->arrId + 1 < p->spline->points.size() && p->spline->points[p->arrId + 1]->isCross)
    {
      markCrossChanged(crossRoads, p->spline->points[p->arrId + 1]);
      crossRoadsChanged = true;
    }
  }
}
void HmapLandObjectEditor::updateCrossRoadsOnPointAdd(SplinePointObject *p)
{
  SplineObject *pspl = p->spline;
  Point3 p3 = p->getProps().pt;
  static Tab<SplinePointObject *> cr(tmpmem);
  CrossRoadData *dest = NULL;

  for (int i = 0; i < splines.size(); i++)
    if (splines[i] && (splines[i] != pspl || pspl->getProps().maySelfCross) && (splines[i]->getSplineBox() & p->getPt()))
    {
      for (int j = splines[i]->points.size() - 1; j >= 0; j--)
        if (splines[i]->points[j] != p && splines[i]->points[j] && lengthSq(splines[i]->points[j]->getProps().pt - p3) < 1e-4)
        {
          if (!dest)
            dest = addCross(crossRoads, splines[i]->points[j], p);
          if (!dest)
            cr.push_back(splines[i]->points[j].get());
          else
          {
            for (int i = 0; i < cr.size(); i++)
              dest->points.push_back(cr[i]);
            cr.clear();
            if (dest->find(p) == -1)
              dest->points.push_back(p);
          }
          break;
        }
    }

  if (!dest && cr.size())
  {
    dest = new CrossRoadData;
    dest->points.resize(cr.size() + 1);
    dest->points[0] = p;
    p->isCross = true;
    for (int i = 0; i < cr.size(); i++)
    {
      dest->points[i + 1] = cr[i];
      cr[i]->isCross = true;
    }
    crossRoads.push_back(dest);
  }

  if (dest)
  {
    dest->changed = true;
    crossRoadsChanged = true;
    for (int i = 0; i < dest->points.size(); i++)
      dest->points[i]->markChanged();
  }
  cr.clear();
}

void HmapLandObjectEditor::updateCrossRoadsOnPointRemove(SplinePointObject *p)
{
  for (int i = 0; i < crossRoads.size(); i++)
  {
    int idx = crossRoads[i]->find(p);
    if (idx != -1)
    {
      if (crossRoads[i]->points.size() == 2)
      {
        crossRoads[i]->points[0]->isCross = false;
        crossRoads[i]->points[1]->isCross = false;
        crossRoads[i]->points[0]->markChanged();
        crossRoads[i]->points[1]->markChanged();
        delete crossRoads[i];
        erase_items(crossRoads, i, 1);
        crossRoadsChanged = true;
        return;
      }
      else
      {
        erase_items(crossRoads[i]->points, idx, 1);
        p->isCross = false;
        p->markChanged();
        crossRoads[i]->changed = true;
        crossRoadsChanged = true;
        return;
      }
    }
  }
}


void HmapLandObjectEditor::calcSnow(StaticGeometryContainer &container)
{
  Tab<SnowSourceObject *> snow_sources(tmpmem);
  snow_sources.reserve(10);

  for (int i = 0; i < objects.size(); ++i)
  {
    SnowSourceObject *s = RTTI_cast<SnowSourceObject>(objects[i]);
    if (s)
      snow_sources.push_back(s);
  }

  SnowSourceObject::calcSnow(snow_sources, HmapLandPlugin::self->getSnowValue(), container);
}

void HmapLandObjectEditor::updateSnowSources()
{
  if (!HmapLandPlugin::self->hasSnowSpherePreview())
    return;

  if (!DAGORED2->getCurrentViewport())
    return;

  TMatrix ctm;
  DAGORED2->getCurrentViewport()->getCameraTransform(ctm);
  Point3 cam_pos = ctm.getcol(3);
  clear_and_shrink(nearSources);

  for (int i = 0; i < objects.size(); ++i)
    if (SnowSourceObject *s = RTTI_cast<SnowSourceObject>(objects[i]))
    {
      float source_dist = (cam_pos - s->getPos()).length();

      if (nearSources.size() < SnowSourceObject::NEAR_SRC_COUNT)
        nearSources.push_back(NearSnowSource(s, source_dist));
      else
      {
        int worst_index = 0;

        for (int j = 0; j < nearSources.size(); ++j)
        {
          if (nearSources[j].distance > nearSources[worst_index].distance)
            worst_index = j;
        }

        if (nearSources[worst_index].distance > source_dist)
        {
          nearSources[worst_index].distance = source_dist;
          nearSources[worst_index].source = s;
        }
      }
    }

  SnowSourceObject::updateSnowSources(nearSources);
}

GeomObject &HmapLandObjectEditor::getClearedWaterGeom()
{
  if (!waterGeom)
    return *(waterGeom = dagGeom->newGeomObject(midmem));
  dagGeom->geomObjectClear(*waterGeom);
  return *waterGeom;
}
void HmapLandObjectEditor::removeWaterGeom() { dagGeom->deleteGeomObject(waterGeom); }


CrossRoadData::CrossRoadData() : points(midmem), geom(NULL), changed(true) {}
CrossRoadData::~CrossRoadData() { removeRoadGeom(); }
GeomObject &CrossRoadData::getClearedRoadGeom()
{
  if (!geom)
    return *(geom = dagGeom->newGeomObject(midmem));
  dagGeom->geomObjectClear(*geom);
  return *geom;
}
void CrossRoadData::removeRoadGeom() { dagGeom->deleteGeomObject(geom); }

bool CrossRoadData::checkCrossRoad()
{
  bool real_cross = false;
  int road_cnt = 0;
  bool changed = false;

  for (int i = points.size() - 1; i >= 0; i--)
    if (!getRoad(points[i]))
      continue;
    else if (points[i]->arrId > 0 && points[i]->arrId + 1 < points[i]->spline->points.size())
      road_cnt += 2;
    else
      road_cnt++;

  // DAEDITOR3.conNote("cross %p: %p pts, %p road ends, is_real=%d",
  //   this, points.size(), road_cnt, (road_cnt>2));
  for (int i = points.size() - 1; i >= 0; i--)
  {
    int old = points[i]->isRealCross;
    points[i]->isRealCross = (road_cnt > 2);

    if (old != points[i]->isRealCross)
    {
      points[i]->spline->splineChanged = true;
      changed = true;
    }
  }
  return changed;
}

bool HmapLandObjectEditor::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (maxt <= 0)
    return false;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  if (!(st_mask & SplineObject::roadsSubtypeMask))
    return false;

  bool hit = false;
  bool can_be_cached = fabsf(maxt * dir.x) <= TRACE_CACHE_EXTENSION && fabsf(maxt * dir.z) <= TRACE_CACHE_EXTENSION;

  if (can_be_cached)
  {
    Tab<SplineObject *> &splineTab = spline_cache.build(splines, p, dir) ? spline_cache.cached : splines;

    for (int i = 0; i < splineTab.size(); i++)
    {
      SplineObject *spline = splineTab[i];
      const BBox3 &bbox = spline->getSplineBox();
      if (p.x >= bbox[0].x && p.x <= bbox[1].x && p.z >= bbox[0].z && p.z <= bbox[1].z && spline->isCreated())
        for (int j = spline->points.size() - 1; j >= 0; j--)
        {
          GeomObject *geom = spline->points[j]->getRoadGeom();
          if (geom && dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm))
            hit = true;
        }
    }
  }
  else
  {
    for (int i = 0; i < splines.size(); i++)
      if (splines[i]->isCreated())
        for (int j = splines[i]->points.size() - 1; j >= 0; j--)
        {
          GeomObject *geom = splines[i]->points[j]->getRoadGeom();
          if (geom && dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm))
            hit = true;
        }
  }

#if CROSSROADS_TRACE_GEOM
  for (int i = 0; i < crossRoads.size(); i++)
  {
    GeomObject *geom = crossRoads[i]->getRoadGeom();
    if (geom && dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm))
      hit = true;
  }
#endif

  return hit;
}
bool HmapLandObjectEditor::shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt)
{
  if (maxt <= 0)
    return false;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  if (!(st_mask & SplineObject::roadsSubtypeMask))
    return false;

  for (int i = 0; i < splines.size(); i++)
    for (int j = splines[i]->points.size() - 1; j >= 0; j--)
    {
      GeomObject *geom = splines[i]->points[j]->getRoadGeom();
      if (geom && dagGeom->geomObjectShadowRayHitTest(*geom, p, dir, maxt))
        return true;
    }

  for (int i = 0; i < crossRoads.size(); i++)
  {
    GeomObject *geom = crossRoads[i]->getRoadGeom();
    if (geom && dagGeom->geomObjectShadowRayHitTest(*geom, p, dir, maxt))
      return true;
  }
  return false;
}
bool HmapLandObjectEditor::isColliderVisible() const
{
  return HmapLandPlugin::self->getVisible() &&
         (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & SplineObject::roadsSubtypeMask);

  return false;
}

bool HmapLandObjectEditor::LoftAndGeomCollider::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (maxt <= 0)
    return false;

  bool hit = false;
  if (loft)
  {
    ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
    if (!splSrv)
      return hit;
    int end_layer = loftLayerOrder < 0 ? LAYER_ORDER_MAX : min(loftLayerOrder, (int)LAYER_ORDER_MAX);
    if (dir.y < -0.999)
    {
      for (int layer = 0; layer < end_layer; ++layer)
        if (bm_loft_mask[layer].isMarked(p.x, p.z))
          if (splSrv->traceRayFoundationGeom(layer, p, dir, maxt, norm))
            hit = true;
      return hit;
    }

    if (splSrv->traceRayFoundationGeom(-1, p, dir, maxt, norm))
      hit = true;
  }
  else
  {
    if (dir.y < -0.999)
      if (!bm_poly_mask.isMarked(p.x, p.z))
        return false;

    for (int i = 0; i < objEd.splines.size(); i++)
      if (objEd.splines[i]->isCreated())
      {
        const objgenerator::LandClassData *lcd = objEd.splines[i]->getLandClass();
        if (!lcd || !lcd->data || !lcd->data->genGeom || !lcd->data->genGeom->foundationGeom)
          continue;

        GeomObject *geom = objEd.splines[i]->polyGeom.mainMesh;
        if (geom && dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm))
          hit = true;

        geom = objEd.splines[i]->polyGeom.borderMesh;
        if (geom && dagGeom->geomObjectTraceRay(*geom, p, dir, maxt, norm))
          hit = true;
      }
  }
  return hit;
}
bool HmapLandObjectEditor::LoftAndGeomCollider::shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt)
{
  if (maxt <= 0)
    return false;

  if (loft)
  {
    ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
    if (!splSrv)
      return false;
    int end_layer = loftLayerOrder < 0 ? LAYER_ORDER_MAX : min(loftLayerOrder, (int)LAYER_ORDER_MAX);
    if (dir.y < -0.999)
    {
      for (int layer = 0; layer < end_layer; ++layer)
        if (bm_loft_mask[layer].isMarked(p.x, p.z))
          if (splSrv->shadowRayFoundationGeomHitTest(layer, p, dir, maxt))
            return true;
      return false;
    }

    if (splSrv->shadowRayFoundationGeomHitTest(-1, p, dir, maxt))
      return true;
  }
  else
  {
    if (dir.y < -0.999)
      if (!bm_poly_mask.isMarked(p.x, p.z))
        return false;

    for (int i = 0; i < objEd.splines.size(); i++)
      if (objEd.splines[i]->isCreated())
      {
        const objgenerator::LandClassData *lcd = objEd.splines[i]->getLandClass();
        if (!lcd || !lcd->data || !lcd->data->genGeom || !lcd->data->genGeom->foundationGeom)
          continue;

        GeomObject *geom = objEd.splines[i]->polyGeom.mainMesh;
        if (geom && dagGeom->geomObjectShadowRayHitTest(*geom, p, dir, maxt))
          return true;

        geom = objEd.splines[i]->polyGeom.borderMesh;
        if (geom && dagGeom->geomObjectShadowRayHitTest(*geom, p, dir, maxt))
          return true;
      }
  }
  return false;
}
bool HmapLandObjectEditor::LoftAndGeomCollider::isColliderVisible() const { return false; }

void HmapLandObjectEditor::moveObjects(PtrTab<RenderableEditableObject> &obj, const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  Tab<SplineObject *> spl;
  spl.reserve(32);
  for (int i = 0; i < obj.size(); ++i)
    if (SplineObject *s = RTTI_cast<SplineObject>(obj[i]))
      spl.push_back(s);

  for (int i = 0; i < obj.size(); ++i)
    if (SplinePointObject *p = RTTI_cast<SplinePointObject>(obj[i]))
      if (find_value_idx(spl, p->spline) < 0)
      {
        p->moveObject(delta, basis);
        p->spline->markModifChanged();
      }

  for (int i = 0; i < obj.size(); ++i)
  {
    if (RTTI_cast<SplinePointObject>(obj[i]))
      continue;
    obj[i]->moveObject(delta, basis);
    if (SplineObject *s = RTTI_cast<SplineObject>(obj[i]))
      s->markModifChanged();
  }
}

void HmapLandObjectEditor::fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &selection_service,
  LandscapeEntityObject &landscape_entity, IObjEntity &entity, const Point3 &ray_origin, const Point3 &ray_direction,
  dag::Vector<IPixelPerfectSelectionService::Hit> &hits)
{
  TMatrix tm;
  entity.getTm(tm);

  float t;
  if (!ray_intersect_box(ray_origin, ray_direction, entity.getBbox(), tm, t))
    return;

  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      fillPossiblePixelPerfectSelectionHits(selection_service, landscape_entity, *subEntity, ray_origin, ray_direction, hits);
    }
  }

  IPixelPerfectSelectionService::Hit hit;
  if (!selection_service.initializeHit(hit, entity))
    return;

  hit.transform = tm;
  hit.userData = &landscape_entity;
  hits.emplace_back(hit);
}

void HmapLandObjectEditor::getPixelPerfectHits(IPixelPerfectSelectionService &selection_service, IGenViewportWnd &wnd, int x, int y,
  Tab<RenderableEditableObject *> &hits)
{
  Point3 rayDirection, rayOrigin;
  wnd.clientToWorld(Point2(x, y), rayOrigin, rayDirection);

  pixelPerfectSelectionHitsCache.clear();

  for (RenderableEditableObject *object : objects)
  {
    if (!canSelectObj(object))
      continue;

    LandscapeEntityObject *landscapeEntityObject = RTTI_cast<LandscapeEntityObject>(object);
    if (!landscapeEntityObject)
      continue;

    IObjEntity *entity = landscapeEntityObject->getEntity();
    if (!entity)
      continue;

    fillPossiblePixelPerfectSelectionHits(selection_service, *landscapeEntityObject, *entity, rayOrigin, rayDirection,
      pixelPerfectSelectionHitsCache);
  }

  selection_service.getHits(wnd, x, y, pixelPerfectSelectionHitsCache);

  for (const IPixelPerfectSelectionService::Hit &hit : pixelPerfectSelectionHitsCache)
    hits.push_back(static_cast<LandscapeEntityObject *>(hit.userData));
}

bool HmapLandObjectEditor::pickObjects(IGenViewportWnd *wnd, int x, int y, Tab<RenderableEditableObject *> &objs)
{
  if (!usePixelPerfectSelection)
    return ObjectEditor::pickObjects(wnd, x, y, objs);

  if (!wnd)
    return false;

  IPixelPerfectSelectionService *pixelPerfectSelectionService = DAGORED2->queryEditorInterface<IPixelPerfectSelectionService>();
  if (!pixelPerfectSelectionService)
    return false;

  getPixelPerfectHits(*pixelPerfectSelectionService, *wnd, x, y, objs);
  return !objs.empty();
}
