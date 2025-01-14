// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <drv/3d/dag_resId.h>

#include <EditorCore/ec_decl.h>
#include <sepGui/wndCommon.h>
#include <sepGui/wndMenuInterface.h>
#include <libTools/util/hdpiUtil.h>

// forward declarations for external classes
namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
class DialogWindow;
class IMenu;
class PanelWindowPropertyControl;
} // namespace PropPanel

struct EcRect;
struct Capsule;
class DataBlock;
class UndoSystem;
class DynRenderBuffer;
class Brush;
class CoolConsole;
class GridObject;
class IWndManager;
class IDagorEdCustomCollider;
class EditorWorkspace;
class BBox3;


/// General viewport window interface.
/// Used to interact with viewport window.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup ViewPort
class IGenViewportWnd
{
public:
  /// Set viewport event handler.
  /// Viewport will send messages associated with it's events to this
  /// event handler.
  /// @param[in] eh - pointer to event handler
  virtual void setEventHandler(IGenEventHandler *eh) = 0;

  virtual int handleCommand(int p1 = 0, int p2 = 0, int p3 = 0) = 0;

  //*******************************************************
  ///@name Methods to set / get viewport parameters.
  //@{
  /// Set viewport projection parameters
  /// @param[in] orthogonal - if @b true the view will be orthogonal,
  /// if @b false - perspective
  /// @param[in] fov - camera's <b>F</b>ield <b>O</b>f <b>V</b>iew
  /// @param[in] near_plane - z-near, a distance to nearest visible parts of
  ///              scene (all parts more close to camera will be invisible)
  /// @param[in] far_plane - z-far, a distance to  the farthest visible parts of
  ///              scene (all parts more distant from camera will be invisible)
  virtual void setProjection(bool orthogonal, real fov, real near_plane, real far_plane) = 0;

  virtual void getZnearZfar(real &zn, real &zf) const = 0;


  /// Set camera's FOV (<b>F</b>ield <b>O</b>f <b>V</b>iew).
  /// @param[in] fov - camera's angle of view (in radians)
  virtual void setFov(real fov) = 0;

  /// Get camera's FOV (<b>F</b>ield <b>O</b>f <b>V</b>iew).
  /// @return fov (in radians)
  virtual real getFov() = 0;

  /// Set camera's direction.
  /// @param[in] forward - direction of camera's view
  /// @param[in] up - direction of camera's top
  virtual void setCameraDirection(const Point3 &forward, const Point3 &up) = 0;

  /// Set camera's mode (orthogonal/perspective).
  /// @param[in] camera_mode - if @b true the view will be orthogonal,
  ///                          if @b false - perspective
  virtual void setCameraMode(bool camera_mode) = 0;

  /// Set camera's projection.
  /// @param[in] view - view matrix
  /// @param[in] fov - camera's <b>F</b>ield <b>O</b>f <b>V</b>iew
  virtual void setCameraViewProjection(const TMatrix &view, real fov) = 0;

  /// Set camera's position.
  /// @param[in] pos - position
  virtual void setCameraPos(const Point3 &pos) = 0;

  /// Set view matrix of the camera.
  /// @param[in] tm - view matrix
  virtual void setCameraTransform(const TMatrix &tm) = 0;

  /// Set zoom property for camera in orthogonal mode.
  /// @param[in] zoom - zoom value
  virtual void setOrthogonalZoom(real zoom) = 0;

  /// Get view matrix of the camera.
  /// @param[out] m - view matrix
  virtual void getCameraTransform(TMatrix &m) const = 0;

  /// Get zoom property for camera in orthogonal mode.
  /// @return zoom value
  virtual real getOrthogonalZoom() const = 0;

  /// Test whether camera is in orthogonal mode
  /// @return @b true if viewport is in orthogonal mode, @b false in other case
  virtual bool isOrthogonal() const = 0;

