#include <EditorCore/ec_brush.h>
#include <dllPluginCore/core.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>

#include <propPanel2/c_panel_base.h>


#define VIEW_DOWN_DIST           50
#define RADIUS_MUL_TO_STOP_PAINT 25


extern IEditorCore *editorCore;

real Brush::maxR = 1000;

//==============================================================================
Brush::Brush(IBrushClient *bc) :
  client(bc),
  radius(1),
  opacity(100),
  hardness(100),
  drawing(false),
  autorepeat(false),
  paintLen(0.0f),
  stepDiv(2.0f),
  rightDrawing(false),
  viewDownDist(VIEW_DOWN_DIST),
  color(255, 0, 0, 255),
  stepPid(-1),
  ignoreStepDirection(0, 0, 0),
  prevCoord(-1, -1)
{
  if (!editorCore)
    fatal("IEditorCore instance not set.\n"
          "Use IEditorCore::setIEditorCore() to set proper instance.");

  step = radius / stepDiv;
}


//==============================================================================
void Brush::mouseMove(IGenViewportWnd *wnd, int x, int y, int btns, int keys)
{
  if (prevCoord.x != -1 && prevCoord.y != -1)
    prevCoord = coord;

  coord = Point2(x, y);

  if (drawing || rightDrawing)
    sendPaintMessage(wnd, btns, keys, true);
  else
    calcCenter(wnd);
}


//==============================================================================
void Brush::mouseLBPress(IGenViewportWnd *wnd, int x, int y, int btns, int keys)
{
  coord = Point2(x, y);

  drawing = true;

  repeatWnd = wnd;
  repeatBtns = btns;

  if (client)
    client->onBrushPaintStart(this, btns, keys);

  paintLen = 0.0f;

  sendPaintMessage(wnd, btns, keys, false);
}


//==============================================================================
void Brush::mouseLBRelease(IGenViewportWnd *wnd, int x, int y, int btns, int keys)
{
  drawing = false;

  if (client)
    client->onBrushPaintEnd(this, btns, keys);
}


//==============================================================================
void Brush::mouseRBPress(IGenViewportWnd *wnd, int x, int y, int btns, int keys)
{
  coord = Point2(x, y);

  rightDrawing = true;

  repeatWnd = wnd;
  repeatBtns = btns;

  if (client)
    client->onRBBrushPaintStart(this, btns, keys);

  paintLen = 0.0f;

  sendPaintMessage(wnd, btns, keys, false);
}


//==============================================================================
void Brush::mouseRBRelease(IGenViewportWnd *wnd, int x, int y, int btns, int keys)
{
  rightDrawing = false;

  if (client)
    client->onRBBrushPaintEnd(this, btns, keys);
}


//==============================================================================
bool Brush::calcCenter(IGenViewportWnd *wnd)
{
  Point3 world;
  Point3 dir;
  real dist = MAX_TRACE_DIST;

  wnd->clientToWorld(coord, world, dir);

  if (IEditorCoreEngine::get()->traceRay(world, dir, dist, &normal))
  {
    center = world + dir * dist;
    return true;
  }

  return false;
}


//==============================================================================
bool Brush::traceDown(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd)
{
  const Point3 world = pos + Point3(0, viewDownDist, 0);
  real dist = viewDownDist * 3;

  if (IEditorCoreEngine::get()->traceRay(world, Point3(0, -1, 0), dist, &normal))
  {
    clip_pos = world + Point3(0, -1, 0) * dist;
    return true;
  }

  return false;
}


//==============================================================================
void Brush::sendPaintMessage(IGenViewportWnd *wnd, int btns, int keys, bool moved)
{
  if (!client || !calcCenter(wnd))
    return;

  if (moved) // if it is mouse move paint
  {
    paintLen += length((center - prevCenter) - ((center - prevCenter) * ignoreStepDirection) * ignoreStepDirection);

    if (paintLen > radius * RADIUS_MUL_TO_STOP_PAINT)
    {
      if (!rightDrawing)
      {
        mouseLBRelease(wnd, prevCoord.x, prevCoord.y, btns, keys);
        mouseLBPress(wnd, coord.x, coord.y, btns, keys);
      }
      else
      {
        mouseRBRelease(wnd, prevCoord.x, prevCoord.y, btns, keys);
        mouseRBPress(wnd, coord.x, coord.y, btns, keys);
      }

      return;
    }

    if (paintLen >= step)
    {
      const Point3 paintDelta = ((center - lastPaint) - ((center - lastPaint) * ignoreStepDirection) * ignoreStepDirection);
      const real deltaLen = length(paintDelta);

      if (deltaLen <= step)
      {
        if (drawing)
          client->onBrushPaint(this, center, lastPaint, normal, btns, keys);
        else
          client->onRBBrushPaint(this, center, lastPaint, normal, btns, keys);

        lastPaint = center;
        paintLen = 0.0f;
      }
      else
      {
        const int paintCnt = deltaLen / step;
        const Point3 paintDeltaNorm = normalize(paintDelta);
        const Point3 endPaint = lastPaint + paintDeltaNorm * paintCnt * step;
        const Point3 delta = paintDeltaNorm * step;

        Point3 pos = lastPaint + delta;
        Point3 prevPos = lastPaint;

        for (int i = 0; i < paintCnt; ++i)
        {
          Point3 clipPos = pos;

          if (traceDown(pos, clipPos, wnd))
          {
            if (drawing)
              client->onBrushPaint(this, clipPos, prevPos, normal, btns, keys);
            else
              client->onRBBrushPaint(this, clipPos, prevPos, normal, btns, keys);

            prevPos = clipPos;
          }

          pos += delta;
        }

        paintLen = length((endPaint - lastPaint) - ((endPaint - lastPaint) * ignoreStepDirection) * ignoreStepDirection);
        lastPaint = endPaint;
      }
    }
  }
  else // if it is mouse click (first paint) or timer paint
  {
    lastPaint = center;
    paintLen = 0.0f;

    if (drawing)
      client->onBrushPaint(this, center, center, normal, btns, keys);
    else
      client->onRBBrushPaint(this, center, center, normal, btns, keys);
  }

  prevCenter = center;
}


