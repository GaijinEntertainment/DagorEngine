#include <EditorCore/ec_brushfilter.h>
#include <EditorCore/ec_brush.h>

#include <debug/dag_debug.h>


//==============================================================================
BrushEventFilter::BrushEventFilter() : active(false), curBrush(NULL) {}


//==============================================================================
void BrushEventFilter::handleTimer()
{
  if (active && curBrush && curBrush->isRepeat())
    curBrush->repeat();
}


//==============================================================================
bool BrushEventFilter::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && active && curBrush)
  {
    curBrush->mouseMove(wnd, x, y, buttons, key_modif);
    IEditorCoreEngine::get()->invalidateViewportCache();
    IEditorCoreEngine::get()->updateViewports();

    return true;
  }

  return false;
}


//==============================================================================
bool BrushEventFilter::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && active && curBrush)
  {
    curBrush->mouseLBPress(wnd, x, y, buttons, key_modif);
    IEditorCoreEngine::get()->invalidateViewportCache();
    IEditorCoreEngine::get()->updateViewports();

    return true;
  }

  return false;
}


//==============================================================================
bool BrushEventFilter::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && active && curBrush)
  {
    curBrush->mouseLBRelease(wnd, x, y, buttons, key_modif);
    IEditorCoreEngine::get()->invalidateViewportCache();
    IEditorCoreEngine::get()->updateViewports();

    return true;
  }

  return false;
}


//==============================================================================
bool BrushEventFilter::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && active && curBrush)
  {
    curBrush->mouseRBPress(wnd, x, y, buttons, key_modif);
    IEditorCoreEngine::get()->invalidateViewportCache();
    IEditorCoreEngine::get()->updateViewports();

    return true;
  }

  return false;
}


//==============================================================================
bool BrushEventFilter::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && active && curBrush)
  {
    curBrush->mouseRBRelease(wnd, x, y, buttons, key_modif);
    IEditorCoreEngine::get()->invalidateViewportCache();
    IEditorCoreEngine::get()->updateViewports();

    return true;
  }

  return false;
}
//==============================================================================
void BrushEventFilter::renderBrush()
{
  if (active && curBrush)
    curBrush->draw();
}
