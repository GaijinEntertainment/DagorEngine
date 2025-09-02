// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/menu.h>
#include <propPanel/messageQueue.h>
#include <EditorCore/ec_decl.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_viewportAxisId.h>
#include <EditorCore/ec_camera_elem.h>

#include <EASTL/optional.h>
#include <3d/dag_textureIDHolder.h>
#include <util/dag_simpleString.h>
#include <util/dag_stdint.h>
#include <dag/dag_vector.h>
#include <math/dag_Quat.h>
#include <gui/dag_stdGuiRenderEx.h>


// forward declarations for external classes
namespace PropPanel
{
class IMenu;
}
class GridEditDialog;
class RenderViewport;
class IMenu;
class IWndManager;
class ViewportWindowStatSettingsDialog;
class GizmoSettingsDialog;

typedef unsigned int ImGuiID;

struct ViewportParams
{
  TMatrix view;
  real fov;
};

/// CTL implementation of viewport window.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup ViewPort
class ViewportWindow : public IGenViewportWnd, public PropPanel::IMenuEventHandler, public PropPanel::IDelayedCallbackHandler
{
public:
  static bool showDagorUiCursor;
  static void (*render_viewport_frame)(ViewportWindow *vpw);

  ViewportWindow();

  /// Destructor.
  ~ViewportWindow();

  /// Initialize viewport
  virtual void init(IGenEventHandler *eh);

  int handleCommand(int p1 = 0, int p2 = 0, int p3 = 0) override;

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  /// Set event handler of viewport.
  /// Viewport will send messages about its events to this event handler.
  /// @param[in] eh - pointer to event handler
  void setEventHandler(IGenEventHandler *eh) override;

  //*******************************************************
  ///@name Methods to set / get viewport parameters.
  //@{
  /// Set viewport projection parameters
  /// @param[in] orthogonal - if @b true the view will be orthogonal,
  ///                         if @b false - perspective
  /// @param[in] fov - camera's <b>F</b>ield <b>O</b>f <b>V</b>iew (in radians)
  /// @param[in] near_plane - z-near, a distance to nearest visible parts
  ///          of scene (all parts more close to camera will be invisible)
  /// @param[in] far_plane - z-far, a distance to  the farthest visible parts
  ///          of scene (all parts more distant from camera will be invisible)
  void setProjection(bool orthogonal, real fov, real near_plane, real far_plane) override;

  /// Set camera's FOV (<b>F</b>ield <b>O</b>f <b>V</b>iew).
  /// @param[in] fov - camera's angle of view (in radians)
  void setFov(real fov) override;

  /// Get camera's FOV (<b>F</b>ield <b>O</b>f <b>V</b>iew).
  /// @return fov (in radians)
  real getFov() override;

  /// Set camera's direction.
  /// @param[in] forward - direction of camera's view
  /// @param[in] up - direction of camera's top
  void setCameraDirection(const Point3 &forward, const Point3 &up) override;

  /// Set camera's position.
  /// @param[in] pos - position
  void setCameraPos(const Point3 &pos) override;

  /// Set view matrix of the camera.
  /// @param[in] tm - view matrix
  void setCameraTransform(const TMatrix &tm) override;

  /// Set zoom property for camera in orthogonal mode.
  /// @param[in] zoom - zoom value
  void setOrthogonalZoom(real zoom) override;

  /// Get view matrix of the camera.
  /// @param[out] m - view matrix
  void getCameraTransform(TMatrix &m) const override;

  /// Get zoom property for camera in orthogonal mode.
  /// @return zoom value
  real getOrthogonalZoom() const override;

  /// Tests whether camera is in orthogonal mode
  /// @return @b true if viewport is in orthogonal mode, @b false in other case
  bool isOrthogonal() const override;

  /// Tests whether viewport is in "fly" mode
  /// @return @b true if viewport is in "fly" mode, @b false in other case
  bool isFlyMode() const override;

  /// Set camera's mode (orthogonal/perspective).
  /// @param[in] camera_mode - if @b true the view will be orthogonal,
  ///                          if @b false - perspective
  void setCameraMode(bool camera_mode) override;

  /// Set camera's projection.
  /// @param[in] view - view matrix
  /// @param[in] fov - camera's <b>F</b>ield <b>O</b>f <b>V</b>iew
  void setCameraViewProjection(const TMatrix &view, real fov) override;
  //@}


  //*******************************************************
  ///@name Methods to convert viewport coordinates from one coordinate system
  ///      to another.
  //@{
  /// Convert coordinates of viewport vertices to world coordinates.
  /// @param[out] pt - pointer to first element of array consisting of
  ///               4 components. World coordinates of viewport vertices.
  /// @param[out] dirs - pointer to first element of array consisting of
  ///               2 components. World directions of viewport lines.
  void clientRectToWorld(Point3 *pt, Point3 *dirs, float fake_persp_side);

