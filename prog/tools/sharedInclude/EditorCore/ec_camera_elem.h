// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <generic/dag_initOnDemand.h>
#include <propPanel/c_common.h>

/// The class for managing cameras.
/// @ingroup EditorCore
/// @ingroup Cameras
class CCameraElem
{
public:
  enum
  {
    FREE_CAMERA, ///< 'Free' camera
    MAX_CAMERA,  ///< 'Max-like' camera
    FPS_CAMERA,  ///< 'FPS' camera
    TPS_CAMERA,  ///< 'TPS' camera
    CAR_CAMERA,  ///< 'CAR' camera
  };
  /// Constructor.
  CCameraElem(int cam_type);

  /// Destructor.
  virtual ~CCameraElem();

  /// Set camera's viewport window.
  /// @param[in] vpw_ - pointer to viewport window
  static void setViewportWindow(IGenViewportWnd *vpw_) { vpw = vpw_; }
  /// Get current camera type.
  /// @return camera type (see class's enum)
  static int getCamera() { return currentType; }
  /// Switch camera between 'max-like', 'free' and 'fps' modes with
  /// [Space], [Ctrl+Space] shortcuts.
  /// @param[in] ctrl_pressed - @b true if <b>[Ctrl]</b> key pressed,
  ///                           @b false in other case
  static void switchCamera(bool ctrl_pressed, bool shift_pressed);

  /// Show cameras configuration dialog window.
  /// @param[in] parent - pointer to parent window
  /// @param[in] max_cc - pointer to 'max-like' camera config
  /// @param[in] free_cc - pointer to 'free' camera config
  /// @param[in] fps_cc - pointer to 'fps' camera config
  static void showConfigDlg(void *parent, CameraConfig *max_cc, CameraConfig *free_cc, CameraConfig *fps_cc, CameraConfig *tps_cc);

  /// Set camera acceleration flag (to use acceleration).
  /// @param[in] multiply_ - <b>true / false: set / clear </b> camera
  ///                        acceleration flag
  void setMultiply(bool multiply_) { multiply = multiply_; }
  /// Get acceleration multiplier.
  /// @return camera's acceleration multiplier
  real getMultiplier();
  /// Set 'above surface fly' flag.
  /// Set / clear flag indicating 'fps' camera mode.
  /// @param[in] above_surface - @b true: set 'above surface fly' flag
  ///                            ('fps' camera mode)
  /// \n                         - @b false: clear flag
  void setAboveSurface(bool above_surface);

  //*****************************************************************
  /// @name Camera config.
  //@{
  /// Get camera config.
  /// @return pointer to camera config
  CameraConfig *getConfig() { return config; }
  /// Load config data from blk file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk);
  /// Save config data to blk file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk);
  //@}

  //*****************************************************************
  /// @name Keyboard / mouse events.
  //@{
  /// Handle keyboard input using polling.
  // @param[in] viewport_id - ImGuiID of the viewport handling the input
  virtual void handleKeyboardInput(unsigned viewport_id);
  /// Handle mouse move.
  /// @param[in] x,y - x,y mouse coordinates in viewport
  void handleMouseMove(int x, int y)
  {
    rotateXFuture += x;
    rotateYFuture += y;
  }
  /// Handle mouse wheel rotation.
  /// @param[in] delta - if it's > 0 then the wheel was rotated forward (away from user), if < 0 then it was rotated backward
  virtual void handleMouseWheel(int delta);
  //@}

  //*****************************************************************
  /// @name Camera manipulation methods.
  //@{
  /// Rotate camera.
  /// @param[in] dX,dY - rotation angles for X,Y axes
  /// @param[in] multiplySencetive - @b true: use acceleration,
  ///                                @b false: camera is not accelerated
  /// @param[in] aroundSelection - @b true: use rotation center,
  ///                              @b false: rotation center is not used
  virtual void rotate(real dX, real dY, bool multiplySencetive, bool aroundSelection);
  /// Move camera in Z direction.
  /// @param[in] dZ - camera Z-shift
  /// @param[in] multiplySencetive - @b true: use acceleration,
  ///                                @b false: camera is not accelerated
  /// @param[in] wnd - pointer to viewport window
  virtual void moveForward(real dZ, bool multiplySencetive, IGenViewportWnd *wnd = NULL);

  /// Strife camera (move along X,Y surface).
  /// @param[in] dx,dy - camera shift in X,Y directions
  /// @param[in] multiply_sencetive - @b true: use acceleration,
  ///                                @b false: camera is not accelerated
  /// @param[in] config_sencetive - @b true: use camera's 'Turbo'
  ///                              coefficient (acceleration for 'free' / 'FPS'
  ///                              camera if [Shift] key pressed),
  ///                              @b false: 'Turbo' coefficient is not used
  virtual void strife(real dx, real dy, bool multiply_sencetive, bool config_sencetive);
  //@}

