// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_camera_dlg.h>

#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathBase.h>
#include <propPanel/control/container.h>

void Inertia::load(const DataBlock &blk)
{
  stop = blk.getReal("stop", 0.05);
  move = blk.getReal("move", 0.9);
}


void Inertia::save(DataBlock &blk)
{
  blk.setReal("stop", stop);
  blk.setReal("move", move);
}


void CameraConfig::load(const DataBlock &blk)
{
  moveStep = blk.getReal("move_step", 8);
  rotationStep = blk.getReal("rotation_step", 18);
  strifeStep = blk.getReal("strife_step", 8);
  controlMultiplier = blk.getReal("control_multiplier", 12);
  speedChangeMultiplier = blk.getReal("speed_change_multiplier", 2);

  commonInertia.load(*blk.getBlockByNameEx("commonInertia"));
  hangInertia.load(*blk.getBlockByNameEx("hangInertia"));
  vangInertia.load(*blk.getBlockByNameEx("vangInertia"));
}


void CameraConfig::save(DataBlock &blk)
{
  blk.setReal("move_step", moveStep);
  blk.setReal("rotation_step", rotationStep);
  blk.setReal("strife_step", strifeStep);
  blk.setReal("control_multiplier", controlMultiplier);
  blk.setReal("speed_change_multiplier", speedChangeMultiplier);
  commonInertia.save(*blk.addBlock("commonInertia"));
  hangInertia.save(*blk.addBlock("hangInertia"));
  vangInertia.save(*blk.addBlock("vangInertia"));
}

void FpsCameraConfig::load(const DataBlock &blk)
{
  CameraConfig::load(blk);
  moveStep = blk.getReal("move_step", 3.5);
  strifeStep = blk.getReal("strife_step", 3.5);
  controlMultiplier = blk.getReal("control_multiplier", 3.5);
  height = blk.getReal("height", 1.8);
  halfHeight = blk.getReal("half_height", 1.2);
  stepHeight = blk.getReal("step_height", 0.5);
  stepHalfHeight = blk.getReal("step_half_height", 0.2);
  radius = blk.getReal("radius", 0.3);
  gravity = blk.getReal("gravity", 9.8);
  jumpSpeed = blk.getReal("jumpSpeed", 4);
}


void FpsCameraConfig::save(DataBlock &blk)
{
  CameraConfig::save(blk);
  blk.setReal("height", height);
  blk.setReal("half_height", halfHeight);
  blk.setReal("step_height", stepHeight);
  blk.setReal("step_half_height", stepHalfHeight);
  blk.setReal("radius", radius);
  blk.setReal("gravity", gravity);
  blk.setReal("jumpSpeed", jumpSpeed);
}


void TpsCameraConfig::load(const DataBlock &blk)
{
  FpsCameraConfig::load(blk);
  minDist = blk.getReal("minDist", 4.5);
  maxMaxDist = blk.getReal("maxMaxDist", 10.0);
  maxOutSpeed = blk.getReal("maxOutSpeed", 5.0);
  minVAng = blk.getReal("minVAng", -88.0 * PI / 180.0);
  maxVAng = blk.getReal("maxVAng", 80.0 * PI / 180.0);
}


void TpsCameraConfig::save(DataBlock &blk)
{
  FpsCameraConfig::save(blk);
  blk.setReal("minDist", minDist);
  blk.setReal("maxMaxDist", maxMaxDist);
  blk.setReal("maxOutSpeed", maxOutSpeed);
  blk.setReal("minVAng", minVAng);
  blk.setReal("maxVAng", maxVAng);
}


//-----------------------------------------------------------------------------


enum
{
  DIALOG_TAB_PANEL,

  DIALOG_TAB_MAX,
  DIALOG_TAB_FREE,
  DIALOG_TAB_FPS,
  DIALOG_TAB_TPS,

  // max-like

  TAB_MAX_MOVE_STEP,
  TAB_MAX_ROT_STEP,
  TAB_MAX_ZOOM_STEP,
  TAB_MAX_MUL,

  // free

  TAB_MOVE_SPEED,
  TAB_MOVE_SPEED_START,
  TAB_MOVE_SPEED_STOP,