  /// Test whether viewport is in "fly" mode
  /// @return @b true if viewport is in "fly" mode, @b false in other case
  virtual bool isFlyMode() const = 0;
  //@}


  /// Switch view of camera from view 1 to view 2.
  /// For example, switch camera from "Left" view to "Top" view
  /// @param[in] from - "from" view
  /// @param[in] to - "to" view
  virtual void switchCamera(unsigned int from, unsigned int to) = 0;


  //*******************************************************
  ///@name Methods to convert viewport coordinates from one coordinate system
  /// to another.
  //@{
  /// Convert viewport screen coordinates to world coordinates.
  /// @param[in] screen - screen coordinates
  /// @param[out] world - world coordinates (on camera's (lens's) surface)
  /// @param[out] world_dir - camera's direction
  virtual void clientToWorld(const Point2 &screen, Point3 &world, Point3 &world_dir) = 0;

  /// Convert viewport world coordinates to screen coordinates.
  /// @param[in] world - world coordinates
  /// @param[out] screen - screen coordinates
  /// @param[out] screen_z - the distance between camera and the world point,\n
  ///                        may be <0 if the world point is placed behind camera
  /// @return @b true if convertion successful, @b false in other case
  virtual bool worldToClient(const Point3 &world, Point2 &screen, real *screen_z = NULL) = 0;

  /// Convert viewport screen coordinates to application window
  /// screen coordinates.
  /// @param[in,out] x,y - <b>x, y</b> screen coordinates
  virtual void clientToScreen(int &x, int &y) = 0;

  /// Convert application window screen coordinates to viewport
  /// screen coordinates.
  /// @param[in,out] x,y - <b>x, y</b> screen coordinates
  virtual void screenToClient(int &x, int &y) = 0;

  /// Get dimensions of viewport in pixels
  /// @param[out] x,y - <b>x, y</b> dimensions of viewport
  virtual void getViewportSize(int &x, int &y) = 0;
  //@}


  /// Set camera so that object bounding box will be centered and zoomed
  /// in viewport window.
  /// @param[in,out] box - object bounding box
  virtual void zoomAndCenter(BBox3 &box) = 0;


  //*******************************************************
  ///@name Viewport activity.
  //@{
  /// Test whether viewport is active.
  /// @return @b true if viewport is active, @b false in other case
  virtual bool isActive() = 0;

  /// Activate viewport.
  virtual void activate() = 0;
  //@}


  /// Get square of visible radius of a circle.
  /// @param[in] pos - world coordinates of center of a circle
  /// @param[in] world_rad - radius of a circle
  /// @param[in] xy - 0-horizontal radius, 1-vertical radius (of ellipse)
  /// @return @b square of visible radius of a circle (ellipse)
  virtual real getLinearSizeSq(const Point3 &pos, real world_rad, int xy) = 0;


  //*******************************************************
  ///@name Viewport redraw methods.
  //@{
  /// Redraw viewport as Dagor Engine object and CTL object
  virtual void redrawClientRect() = 0;

  /// Redraw viewport.
  virtual void invalidateCache() = 0;

  /// Enable / disable viewport cache.
  /// @param[in] en - @b true to enable cache, @b false to disable
  virtual void enableCache(bool en) = 0;
  //@}


  /// Set parameters (projection matrix, etc) of a videocard driver camera\n
  /// equal to parameters of viewport camera
  virtual void setViewProj() = 0;


  //*******************************************************
  ///@name Start / stop drawing rectangular selection box.
  /// User interface for editing
  //@{
  /// Start drawing rectangular selection box.
  /// @param[in] mx,my - starting point of selection box
  /// @param[in] type - type of selection box
  virtual void startRectangularSelection(int mx, int my, int type) = 0;

  /// End drawing rectangular selection box.
  /// @param[out] result - pointer to CtlRect with coordinates of a selected
  ///       area (upper left and lower bottom corners of area). May be @b NULL
  /// @param[out] type - type of selection box
  /// @return @b true if selection successful, @b false if selection aborted
  virtual bool endRectangularSelection(EcRect *result, int *type) = 0;
  //@}