  /// Convert viewport screen coordinates to world coordinates.
  /// @param[in] screen - screen coordinates
  /// @param[out] world - world coordinates (on camera's (lens's) surface)
  /// @param[out] world_dir - camera's direction
  void clientToWorld(const Point2 &screen, Point3 &world, Point3 &world_dir) override;

  /// Convert viewport world coordinates to normal device coordinates.
  /// @param[in] world - world coordinates
  /// @param[out] ndc - normal device coordinates
  void worldToNDC(const Point3 &world, Point3 &ndc) const override;

  /// Convert viewport world coordinates to screen coordinates.
  /// @param[in] world - world coordinates
  /// @param[out] screen - screen coordinates
  /// @param[out] screen_z - the distance between camera and the world point,
  ///                        may be <0 if the world point is placed behind camera
  /// @return @b true if convertion successful, @b false in other case
  bool worldToClient(const Point3 &world, Point2 &screen, real *screen_z = NULL) override;

  /// Convert viewport screen coordinates to application window screen
  /// coordinates.
  /// @param[in,out] x,y - <b>x, y</b> screen coordinates
  void clientToScreen(int &x, int &y) override;

  /// Convert application window screen coordinates to viewport screen
  /// coordinates.
  /// @param[in,out] x,y - <b>x, y</b> screen coordinates
  void screenToClient(int &x, int &y) override;

  /// Get dimensions of viewport in pixels
  /// @param[out] x,y - <b>x, y</b> dimensions of viewport
  void getViewportSize(int &x, int &y) override;
  //@}


  /// Switch view of camera from view 1 to view 2.
  /// For example, switch camera from "Left" view to "Top" view
  /// @param[in] from - "from" view
  /// @param[in] to - "to" view
  void switchCamera(unsigned int from, unsigned int to) override;

  /// Get top-left menu area size
  void getMenuAreaSize(hdpi::Px &w, hdpi::Px &h);

  // It always returns true, just like EcWindow did.
  bool isVisible() const { return true; }

  int getW() const;
  int getH() const;
  void getClientRect(EcRect &clientRect) const;

  void captureMouse() override;
  void releaseMouse() override;

  //*******************************************************
  ///@name Viewport activity.
  //@{
  /// Activate viewport.
  void activate() override;

  /// Tests whether viewport is active.
  /// @return @b true if viewport is active, @b false in other case
  bool isActive() override;
  //@}


  /// Get square of visible radius of a circle
  /// @param[in] pos - world coordinates of center of a circle
  /// @param[in] world_rad - radius of a circle
  /// @param[in] xy - 0-horizontal radius, 1-vertical radius (of ellipse)
  /// @return @b square of visible radius of a circle (ellipse)
  real getLinearSizeSq(const Point3 &pos, real world_rad, int xy) override;


  //*******************************************************
  ///@name Viewport redraw methods.
  //@{
  /// Redraw viewport as Dagor Engine object and CTL object
  void redrawClientRect() override;

  /// Redraw viewport.
  void invalidateCache() override;

  /// Enable / disable viewport cache.
  /// @param[in] en - @b true to enable cache, @b false to disable
  void enableCache(bool en) override;
  //@}


  //*******************************************************
  ///@name Start / stop drawing rectangular selection box.
  /// User interface for editing
  //@{
  /// Start drawing rectangular selection box.
  /// @param[in] mx,my - starting point of selection box
  /// @param[in] type - type of selection box
  void startRectangularSelection(int mx, int my, int type) override;

  /// End drawing rectangular selection box.
  /// @param[out] result - pointer to CtlRect with coordinates of a selected area
  ///              (upper left and lower bottom corners of area). May be @b NULL
  /// @param[out] type - type of selection box
  /// @return @b true if selection successful, @b false if selection aborted
  bool endRectangularSelection(EcRect *result, int *type) override;
  //@}


  /// Set parameters (projection matrix, etc) of a videocard driver camera
  /// equal to parameters of viewport camera
  void setViewProj() override;


  /// Zoom and center bounding box.
  /// Function centers and zooms bounding box to max visible size in viewport.
  /// @param[in] box - bounding box (see #BBox3)
  void zoomAndCenter(BBox3 &box) override;


  //*******************************************************
  ///@name Load / save viewport settings.
  //@{
  /// Load viewport settings from blk file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  virtual void load(const DataBlock &blk);

  /// Save viewport settings to blk file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  virtual void save(DataBlock &blk) const;
  //@}

