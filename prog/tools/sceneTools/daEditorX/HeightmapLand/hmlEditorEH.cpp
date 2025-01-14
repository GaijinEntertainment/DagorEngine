// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlObjectsEditor.h"
#include "hmlPlugin.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlHoleObject.h"
#include "hmlOutliner.h"
#include "hmlSplineUndoRedo.h"
#include "hmlCm.h"
#include "common.h"
#include "hmlEntity.h"
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_randomSeed.h>
#include <de3_hmapService.h>
#include <obsolete/dag_cfg.h>
#include <de3_baseInterfaces.h>
#include <obsolete/dag_cfg.h>

#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_outliner.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <math/random/dag_random.h>
#include <math/dag_rayIntersectBox.h>
#include <debug/dag_debug.h>
#include <physMap/physMap.h>

using hdpi::_pxScaled;

#define MAX_TRACE_DIST 10000

static Ptr<SplinePointObject> startPt = NULL, dispPt = NULL;
static SplineObject *refSpline = NULL, *splPoly = NULL;
static int pointsCnt = 0;
static bool splittingPolyFirstPoint = true;
static int splittingFirstPointId, splittingSecondPointId;

inline bool is_creating_entity_mode(int m) { return m == CM_CREATE_ENTITY; }

static SimpleString getMatScript(SplineObject *s)
{
  SimpleString res;
  if (s->points.empty())
    return res;
  if (!s->points[0])
    return res;
  ISplineGenObj *splineGenObject = s->points[0]->getSplineGen();
  if (!splineGenObject || !splineGenObject->loftGeom)
    return res;
  GeomObject *geomObject = splineGenObject->loftGeom;
  if (!geomObject->getGeometryContainer())
    return res;
  StaticGeometryContainer *sgc = geomObject->getGeometryContainer();
  if (sgc->nodes.empty())
    return res;
  if (!sgc->nodes[0]->mesh)
    return res;
  StaticGeometryMesh *mesh = sgc->nodes[0]->mesh;
  if (!mesh || mesh->mats.empty())
    return res;
  return mesh->mats[0]->scriptText;
}

SplineObject *HmapLandObjectEditor::findSplineAndDirection(IGenViewportWnd *wnd, SplinePointObject *p, bool &dirs_equal,
  SplineObject *excl)
{
  for (int i = 0; i < splines.size(); i++)
  {
    if (!splines[i]->points.size() || splines[i] == excl || splines[i]->isPoly() || splines[i]->isClosed())
      continue;

    real screenDist = screenDistBetweenPoints(wnd, p, splines[i]->points[0]);
    if (screenDist >= 0 && screenDist <= 25)
    {
      dirs_equal = true;
      return splines[i];
    }
    else
    {
      screenDist = screenDistBetweenPoints(wnd, p, splines[i]->points[splines[i]->points.size() - 1]);
      if (screenDist >= 0 && screenDist <= 25)
      {
        dirs_equal = false;
        return splines[i];
      }
    }
  }

  return NULL;
}


void HmapLandObjectEditor::registerViewportAccelerators(IWndManager &wndManager)
{
  ObjectEditor::registerViewportAccelerators(wndManager);

  wndManager.addViewportAccelerator(CM_SPLINE_REGEN, wingw::V_F1);
  wndManager.addViewportAccelerator(CM_SPLINE_REGEN_CTRL, wingw::V_F1, true);
  wndManager.addViewportAccelerator(CM_REBUILD_SPLINES_BITMASK, wingw::V_F2);
  wndManager.addViewportAccelerator(CM_SELECT_PT, '1', true);
  wndManager.addViewportAccelerator(CM_SELECT_SPLINES, '2', true);
  wndManager.addViewportAccelerator(CM_SELECT_ENT, '3', true);
  wndManager.addViewportAccelerator(CM_SELECT_LT, '4', true);
  wndManager.addViewportAccelerator(CM_SELECT_SNOW, '5', true);
  wndManager.addViewportAccelerator(CM_SELECT_NONE, '6', true);
  wndManager.addViewportAccelerator(CM_SELECT_SPL_ENT, '7', true);
  wndManager.addViewportAccelerator(CM_HIDE_SPLINES, '0', true);
  wndManager.addViewportAccelerator(CM_USE_PIXEL_PERFECT_SELECTION, 'Q', true);
}


void HmapLandObjectEditor::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  if (is_creating_entity_mode(getEditMode()) && wingw::is_key_pressed(wingw::V_ALT))
  {
    newObj->setPlaceOnCollision(true);
    wnd->invalidateCache();
    return;
  }
  return ObjectEditor::handleKeyPress(wnd, vk, modif);
}

void HmapLandObjectEditor::handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif)
{
  if (is_creating_entity_mode(getEditMode()) && !wingw::is_key_pressed(wingw::V_ALT))
  {
    newObj->setPlaceOnCollision(false);
    wnd->invalidateCache();
    return;
  }
  return ObjectEditor::handleKeyRelease(wnd, vk, modif);
}