  // act camera dynamics
  void act()
  {
    if (getCamera() != thisCamType)
    {
      initPosition = true;
      stop();
      return;
    }
    actInternal();
  }

  // render camera-related objects (e.g., camera target for TP)
  virtual void render();

protected:
  CameraConfig *config;

  virtual void clear();

  void setAboveSurf();
  void moveUp(real deltaY, bool multiplySencetive);
  static void setCamera(int type_) { currentType = type_; }
  Point3 getSurfPos(const Point3 &pos);
  virtual bool canPutCapsule(const Point3 &pt);
  virtual void actInternal();
  void stop();

  static IGenViewportWnd *vpw;
  static int currentType;
  static IPoint2 freeCamEnterPos;
  bool setAboveSurfFuture;
  bool initPosition;
  real forwardZFuture;
  real forwardZCurrent;
  real rotateXFuture = 0.f;
  real rotateXCurrent;
  real rotateYFuture = 0.f;
  real rotateYCurrent;
  real strifeXFuture;
  real strifeXCurrent;
  real strifeYFuture;
  real strifeYCurrent;
  real upYCurrent;
  real upYFuture;
  bool multiply;
  bool bow;
  bool aboveSurface;
  int thisCamType;
};


class FreeCameraElem : public CCameraElem
{
public:
  FreeCameraElem() : CCameraElem(FREE_CAMERA) {}
  using CCameraElem::actInternal;
};

class MaxCameraElem : public CCameraElem
{
public:
  MaxCameraElem() : CCameraElem(MAX_CAMERA) {}

  virtual void handleKeyboardInput(unsigned viewport_id) override {}

  virtual void handleMouseWheel(int delta) override
  {
    // For the Max camera the mouse wheel is handled in ViewportWindow::processCameraControl.
    // This function will not be called at all, so the empty override is not really necessary.
  }
};

//=================================================================================================
// FPS Camera
//=================================================================================================
class FpsCameraElem : public CCameraElem
{
public:
  FpsCameraElem();

  virtual void actInternal();
  virtual void render() {}
  virtual void clear();
  virtual void handleKeyboardInput(unsigned viewport_id) override;
  virtual void moveForward(real deltaZ, bool multiplySencetive, IGenViewportWnd *wnd);
  virtual void strife(real dx, real dy, bool multiply_sencetive, bool config_sencetive);
  virtual void moveOn(const Point3 &dpos);

protected:
  Point3 pos, prevPos, speed, accelerate;
  bool mayJump;
};


//=================================================================================================
// TPS Camera
//=================================================================================================
class TpsCameraElem : public CCameraElem
{
public:
  TpsCameraElem();
  TpsCameraElem(int cam_type);

  virtual void actInternal();
  virtual void render();
  virtual void clear();
  virtual void handleKeyboardInput(unsigned viewport_id) override;
  virtual void rotate(real dX, real dY, bool multiplySencetive, bool aroundSelection);
  virtual void moveForward(real deltaZ, bool multiply_sensitive, IGenViewportWnd *wnd);
  virtual void strife(real dx, real dy, bool multiply_sensitive, bool config_sensitive);
  virtual void moveOn(const Point3 &dpos);

  virtual void handleMouseWheel(int dz) override;

protected:
  struct Target
  {
    Point3 pos, prevPos;
    Point3 vel, acc;
    bool mayJump;
  } target;
  float maxDist;
  struct Cam
  {
    float curDist;
    Point2 ang;
    bool changed;
  } cam;
};

//=================================================================================================
// Car Camera
//=================================================================================================

class VehicleViewer;
class CameraController;

class CarCameraElem : public TpsCameraElem
{
public:
  CarCameraElem();
  virtual ~CarCameraElem();

  virtual bool begin();
  virtual void end();

  virtual void actInternal();
  virtual void render();
  virtual void externalRender();
  virtual void handleMouseWheel(int dz) override;

protected:
  float zoom;
  VehicleViewer *vehicleViewer;
  CameraController *camController;
};

//=================================================================================================

namespace ec_camera_elem
{
extern InitOnDemand<FreeCameraElem> freeCameraElem;
extern InitOnDemand<MaxCameraElem> maxCameraElem;
extern InitOnDemand<FpsCameraElem> fpsCameraElem;
extern InitOnDemand<TpsCameraElem> tpsCameraElem;
extern InitOnDemand<CarCameraElem> carCameraElem;
} // namespace ec_camera_elem