  /// Draw statistics/debug texts in the viewport area.
  /// @param x - the horizontal viewport draw coordinate.
  /// @param y - the vertical viewport draw coordinate.
  /// @param text - the statistics/debug text to draw.
  virtual void drawText(int x, int y, const String &text) = 0;

  virtual void captureMouse() = 0;
  virtual void releaseMouse() = 0;

  /// Sets a custom menu event handler for the custom context menu
  /// of the viewport.
  virtual void setMenuEventHandler(IMenuEventHandler *meh) = 0;

  /// Retrieves the custom context menu of the viewport if it's active/open.
  virtual PropPanel::IMenu *getContextMenu() = 0;
};


/// General event handler interface.
/// Usually used to send messages from EditorCore to plugins.
/// By using this interface plugins get to know about current events
/// in Dagor Editor.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup EventHandler
class IGenEventHandler
{
public:
  //*******************************************************
  ///@name Keyboard commands handlers.
  //@{
  /// Handle key press
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] vk - virtual key code (see #CtlVirtualKeys)
  ///@param[in] modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif) = 0;

  /// Handle key release
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] vk - virtual key code (see #CtlVirtualKeys)
  ///@param[in] modif - <b>shift keys</b> state (see #CtlShiftKeys)
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) = 0;
  //@}

  //*******************************************************
  ///@name Mouse events handlers.
  //@{
  /// Handle mouse move.
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse left button press
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse left button release
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse right button press
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse right button release
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse center button press
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse center button release
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if an event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Handle mouse scroll wheel
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] wheel_d - scroll wheel steps
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) = 0;

  /// Handle mouse double-click
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if an event successfully processed, @b false in other case
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) = 0;
  //@}

  //*******************************************************
  ///@name Viewport redraw/change events handlers.
  //@{
  /// Viewport CTL window redraw
  ///@param[in] wnd - pointer to viewport window that generated the message
  ///@param[in] dc - viewport's device context
  virtual void handleViewportPaint(IGenViewportWnd *wnd) = 0;

  /// Viewport view change notification
  ///@param[in] wnd - pointer to viewport window that generated the message
  virtual void handleViewChange(IGenViewportWnd *wnd) = 0;
  //@}

  virtual ~IGenEventHandler() = default;
};


/// Interface class to Gizmo.
/// Used to receive messages from Gizmo.
/// @ingroup EditorCore
/// @ingroup Gizmo
class IGizmoClient
{
public:
  //*******************************************************
  ///@name Methods called by Gizmo to get its own coordinates, angles, scaling.
  //@{
  /// Get current Gizmo position
  /// @return Gizmo position
  virtual Point3 getPt() = 0;

  /// Get current Gizmo Euler angles.
  /// Used to set values in proper toolbar fields.
  /// @param[out] p - Euler angles
  /// @return @b true if the function succeeds, @b false in other case
  virtual bool getRot(Point3 &p) { return false; }

  /// Get current Gizmo scaling in X,Y,Z dimensions.
  /// @param[out] p - <b>X,Y,Z</b> scaling ratio
  /// @return @b true if the function succeeds, @b false in other case
  virtual bool getScl(Point3 &p) { return false; }

  /// Get current Gizmo X,Y,Z axes
  /// @param[out] ax - <b>X axis</b>
  /// @param[out] ay - <b>Y axis</b>
  /// @param[out] az - <b>Z axis</b>
  /// @return @b true if the function succeeds, @b false in other case
  virtual bool getAxes(Point3 &ax, Point3 &ay, Point3 &az)
  {
    ax = Point3(1, 0, 0);
    ay = Point3(0, 1, 0);
    az = Point3(0, 0, 1);
    return true;
  }
  //@}


