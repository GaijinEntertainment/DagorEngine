// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <propPanel/commonWindow/dialogWindow.h>

class DataBlock;

/// Inertia parameters of a camera.
/// @ingroup EditorCore
/// @ingroup Cameras
struct Inertia
{
  /// Smoothness factor of a camera stopping process.
  /// The greater this value, the longer camera will do stoppinng.
  real stop;

  /// Smoothness factor of a camera acceleration process.
  /// The greater this value, the longer camera will do acceleration.
  real move;

  /// Load inertia parameters from BLK file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk);

  /// Save inertia parameters to BLK file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk);

  /// Constructor.
  Inertia() : stop(0.05), move(0.9) {}
};


/// Camera parameters.
/// @ingroup EditorCore
/// @ingroup Cameras
struct CameraConfig
{
  /// moveWnd step.
  real moveStep;

  /// Rotation step.
  real rotationStep;

  /// Strafe step.
  /// For this version of Editor Core strifeStep == moveStep.
  real strifeStep;

  /// Speed / rotation multiplier factor, acting when [Ctrl] key is held down.
  real controlMultiplier;

  /// Speed change multiplier factor when using the mouse wheel.
  real speedChangeMultiplier;

  /// Camera's moving inertia.
  Inertia commonInertia;

  /// Camera's rotation inertia in horizontal plane.
  Inertia hangInertia;

  /// Camera's rotation inertia in vertical plane.
  /// For this version of Editor Core vangInertia == hangInertia.
  Inertia vangInertia;

  /// Load camera parameters from BLK file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  virtual void load(const DataBlock &blk);

  /// Save camera parameters to BLK file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  virtual void save(DataBlock &blk);

  /// Constructor.
  CameraConfig() : moveStep(8), rotationStep(18), strifeStep(8), controlMultiplier(12), speedChangeMultiplier(2) {}
  virtual ~CameraConfig() {}
};

/// FPS Camera parameters.
/// @ingroup EditorCore
/// @ingroup Cameras
struct FpsCameraConfig : public CameraConfig
{
  /// Radius of the capsula (representing FPS camera).
  real radius;

  /// Height above collision for FPS camera.
  real height;

  /// Height above collision for FPS camera with key [C] pressed (bowing mode).
  real halfHeight;

  /// Step height FPS camera may climb up.
  real stepHeight;

  /// Step height FPS camera may climb up with key [C] pressed (bowing mode).
  real stepHalfHeight;

  real gravity, jumpSpeed;

  /// Constructor.
  FpsCameraConfig() :
    CameraConfig(), radius(0.3), height(1.8), halfHeight(1.2), stepHeight(0.5), stepHalfHeight(0.2), gravity(9.8), jumpSpeed(4)
  {
    moveStep = 3.5;
    controlMultiplier = 3.5;
    strifeStep = 3.5;
  }
  /// Destructor.
  ~FpsCameraConfig() override {}

  /// Load camera parameters from BLK file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk) override;

  /// Save camera parameters to BLK file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk) override;
};

struct TpsCameraConfig : public FpsCameraConfig
{
  float minDist, maxMaxDist, maxOutSpeed;
  float minVAng, maxVAng;

  /// Constructor.
  TpsCameraConfig() :
    FpsCameraConfig(),
    minDist(2.0),
    maxMaxDist(10.0),
    maxOutSpeed(5.0),
    minVAng(-88.0 * 3.1415926 / 180.0),
    maxVAng(80.0 * 3.1415926 / 180.0)
  {
    moveStep = 4.5;
  }
  /// Destructor.
  ~TpsCameraConfig() override {}

  /// Load camera parameters from BLK file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk) override;

  /// Save camera parameters to BLK file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk) override;
};


class FreeCameraTab
{
public:
  FreeCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *options);
  void fill();
  void updateConfigFromUserInterface(int pcb_id);

protected:
  PropPanel::ContainerPropertyControl *mTabPage;
  CameraConfig *mConfig;
};


class FPSCameraTab : public FreeCameraTab
{
public:
  FPSCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *options);
  void fill();
  void updateConfigFromUserInterface(int pcb_id);
};


class TPSCameraTab : public FPSCameraTab
{
public:
  TPSCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *options);
  void fill();
  void updateConfigFromUserInterface(int pcb_id);
};


class CamerasConfigDlg : public PropPanel::DialogWindow
{
public:
  CamerasConfigDlg(void *phandle, CameraConfig *max_cc, CameraConfig *free_cc, CameraConfig *fps_cc, CameraConfig *tps_cc);
  ~CamerasConfigDlg() override;

  void fill();

protected:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  PropPanel::ContainerPropertyControl *mTabPage;
  CameraConfig *mConfig;

  FreeCameraTab *mFreeCameraTab;
  FPSCameraTab *mFPSCameraTab;
  TPSCameraTab *mTPSCameraTab;
};
