#ifndef __GAIJIN_SIMPLEEDITOR_ED_APP_EHFILTER_H__
#define __GAIJIN_SIMPLEEDITOR_ED_APP_EHFILTER_H__
#pragma once

#include <EditorCore/ec_genappwnd.h>


/// Basic application event handler.
/// Depending on current settings and core mode it passes events to default
/// event handler, to Gizmo (see GizmoEventFilter) or to %Brush event handler
/// (see BrushEventFilter).\n
/// Additionally %AppEventHandler recognizes some shortcuts and converts them
/// to corresponding commands (also when editor is in 'Fly' mode).
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup AppWindow
class GenericEditorAppWindow::AppEventHandler : public IGenEventHandler
{
public:
  /// Constructor.
  /// @param[in] m - reference to Editor window
  AppEventHandler(GenericEditorAppWindow &m) : main(m) {}
  /// Destructor.
  virtual ~AppEventHandler() {}

  //*****************************************************************
  /// @name Keyboard commands handlers.
  //@{
  /// Handles key press.
  /// Additionally the function tests for shortcuts and calls
  /// AppEventHandler::handleCommand() if needed.
  /// @copydoc IGenEventHandler::handleKeyPress()
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  // handles key release
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif);
  //@}

  //*****************************************************************
  /// @name Mouse events handlers.
  //@{
  // handles mouse move
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // handles mouse left button press/releas
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // handles mouse right button press/releas
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // handles mouse center button press/releas
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // handles mouse wheel scroll
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif);
  // handles mouse double-click
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif);
  //@}

  //*****************************************************************
  /// @name Viewport redraw/change events handlers.
  //@{
  // viewport CTL window redraw
  virtual void handleViewportPaint(IGenViewportWnd *wnd);
  // viewport view change notification
  virtual void handleViewChange(IGenViewportWnd *wnd);
  //@}

protected:
  GenericEditorAppWindow &main;
};


#endif //__GAIJIN_APPEVENTHANDLER__