  //*******************************************************
  ///@name Methods called by Gizmo to inform client code about Gizmo's changes.
  //@{
  /// <b>Gizmo changed</b> event handler.
  /// Called when Gizmo changed its coordinates, axes, scaling etc
  /// @param[in] delta - Gizmo changing (depends on mode):\n
  /// - @b Move - position relative to last call of getPt()\n
  /// - @b Rotation - current Euler angles\n
  /// - @b Scale - current scale
  virtual void changed(const Point3 &delta) = 0;

  /// Called when user clicks any Gizmo axis.
  /// i.e. at the begining of changing Gizmo's position, axes, etc
  virtual void gizmoStarted() = 0;

  /// Called at the end of Gizmo's changing
  /// @param[in] apply -   @b true - apply changes that take place in interval
  ///                      between gizmoStarted() and gizmoEnded() \n
  ///                      @b false - do not apply changes
  virtual void gizmoEnded(bool apply) = 0;

  /// Called at the moment of destroying Gizmo
  virtual void release() = 0;
  //@}


  //*******************************************************
  /// Informs Gizmo that it can begin changing.
  /// Called when user presses left mouse button.
  /// @param[in] wnd - pointer to viewport window (where user clicks)
  /// @param[in] x,y - <b>x,y</b> coordinates where user clicks
  /// @param[in] gizmo_sel - axes flags (what axes selected)
  /// @return @b true - method gizmoStarted() will be called and normal work
  ///                   with Gizmo begins\n
  ///                   @b false - no actions
  virtual bool canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel) = 0;

  /// Returns flags which informs Gizmo about modes supported by plugin
  /// @return flags representing types of modes supported by plugin
  virtual int getAvailableTypes() { return 0; }

  virtual bool usesRendinstPlacement() const { return false; }
  virtual void saveNormalOnSelection(const Point3 &) {}
  virtual void setCollisionIgnoredOnSelection() {}
  virtual void resetCollisionIgnoredOnSelection() {}
};


/// Interface class to editor core.
/// Used to call core functions.
/// Editor interface to be used by plugins.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
class IEditorCoreEngine
{
public:
  /// Current version of interface
  static const int VERSION = 0x203;


  //*******************************************************
  ///@name Gizmo enums.
  //@{
  /// Gizmo modes
  enum ModeType
  {
    MODE_None = 0,             ///< No Gizmo modes selected.
    MODE_Move = 0x0100,        ///< Gizmo is in 'Move' mode.
    MODE_Scale = 0x0200,       ///< Gizmo is in 'Scale' mode.
    MODE_Rotate = 0x0400,      ///< Gizmo is in 'Rotate' mode.
    MODE_MoveSurface = 0x0800, ///< Gizmo is in 'Move on surface' mode.
  };

  /// Gizmo basis
  enum BasisType
  {
    BASIS_None = 0,       ///< No Gizmo basis.
    BASIS_World = 0x0001, ///< Gizmo has 'World' basis.
    BASIS_Local = 0x0002, ///< Gizmo has 'Local' basis.
  };
  /// Gizmo bit masks
  enum
  {
    GIZMO_MASK_Mode = 0xFF00,     ///< Mask for getting Gizmo mode
                                  ///< (see #ModeType).
    GIZMO_MASK_Basis = 0x00FF,    ///< Mask for getting type of Gizmo basis
                                  ///< (see #BasisType).
    GIZMO_MASK_CENTER = 0xFF0000, ///< Mask for getting type of Gizmo center
                                  ///< (see #CenterType).
  };

  /// Gizmo center types
  enum CenterType
  {
    CENTER_None = 0,                     ///< No Gizmo center type.
    CENTER_Pivot = 0x010000,             ///< Gizmo has 'Pivot' center type.
                                         ///< Gizmo center is coincides with
                                         ///< center of first selected object.
    CENTER_Selection = 0x020000,         ///< Gizmo has 'Selection' center type.
                                         ///< Gizmo center is in geometric
                                         ///< center of selected objects.
    CENTER_Coordinates = 0x040000,       ///< Gizmo is set in center of world
                                         ///< coordinates (0,0,0)
    CENTER_SelectionNotRotObj = 0x080000 ///< Gizmo not rotates objects in
                                         ///< group of selected objects.
  };
  //@}