  /// Set z-near / z-far properties of viewport.
  /// @param[in] zn - z-near, a distance to nearest visible parts of scene
  ///      in viewport (all parts more close to camera will be invisible)
  /// @param[in] zf - z-far, a distance to  the farthest visible parts of scene
  ///      in viewport (all parts more distant from camera will be invisible)
  /// @param[in] change_defaults - @b true - set z-near/z-far as defaults for
  ///      all viewports
  void setZnearZfar(real zn, real zf, bool change_defaults);

  /// Get z-near / z-far values of viewport.
  /// @param[in] zn - z-near, a distance to nearest visible parts of scene in viewport
  /// @param[in] zf - z-far, a distance to  the farthest visible parts of scene in viewport
  void getZnearZfar(real &zn, real &zf) const override;

  //*******************************************************
  ///@name Process custom cameras (if they exist).
  //@{
  /// For internal use in EditorCore.
  void act(real dt);

  /// Set custom camera in viewport.
  /// @param[in] in_customCameras - pointer to custom camera (see ICustomCameras).
  ///                               May be @b NULL to switch off cameras.
  void setCustomCameras(ICustomCameras *in_customCameras) { customCameras = in_customCameras; }
  //@}

  //*******************************************************
  ///@name Stat3D routine
  //@{
  /// Returns true if viewport have to show its 3D statistics
  bool needStat3d() const { return showStats && calcStat3d; }
  /// Draws statistics in viewport window
  void drawStat3d();
  //@}

  /// Draw statistics/debug texts in the viewport area.
  void drawText(int x, int y, const String &text) override;

  bool wireframeOverlayEnabled() const { return wireframeOverlay; }

  /// Set secondary menu event handler of viewport.
  void setMenuEventHandler(PropPanel::IMenuEventHandler *meh) override;

  /// Retrieves the custom context menu of the viewport if it's active/open.
  PropPanel::IMenu *getContextMenu() override { return selectionMenu; }

  /// Render viewport gui
  virtual void paint(int w, int h);

  // Called by the main window when the user drag and drops a file into the application.
  // Return true if it has been handled.
  virtual bool onDropFiles(const dag::Vector<String> &files);

  void showGridSettingsDialog();
  void showStatSettingsDialog();
  void showGizmoSettingsDialog();

  /// ViewportWindowStatSettingsDialog uses this to forward its onChange notification.
  virtual void handleStatSettingsDialogChange(int pcb_id, bool value);

  static unsigned restoreFlags;
  static Tab<ViewportParams> viewportsParams;

  TMatrix getViewTm() const;
  TMatrix4 getProjTm() const;

  struct Input
  {
    int mouseX = 0;
    int mouseY = 0;
    bool lmbPressed = false;
    bool rmbPressed = false;
  };

  const Input &getInput() const { return input; }

  void registerViewportAccelerators(IWndManager &wnd_manager);

  // Returns with true if the command has been handled.
  bool handleViewportAcceleratorCommand(unsigned id);

  bool isViewportTextureReady() const;
  void copyTextureToViewportTexture(BaseTexture &source_texture, int source_width, int source_height);

  void updateImgui(ImGuiID canvas_id, const Point2 &size, float item_spacing, bool vr_mode = false);

