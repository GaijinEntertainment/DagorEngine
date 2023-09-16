#ifndef __GAIJIN_EDITOR_BRUSH__
#define __GAIJIN_EDITOR_BRUSH__
#pragma once


#include <util/dag_globDef.h>
#include <util/dag_simpleString.h>

#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>


class Brush;
class IGenViewportWnd;
class PropertyContainerControlBase;

// Groups for DoxyGen ***********************************************
/// @defgroup EditorCore Editor Core

/// @defgroup EditorBaseClasses Editor base classes
/// @ingroup EditorCore

/// @defgroup AppWindow Application Window
/// @ingroup EditorBaseClasses

/// @defgroup ViewPort Viewports
/// @ingroup EditorBaseClasses

/// @defgroup EventHandler Event Handler
/// @ingroup EditorBaseClasses

/// @defgroup EditableObject Editable Objects
/// @ingroup EditorBaseClasses

/// @defgroup Cameras Cameras
/// @ingroup EditorCore

/// @defgroup Brush Brush
/// @ingroup EditorCore

/// @defgroup Gizmo Gizmo
/// @ingroup EditorCore

/// @defgroup Grid Grid
/// @ingroup EditorCore

/// @defgroup Log Log management
/// @ingroup EditorCore

/// @defgroup Misc Miscellaneous
/// @ingroup EditorCore
/// @defgroup SelectByName 'Select by name' window
/// @ingroup Misc

/// @defgroup ObjectCreating Miscellaneous Objects Creating
/// @ingroup Misc

// End of Groups for DoxyGen ****************************************

//==============================================================================
/// Interface class for receiving messages from Brush.
/// @ingroup EditorCore
/// @ingroup Brush
class IBrushClient
{
public:
  /// Informs client code about start of painting.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onBrushPaintStart(Brush *brush, int buttons, int key_modif) = 0;

  /// Informs client code about start of painting by right mouse button.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onRBBrushPaintStart(Brush *brush, int buttons, int key_modif) = 0;

  /// Informs client code about end of painting.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onBrushPaintEnd(Brush *brush, int buttons, int key_modif) = 0;

  /// Informs client code about end of painting by right mouse button.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onRBBrushPaintEnd(Brush *brush, int buttons, int key_modif) = 0;

  /// Informs client code about process of painting.
  /// Function is called either at the beginning of painting, when user moves
  /// mouse with left button pressed or periodically (if the brush is in repeat
  /// mode) while left mouse button is pressed.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] center - world coordinates of brush center
  /// @param[in] prev_center - world coordinates of brush center at previous
  ///                call to onBrushPaint(). If onBrushPaint() is called after
  ///                onBrushPaintStart() then center == prev_center.
  /// @param[in] normal - surface normal at @b center point
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif) = 0;

  /// Informs client code about process of painting.
  /// Function is called either at the beginning of painting, when user moves
  /// mouse with right button pressed or periodically (if the brush is in repeat
  /// mode) while left mouse button is pressed.
  /// @param[in] brush - pointer to brush (see #Brush)
  /// @param[in] center - world coordinates of brush center
  /// @param[in] prev_center - world coordinates of brush center at previous
  ///            call to onRBBrushPaint(). If onRBBrushPaint() is called after
  ///            onBrushPaintStart() then center == prev_center.
  /// @param[in] normal - surface normal at @b center point
  /// @param[in] buttons - mouse buttons
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void onRBBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif) = 0;
};


//==============================================================================
/// Brush.
/// @ingroup EditorCore
/// @ingroup Brush
class Brush
{
public:
  /// Maximum raytrace distance which brushes use by default
  static const int MAX_TRACE_DIST = 2700;

  /// Constructor.
  /// @param[in] bc - pointer to interface #IBrushClient by means of which
  ///                 client code will be notified
  ///                 about brush events
  Brush(IBrushClient *bc);


  //*******************************************************
  ///@name Methods to get / set brush properties.
  //@{
  /// Get brush radius.
  /// @return brush radius
  inline real getRadius() const { return radius; }