//==============================================================================
void Brush::draw()
{
  editorCore->getRender()->startLinesRender();
  editorCore->getRender()->setLinesTm(TMatrix::IDENT);
  editorCore->getRender()->renderSphere(center, radius, color);
  editorCore->getRender()->endLinesRender();
}


//==============================================================================
void Brush::repeat()
{
  /*
  if (autorepeat && (drawing || rightDrawing))
    sendPaintMessage(repeatWnd, repeatBtns,
    editorCoreIface->getGui()->ctlInpDevGetModifiers(), false);
  */
}


void Brush::fillCommonParams(PropertyContainerControlBase &group, int radius_pid, int opacity_pid, int hardness_pid,
  int autorepeat_pid, int step_pid)
{

  radiusPid = radius_pid;
  opacityPid = opacity_pid;
  hardnessPid = hardness_pid;
  autorepeatPid = autorepeat_pid;
  stepPid = step_pid;

  group.createEditFloat(radiusPid, "Brush radius:", radius);
  group.createTrackInt(opacityPid, "Opacity:", opacity, 0, 100, 1);
  group.createTrackInt(hardnessPid, "Hardness:", hardness, 0, 100, 1);
  if (stepPid > 0)
    group.createTrackInt(stepPid, "Spacing:", 500 / getStepDiv(), 5, 1000, 1);

  group.createCheckBox(autorepeatPid, "Autorepeat", autorepeat);
}


void Brush::updateToPanel(PropertyContainerControlBase &group)
{
  group.setFloat(radiusPid, radius);
  group.setInt(opacityPid, opacity);
  group.setInt(hardnessPid, hardness);
  if (stepPid > 0)
    group.setInt(stepPid, 500 / getStepDiv());
  group.setBool(autorepeatPid, autorepeat);
}


//==================================================================================================
void Brush::addMaskList(int pid, PropertyContainerControlBase &params, const char *def)
{
  Tab<String> masx(tmpmem);

  masx.push_back() = "<none>";

  const char *ext[3];
  ext[0] = "*.tga";
  ext[1] = "*.r16";
  ext[2] = "*.r32";
  int cur = 0;

  for (int i = 0; i < 3; ++i)
  {
    String mask(getMaskPath(ext[i]));
    alefind_t ff;

    for (bool ok = ::dd_find_first(mask, DA_SUBDIR, &ff); ok; ok = ::dd_find_next(&ff))
    {
      masx.push_back() = ff.name;
      if (def && !strcmp(ff.name, def))
        cur = masx.size() - 1;
    }

    ::dd_find_close(&ff);
  }

  if (!def || !*def)
    def = "<none>";

  params.createCombo(pid, "Use mask:", masx, cur, masx.size() > 1);
}


//==================================================================================================
SimpleString Brush::getMaskPath(const char *mask)
{
  return SimpleString(::make_full_start_path(String(260, "/../commonData/brushes/%s", mask)).str());
}


//==============================================================================
bool Brush::updateFromPanel(PropertyContainerControlBase *panel, int pid)
{
  if (pid == radiusPid)
  {
    const real rad = panel->getFloat(pid);
    const real sRad = setRadius(rad);
    if (sRad != rad)
      panel->setFloat(pid, sRad);
    return true;
  }

  if (pid == opacityPid)
  {
    opacity = panel->getInt(pid);
    return true;
  }

  if (stepPid > 0 && pid == stepPid)
  {
    setStepDiv(500.0 / panel->getInt(pid));
    return true;
  }

  if (pid == hardnessPid)
  {
    hardness = panel->getInt(pid);
    return true;
  }

  if (pid == autorepeatPid)
  {
    int ar = panel->getBool(pid);
    if (ar < 2)
      autorepeat = (bool)ar;

    return true;
  }

  return false;
}
