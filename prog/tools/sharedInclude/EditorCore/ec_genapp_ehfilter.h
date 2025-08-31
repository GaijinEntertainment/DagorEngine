// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  ~AppEventHandler() override {}

  //*****************************************************************
  /// @name Mouse events handlers.
  //@{
  // handles mouse move
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // handles mouse left button press/releas
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // handles mouse right button press/releas
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // handles mouse center button press/releas
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // handles mouse wheel scroll
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override;
  // handles mouse double-click
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override;
  //@}

  //*****************************************************************
  /// @name Viewport redraw/change events handlers.
  //@{
  // viewport CTL window redraw
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  // viewport view change notification
  void handleViewChange(IGenViewportWnd *wnd) override;
  //@}

protected:
  GenericEditorAppWindow &main;
};