bool HmapLandObjectEditor::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseMove(wnd, x, y, inside, buttons, key_modif, false);

  if (inside)
  {
    Point3 pos;
    if (getEditMode() == CM_CREATE_SPLINE || getEditMode() == CM_CREATE_POLYGON)
    {
      const bool placeOnRiCollision = static_cast<bool>(wingw::get_modif() & wingw::M_SHIFT);
      if (curPt && findTargetPos(wnd, x, y, pos, placeOnRiCollision))
      {
        if (DAGORED2->getGrid().getMoveSnap())
          pos = DAGORED2->snapToGrid(pos);

        curPt->setPos(pos);

        bool is = false;
        if (wingw::is_key_pressed(wingw::V_CONTROL))
          for (int i = 0; i < splines.size(); i++)
          {
            for (int j = 0; j < splines[i]->points.size(); j++)
            {
              if (curPt == splines[i]->points[j])
                continue;

              real ddist = screenDistBetweenPoints(wnd, curPt, splines[i]->points[j]);
              if (ddist <= 9 && ddist > 0)
              {
                is = true;
                curPt->setPos(splines[i]->points[j]->getPos());
                break;
              }
            }
            if (is)
              break;
          }
      }

      if (getEditMode() == CM_CREATE_SPLINE)
      {
        recalcCatmul();
        if (curSpline)
          curSpline->pointChanged(curSpline->points.size() - 1);
      }
    }
    else if (getEditMode() == CM_SPLIT_SPLINE)
    {
      bool is = false;
      for (int i = 0; i < splines.size(); i++)
      {
        if (!splines[i]->points.size() || splines[i]->isPoly() || splines[i]->isClosed())
          continue;

        for (int j = 0; j < splines[i]->points.size(); j++)
        {
          if (splines[i]->points[j]->arrId <= 0 || splines[i]->points[j]->arrId + 1 >= splines[i]->points.size())
            continue;

          real dist = screenDistBetweenCursorAndPoint(wnd, x, y, splines[i]->points[j]);
          if (dist > 0 && dist <= 20)
          {
            curPt->setPos(splines[i]->points[j]->getPt());
            curPt->visible = true;
            dispPt = splines[i]->points[j];
            is = true;
            break;
          }
        }

        if (is)
          break;
      }

      if (!is)
      {
        curPt->visible = false;
        dispPt = NULL;
      }
    }
    else if (getEditMode() == CM_REFINE_SPLINE)
    {
      bool is = false;
      for (int i = 0; i < splines.size(); i++)
      {
        Point3 p;
        real refT;
        int segId;

        bool hasTarget = splines[i]->getPosOnSpline(wnd, x, y, 4, &segId, &refT, &p);

        if (hasTarget && segId >= 0)
        {
          curPt->visible = true;
          refSpline = splines[i];
          curPt->setPos(p);
          curPt->arrId = segId + 1;
          is = true;

          break;
        }
      }
      if (!is)
      {
        curPt->visible = false;
        refSpline = NULL;
      }
    }
    else if (is_creating_entity_mode(getEditMode()))
    {
      newObj->setPlaceOnCollision(wingw::is_key_pressed(wingw::V_ALT));
      return ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
    }
    else if (getEditMode() == CM_SPLIT_POLY)
    {
      bool is = false;
      for (int i = 0; i < splines.size(); i++)
      {
        if (!splines[i]->points.size() || !splines[i]->isPoly())
          continue;

        for (int j = 0; j < splines[i]->points.size(); j++)
        {
          if (!splittingPolyFirstPoint)
          {
            int nextIdx = (splittingFirstPointId + 1) % splines[i]->points.size();
            int prevIdx = (splittingFirstPointId == 0 ? splines[i]->points.size() - 1 : splittingFirstPointId - 1);
            if (j == splittingFirstPointId || j == nextIdx || j == prevIdx)
              continue;
          }

          real dist = screenDistBetweenCursorAndPoint(wnd, x, y, splines[i]->points[j]);
          if (dist > 0 && dist <= 20)
          {
            Point3 p = splines[i]->points[j]->getPt();
            curPt->visible = true;
            splPoly = splines[i];
            curPt->setPos(p);
            if (!splittingPolyFirstPoint && debugP2)
              *debugP2 = p;
            is = true;

            break;
          }
        }

        if (is)
          break;
      }

      if (!is)
      {
        curPt->visible = false;
        splPoly = NULL;

        if (debugP2)
        {
          Point3 dir, p0;
          wnd->clientToWorld(Point2(x, y), p0, dir);
          real dist = MAX_TRACE_DIST;
          if (IEditorCoreEngine::get()->traceRay(p0, dir, dist, NULL))
            *debugP2 = p0 + dir * dist;
        }
      }
    }
    else
    {
      if (showPhysMat)
      {
        Tab<RenderableEditableObject *> objs(tmpmem);
        IEditorCoreEngine::get()->setShowMessageAt(x, y, SimpleString());
        bool splineHit = false;
        if (pickObjects(wnd, x, y, objs) && objs.size())
        {
          SimpleString matScript;
          if (SplineObject *s = RTTI_cast<SplineObject>(objs[0]))
          {
            splineHit = true;
            matScript = getMatScript(s);
          }
          if (!matScript.empty())
          {
            CfgReader c;
            c.getdiv_text(String(128, "[q]\r\n%s\r\n", matScript.str()), "q");
            const char *physMatName = c.getstr("phmat", NULL);
            if (physMatName)
            {
              int mat = HmapLandPlugin::hmlService->getPhysMatId(physMatName);
              if (mat >= 0)
              {
                String msg(128, "Spline physMatId = %d, physMat = '%s'", mat, physMatName);
                IEditorCoreEngine::get()->setShowMessageAt(x, y, SimpleString(msg.c_str()));
              }
            }
          }
        }
        if (!splineHit)
        {
          if (physMap)
          {
            Point3 dir, p0;
            real dist = MAX_TRACE_DIST;
            wnd->clientToWorld(Point2(x, y), p0, dir);
            if (IEditorCoreEngine::get()->traceRay(p0, dir, dist, NULL))
            {
              Point3 rayHit = p0 + dir * dist;
              int mat = physMap->getMaterialAt(Point2(rayHit.x, rayHit.z));
              if (mat >= 0)
              {
                int biomes = HmapLandPlugin::hmlService->getBiomeIndices(rayHit);
                const char *physMatName = HmapLandPlugin::hmlService->getLandPhysMatName(mat);
                String msg(128, "physMatId = %d, physMat = '%s', biomeIndices(byWeight) = %d, %d", mat, physMatName, biomes & 0xff,
                  biomes >> 8);
                IEditorCoreEngine::get()->setShowMessageAt(x, y, SimpleString(msg.c_str()));
              }
            }
          }
        }
      }
      return false;
    }
    updateGizmo();
    wnd->invalidateCache();
  }
  return ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}

bool HmapLandObjectEditor::usesRendinstPlacement() const
{
  if (!selection.size())
    return false;
  for (int i = 0; i < selection.size(); i++)
  {
    LandscapeEntityObject *leo = RTTI_cast<LandscapeEntityObject>(selection[0]);
    if (!leo || !leo->usesRendinstPlacement())
      return false;
  }
  return true;
}

