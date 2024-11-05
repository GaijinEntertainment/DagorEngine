// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_genapp_ehfilter.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_brushfilter.h>

#include <debug/dag_debug.h>


#define APP_EVENT_HANDLER_VOID(command)                                                               \
  if (main.gizmoEH->getGizmoClient() && main.gizmoEH->getGizmoType() != IEditorCoreEngine::MODE_None) \
    main.gizmoEH->##command;                                                                          \
  else if (main.ged.curEH)                                                                            \
    main.ged.curEH->##command;

#define APP_EVENT_HANDLER_BOOL(command)                                                                    \
  if (main.brushEH->isActive())                                                                            \
  {                                                                                                        \
    if (!main.brushEH->##command && main.ged.curEH)                                                        \
      return main.ged.curEH->##command;                                                                    \
  }                                                                                                        \
  else if (main.gizmoEH->getGizmoClient() && main.gizmoEH->getGizmoType() != IEditorCoreEngine::MODE_None) \
    return main.gizmoEH->##command;                                                                        \
  else if (main.ged.curEH)                                                                                 \
    return main.ged.curEH->##command;                                                                      \
  return false;


//==============================================================================
void GenericEditorAppWindow::AppEventHandler::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  const bool isFly = wnd && wnd->isFlyMode();

  if (!isFly)
  {
    APP_EVENT_HANDLER_VOID(handleKeyPress(wnd, vk, modif));
  }
}


//==============================================================================
void GenericEditorAppWindow::AppEventHandler::handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif)
{
  APP_EVENT_HANDLER_VOID(handleKeyRelease(wnd, vk, modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseMove(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseLBPress(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseRBPress(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseRBRelease(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseCBPress(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons,
  int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseCBRelease(wnd, x, y, inside, buttons, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseWheel(wnd, wheel_d, x, y, key_modif));
}


//==============================================================================
bool GenericEditorAppWindow::AppEventHandler::handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif)
{
  APP_EVENT_HANDLER_BOOL(handleMouseDoubleClick(wnd, x, y, key_modif));
}


//==============================================================================
void GenericEditorAppWindow::AppEventHandler::handleViewportPaint(IGenViewportWnd *wnd)
{
  APP_EVENT_HANDLER_VOID(handleViewportPaint(wnd));
}


//==============================================================================
void GenericEditorAppWindow::AppEventHandler::handleViewChange(IGenViewportWnd *wnd) { APP_EVENT_HANDLER_VOID(handleViewChange(wnd)); }