  TAB_SPEED_TURBO,
  TAB_SPEED_TURBO_MULT,
  TAB_FREE_SPEED_CHANGE_MULTIPLIER,

  TAB_ROT_SPEED,
  TAB_ROT_SPEED_START,
  TAB_ROT_SPEED_STOP,

  // fps

  TAB_FPS_RADIUS,
  TAB_FPS_HEIGHT,
  TAB_FPS_HEIGHT_H2,
  TAB_FPS_GRAVITY,
  TAB_FPS_STEP,
  TAB_FPS_STEP_H2,
  TAB_FPS_JUMP_SPEED,

  // tps

  TAB_TPS_MIN_DIST,
  TAB_TPS_MAX_DIST,
  TAB_TPS_AWAY_SPEED,
  TAB_TPS_MAX_V_ANG,
  TAB_TPS_MIN_V_ANG,
};


//-----------------------------------------------------------------------------


FreeCameraTab::FreeCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *config) :

  mTabPage(tab_page), mConfig(config)
{}


void FreeCameraTab::fill()
{
  mTabPage->clear();

  mTabPage->createStatic(0, "Speed, ms");
  mTabPage->createStatic(0, "   Inertia", false);
  mTabPage->createStatic(0, "", false);

  mTabPage->createEditFloat(TAB_MOVE_SPEED, "Move", mConfig->moveStep);
  mTabPage->createEditFloat(TAB_MOVE_SPEED_START, "  Start", mConfig->commonInertia.move, 3, true, false);
  mTabPage->createEditFloat(TAB_MOVE_SPEED_STOP, "  Stop", mConfig->commonInertia.stop, 3, true, false);
  mTabPage->createEditFloat(TAB_SPEED_TURBO, "Turbo, ms", mConfig->moveStep * mConfig->controlMultiplier, 3);

  mTabPage->createStatic(0, "", false);
  mTabPage->createStatic(0, "", false);

  mTabPage->createEditFloat(TAB_ROT_SPEED, "Rotate, turn/s", mConfig->rotationStep);
  mTabPage->createEditFloat(TAB_ROT_SPEED_START, "  Start", mConfig->vangInertia.move, 3, true, false);
  mTabPage->createEditFloat(TAB_ROT_SPEED_STOP, "  Stop", mConfig->vangInertia.stop, 3, true, false);

  mTabPage->createSeparator(0);
  mTabPage->createEditFloat(TAB_FREE_SPEED_CHANGE_MULTIPLIER, "Mouse wheel speed multiplier", mConfig->speedChangeMultiplier, 3);
  mTabPage->createStatic(0, "", false);
  mTabPage->createEditFloat(TAB_SPEED_TURBO_MULT, "Turbo multiplier", mConfig->controlMultiplier, 3);
  mTabPage->setMinMaxStep(TAB_SPEED_TURBO_MULT, 1.f, 1000.f, 0.1f);
  mTabPage->createStatic(0, "", false);
}


void FreeCameraTab::updateConfigFromUserInterface(int pcb_id)
{
  mConfig->moveStep = mTabPage->getFloat(TAB_MOVE_SPEED);
  mConfig->rotationStep = mTabPage->getFloat(TAB_ROT_SPEED);

  if (pcb_id == TAB_SPEED_TURBO && mConfig->moveStep)
  {
    mConfig->controlMultiplier = mTabPage->getFloat(TAB_SPEED_TURBO) / mConfig->moveStep;
  }

  if (pcb_id == TAB_SPEED_TURBO_MULT)
  {
    mConfig->controlMultiplier = mTabPage->getFloat(TAB_SPEED_TURBO_MULT);
  }

  mConfig->strifeStep = mConfig->moveStep;
  mConfig->speedChangeMultiplier = mTabPage->getFloat(TAB_FREE_SPEED_CHANGE_MULTIPLIER);

  mConfig->commonInertia.stop = mTabPage->getFloat(TAB_MOVE_SPEED_STOP);
  mConfig->commonInertia.move = mTabPage->getFloat(TAB_MOVE_SPEED_START);
  mConfig->hangInertia.stop = mTabPage->getFloat(TAB_ROT_SPEED_STOP);
  mConfig->hangInertia.move = mTabPage->getFloat(TAB_ROT_SPEED_START);
  mConfig->vangInertia.stop = mTabPage->getFloat(TAB_ROT_SPEED_STOP);
  mConfig->vangInertia.move = mTabPage->getFloat(TAB_ROT_SPEED_START);

  if (pcb_id == TAB_SPEED_TURBO || pcb_id == TAB_SPEED_TURBO_MULT || pcb_id == TAB_MOVE_SPEED)
  {
    if (mConfig->controlMultiplier < 1.f)
    {
      mConfig->controlMultiplier = 1.f;
    }

    mTabPage->setFloat(TAB_SPEED_TURBO, mConfig->moveStep * mConfig->controlMultiplier);
    mTabPage->setFloat(TAB_SPEED_TURBO_MULT, mConfig->controlMultiplier);
  }
}