bool HmapLandObjectEditor::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);

  if (inside && wnd->isActive())
  {
    wnd->invalidateCache();

    if (getEditMode() == CM_CREATE_SPLINE || getEditMode() == CM_CREATE_POLYGON)
    {
      Ptr<SplinePointObject> newPt = new SplinePointObject;
      bool needReverse = false, wasResumed = false;
      ;
      newPt->setPos(curPt->getPos());

      if (!startPt)
      {
        if (getEditMode() == CM_CREATE_SPLINE)
        {
          bool dirsEqual;
          SplineObject *resumeS = findSplineAndDirection(wnd, newPt, dirsEqual);
          if (resumeS)
          {
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Continue spline \"%s\"?",
                  resumeS->getName()) == wingw::MB_ID_YES)
            {
              pointsCnt = resumeS->points.size();
              if (dirsEqual)
                needReverse = true;

              curSpline = resumeS;
              wasResumed = true;
            }
            else
            {
              curSpline = NULL;
            }
          }
        }

        if (!curSpline)
        {
          pointsCnt = 1;
          curSpline = new SplineObject(getEditMode() == CM_CREATE_POLYGON);
          curSpline->setEditLayerIdx(EditLayerProps::activeLayerIdx[curSpline->lpIndex()]);
          if (wingw::is_key_pressed(wingw::V_SHIFT))
            curSpline->setCornerType(-1);
          curSpline->selectObject();
          splines.push_back(curSpline);
        }

        if (curSpline)
        {
          getUndoSystem()->begin();

          if (needReverse)
          {
            curSpline->reverse();

            Tab<SplineObject *> spls(tmpmem);
            spls.resize(1);
            spls[0] = curSpline;

            getUndoSystem()->put(new UndoReverse(spls));
          }

          if (!curSplineAsset.empty())
          {
            String objname(128, "%s_001", curSplineAsset.str());
            setUniqName(curSpline, objname);
          }
          addObject(curSpline);

          curPt->arrId = curSpline->points.size();
          curPt->spline = curSpline;

          curSpline->addPoint(curPt);
          if (curSpline->points.size() == 1)
            curSpline->changeAsset(curSplineAsset, false);
        }

        startPt = newPt;
      }
      else
      {
        if (getEditMode() == CM_CREATE_SPLINE)
        {
          bool dirsEqual;
          SplineObject *att = findSplineAndDirection(wnd, newPt, dirsEqual, curSpline);

          if (att)
          {
            if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Attach spline to\"%s\"?", att->getName()) ==
                wingw::MB_ID_YES)
            {
              if (!dirsEqual)
              {
                att->reverse();

                Tab<SplineObject *> spls(tmpmem);
                spls.resize(1);
                spls[0] = att;

                getUndoSystem()->put(new UndoReverse(spls));
              }

              pointsCnt += att->points.size();
              int pt_idx = curSpline->points.size() - 2;
              att->attachTo(curSpline, curSpline->points.size() - 1);

              memset(catmul, 0, sizeof(catmul));
              prepareCatmul(curSpline->points[pt_idx], curPt);

              curSpline->pointChanged(-1);
              curSpline->getSpline();

              handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
              return true;
            }
          }
        }

        pointsCnt++;
      }

      if (curSpline)
      {
        if (!wasResumed)
        {
          if (curSpline->points.size() > 2)
          {
            real scrDist = screenDistBetweenPoints(wnd, newPt, curSpline->points[0]);
            if (scrDist >= 0 && scrDist <= 25 &&
                wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
                  curSpline->isPoly() ? "Close polygon?" : "Close spline?") == wingw::MB_ID_YES)
            {
              for (int i = 0; i < curSpline->points.size(); i++)
                if (curSpline->points[i] == curPt)
                {
                  curSpline->onPointRemove(i);
                  break;
                }

              if (getEditMode() == CM_CREATE_SPLINE)
              {
                getUndoSystem()->put(curSpline->makePointListUndo());
                curSpline->points.push_back(curSpline->points[0]);
                curSpline->pointChanged(curSpline->points.size() - 1);
              }

              getUndoSystem()->accept(curSpline->isPoly() ? "Create polygon" : "Create spline");

              memset(catmul, 0, sizeof(catmul));

              if (curSpline)
                curSpline->onCreated();

              startPt = NULL;
              setEditMode(CM_OBJED_MODE_SELECT);

              return true;
            }
          }

          newPt->spline = curSpline;
          newPt->arrId = curSpline->points.size() - 1;

          addObject(newPt);

          if (getEditMode() == CM_CREATE_SPLINE)
            prepareCatmul(newPt, curPt);
        }

        if (newPt)
          newPt->markChanged();
        if (curPt)
          curPt->markChanged();
      }

      return true;
    }
    else if (getEditMode() == CM_SPLIT_SPLINE)
    {
      if (curPt && curPt->visible && dispPt)
      {
        SplineObject *s = dispPt->spline;
        if (s)
          s->split(dispPt->arrId);
      }
    }
    else if (getEditMode() == CM_REFINE_SPLINE)
    {
      if (curPt->visible && refSpline)
      {
        real refT;
        int segId;
        Point3 p;
        refSpline->getPosOnSpline(wnd, x, y, 4, &segId, &refT, &p);

        refSpline->refine(segId, refT, p);
      }
    }
    else if (getEditMode() == CM_SPLIT_POLY)
    {
      if (curPt->visible && splPoly)
      {
        for (int j = 0; j < splPoly->points.size(); j++)
        {
          real dist = screenDistBetweenCursorAndPoint(wnd, x, y, splPoly->points[j]);
          if (dist > 0 && dist <= 20)
          {
            if (splittingPolyFirstPoint)
            {
              splittingPolyFirstPoint = false;

              debugP1 = new Point3;
              debugP2 = new Point3;

              splittingFirstPointId = j;

              *debugP1 = splPoly->points[splittingFirstPointId]->getPt();
              *debugP2 = splPoly->points[splittingFirstPointId]->getPt();
            }
            else
            {
              splittingSecondPointId = j;

              int nextIdx = (j + 1) % splPoly->points.size();
              int prevIdx = (j == 0 ? splPoly->points.size() - 1 : j - 1);

              if (splittingFirstPointId != splittingSecondPointId && splittingFirstPointId != nextIdx &&
                  splittingFirstPointId != prevIdx)
              {
                splPoly->splitOnTwoPolys(splittingFirstPointId, splittingSecondPointId);

                splittingPolyFirstPoint = true;
                setEditMode(CM_OBJED_MODE_SELECT);

                del_it(debugP1);
                del_it(debugP2);
              }
            }
          }
        }
      }
    }
    else
      ObjectEditor::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
  }
  else
    ObjectEditor::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);

  updateGizmo();
  return true;
}