  /// Viewport cache modes
  enum ViewportCacheMode
  {
    VCM_NoCacheAll,        ///< Cache disabled
    VCM_CacheAll,          ///< Cache all viewports
    VCM_CacheAllButActive, ///< Cache all viewports except active one
  };


  /// Constructor.
  inline IEditorCoreEngine() { interfaceVer = VERSION; }

  /// Interface version check.
  /// @return @b true if version check successful, @b false in other case
  inline bool checkVersion() { return interfaceVer == VERSION; }


  //*******************************************************
  ///@name Register / unregister Plugin.
  //@{
  // register/unregister plugins (every plugin should be registered once)
  /// Every plugin should be registered once.

  /// Register plugin (every plugin should be registered once).
  /// @param[in] plugin - pointer to plugin
  /// @return @b true if register successful, @b false in other case
  virtual bool registerPlugin(IGenEditorPlugin *plugin) = 0;

  /// Unregister plugin
  /// @param[in] plugin - pointer to plugin
  /// @return @b true if unregister successful, @b false in other case
  virtual bool unregisterPlugin(IGenEditorPlugin *plugin) = 0;
  //@}


  //*******************************************************
  ///@name Plugins management.
  //@{
  /// Get a number of registered plugins.
  /// @return number of registered plugins
  virtual int getPluginCount() = 0;

  /// Get a pointer to plugin by index.
  /// @param[in] idx - index in plugins array
  /// @return pointer to plugin
  virtual IGenEditorPlugin *getPlugin(int idx) = 0;
  virtual IGenEditorPluginBase *getPluginBase(int idx) = 0;

  /// Get a pointer to current (active) plugin.
  /// @return pointer to current plugin
  virtual IGenEditorPlugin *curPlugin() = 0;
  virtual IGenEditorPluginBase *curPluginBase() = 0;

  /// Get a pointer to the desired interface
  virtual void *getInterface(int interface_uid) = 0;

  /// Get a set of pointers to the desired interface
  virtual void getInterfaces(int interface_uid, Tab<void *> &interfaces) = 0;
  //@}

  ///@name UI management
  //@{
  /// Get pointer to sepGui window manager
  virtual IWndManager *getWndManager() const = 0;

  /// Get custom panel (property/toolbar/etc)
  virtual PropPanel::ContainerPropertyControl *getCustomPanel(int id) const = 0;

  /// Create new property panel
  virtual void addPropPanel(int type, hdpi::Px width) = 0;
  virtual void removePropPanel(void *hwnd) = 0;
  virtual void managePropPanels() = 0;
  virtual void skipManagePropPanels(bool skip) = 0;
  virtual PropPanel::PanelWindowPropertyControl *createPropPanel(PropPanel::ControlEventHandler *eh, const char *caption) = 0;
  virtual PropPanel::IMenu *getMainMenu() = 0;

  /// Delete custom panel
  virtual void deleteCustomPanel(PropPanel::ContainerPropertyControl *panel) = 0;

  /// Create dialog with property panel
  virtual PropPanel::DialogWindow *createDialog(hdpi::Px w, hdpi::Px h, const char *title) = 0;
  /// Delete dialog
  virtual void deleteDialog(PropPanel::DialogWindow *dlg) = 0;
  //@}


  //*******************************************************
  ///@name Viewport methods.
  //@{
  /// Inform editor that viewports need redraw.
  /// Usually used before invalidateViewportCache().
  virtual void updateViewports() = 0;

  /// Immediately redraw viewports.
  /// Usually used after updateViewports().
  virtual void invalidateViewportCache() = 0;

  /// Set viewport cache mode.
  /// @param[in] mode - viewport cache mode (see #ViewportCacheMode)
  virtual void setViewportCacheMode(ViewportCacheMode mode) = 0;