//-----------------------------------------------------------------------------


FPSCameraTab::FPSCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *options) : FreeCameraTab(tab_page, options) {}


void FPSCameraTab::fill()
{
  FreeCameraTab::fill();

  mTabPage->createSeparator(0);

  FpsCameraConfig *fpsConf = (FpsCameraConfig *)mConfig;
  if (!fpsConf)
    return;

  mTabPage->createEditFloat(TAB_FPS_RADIUS, "Radius", fpsConf->radius);
  mTabPage->createEditFloat(TAB_FPS_HEIGHT, "  Height", fpsConf->height, 2, true, false);
  mTabPage->createEditFloat(TAB_FPS_HEIGHT_H2, "  1/2 H", fpsConf->halfHeight, 2, true, false);

  mTabPage->createEditFloat(TAB_FPS_GRAVITY, "Gravity", fpsConf->gravity);
  mTabPage->createEditFloat(TAB_FPS_STEP, "  Step", fpsConf->stepHeight, 2, true, false);

  mTabPage->createEditFloat(TAB_FPS_STEP_H2, "  1/2 H", fpsConf->stepHalfHeight, 2, true, false);

  mTabPage->createEditFloat(TAB_FPS_JUMP_SPEED, "Jump speed", fpsConf->jumpSpeed, 2);
  mTabPage->createStatic(0, "", false);
  mTabPage->createStatic(0, "", false);
}


void FPSCameraTab::updateConfigFromUserInterface(int pcb_id)
{
  FreeCameraTab::updateConfigFromUserInterface(pcb_id);

  FpsCameraConfig *fpsConf = (FpsCameraConfig *)mConfig;
  if (!fpsConf)
    return;

  fpsConf->radius = mTabPage->getFloat(TAB_FPS_RADIUS);
  fpsConf->height = mTabPage->getFloat(TAB_FPS_HEIGHT);
  fpsConf->halfHeight = mTabPage->getFloat(TAB_FPS_HEIGHT_H2);
  fpsConf->gravity = mTabPage->getFloat(TAB_FPS_GRAVITY);
  fpsConf->stepHeight = mTabPage->getFloat(TAB_FPS_STEP);
  fpsConf->stepHalfHeight = mTabPage->getFloat(TAB_FPS_STEP_H2);
  fpsConf->jumpSpeed = mTabPage->getFloat(TAB_FPS_JUMP_SPEED);
}


//-----------------------------------------------------------------------------


TPSCameraTab::TPSCameraTab(PropPanel::ContainerPropertyControl *tab_page, CameraConfig *options) : FPSCameraTab(tab_page, options) {}


void TPSCameraTab::fill()
{
  FPSCameraTab::fill();

  mTabPage->createSeparator(0);

  TpsCameraConfig *tpsConf = (TpsCameraConfig *)mConfig;
  if (!tpsConf)
    return;

  mTabPage->createEditFloat(TAB_TPS_MIN_DIST, "Min Dist", tpsConf->minDist);
  mTabPage->createEditFloat(TAB_TPS_MAX_DIST, "  Max Dist", tpsConf->maxMaxDist, 2, true, false);
  mTabPage->createEditFloat(TAB_TPS_AWAY_SPEED, "  Away Speed", tpsConf->maxOutSpeed, 2, true, false);

  mTabPage->createEditFloat(TAB_TPS_MAX_V_ANG, "Max V ang", tpsConf->maxVAng * 180 / PI);
  mTabPage->createEditFloat(TAB_TPS_MIN_V_ANG, "  Min V ang", tpsConf->minVAng * 180 / PI, 2, true, false);

  mTabPage->createStatic(0, "", false);
}