bool HmapLandObjectEditor::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);

  bool l_result = false;

  const int prevEditMode = getEditMode();
  if (getEditMode() == CM_CREATE_SPLINE || getEditMode() == CM_CREATE_POLYGON)
  {
    if (pointsCnt > 0)
    {
      if ((getEditMode() == CM_CREATE_POLYGON && pointsCnt < 3) || (getEditMode() == CM_CREATE_SPLINE && pointsCnt < 2))
      {
        getUndoSystem()->cancel();
        curSpline = NULL;
      }
      else if (startPt && curSpline)
      {
        for (int i = 0; i < curSpline->points.size(); i++)
          if (curSpline->points[i] == curPt)
          {
            curSpline->onPointRemove(i);
            break;
          }

        getUndoSystem()->accept(curSpline->isPoly() ? "Create polygon" : "Create spline");
      }
    }

    memset(catmul, 0, sizeof(catmul));
    curPt->spline = NULL;

    if (curSpline)
      curSpline->onCreated();

    startPt = NULL;

    setEditMode(CM_OBJED_MODE_SELECT);

    setButton(CM_CREATE_SPLINE, false);
    setButton(CM_CREATE_POLYGON, false);
    wnd->invalidateCache();

    l_result = true;
  }
  else if (getEditMode() == CM_SPLIT_SPLINE)
  {
    setEditMode(CM_OBJED_MODE_SELECT);
  }
  else if (is_creating_entity_mode(getEditMode()))
  {
    if (buttons & 1) // This is actually MK_LBUTTON.
      return ObjectEditor::handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
    setCreateBySampleMode(NULL);
    selectNewObjEntity(NULL);
    setEditMode(CM_OBJED_MODE_SELECT);

    l_result = true;
  }
  else if (getEditMode() == CM_SPLIT_POLY)
  {
    if (!splittingPolyFirstPoint)
    {
      splittingPolyFirstPoint = true;
      setEditMode(CM_OBJED_MODE_SELECT);
    }
  }

  del_it(debugP1);
  del_it(debugP2);

  setEditMode(CM_OBJED_MODE_SELECT);

  if (l_result)
    return true;

  return ObjectEditor::handleMouseRBPress(wnd, x, y, inside, buttons, key_modif) || prevEditMode != getEditMode();
}


bool HmapLandObjectEditor::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);

  if (inside && wnd->isActive())
    wnd->invalidateCache();

  bool needResetRenderer = false;

  if (objectWasMoved || objectWasRotated || objectWasScaled)
  {
    Tab<SplineObject *> transfSplines(tmpmem);
    Tab<SplinePointObject *> transfPoints(tmpmem);

    for (int i = 0; i < selection.size(); i++)
    {
      SplineObject *s = RTTI_cast<SplineObject>(selection[i]);
      if (s)
      {
        bool is = false;
        for (int j = 0; j < transfSplines.size(); j++)
          if (transfSplines[j] == s)
          {
            is = true;
            break;
          }

        if (!is)
          transfSplines.push_back(s);
      }
      else if (objectWasMoved)
      {
        SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[i]);

        if (p)
        {
          bool is = false;
          for (int j = 0; j < transfSplines.size(); j++)
            if (transfSplines[j] == p->spline)
            {
              is = true;
              break;
            }

          if (!is)
            transfPoints.push_back(p);
        }
      }

      HmapLandHoleObject *ho = RTTI_cast<HmapLandHoleObject>(selection[i]);
      if (ho && objectWasScaled)
        needResetRenderer = true;
    }

    if (needResetRenderer)
    {
      invalidateObjectProps();
      HmapLandPlugin::self->resetRenderer();
    }

    Tab<SplineObject *> modifSplines(tmpmem);
    for (int i = 0; i < transfSplines.size(); i++)
    {
      SplineObject *s = transfSplines[i];
      if (s->isAffectingHmap())
        modifSplines.push_back(s);
    }

    for (int i = 0; i < transfPoints.size(); i++)
    {
      SplinePointObject *p = transfPoints[i];
      if (p->spline->isAffectingHmap())
      {
        bool is = false;
        for (int j = 0; j < modifSplines.size(); j++)
          if (p->spline == modifSplines[i])
          {
            is = true;
            break;
          }

        if (!is)
          modifSplines.push_back(p->spline);
      }
    }

    for (int i = 0; i < transfPoints.size(); i++)
      transfPoints[i]->spline->pointChanged(transfPoints[i]->arrId);

    if (modifSplines.size())
    {
      for (int i = 0; i < modifSplines.size(); i++)
        modifSplines[i]->markModifChanged();
      HmapLandPlugin::self->applyHmModifiers();
    }

    for (int i = 0; i < transfSplines.size(); i++)
      transfSplines[i]->pointChanged(-1);
  }

  objectWasMoved = objectWasRotated = objectWasScaled = false;

  return ObjectEditor::handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
}

HmapLandObjectEditor::PlacementRotation HmapLandObjectEditor::buttonIdToPlacementRotation(int id)
{
  G_ASSERT(id >= CM_ROTATION_NONE && id <= CM_ROTATION_Z);
  return static_cast<PlacementRotation>(id - CM_ROTATION_X);
}

void HmapLandObjectEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE:
      if (getEditMode() == CM_CREATE_SPLINE || getEditMode() == CM_CREATE_POLYGON)
        return;
      setEditMode(pcb_id);
      updateGizmo();
      break;

    case CM_SHOW_PANEL: HmapLandPlugin::self->onPluginMenuClick(pcb_id); break;

    case CM_SHOW_LAND_OBJECTS:
      areLandHoleBoxesVisible = !areLandHoleBoxesVisible;
      updateToolbarButtons();
      DAGORED2->repaint();
      break;
    case CM_HIDE_SPLINES:
      hideSplines = !hideSplines;
      updateToolbarButtons();
      DAGORED2->repaint();
      break;
    case CM_SHOW_PHYSMAT:
      showPhysMat = !showPhysMat;
      if (!showPhysMat)
        IEditorCoreEngine::get()->setShowMessageAt(0, 0, SimpleString());
      break;
    case CM_SHOW_PHYSMAT_COLORS: showPhysMatColors = !showPhysMatColors; break;
    case CM_HILL_UP:
    case CM_HILL_DOWN:
    case CM_ALIGN:
    case CM_SMOOTH:
    case CM_SHADOWS:
    case CM_SCRIPT: HmapLandPlugin::self->onPluginMenuClick(pcb_id); break;

    case CM_CREATE_HOLEBOX_MODE:
    case CM_CREATE_LT:
    case CM_CREATE_SNOW_SOURCE: setEditMode(pcb_id); break;

    case CM_CREATE_ENTITY:
      setEditMode(pcb_id);
      if (!newObj.get())
      {
        newObj = new LandscapeEntityObject(NULL);
        newObj->setEditLayerIdx(EditLayerProps::activeLayerIdx[newObj->lpIndex()]);
        newObj->setCollisionIgnored();
      }
      setCreateBySampleMode(newObj);
      DAEDITOR3.showAssetWindow(true, "Select entity to create", this, DAEDITOR3.getGenObjAssetTypes());
      break;

    case CM_ROTATION_NONE:
    case CM_ROTATION_X:
    case CM_ROTATION_Y:
    case CM_ROTATION_Z: selectedPlacementRotation = buttonIdToPlacementRotation(pcb_id); break;

    case CM_CREATE_SPLINE:
    case CM_CREATE_POLYGON:
      pointsCnt = 0;
      setEditMode(pcb_id);
      curPt = new SplinePointObject;
      unselectAll();

      {
        int t = DAEDITOR3.getAssetTypeId(pcb_id == CM_CREATE_SPLINE ? "spline" : "land");

        DAEDITOR3.showAssetWindow(true, pcb_id == CM_CREATE_SPLINE ? "Assign asset to spline" : "Assign asset to polygon", this,
          make_span_const(&t, 1));
      }
      break;

    case CM_REFINE_SPLINE:
      setEditMode(pcb_id);
      curPt = new SplinePointObject;
      curPt->visible = false;
      break;

    case CM_SPLIT_SPLINE:
      if (selection.size() == 1 && RTTI_cast<SplinePointObject>(selection[0]))
      {
        SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[0]);
        if (p->spline->isClosed())
          onClick(CM_OPEN_SPLINE, panel);
        else
          p->spline->split(p->arrId);
        break;
      }
      DAEDITOR3.conWarning("invalid selection for split command:"
                           " must be selected exactly one point to split at");

      // setEditMode(cmd);
      // curPt = new SplinePointObject;
      // curPt->visible = false;
      break;

    case CM_SPLIT_POLY:
      setEditMode(pcb_id);
      curPt = new SplinePointObject;
      curPt->visible = false;
      break;

    case CM_REVERSE_SPLINE:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplineObject *> spl(tmpmem);

      for (int i = 0; i < selection.size(); i++)
      {
        if (SplineObject *s = RTTI_cast<SplineObject>(selection[i]))
        {
          bool is = false;
          for (int j = 0; j < spl.size(); j++)
            if (spl[j] == s)
            {
              is = true;
              break;
            }

          if (!is)
            spl.push_back(s);
        }
        else if (SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[i]))
        {
          bool is = false;
          for (int j = 0; j < spl.size(); j++)
            if (spl[j] == p->spline)
            {
              is = true;
              break;
            }

          if (!is)
            spl.push_back(p->spline);
        }
      }

      if (!spl.size())
        break;

      getUndoSystem()->begin();

      for (int i = 0; i < spl.size(); i++)
        spl[i]->reverse();

      getUndoSystem()->put(new UndoReverse(spl));
      getUndoSystem()->accept("Reverse spline(s)");
    }
    break;

    case CM_CLOSE_SPLINE:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplineObject *> spl(tmpmem);

      for (int i = 0; i < selection.size(); i++)
      {
        SplineObject *s = RTTI_cast<SplineObject>(selection[i]);
        if (s && !s->isPoly() && !s->isClosed())
          spl.push_back(s);
      }

      if (!spl.size())
      {
        DAEDITOR3.conWarning("No splines selected to be closed");
        break;
      }

      getUndoSystem()->begin();

      for (int i = 0; i < spl.size(); i++)
      {
        getUndoSystem()->put(spl[i]->makePointListUndo());
        spl[i]->points.push_back(spl[i]->points[0]);
        spl[i]->pointChanged(spl[i]->points.size() - 1);
      }

      getUndoSystem()->accept("Close spline(s)");
    }
    break;

    case CM_OPEN_SPLINE:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplinePointObject *> pt(tmpmem);

      for (int i = 0; i < selection.size(); i++)
      {
        SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[i]);

        if (p && !p->spline->isPoly() && p->spline->isClosed())
          pt.push_back(p);
      }

      if (!pt.size())
      {
        DAEDITOR3.conWarning("No points selected for splines to be open");
        break;
      }

      getUndoSystem()->begin();

      for (int i = 0; i < pt.size(); i++)
      {
        if (!pt[i]->spline->isClosed())
          continue;

        SplineObject &s = *pt[i]->spline;
        getUndoSystem()->put(s.makePointListUndo());

        PtrTab<SplinePointObject> spt(midmem);
        s.points.pop_back();

        spt.reserve(s.points.size() + 1);

        SplinePointObject::Props s_props = s.points[pt[i]->arrId]->getProps();
        for (int j = pt[i]->arrId; j < s.points.size(); j++)
        {
          s.points[j]->arrId = spt.size();
          spt.push_back(s.points[j]);
        }
        for (int j = 0; s.points[j] != pt[i]; j++)
        {
          s.points[j]->arrId = spt.size();
          spt.push_back(s.points[j]);
        }

        SplinePointObject *p = new SplinePointObject;
        p->setProps(s_props);
        p->setPos(s_props.pt);
        p->arrId = spt.size();
        spt.push_back(p);
        p->spline = &s;
        addObject(p);

        s.points = spt;
        pt[i]->markChanged();

        spt.clear();
      }

      getUndoSystem()->accept("Open spline(s)");
    }
    break;

    case CM_SELECT_NONE: setSelectMode(-1); return;

    case CM_SELECT_PT:
    case CM_SELECT_SPLINES:
    case CM_SELECT_ENT:
    case CM_SELECT_SPL_ENT:
    case CM_SELECT_LT:
    case CM_SELECT_SNOW:
      setEditMode(CM_OBJED_MODE_SELECT);
      setSelectMode(selectMode == pcb_id ? -1 : pcb_id);
      return;

    case CM_USE_PIXEL_PERFECT_SELECTION:
      usePixelPerfectSelection = !usePixelPerfectSelection;
      updateToolbarButtons();
      break;

    case CM_SELECT_ONLY_IF_ENTIRE_OBJECT_IN_RECT:
      selectOnlyIfEntireObjectInRect = !selectOnlyIfEntireObjectInRect;
      updateToolbarButtons();
      break;

    case CM_OBJED_DROP:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplinePointObject *> pts(tmpmem);

      for (int i = 0; i < selection.size(); i++)
      {
        SplinePointObject *p = RTTI_cast<SplinePointObject>(selection[i]);
        if (p)
        {
          bool is = false;
          for (int j = 0; j < pts.size(); j++)
            if (pts[j] == p)
            {
              is = true;
              break;
            }

          if (!is)
            pts.push_back(p);
        }
        else
        {
          SplineObject *s = RTTI_cast<SplineObject>(selection[i]);
          if (s)
          {
            for (int j = 0; j < s->points.size(); j++)
            {
              bool is = false;
              for (int k = 0; k < pts.size(); k++)
                if (s->points[j] == pts[k])
                {
                  is = true;
                  break;
                }

              if (!is)
                pts.push_back(s->points[j]);
            }
          }
        }
      }

      if (!pts.size())
        break;

      getUndoSystem()->begin();

      const Point3 dir = Point3(0, -1, 0);

      for (int i = 0; i < pts.size(); i++)
      {
        float dist = 2.f * DAGORED2->getMaxTraceDistance();
#if defined(USE_HMAP_ACES)
        const Point3 pos = pts[i]->getPos() + Point3(0, dist / 2, 0);
#else
        const Point3 pos = pts[i]->getPos() + Point3(0, 0.2, 0);
#endif

        if (IEditorCoreEngine::get()->traceRay(pos, dir, dist))
        {
          pts[i]->putMoveUndo();
          pts[i]->setPos(pos + dir * dist);
        }
      }

      getUndoSystem()->accept("Drop object(s)");

      updateGizmo();
      DAGORED2->repaint();
      updateToolbarButtons();

      return;
    }

    case CM_SPLINE_EXPORT_TO_DAG:
      setEditMode(CM_OBJED_MODE_SELECT);
      exportToDag();
      break;

    case CM_EXPORT_AS_COMPOSIT:
      setEditMode(CM_OBJED_MODE_SELECT);
      exportAsComposit();
      break;

    case CM_SPLIT_COMPOSIT:
      setEditMode(CM_OBJED_MODE_SELECT);
      splitComposits();
      break;

    case CM_INSTANTIATE_GENOBJ_INTO_ENTITIES:
      setEditMode(CM_OBJED_MODE_SELECT);
      instantiateGenToEntities();
      break;

    case CM_SPLINE_IMPORT_FROM_DAG:
      setEditMode(CM_OBJED_MODE_SELECT);
      importFromDag();
      break;

    case CM_HIDE_UNSELECTED_SPLINES:
      splines.clear();
      for (int i = 0; i < objects.size(); i++)
        if (SplineObject *s = RTTI_cast<SplineObject>(objects[i]))
        {
          bool used = s->isSelected();
          if (!used)
            for (int j = 0; j < selection.size(); ++j)
              if (SplinePointObject *o = RTTI_cast<SplinePointObject>(selection[j]))
                if (o->spline == s)
                {
                  used = true;
                  break;
                }
          s->splineInactive = !used;
          if (!s->splineInactive)
            splines.push_back(s);
        }
      break;
    case CM_UNHIDE_ALL_SPLINES:
      splines.clear();
      for (int i = 0; i < objects.size(); i++)
        if (SplineObject *s = RTTI_cast<SplineObject>(objects[i]))
        {
          if (s->splineInactive)
          {
            s->splineInactive = false;
            s->modifChanged = true;
            s->splineChanged = true;
            splinesChanged = true;
            DAGORED2->invalidateViewportCache();
            HmapLandPlugin::hmlService->invalidateClipmap(true);
          }
          splines.push_back(s);
        }
      break;

    case CM_COLLAPSE_MODIFIERS:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplineObject *> spls(tmpmem);
      for (int i = 0; i < selection.size(); ++i)
      {
        SplineObject *o = RTTI_cast<SplineObject>(selection[i]);
        if (!o)
          continue;

        if (o->isAffectingHmap())
          spls.push_back(o);
      }

      if (spls.size())
      {
        HmapLandPlugin::self->collapseModifiers(spls);
        invalidateObjectProps();
      }
      else if (!selection.size())
      {
        if (wingw::message_box(wingw::MBS_YESNO, "Confirmation", "No objects selected, copy all HMAP final to initial?") ==
            wingw::MB_ID_YES)
          HmapLandPlugin::self->copyFinalHmapToInitial();
      }
      else
        wingw::message_box(wingw::MBS_EXCL, "Warning", "No splines selected for collapse!");

      break;
    }

    case CM_MAKE_SPLINES_CROSSES:
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      Tab<SplineObject *> spls(tmpmem);
      for (int i = 0; i < selection.size(); ++i)
      {
        SplineObject *o = RTTI_cast<SplineObject>(selection[i]);
        if (!o)
          continue;

        if (!o->isPoly())
          spls.push_back(o);
      }

      if (!spls.size())
        wingw::message_box(wingw::MBS_EXCL, "Warning", "No splines selected for collapse!");
      else
      {
        int num = SplineObject::makeSplinesCrosses(spls);
        invalidateObjectProps();
        wingw::message_box(wingw::MBS_INFO, "Results", "Added %d new point(s) at detected crosses of %d spline(s)", num, spls.size());
      }
      break;
    }

    case CM_UNIFY_OBJ_NAMES:
    {
      Tab<SplineObject *> spl(tmpmem);
      Tab<LandscapeEntityObject *> ent(tmpmem);

      if (!selection.size())
      {
        if (
          wingw::message_box(wingw::MBS_YESNO, "Confirmation", "No objects selected, unify names for ALL object?") != wingw::MB_ID_YES)
          break;
        for (int i = 0; i < objects.size(); ++i)
          if (SplineObject *o = RTTI_cast<SplineObject>(objects[i]))
            spl.push_back(o);
          else if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]))
            ent.push_back(o);
      }
      else
        for (int i = 0; i < selection.size(); ++i)
          if (SplineObject *o = RTTI_cast<SplineObject>(selection[i]))
            spl.push_back(o);
          else if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(selection[i]))
            ent.push_back(o);

      if (spl.size() + ent.size())
      {
        if (selection.size() && wingw::message_box(wingw::MBS_YESNO, "Confirmation",
                                  "Unify names for %d selected splines and %d entities?", spl.size(), ent.size()) != wingw::MB_ID_YES)
          break;
      }
      else
      {
        wingw::message_box(wingw::MBS_EXCL, "Warning", "No objects selected for names unification!");
        break;
      }

      // clear names
      for (int i = 0; i < spl.size(); ++i)
        spl[i]->setName("");
      for (int i = 0; i < ent.size(); ++i)
        ent[i]->setName("");

      // build enumerated names
      setSuffixDigitsCount(3);
      for (int i = 0; i < spl.size(); ++i)
        setUniqName(spl[i],
          String(0, "%s_%03d", spl[i]->getProps().blkGenName.empty() ? "spline" : spl[i]->getProps().blkGenName.str(), i));
      for (int i = 0; i < ent.size(); ++i)
        setUniqName(ent[i],
          String(0, "%s_%03d", ent[i]->getProps().entityName.empty() ? "entity" : ent[i]->getProps().entityName.str(), i));

      wingw::message_box(wingw::MBS_INFO, "Success", "Names of %d objects unified!", spl.size() + ent.size());
      break;
    }

    case CM_MANUAL_SPLINE_REGEN_MODE:
      autoUpdateSpline = !autoUpdateSpline;
      updateToolbarButtons();
      DAGORED2->repaint();
      break;

    case CM_OBJED_SELECT_BY_NAME: showOutlinerWindow(!isOutlinerWindowOpen()); break;

    case CM_SPLINE_REGEN:
    case CM_SPLINE_REGEN_CTRL:
      if (pcb_id == CM_SPLINE_REGEN_CTRL)
      {
        int s_num = 0, p_num = 0;
        for (int i = 0; i < selection.size(); i++)
          if (RTTI_cast<SplineObject>(selection[i]))
          {
            RTTI_cast<SplineObject>(selection[i])->pointChanged(-1);
            s_num++;
          }
          else if (RTTI_cast<SplinePointObject>(selection[i]))
          {
            RTTI_cast<SplinePointObject>(selection[i])->markChanged();
            p_num++;
          }
        if (s_num + p_num)
          DAEDITOR3.conNote("Invalidated %d spline(s), %d point(s)", s_num, p_num);
      }

      updateSplinesGeom();

      if (pcb_id == CM_SPLINE_REGEN_CTRL)
        for (int i = 0; i < selection.size(); i++)
          if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(selection[i]))
            o->updateEntityPosition(true);

      DAGORED2->invalidateViewportCache();
      break;

    case CM_REBUILD_SPLINES_BITMASK:
      HmapLandPlugin::self->rebuildSplinesBitmask();
      DAGORED2->invalidateViewportCache();
      break;

    case CM_MOVE_OBJECTS:
      if (objects.size())
      {
        PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(250), _pxScaled(190), "Move multiple objects...");
        PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
        if (selection.size())
          panel.createStatic(-1, String(0, "Move %d object(s)", selection.size()));
        else
          panel.createStatic(-1, String(0, "No selection, move all (%d) objects", objects.size()));
        panel.createEditFloat(1, "Move X", 0);
        panel.createEditFloat(2, "Move Y", 0);
        panel.createEditFloat(3, "Move Z", 0);

        if (dlg->showDialog() == PropPanel::DIALOG_ID_OK)
        {
          getUndoSystem()->begin();
          moveObjects(selection.size() ? selection : objects, Point3(panel.getFloat(1), panel.getFloat(2), panel.getFloat(3)),
            IEditorCoreEngine::BASIS_World);
          getUndoSystem()->accept("Move objects");
          DAGORED2->invalidateViewportCache();
        }
        del_it(dlg);
      }
      break;
  }

  updateGizmo();
  updateToolbarButtons();

  // In the Landscape editor instead of the "Select objects by name" dialog we show the Outliner.
  if (pcb_id != CM_OBJED_SELECT_BY_NAME)
    ObjectEditor::onClick(pcb_id, panel);
}


