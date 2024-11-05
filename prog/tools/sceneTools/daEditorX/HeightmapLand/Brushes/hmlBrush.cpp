// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <de3_interface.h>
#include <oldEditor/de_workspace.h>
#include <ioSys/dag_dataBlock.h>
#include <heightMapLand/dag_hmlGetHeight.h>


#define NONE_MASK_NAME ("<none>")

static DataBlock defProps;

HmapLandBrush::HmapLandBrush(IBrushClient *client, IHmapBrushImage &height_map) :
  Brush(client),
  heightMap(height_map),
  maskName(""),
  applied(tmpmem),
  applyOnlyOnce(false),
  useAutoAngle(false),
  currentAngle(0),
  gridCellSize(0),
  heightMapOffset(0, 0)
{
  setStepIgnoreDirection(Point3(0, 1, 0));

  if (!defProps.paramCount())
  {
    DataBlock app_blk;
    if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
      DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());

    defProps = *app_blk.getBlockByNameEx("heightMap")->getBlockByNameEx("brush");
    defProps.setBool("defauldPropsInited", true);
  }
}

void HmapLandBrush::fillParams(PropPanel::ContainerPropertyControl &panel)
{
  fillCommonParams(panel, PID_BRUSH_RADIUS, PID_BRUSH_OPACITY, PID_BRUSH_HARDNESS, PID_BRUSH_AUTOREPEAT, PID_BRUSH_SPACING);

  if (!updateMask(maskName))
    maskName = NONE_MASK_NAME;

  Brush::addMaskList(PID_BRUSH_MASK, panel, maskName);
  maskInd = panel.getInt(PID_BRUSH_MASK);

  // commented following option, 'couse it worked strange
  //  panel.createCheckBox(PID_BRUSH_APPLY_ONCE, "Apply once", applyOnlyOnce);
  panel.createCheckBox(PID_BRUSH_AUTO_ANGLE, "Auto angle", useAutoAngle);
  currentAngle = ::norm_s_ang(currentAngle);
  panel.createEditFloat(PID_BRUSH_ANGLE, "Angle:", RadToDeg(currentAngle));
}


void HmapLandBrush::updateToPanel(PropPanel::ContainerPropertyControl &panel)
{
  Brush::updateToPanel(panel);

  panel.setInt(PID_BRUSH_MASK, maskInd);
  // commented following option, 'couse it worked strange
  //  panel.setBool(PID_BRUSH_APPLY_ONCE, applyOnlyOnce);
  panel.setBool(PID_BRUSH_AUTO_ANGLE, useAutoAngle);
  currentAngle = ::norm_s_ang(currentAngle);
  panel.setFloat(PID_BRUSH_ANGLE, RadToDeg(currentAngle));
}


bool HmapLandBrush::updateMask(const char *newMask)
{
  if (strcmp(newMask, NONE_MASK_NAME) == 0)
    return mask.load("");
  if (mask.load(Brush::getMaskPath(newMask)))
    return true;

  return false;
}

bool HmapLandBrush::updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid)
{
  String maskName2(panel.getText(PID_BRUSH_MASK));
  maskInd = panel.getInt(PID_BRUSH_MASK);

  if (stricmp(maskName2, maskName) != 0)
  {
    if (!updateMask(maskName2))
    {
      maskName2 = NONE_MASK_NAME;
      // panel.setParam(PID_BRUSH_MASK, (const char*)maskName2);
      panel.setInt(PID_BRUSH_MASK, 0);
    }
    maskName = maskName2;
  }

  switch (pid)
  {
    case PID_BRUSH_APPLY_ONCE:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        applyOnlyOnce = (bool)ar;
      return true;
    }
    case PID_BRUSH_AUTO_ANGLE:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        useAutoAngle = (bool)ar;
      return true;
    }
    case PID_BRUSH_ANGLE:
    {
      currentAngle = DegToRad(panel.getFloat(pid));
      return true;
    }
    default: return Brush::updateFromPanel(&panel, pid);
  }
}


void HmapLandBrush::brushPaintApplyStart(const IBBox2 &where) {}


void HmapLandBrush::brushStartEnd()
{
  clear_and_shrink(applied);
  currentVector = Point2(0, 0);
}


void HmapLandBrush::dynamicItemChange(PropPanel::ContainerPropertyControl &panel)
{
  if (useAutoAngle)
  {
    real angle = RadToDeg(currentAngle);
    panel.setFloat(PID_BRUSH_ANGLE, angle);
  }
}

