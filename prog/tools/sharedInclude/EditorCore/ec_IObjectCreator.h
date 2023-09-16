#ifndef __GAIJIN_EDITORCORE_EC_I_OBJECT_CREATOR_H__
#define __GAIJIN_EDITORCORE_EC_I_OBJECT_CREATOR_H__
#pragma once

#include <math/dag_TMatrix.h>

class IGenViewportWnd;


class IObjectCreator
{
public:
  /// Matrix defining box parameters.
  TMatrix matrix;

  /// Constructor.
  IObjectCreator() : matrix(TMatrix::IDENT), stateFinished(false), stateOk(false) {}

  /// Test whether drawing of plane is finished.
  /// @return @b true if drawing of plane is finished - plane is finished
  ///            or drawing canceled (user clicked on right mouse button),\n
  ///         @b false if drawing is not finished yet
  inline bool isFinished() { return stateFinished; }

  /// Test whether plane creation is successful.
  /// @return @b true if plane creation is successful -
  ///         user clicked 2 times and defined length,width of the plane,\n
  ///         @b false if plane creation is not successful
  inline bool isOk() { return stateOk; }

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @param[in] rotate - if @b false, then box sides will be parallel to world
  ///                           coordinate axes
  /// \n                  - if @b true, then box base will be set
  ///                           as pointed by user
  /// @return @b true if event handling successful, @b false in other case
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true)
  {
    return false;
  }

  /// Mouse left button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

  /// Mouse left button release event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }

  /// Mouse right button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  //@}

  /// Render BoxCreator.
  /// Called from program code that created BoxCreator.
  virtual void render() {}

protected:
  bool stateFinished;
  bool stateOk;
};

#endif //__GAIJIN_EDITORCORE_EC_I_OBJECT_CREATOR_H__