void HmapLandObjectEditor::autoAttachSplines()
{
  enum
  {
    FLOAT_EDIT_ID = 100,
  };

  PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(100), "Attach points maximum distance");
  PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
  panel->createEditFloat(FLOAT_EDIT_ID, "Attach distance:", 0.3);

  if (dialog->showDialog() == PropPanel::DIALOG_ID_OK)
  {
    double value = panel->getFloat(FLOAT_EDIT_ID);

    getUndoSystem()->begin();

    for (int i = splines.size() - 1; i >= 0; i--)
    {
      if (!splines[i]->points.size() || splines[i]->isPoly() || splines[i]->isClosed())
        continue;

      for (int j = splines.size() - 1; j >= 0; j--)
      {
        if (splines[i] == splines[j])
          continue;

        if (!splines[j]->points.size() || splines[j]->isPoly() || splines[j]->isClosed())
          continue;

        bool needReverse = false;

        Point3 diff = splines[i]->points[splines[i]->points.size() - 1]->getPt() - splines[j]->points[0]->getPt(); //  *------ *------
        if (diff.length() < value)
        {
          removeObject(splines[i]->points[splines[i]->points.size() - 1]);
          splines[j]->attachTo(splines[i]);
        }
        else
        {
          diff = splines[i]->points[0]->getPt() - splines[j]->points[0]->getPt(); //  ------* *------
          if (diff.length() < value)
          {
            splines[i]->reverse();
            Tab<SplineObject *> spls(tmpmem);
            spls.resize(1);
            spls[0] = splines[i];
            getUndoSystem()->put(new UndoReverse(spls));

            removeObject(splines[i]->points[splines[i]->points.size() - 1]);
            splines[j]->attachTo(splines[i]);
          }
          else
          {
            diff = splines[i]->points[splines[i]->points.size() - 1]->getPt() -
                   splines[j]->points[splines[j]->points.size() - 1]->getPt(); //  *------ ------*
            if (diff.length() <= value)
            {
              splines[j]->reverse();
              Tab<SplineObject *> spls(tmpmem);
              spls.resize(1);
              spls[0] = splines[j];
              getUndoSystem()->put(new UndoReverse(spls));

              removeObject(splines[i]->points[splines[i]->points.size() - 1]);
              splines[j]->attachTo(splines[i]);
            }
          }
        }
      }

      splines[i]->pointChanged(-1);
      splines[i]->getSpline();
    }

    getUndoSystem()->accept("Auto attach splines");
    DAGORED2->invalidateViewportCache();
  }

  DAGORED2->deleteDialog(dialog);

  updateGizmo();
  DAGORED2->repaint();
  updateToolbarButtons();
}