void TPSCameraTab::updateConfigFromUserInterface(int pcb_id)
{
  FPSCameraTab::updateConfigFromUserInterface(pcb_id);

  TpsCameraConfig *tpsConf = (TpsCameraConfig *)mConfig;
  if (!tpsConf)
    return;

  tpsConf->minDist = mTabPage->getFloat(TAB_TPS_MIN_DIST);
  tpsConf->maxMaxDist = mTabPage->getFloat(TAB_TPS_MAX_DIST);
  tpsConf->maxOutSpeed = mTabPage->getFloat(TAB_TPS_AWAY_SPEED);
  tpsConf->maxVAng = mTabPage->getFloat(TAB_TPS_MAX_V_ANG) * PI / 180.0;
  tpsConf->minVAng = mTabPage->getFloat(TAB_TPS_MIN_V_ANG) * PI / 180.0;
}


//-----------------------------------------------------------------------------


CamerasConfigDlg::CamerasConfigDlg(void *phandle, CameraConfig *max_cc, CameraConfig *free_cc, CameraConfig *fps_cc,
  CameraConfig *tps_cc) :

  DialogWindow(phandle, hdpi::_pxScaled(570), hdpi::_pxScaled(440), "Camera settings"),

  mConfig(max_cc)
{
  PropPanel::ContainerPropertyControl *_panel = getPanel();
  G_ASSERT(_panel && "No panel in CamerasConfigDlg");

  PropPanel::ContainerPropertyControl *_tpanel = _panel->createTabPanel(DIALOG_TAB_PANEL, "");

  mTabPage = _tpanel->createTabPage(DIALOG_TAB_MAX, "MAX-like camera");

  PropPanel::ContainerPropertyControl *_tpage = _tpanel->createTabPage(DIALOG_TAB_FREE, "Free camera");
  mFreeCameraTab = new FreeCameraTab(_tpage, free_cc);

  _tpage = _tpanel->createTabPage(DIALOG_TAB_FPS, "FPS camera");
  mFPSCameraTab = new FPSCameraTab(_tpage, fps_cc);

  _tpage = _tpanel->createTabPage(DIALOG_TAB_TPS, "TPS camera");
  mTPSCameraTab = new TPSCameraTab(_tpage, tps_cc);
}


CamerasConfigDlg::~CamerasConfigDlg()
{
  delete mFreeCameraTab;
  delete mFPSCameraTab;
  delete mTPSCameraTab;
}


void CamerasConfigDlg::fill()
{
  mTabPage->clear();
  mTabPage->createEditFloat(TAB_MAX_MOVE_STEP, "Move", mConfig->strifeStep);
  mTabPage->createStatic(0, "", false);
  mTabPage->createEditFloat(TAB_MAX_ROT_STEP, "Rotate", mConfig->rotationStep);
  mTabPage->createStatic(0, "", false);

  mTabPage->createEditFloat(TAB_MAX_ZOOM_STEP, "Zoom", mConfig->moveStep);
  mTabPage->createStatic(0, "", false);
  mTabPage->createEditFloat(TAB_MAX_MUL, "Acceleration", mConfig->controlMultiplier);
  mTabPage->createStatic(0, "", false);

  mFreeCameraTab->fill();
  mFPSCameraTab->fill();
  mTPSCameraTab->fill();
}


void CamerasConfigDlg::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  mConfig->moveStep = mTabPage->getFloat(TAB_MAX_ZOOM_STEP);
  mConfig->rotationStep = mTabPage->getFloat(TAB_MAX_ROT_STEP);
  mConfig->controlMultiplier = mTabPage->getFloat(TAB_MAX_MUL);
  mConfig->strifeStep = mTabPage->getFloat(TAB_MAX_MOVE_STEP);

  mFreeCameraTab->updateConfigFromUserInterface(pcb_id);
  mFPSCameraTab->updateConfigFromUserInterface(pcb_id);
  mTPSCameraTab->updateConfigFromUserInterface(pcb_id);
}