  /// Get number of viewports.
  /// @return number of viewports
  virtual int getViewportCount() = 0;

  /// Get a pointer to viewport by index.
  /// @param[in] n - viewport index
  /// @return pointer to viewport
  virtual IGenViewportWnd *getViewport(int n) = 0;

  /// Get a pointer to viewport that is rendering now.
  /// Used in plugins during a rendering stage.
  /// @return pointer to rendering viewport
  virtual IGenViewportWnd *getRenderViewport() = 0;

  /// Get a pointer to the last active viewport.
  /// @return - pointer to the last active viewport
  /// \n- @b NULL if no viewports were active yet
  virtual IGenViewportWnd *getCurrentViewport() = 0;

  /// Set visibility bounds (z-near, z-far) in all viewports.
  /// @param[in] zn - z-near, a distance to nearest visible parts of scene
  ///                 (all parts more close to camera will be invisible)
  /// @param[in] zf - z-far, a distance to  the farthest visible parts of scene
  ///                 (all parts more distant from camera will be invisible)
  virtual void setViewportZnearZfar(real zn, real zf) = 0;
  //@}


  //*******************************************************
  ///@name Ray tracing methods.
  //@{
  /// Convert screen coordinates to coordinates of one of viewports.
  /// @param[in] x,y - <b>x,y</b> screen coordinates
  /// @param[out] x,y - <b>x,y</b> viewport coordinates
  /// @return pointer to viewport where output coordinates reside or @b NULL
  virtual IGenViewportWnd *screenToViewport(int &x, int &y) = 0;

  /// Do a ray tracing from screen point to world point.
  /// @param[in] x,y - <b>x, y</b> screen coordinates
  /// @param[out] world - world coordinates
  /// @param[in] maxdist - max distance of ray tracing.
  ///                    If max distance achieved and intersection with
  ///                    clipping surface not occurred function returns @b false.
  /// @param[out] out_norm - surface normal in world point, NULL allowed.
  /// @return @b true if intersection with clipping surface occurred,
  ///         @b false in other case
  virtual bool screenToWorldTrace(int x, int y, Point3 &world, real maxdist = 1000, Point3 *out_norm = NULL) = 0;

  /// Do a ray tracing from viewport point to world point.
  /// @param[in] wnd - pointer to viewport
  /// @param[in] x,y - <b>x, y</b> viewport coordinates
  /// @param[out] world - world coordinates
  /// @param[in] maxdist - max distance of ray tracing. If max distance achieved
  ///                      and intersection with clipping surface not occurred
  ///                      function returns @b false.
  /// @param[out] out_norm - surface normal in world point, NULL allowed.
  /// @return @b true if intersection with clipping surface occurred,
  ///         @b false in other case
  virtual bool clientToWorldTrace(IGenViewportWnd *wnd, int x, int y, Point3 &world, real maxdist = 1000, Point3 *out_norm = NULL) = 0;


  /// Setup collider params for ray tracing
  /// @param[in] mode - mode of ray tracing
  /// @param[in] area - area to prepare
  virtual void setupColliderParams(int mode, const BBox3 &area) = 0;

  /// Do a ray tracing from world point to a given direction.
  /// @param[in] src - start point of ray tracing
  /// @param[in] dir - ray tracing direction
  /// @param[in] dist - max distance of ray tracing
  /// @param[out] dist - the distance from start point to clipping point
  /// @param[out] out_norm - surface normal in clipping point, NULL allowed.
  /// @param[in] use_zero_plane - trace to zero plane if no other collision occurred.
  /// @return @b true if intersection with collision surface occurred,
  ///         @b false in other case
  virtual bool traceRay(const Point3 &src, const Point3 &dir, real &dist, Point3 *out_norm = NULL, bool use_zero_plane = true) = 0;