void HmapLandObjectEditor::makeBottomSplines()
{
  Tab<SplineObject *> poly(tmpmem);

  for (int i = 0; i < selection.size(); i++)
  {
    SplineObject *s = RTTI_cast<SplineObject>(selection[i]);
    if (s && s->isPoly())
      poly.push_back(s);
  }
  if (!poly.size())
  {
    wingw::message_box(wingw::MBS_OK, "No polygons selected", "To build bottom lines select one or more polygons");
    return;
  }

  PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(180), "Bottom splines properties");
  PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
  panel->createStatic(-1, String(128, "%d polygons selected", poly.size()));
  panel->createEditFloat(1, "Border width:", 40);
  panel->createEditFloat(2, "Underwater depth:", 4);
  panel->createEditFloat(3, "Line tolerance", 0.5);

  if (dialog->showDialog() == PropPanel::DIALOG_ID_OK)
  {
    double border_w = panel->getFloat(1);
    double bottom_y = HmapLandPlugin::self->getWaterSurfLevel() - panel->getFloat(2);
    double tolerance = panel->getFloat(3);

    getUndoSystem()->begin();
    for (int j = 0; j < poly.size(); j++)
    {
      Tab<Point3> pts(tmpmem);
      poly[j]->buildInnerSpline(pts, border_w, bottom_y, tolerance);
      if (pts.size())
      {
        SplineObject *so = new SplineObject(false);
        so->setEditLayerIdx(EditLayerProps::activeLayerIdx[so->lpIndex()]);
        so->setName(String(128, "%s_bottom", poly[j]->getName()));
        addObject(so);

        for (int i = 0; i < pts.size(); ++i)
        {
          SplinePointObject *p = new SplinePointObject;
          p->setPos(pts[i]);
          p->arrId = i;
          p->spline = so;

          addObject(p);
        }
        so->applyCatmull(true, true);
        so->onCreated(false);
      }
    }
    getUndoSystem()->accept("Make bottom splines");
    DAGORED2->invalidateViewportCache();
  }
  DAGORED2->deleteDialog(dialog);

  updateGizmo();
  DAGORED2->repaint();
  updateToolbarButtons();
}