  /// Get brush square radius.
  /// @return square of brush radius
  inline real getRadiusSq() const { return radius * radius; }

  /// Get brush step.
  /// @return brush step (step = radius / step divider)
  inline real getStep() const { return step; }

  /// Get brush step divider (used to calculate brush step).
  /// @return brush step devider
  inline real getStepDiv() const { return stepDiv; }

  /// Get brush opacity.
  /// @return brush opacity (0-100)
  inline int getOpacity() const { return opacity; }

  /// Get brush hardness.
  /// @return brush hardness (0-100)
  inline int getHardness() const { return hardness; }

  /// Get brush hardness from distance.
  /// @return brush hardness (0-100)
  inline float getHardnessFromDistance(real dist) const
  {
    if (dist > getRadius())
      return 0.0f;
    real hardDist = getHardness() / 100.0f * getRadius();
    if (dist <= hardDist)
      return 1.0f;
    real distFromHard = dist - hardDist;
    distFromHard /= (getRadius() - hardDist);

    return expf(-distFromHard * distFromHard * TWOPI);
  }


  /// Tests whether brush is in repeat mode.
  /// In repeat mode pressed left mouse button will call periodically an event
  /// IBrushClient::onBrushPaint().
  /// @return @b true if brush is in repeat mode
  inline bool isRepeat() const { return autorepeat; }


  /// Set brush radius.
  /// @param[in] r - brush radius
  inline real setRadius(real r)
  {
    if (r < 0.01)
      r = 0.01;
    else if (r > maxR)
      r = maxR;

    radius = r;
    step = radius / stepDiv;

    return radius;
  }

  inline void setMaxRadius(real r) { maxR = r; }

  /// Set brush step divider.
  /// @param[in] div - step divider
  inline void setStepDiv(real div);

  /// Set brush opacity.
  /// @param[in] o - brush opacity
  inline void setOpacity(int o)
  {
    if (o > 0)
      opacity = o;
  }

  /// Set brush hardness.
  /// @param[in] h - brush hardness
  inline void setHardness(int h)
  {
    if (h >= 0 && h <= 100)
      hardness = h;
  }

  /// Set brush autorepeat.
  /// @param[in] repeat - <b>true / false</b> - set autorepeat <b>on / off</b>
  inline void setRepeat(bool repeat) { autorepeat = repeat; }
  //@}


  //*******************************************************
  ///@name Mouse events.
  //@{
  /// Notify the brush about mouse move. Called by Editor Core.
  /// @param[in] wnd - pointer to interface IGenViewportWnd of the viewport
  ///                  window that generated the event
  /// @param[in] x,y - <b>x, y</b> mouse coordinates
  /// @param[in] btns - mouse buttons
  /// @param[in] keys - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void mouseMove(IGenViewportWnd *wnd, int x, int y, int btns, int keys);

  /// Notify the brush that left mouse button is pressed. Called by Editor Core.
  /// @param[in] wnd - pointer to interface IGenViewportWnd of the viewport
  ///                  window that generated the event
  /// @param[in] x,y - <b>x, y</b> mouse coordinates
  /// @param[in] btns - mouse buttons
  /// @param[in] keys - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void mouseLBPress(IGenViewportWnd *wnd, int x, int y, int btns, int keys);

  /// Notify the brush that left mouse button is released. Called by Editor Core.
  /// @param[in] wnd - pointer to interface IGenViewportWnd of the viewport
  ///                  window that generated the event
  /// @param[in] x,y - <b>x, y</b> mouse coordinates
  /// @param[in] btns - mouse buttons
  /// @param[in] keys - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void mouseLBRelease(IGenViewportWnd *wnd, int x, int y, int btns, int keys);

  /// Notify the brush that right mouse button is pressed. Called by Editor Core.
  /// @param[in] wnd - pointer to interface IGenViewportWnd of the viewport
  ///                  window that generated the event
  /// @param[in] x,y - <b>x, y</b> mouse coordinates
  /// @param[in] btns - mouse buttons
  /// @param[in] keys - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void mouseRBPress(IGenViewportWnd *wnd, int x, int y, int btns, int keys);