  /// Get intersection of the capsule and the collision surface.
  /// @param[in] c - reference to the capsule
  /// @param[out] cap_pt - capsule point that is placed most deeply behind
  ///                      collision surface
  /// @param[out] world_pt - point of collision surface that is placed most
  ///                        deeply inside capsule
  /// @return distance between cap_pt and world_pt. If capsule not intersects
  ///         collision surface the function <b>returns 0</b>
  virtual real clipCapsuleStatic(const Capsule &c, Point3 &cap_pt, Point3 &world_pt) { return 0; }
  //@}


  /// Get current selection box.
  /// Selection box depends on plugin active and objects selected.
  /// @param[out] box - selection box
  /// @return @b true if operation successful, @b false in other case
  virtual bool getSelectionBox(BBox3 &box) = 0;

  /// Zoom and center objects.
  virtual void zoomAndCenter() = 0;


  //*******************************************************
  ///@name Gizmo methods.
  //@{
  /// Set Gizmo.
  /// @param[in] gc - pointer to object of an interface class IGizmoClient
  /// @param[in] type - Gizmo type and mode
  virtual void setGizmo(IGizmoClient *gc, ModeType type) = 0;

  /// Force start of changing objects with Gizmo, as if user clicks Gizmo.
  /// Function calls handleMouseLBPress() method of Gizmo's event handler
  ///@param[in] wnd - pointer to viewport window
  ///@param[in] x, y - <b>x,y</b> coordinates inside viewport
  ///@param[in] inside - @b true if event occurred inside viewport
  ///@param[in] buttons - mouse buttons state flags
  ///@param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  ///@return @b true if event successfully processed, @b false in other case
  virtual void startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) = 0;

  /// Get Gizmo mode.
  ///@return Gizmo mode (see #ModeType)
  virtual ModeType getGizmoModeType() = 0;

  /// Get Gizmo basis.
  ///@return Gizmo basis (see #BasisType)
  virtual BasisType getGizmoBasisType() = 0;

  /// Get Gizmo basis for a specific mode.
  ///@return Gizmo basis for a specific mode (see #BasisType and see #ModeType)
  virtual BasisType getGizmoBasisTypeForMode(ModeType tp) = 0;

  /// Get Gizmo center type.
  ///@return Gizmo center type (see #CenterType)
  virtual CenterType getGizmoCenterType() = 0;

  /// Tests whether a gizmo operation is in progress.
  /// (For example pressing the left mouse button on the translation gizmo and moving an object.)
  ///@return @b true if operation is in progress
  virtual bool isGizmoOperationStarted() const = 0;
  //@}


  //*******************************************************
  ///@name Brush methods.
  //@{
  // brush routines

  /// Force the core to begin working with brush.
  virtual void beginBrushPaint() = 0;

  /// Force the core to render brush.
  virtual void renderBrush() = 0;

  /// Set brush.
  /// @param[in] brush - pointer to brush
  virtual void setBrush(Brush *brush) = 0;

  /// Get current brush.
  /// @return brush - pointer to Brush
  virtual Brush *getBrush() const = 0;

  /// Stop working with brush.
  virtual void endBrushPaint() = 0;
  /// Is working with brush.
  virtual bool isBrushPainting() const = 0;
  //@}


  //*******************************************************
  ///@name Spatial cursor handling.
  //@{
  /// Show / hide spatial cursor.
  /// @param[in] vis - true / false:  show / hide
  virtual void showUiCursor(bool vis) = 0;

  /// Set spatial cursor position.
  /// @param[in] pos - world coordinates
  /// @param[in] norm - pointer to surface normal (if NULL then surface normal
  ///                   to be considered as Point3(0, 1, 0))
  virtual void setUiCursorPos(const Point3 &pos, const Point3 *norm = NULL) = 0;

  /// Get spatial cursor position and surface normal.
  /// @param[out] pos - world coordinates
  /// @param[out] norm - surface normal
  virtual void getUiCursorPos(Point3 &pos, Point3 &norm) = 0;

  /// Set spatial cursor texture.
  /// @param[in] tex_id - texture's id
  virtual void setUiCursorTex(TEXTUREID tex_id) = 0;

  /// Set spatial cursor properties.
  /// @param[in] size - size (in metres)
  /// @param[in] always_xz - @b true - UI cursor surface always lies on XZ surface
  virtual void setUiCursorProps(real size, bool always_xz) = 0;
  //@}


  //*******************************************************
  /// Show dialog window 'Select by name'.
  /// @param[in] obj_list - pointer to objects list (see #IObjectsList)
  virtual void showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name = NULL) = 0;

  /// Get pointer to editor's render buffer.
  /// @return pointer to editor's render buffer
  // virtual DynRenderBuffer *getDRB() = 0;

  /// Get pointer to editor's undo system.
  /// @return pointer to editor's undo system
  virtual UndoSystem *getUndoSystem() = 0;

  /// Returns link to the application console.
  virtual CoolConsole &getConsole() = 0;

  /// Get Grid object.
  /// @return reference to grid
  virtual GridObject &getGrid() = 0;


  /// Get pointer to current editor core instance.
  /// @return pointer to current editor core instance
  static inline IEditorCoreEngine *get() { return __global_instance; }

  /// Set current editor core instance.
  /// @param[in] eng - editor core instance
  static inline void set(IEditorCoreEngine *eng) { __global_instance = eng; }


  //*******************************************************
  ///@name Internal interface (NOT TO BE USED BY PLUGINS!).
  //@{
  /// Objects in 'Animation' stage
  /// @param dt -
  virtual void actObjects(real dt) = 0;

  /// Objects in 'before Render' stage
  virtual void beforeRenderObjects() = 0;

  /// Objects in 'Render' stage
  virtual void renderObjects() = 0;

  virtual void renderIslDecalObjects() = 0;

  /// Transparent objects in 'Render' stage
  virtual void renderTransObjects() = 0;
  //@}

  /// Snap functions
  virtual Point3 snapToGrid(const Point3 &p) const = 0;
  virtual Point3 snapToAngle(const Point3 &p) const = 0;
  virtual Point3 snapToScale(const Point3 &p) const = 0;


  // query/get interfaces
  virtual void *queryEditorInterfacePtr(unsigned huid) = 0;
  template <class T>
  T *queryEditorInterface()
  {
    return (T *)queryEditorInterfacePtr(T::HUID);
  }

  template <class T>
  T *getInterfaceEx()
  {
    return (T *)getInterface(T::HUID);
  }
  template <class T>
  void getInterfacesEx(Tab<T *> &tab)
  {
    getInterfaces(T::HUID, (Tab<void *> &)tab);
  }

  virtual const char *getLibDir() const = 0;
  virtual class LibCache *getLibCachePtr() = 0;
  virtual Tab<struct WspLibData> *getLibData() = 0;

  virtual void setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filter_mask) const = 0;
  virtual void restoreEditorColliders() const = 0;
  virtual float getMaxTraceDistance() const = 0;
  virtual const EditorWorkspace &getBaseWorkspace() = 0;

  virtual void setShowMessageAt(int x, int y, const SimpleString &msg) = 0;
  virtual void showMessageAt() = 0;

private:
  int interfaceVer;

  static IEditorCoreEngine *__global_instance;
};

class IGenEditorPluginBase
{
public:
  virtual ~IGenEditorPluginBase() {}

  virtual bool getVisible() const { return true; }

  // COM-like facilities
  virtual void *queryInterfacePtr(unsigned huid) = 0;
  template <class T>
  inline T *queryInterface()
  {
    return (T *)queryInterfacePtr(T::HUID);
  }
};

#define EDITORCORE IEditorCoreEngine::get()

#ifndef RETURN_INTERFACE
#define RETURN_INTERFACE(huid, T) \
  if ((unsigned)huid == T::HUID)  \
  return static_cast<T *>(this)
#endif