void HmapLandObjectEditor::selectNewObjEntity(const char *name)
{
  if (!newObj.get())
    return;

  LandscapeEntityObject::Props p;
  p.entityName = name;
  p.placeType = p.PT_coll;
  if (DagorAsset *a = DAEDITOR3.getGenObjAssetByName(name))
    if (IObjEntity *entity = DAEDITOR3.createEntity(*a))
    {
      if (ICompositObj *ico = entity->queryInterface<ICompositObj>())
      {
        int defPlaceTypeOverride = ico->getCompositPlaceTypeOverride();
        if (defPlaceTypeOverride >= 0)
        {
          p.placeType = defPlaceTypeOverride;
          p.overridePlaceTypeForComposit = true;
        }
      }
      destroy_it(entity);
    }
  newObj->setProps(p);
  if (newObj->getEntity())
    newObj->getEntity()->setSubtype(IObjEntity::ST_NOT_COLLIDABLE);

  if (!name && !is_creating_entity_mode(getEditMode()))
    newObj = NULL;
}

void HmapLandObjectEditor::createObjectBySample(RenderableEditableObject *sample)
{
  LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(sample);
  G_ASSERT(o);

  IObjEntity *e = o->getEntity();
  if (!e)
    return;

  IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>();
  int rndSeed = 0;
  if (irsh)
  {
    rndSeed = irsh->getSeed();
    irsh->setSeed(grnd());
  }

  LandscapeEntityObject *no = new LandscapeEntityObject(NULL, rndSeed);
  no->setEditLayerIdx(EditLayerProps::activeLayerIdx[no->lpIndex()]);

  TMatrix tm;
  e->getTm(tm);

  no->setProps(o->getProps());
  no->setSavedPlacementNormal(sample->getSavedPlacementNormal());
  no->setWtm(tm);
  getUndoSystem()->begin();
  addObject(no);
  unselectAll();
  no->selectObject();
  getUndoSystem()->accept("Add Entity");

  DAEDITOR3.addAssetToRecentlyUsed(o->getProps().entityName);
}

void HmapLandObjectEditor::onAddObject(RenderableEditableObject &obj)
{
  ObjectEditor::onAddObject(obj);

  if (outlinerWindow)
  {
    RenderableEditableObject &mainObj = getMainObjectForOutliner(obj);
    if (&mainObj == &obj)
      outlinerWindow->onAddObject(mainObj);
    else
      outlinerWindow->onObjectSelectionChanged(mainObj);
  }
}

void HmapLandObjectEditor::onRemoveObject(RenderableEditableObject &obj)
{
  ObjectEditor::onRemoveObject(obj);

  if (outlinerWindow)
  {
    RenderableEditableObject &mainObj = getMainObjectForOutliner(obj);
    if (&mainObj == &obj)
      outlinerWindow->onRemoveObject(mainObj);
    else
      outlinerWindow->onObjectSelectionChanged(mainObj);
  }
}

void HmapLandObjectEditor::onRegisteredObjectNameChanged(RenderableEditableObject &object)
{
  if (outlinerWindow)
    outlinerWindow->onRenameObject(object);
}

void HmapLandObjectEditor::onObjectEntityNameChanged(RenderableEditableObject &object)
{
  if (outlinerWindow)
    outlinerWindow->onObjectAssetNameChanged(object);
}

void HmapLandObjectEditor::onObjectEditLayerChanged(RenderableEditableObject &object)
{
  if (outlinerWindow)
    outlinerWindow->onObjectEditLayerChanged(object);
}

void HmapLandObjectEditor::showOutlinerWindow(bool show)
{
  if (show == isOutlinerWindowOpen())
    return;

  if (isOutlinerWindowOpen())
  {
    outlinerWindow->saveOutlinerSettings(outlinerSettings);
    outlinerWindow.reset();
    outlinerInterface.reset();
  }
  else
  {
    outlinerWindow.reset(DAEDITOR3.createOutlinerWindow());

    G_ASSERT(!outlinerInterface);
    outlinerInterface.reset(new HeightmapLandOutlinerInterface(*this));

    outlinerWindow->setTreeInterface(outlinerInterface.get());
    outlinerWindow->fillTypesAndLayers();
    outlinerWindow->loadOutlinerSettings(outlinerSettings);

    for (int i = 0; i < objects.size(); ++i)
      outlinerWindow->onAddObject(*objects[i]);
  }

  setButton(CM_OBJED_SELECT_BY_NAME, outlinerWindow.get() != nullptr);
}

void HmapLandObjectEditor::loadOutlinerSettings(const DataBlock &settings)
{
  outlinerSettings.setFrom(&settings);
  if (outlinerWindow)
    outlinerWindow->loadOutlinerSettings(outlinerSettings);
}

void HmapLandObjectEditor::saveOutlinerSettings(DataBlock &settings)
{
  if (outlinerWindow)
    outlinerWindow->saveOutlinerSettings(outlinerSettings);
  settings.setFrom(&outlinerSettings);
}

RenderableEditableObject &HmapLandObjectEditor::getMainObjectForOutliner(RenderableEditableObject &object)
{
  SplinePointObject *splinePoint = RTTI_cast<SplinePointObject>(&object);
  if (splinePoint && splinePoint->spline)
    return *splinePoint->spline;
  else
    return object;
}

void HmapLandObjectEditor::updateImgui()
{
  ObjectEditor::updateImgui();

  if (!outlinerWindow.get())
    return;

  bool open = true;
  DAEDITOR3.imguiBegin("Outliner", &open);
  outlinerWindow->updateImgui();
  DAEDITOR3.imguiEnd();

  if (!open)
    showOutlinerWindow(false);
}