  /// Notify the brush that right mouse button is released. Called by Editor Core.
  /// @param[in] wnd - pointer to interface IGenViewportWnd of the viewport
  ///                  window that generated the event
  /// @param[in] x,y - <b>x, y</b> mouse coordinates
  /// @param[in] btns - mouse buttons
  /// @param[in] keys - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void mouseRBRelease(IGenViewportWnd *wnd, int x, int y, int btns, int keys);
  //@}


  //*******************************************************
  ///@name Property Panel functions.
  //@{
  /// Used to place brush properties on Property Panel.
  /// @param[in] group - group where brush properties to be placed
  /// @param[in] radius_pid - property ID on Property Panel for brush radius
  /// @param[in] opacity_pid - property ID on Property Panel for brush opacity
  /// @param[in] hardness_pid - property ID on Property Panel for brush hardness
  /// @param[in] autorepeat_pid - property ID on Property Panel for brush
  ///                             autorepeat
  virtual void fillCommonParams(PropertyContainerControlBase &group, int radius_pid, int opacity_pid, int hardness_pid,
    int autorepeat_pid, int step_pid = -1);

  /// Used to place list of brush masks on Property Panel.
  /// @param[in] params - ObjectParameters where masks list will be placed
  static void addMaskList(int pid, PropertyContainerControlBase &params, const char *def = NULL);

  /// Force brush to update its property with value from Property Panel.
  /// @param[in] panel - pointer to Property Panel
  /// @param[in] pid - property ID on Property Panel
  /// @return @b true if update is successful
  virtual bool updateFromPanel(PropertyContainerControlBase *panel, int pid);

  /// Force brush to update its property to Property Panel.
  /// @param[in] panel - pointer to Property Panel
  virtual void updateToPanel(PropertyContainerControlBase &panel);
  //@}

  /// Brush draws itself.
  virtual void draw();

  /// Called by Editor Core on every timer event if brush is in autorepeat mode.
  virtual void repeat();

  /// Get Color of brush
  E3DCOLOR getColor() const { return color; }

  /// Set Color of brush
  void setColor(E3DCOLOR col) { color = col; }

  /// Set ignore step direction (default is 0,0,0)
  void setStepIgnoreDirection(const Point3 &ignore_step_direction) { ignoreStepDirection = normalize(ignore_step_direction); }

  // returns full path to mask file
  static SimpleString getMaskPath(const char *mask);

protected:
  IBrushClient *client; // brush messages handler

  IGenViewportWnd *repeatWnd; // repeat window
  int repeatBtns;             // repeat mouse buttons
  E3DCOLOR color;             // brush color

  Point2 coord;               // brush screen coordinates
  Point2 prevCoord;           // previous brush screen coordinates
  Point3 center;              // brush world position
  Point3 prevCenter;          // previous brush world position
  Point3 normal;              // surface normal in center point
  Point3 ignoreStepDirection; // direction to ignore in stepping

  int viewDownDist;

  bool drawing;      // drawing flag
  bool rightDrawing; // right-button drawing flag

  real paintLen;    // length of brush's paint path
  Point3 lastPaint; // last paint point

  // parameters
  real radius;     // brush radius
  real step;       // brush step (radius / stepDiv)
  real stepDiv;    // brush step divider
  int opacity;     // brush opacity
  int hardness;    // brush hardness
  bool autorepeat; // autorepeat flag

  // parameters's pids
  int radiusPid;
  int opacityPid;
  int hardnessPid;
  int autorepeatPid;
  int stepPid;

  static real maxR; // max brush radius;

  // calculates world center coordinates
  virtual bool calcCenter(IGenViewportWnd *wnd);

  // calculates position when applying brush step
  virtual bool traceDown(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd);

private:
  // sends paint message to client
  void sendPaintMessage(IGenViewportWnd *wnd, int btns, int keys, bool moved);
};


//==============================================================================
inline void Brush::setStepDiv(real div)
{
  if (div > 0.001)
  {
    stepDiv = div;
    step = radius / stepDiv;
  }
}


#endif //__GAIJIN_EDITOR_BRUSH__
