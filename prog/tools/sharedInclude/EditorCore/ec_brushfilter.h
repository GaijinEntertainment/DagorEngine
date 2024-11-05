// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>


class Brush;


/// Used to receive messages from editor core engine and send them
/// to current brush (if it is set).
/// Message sending begins after call to IEditorCoreEngine::beginBrushPaint()
/// (at that time BrushEventFilter is activated)
/// and ends after call to IEditorCoreEngine::endBrushPaint().
/// @ingroup EditorCore
/// @ingroup Brush
class BrushEventFilter : public IGenEventHandler
{
public:
  /// Constructor.
  BrushEventFilter();

  /// Test whether BrushEventFilter is active.
  /// @return @b true if BrushEventFilter is active, @b false in other case
  inline bool isActive() const { return active; }

  /// Set current brush.
  /// @param[in] brush - pointer to Brush
  inline void setBrush(Brush *brush) { curBrush = brush; }

  /// Get current brush.
  /// @return brush - pointer to Brush
  inline Brush *getBrush() const { return curBrush; }

  /// Activate BrushEventFilter.
  inline void activate() { active = true; }

  /// Deactivate BrushEventFilter.
  inline void deactivate() { active = false; }

  /// Render current brush.
  void renderBrush();

  /// #CTLWM_TIMER event handler.
  /// Used to work with such drawing tools as air brush
  /// (brush in autorepeat mode).
  virtual void handleTimer();

  // IGenEventHandler
  //*******************************************************
  ///@name UI commands handlers.
  //@{
  // virtual void handleCommand(int cmd) {}

  // virtual void handleButtonClick(int btn_id, CtlBtnTemplate* btn,
  //                                bool btn_pressed) {}
  //@}

  //*******************************************************
  ///@name Keyboard commands handlers.
  //@{
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif) {}
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) {}
  //@}

  //*******************************************************
  ///@name Mouse events handlers.
  //@{
  /// Handles mouse move.
  /// The function calls Brush::mouseMove().
  /// @copydoc IGenEventHandler::handleMouseMove()
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Handles mouse left button press.
  /// The function calls Brush::mouseLBPress().
  /// @copydoc IGenEventHandler::handleMouseLBPress()
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Handles mouse left button release.
  /// The function calls Brush::mouseLBRelease().
  /// @copydoc IGenEventHandler::handleMouseLBRelease()
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Handles mouse right button press.
  /// The function calls Brush::mouseRBPress().
  /// @copydoc IGenEventHandler::handleMouseRBPress()
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Handles mouse right button release.
  /// The function calls Brush::mouseRBRelease().
  /// @copydoc IGenEventHandler::handleMouseRBRelease()
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }

  //@}

  //*******************************************************
  ///@name Viewport redraw/change events handlers.
  //@{
  virtual void handleViewportPaint(IGenViewportWnd *wnd) {}
  virtual void handleViewChange(IGenViewportWnd *wnd) {}
  //@}

  //*******************************************************
  ///@name CTL child window creation/close events handlers.
  //@{
  // virtual void handleChildCreate(CtlWndObject* child) {}
  // virtual void handleChildClose(CtlWndObject* child) {}
  //@}

private:
  Brush *curBrush;

  bool active;
};