void HmapLandBrush::paint(const Point3 &center, const Point3 &prev_center, real cell_size, bool rb)
{
  gridCellSize = cell_size;

  if (gridCellSize < 0.00001f)
    return;
  if (useAutoAngle)
  {
    Point2 nextVector = Point2(center.x - prev_center.x, center.z - prev_center.z);
    nextVector.normalize();
    float angle = atan2f(nextVector.y * currentVector.x - nextVector.x * currentVector.y, nextVector * currentVector);
    currentAngle += norm_s_ang(angle);
    currentAngle = norm_s_ang(currentAngle);
    currentVector = nextVector;
  }

  const real radius = getRadius();
  const Point2 c = Point2(center.x, center.z);
  IBBox2 where;
  where[0].x = (int)((c.x - radius) / gridCellSize);
  where[1].x = (int)((c.x + radius) / gridCellSize);

  where[0].y = (int)((c.y - radius) / gridCellSize);
  where[1].y = (int)((c.y + radius) / gridCellSize);
  brushPaintApplyStart(where);
  float opacity = ((float)getOpacity()) / 100.0f;
  int appliedAt = -1;

  for (int y = where[0].y; y <= where[1].y; ++y)
  {
    for (int x = where[0].x; x <= where[1].x; ++x)
    {
      float *applyOnceWeight = NULL;
      if (applyOnlyOnce)
      {
        if (appliedAt >= 0 && appliedAt < applied.size())
          if (applied[appliedAt].at & IPoint2(x, y))
            applyOnceWeight = (applied[appliedAt][IPoint2(x, y)]);

        if (!applyOnceWeight)
          for (appliedAt = 0; appliedAt < applied.size(); ++appliedAt)
            if (applied[appliedAt].at & IPoint2(x, y))
            {
              applyOnceWeight = (applied[appliedAt][IPoint2(x, y)]);
              break;
            }
        if (!applyOnceWeight)
        {
          int rememberGridCell;
          if (applied.size())
            rememberGridCell = applied[0].at[1].x - applied[0].at[0].x + 1;
          else
          {
            rememberGridCell = 2 * int(radius / gridCellSize) + 1;
            if (rememberGridCell < 5)
              rememberGridCell = 5;
          }
          IBBox2 at;
          at[0] =
            IPoint2(floorf(float(x) / rememberGridCell) * rememberGridCell, floorf(float(y) / rememberGridCell) * rememberGridCell);
          at[1] = at[0] + IPoint2(rememberGridCell - 1, rememberGridCell - 1);

          G_VERIFY(at & IPoint2(x, y));
          appliedAt = append_items(applied, 1);
          applied[appliedAt].init(at);
          applyOnceWeight = (applied[appliedAt][IPoint2(x, y)]);
        }
      };

      Point2 brushCoord = Point2(x * gridCellSize, y * gridCellSize) - c;
      const real dist = length(brushCoord);

      float inc = getHardnessFromDistance(dist) * opacity;

      if (inc <= 0)
        continue;
      if (applyOnceWeight && inc > 1.0f - *applyOnceWeight)
        inc = 1.0f - *applyOnceWeight;
      float brushMaskStrength =
        mask.getCenteredMask(brushCoord.x / radius, brushCoord.y / radius, radius / gridCellSize, currentAngle);
      if (brushPaintApply(x, y, inc * brushMaskStrength, rb))
      {
        dirtyBrushBox += IPoint2(x, y);
        if (applyOnceWeight)
        {
          *applyOnceWeight += inc;
        }
      }
    }
  }
  brushPaintApplyEnd();
}

void HmapLandBrush::saveToBlk(DataBlock &blk) const
{
  blk.addReal("stepDiv", getStepDiv());
  blk.addReal("radius", getRadius());
  blk.addInt("opacity", getOpacity());
  blk.addInt("hardness", getHardness());
  blk.addBool("autorepeat", isRepeat());
  blk.addBool("useAutoAngle", useAutoAngle);
  //  blk.addBool("applyOnlyOnce", applyOnlyOnce);
  blk.addReal("currentAngle", currentAngle);
  blk.addStr("maskName", maskName);
}

void HmapLandBrush::loadFromBlk(const DataBlock &blk)
{
  setStepDiv(blk.getReal("stepDiv", defProps.getReal("stepDiv", getStepDiv())));
  setRadius(blk.getReal("radius", defProps.getReal("radius", getRadius())));
  setOpacity(blk.getInt("opacity", defProps.getInt("opacity", getOpacity())));
  setHardness(blk.getInt("hardness", defProps.getInt("hardness", getHardness())));
  setRepeat(blk.getBool("autorepeat", defProps.getBool("autorepeat", isRepeat())));
  useAutoAngle = blk.getBool("useAutoAngle", defProps.getBool("useAutoAngle", useAutoAngle));
  //  applyOnlyOnce = blk.getBool("applyOnlyOnce", defProps.getBool("applyOnlyOnce", applyOnlyOnce));
  currentAngle = blk.getReal("currentAngle", defProps.getReal("currentAngle", currentAngle));
  maskName = blk.getStr("maskName", defProps.getStr("maskName", NONE_MASK_NAME));
  if (maskName.empty())
    maskName = defProps.getStr("maskName", NONE_MASK_NAME);
  if (!updateMask(maskName) && strcmp(maskName, NONE_MASK_NAME) != 0)
  {
    maskName = defProps.getStr("maskName", NONE_MASK_NAME);
    if (!updateMask(maskName))
      maskName = NONE_MASK_NAME;
  }
}


void HmapLandBrush::draw()
{
  setColor(HmapLandPlugin::self->getEditedScriptImage() ? E3DCOLOR(255, 0, 255, 255) : E3DCOLOR(255, 0, 0, 255));

  Brush::draw();
}


//==================================================================================================
bool HmapLandBrush::calcCenter(IGenViewportWnd *wnd)
{
  Point3 world;
  Point3 dir;
  real dist = DAGORED2->getMaxTraceDistance();

  wnd->clientToWorld(coord, world, dir);

  if (HmapLandPlugin::self->traceRayPrivate(world, dir, dist, &normal))
  {
    center = world + dir * dist;
    return true;
  }

  return false;
}


//==================================================================================================
bool HmapLandBrush::traceDown(const Point3 &pos, Point3 &clip_p, IGenViewportWnd *wnd)
{
  clip_p = pos;
  return HmapLandPlugin::self->getHeightmapOnlyPointHt(clip_p, NULL);
}