  void setScreenshotMode(Point2 size) override { screenshotSize = size; }
  void resetScreenshotMode() override { screenshotSize.reset(); }

protected:
  virtual void fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog);

  void onImguiDelayedCallback(void *user_data) override;

  class ViewportClippingDlg;

  int32_t prevMousePositionX;
  int32_t prevMousePositionY;
  bool isMoveRotateAllowed;
  bool isXLocked;
  bool isYLocked;
  bool orthogonalProjection;
  real projectionFov;
  real projectionFarPlane;
  real projectionNearPlane;
  real orthogonalZoom;
  unsigned int currentProjection;
  bool updatePluginCamera;
  ICustomCameras *customCameras;
  PropPanel::IMenu *popupMenu;
  PropPanel::IMenu *selectionMenu;
  bool wireframeOverlay;
  bool showViewportAxis;
  ViewportAxisId highlightedViewportAxisId;
  ViewportAxisId mouseDownOnViewportAxis;
  TMatrix cameraTransitionLastViewMatrix;
  Quat cameraTransitionStartQuaternion;
  Quat cameraTransitionEndQuaternion;
  float cameraTransitionElapsedTime;
  bool cameraTransitioning = false;

  bool allowPopupMenu;

  struct
  {
    bool active;
    int type;
    EcRect sel;
  } rectSelect;

  IGenEventHandler *curEH;
  IMenuEventHandler *curMEH;
  RenderViewport *viewport;
  int vpId;
  SimpleString viewText;
  hdpi::Px nextStat3dLineY;
  bool showStats;
  bool calcStat3d, opaqueStat3d;
  bool showCameraStats;
  bool showCameraPos;
  bool showCameraDist;
  bool showCameraFov;
  bool showCameraSpeed;
  bool showCameraTurboSpeed;
  ViewportWindowStatSettingsDialog *statSettingsDialog;
  static GizmoSettingsDialog *gizmoSettingsDialog;

  IPoint2 restoreCursorAt = IPoint2(0, 0);

  Input input;

  void drawText(hdpi::Px x, hdpi::Px y, const String &text);

  void paintRect();
  void paintSelectionRect();

  void processRectangularSelectionMouseMove(int mouse_x, int mouse_y);
  void processMouseMoveInfluence(real &deltaX, real &deltaY, int mouse_x, int mouse_y);

  void OnChangePosition();
  void OnDestroy();
  void OnChangeState();
  void OnCameraChanged();
  void setCameraViewText();

  const char *viewportCommandToName(int id) const;
  int viewportNameToCommand(const char *name);
  void clientToZeroLevelPlane(int x, int y, Point3 &world);

  virtual void fillPopupMenu(PropPanel::IMenu &menu);
  void fillStat3dStatSettings(ViewportWindowStatSettingsDialog &dialog);
  void handleStat3dStatSettingsDialogChange(int pcb_id, bool value);

  virtual bool canStartInteractionWithViewport();
  bool canRouteMessagesToExternalEventHandler() const;

  void handleViewportAxisMouseLButtonDown();
  void handleViewportAxisMouseLButtonUp();
  void processViewportAxisCameraRotation(int mouse_x, int mouse_y);
  void setViewportAxisTransitionEndDirection(const Point3 &forward, const Point3 &up);

  void processCameraMouseMove(CCameraElem *camera_elem, int mouse_x, int mouse_y);
  void processCameraMouseWheel(CCameraElem *camera_elem, int delta);

  void processMaxCameraMouseMButtonDown(int mouse_x, int mouse_y);
  void processMaxCameraMouseMButtonUp();
  void processMaxCameraMouseMove(int mouse_x, int mouse_y, bool m_button_down);
  void processMaxCameraMouseWheel(int multiplied_delta);

  void processMouseLButtonPress(int mouse_x, int mouse_y);
  void processMouseLButtonRelease(int mouse_x, int mouse_y);
  void processMouseLButtonDoubleClick(int mouse_x, int mouse_y);
  void processMouseMButtonPress(int mouse_x, int mouse_y);
  void processMouseMButtonRelease(int mouse_x, int mouse_y);
  void processMouseRButtonPress(int mouse_x, int mouse_y);
  void processMouseRButtonRelease(int mouse_x, int mouse_y);
  void processMouseMove(int mouse_x, int mouse_y);
  void processMouseWheel(int mouse_x, int mouse_y, int multiplied_delta);

  void *getMainHwnd();

  void resizeViewportTexture();

  Point3 getCameraPanAnchorPoint();
  virtual BaseTexture *getDepthBuffer() { return nullptr; }

  struct DelayedMouseEvent
  {
    int x;
    int y;
    bool inside;
    int buttons;
    int modifierKeys;
  };

  bool mIsCursorVisible;
  bool active = false;

  TextureIDHolder viewportTexture;
  IPoint2 viewportTextureSize = IPoint2(0, 0);
  IPoint2 requestedViewportTextureSize = IPoint2(0, 0);

  static const int mouseButtonDownArraySize = 5;
  bool mouseButtonDown[mouseButtonDownArraySize];

  IPoint2 lastMousePosition = IPoint2(0, 0);

  // Grid settings are shared among the viewports, so do not allow multiple dialogs.
  static GridEditDialog *gridSettingsDialog;

  dag::Vector<DelayedMouseEvent *> delayedMouseEvents;

  eastl::optional<Point3> lastValidCamPanAnchorPoint;
  eastl::optional<Point3> cameraPanAnchorPoint;

  eastl::optional<Point2> screenshotSize;
};


/// @addtogroup EditorCore
//@{
/// Save cameras properties to blk file.
/// @param[in] blk - Data Block that contains data to save (see #DataBlock)
void save_camera_objects(DataBlock &blk);
/// Load cameras properties from blk file.
/// @param[in] blk - Data Block that contains data to load (see #DataBlock)
void load_camera_objects(const DataBlock &blk);

/// Toggles the display of the Camera settings dialog for all cameras.
/// @param[in] parent - pointer to parent window
void show_camera_objects_config_dialog(void *parent);

/// Closes and deallocates the Camera settings dialog.
void close_camera_objects_config_dialog();

/// Act (tick/update) camera settings dialog.
void act_camera_objects_config_dialog();
//@}
