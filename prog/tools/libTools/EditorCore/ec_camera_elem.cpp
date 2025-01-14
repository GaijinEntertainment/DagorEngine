// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_camera_dlg.h>

#include <stdlib.h>
#include <windows.h>

#include <math/dag_capsule.h>
#include <math/dag_mathAng.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <winGuiWrapper/wgw_input.h>
#include <imgui/imgui_internal.h>


IGenViewportWnd *CCameraElem::vpw = NULL;
int CCameraElem::currentType = CCameraElem::MAX_CAMERA;
IPoint2 CCameraElem::freeCamEnterPos = IPoint2(0, 0);


static real setSoftDelta(real &current, real &future, real alpha, bool normalize)
{
  if (current == future)
    return current;

  const real EPS = 0.0002;
  if (fabs(current) - fabs(future) < EPS)
  {
    current = future;
    return current;
  }

  return current = current * alpha + future * (1 - alpha);
}


CCameraElem::CCameraElem(int cam_type)
{
  thisCamType = cam_type;
  clear();
  config = new CameraConfig();
  aboveSurface = false;
}

CCameraElem::~CCameraElem() { del_it(config); }

real CCameraElem::getMultiplier() { return config->controlMultiplier; }

void CCameraElem::load(const DataBlock &blk) { config->load(blk); }

void CCameraElem::save(DataBlock &blk) { config->save(blk); }

void CCameraElem::clear()
{
  initPosition = true;
  setAboveSurfFuture = false;

  forwardZFuture = 0;
  forwardZCurrent = 0;

  rotateXFuture = 0;
  rotateXCurrent = 0;
  rotateYFuture = 0;
  rotateYCurrent = 0;

  strifeXFuture = 0;
  strifeXCurrent = 0;
  strifeYFuture = 0;
  strifeYCurrent = 0;

  upYCurrent = 0;
  upYFuture = 0;

  multiply = false;
  bow = false;
}

void CCameraElem::actInternal()
{
  if (!vpw)
    return;

  if (forwardZFuture || forwardZCurrent)
  {
    real inertia = fabs(forwardZFuture) > fabs(forwardZCurrent) ? config->commonInertia.move : config->commonInertia.stop;

    moveForward(::setSoftDelta(forwardZCurrent, forwardZFuture, inertia, false) * ::dagor_game_act_time, true);
  }

  if (upYFuture || upYCurrent)
  {
    real inertia = fabs(upYFuture) > fabs(upYCurrent) ? config->commonInertia.move : config->commonInertia.stop;
    moveUp(::setSoftDelta(upYCurrent, upYFuture, inertia, false) * ::dagor_game_act_time, true);
  }

  if (strifeXFuture || strifeYFuture || strifeXCurrent || strifeYCurrent)
  {
    real inertia = fabs(strifeXFuture) > fabs(strifeXCurrent) ? config->commonInertia.move : config->commonInertia.stop;
    strife(::setSoftDelta(strifeXCurrent, strifeXFuture, inertia, false) * ::dagor_game_act_time,
      ::setSoftDelta(strifeYCurrent, strifeYFuture, inertia, false) * ::dagor_game_act_time, true, true);
  }

  if (rotateXCurrent || rotateYCurrent || rotateXFuture || rotateYFuture)
  {
    real inertia = fabs(rotateXFuture) > fabs(rotateXCurrent) ? config->hangInertia.move : config->hangInertia.stop;
    rotate(::setSoftDelta(rotateXCurrent, rotateXFuture, inertia, true) * ::dagor_game_act_time,
      ::setSoftDelta(rotateYCurrent, rotateYFuture, inertia, true) * ::dagor_game_act_time, false, false);
    if (rotateXFuture == rotateXCurrent)
      rotateXFuture = 0;
    if (rotateYFuture == rotateYCurrent)
      rotateYFuture = 0;
  }

  if (setAboveSurfFuture)
  {
    setAboveSurf();
    setAboveSurfFuture = false;
  }
}

void CCameraElem::stop()
{
  setAboveSurfFuture = true;
  forwardZFuture = 0;
  rotateXFuture = 0.f;
  rotateYFuture = 0.f;
  strifeXFuture = 0;
  strifeYFuture = 0;
  upYFuture = 0;
  bow = false;
}

void CCameraElem::render() {}


void CCameraElem::handleKeyboardInput(unsigned viewport_id)
{
  // Shift is the speed accelerator key, it is allowed.
  const bool modOk = (ImGui::GetIO().KeyMods & (ImGuiMod_Ctrl | ImGuiMod_Alt)) == 0;

  if (modOk && (ImGui::IsKeyDown(ImGuiKey_UpArrow, viewport_id) || ImGui::IsKeyDown(ImGuiKey_W, viewport_id)))
    forwardZFuture = 1;
  else if (modOk && (ImGui::IsKeyDown(ImGuiKey_DownArrow, viewport_id) || ImGui::IsKeyDown(ImGuiKey_S, viewport_id) ||
                      ImGui::IsKeyDown(ImGuiKey_X, viewport_id)))
    forwardZFuture = -1;
  else
    forwardZFuture = 0;

  if (modOk && (ImGui::IsKeyDown(ImGuiKey_Keypad9, viewport_id) || ImGui::IsKeyDown(ImGuiKey_E, viewport_id)))
  {
    upYFuture = 1;
  }
  else if (modOk && (ImGui::IsKeyDown(ImGuiKey_Keypad3, viewport_id) || ImGui::IsKeyDown(ImGuiKey_C, viewport_id)))
  {
    if (upYFuture != -1) // Do not change setAboveSurfFuture multiple times.
    {
      upYFuture = -1;
      bow = true;
      setAboveSurfFuture = true;
    }
  }
  else if (upYFuture != 0) // Do not change setAboveSurfFuture multiple times.
  {
    upYFuture = 0;
    bow = false;
    setAboveSurfFuture = true;
  }

  if (modOk && (ImGui::IsKeyDown(ImGuiKey_Keypad7, viewport_id) || ImGui::IsKeyDown(ImGuiKey_Q, viewport_id)))
    strifeYFuture = 1;
  else if (modOk && (ImGui::IsKeyDown(ImGuiKey_Keypad1, viewport_id) || ImGui::IsKeyDown(ImGuiKey_Z, viewport_id)))
    strifeYFuture = -1;
  else
    strifeYFuture = 0;

  if (modOk && (ImGui::IsKeyDown(ImGuiKey_LeftArrow, viewport_id) || ImGui::IsKeyDown(ImGuiKey_A, viewport_id)))
    strifeXFuture = 1;
  else if (modOk && (ImGui::IsKeyDown(ImGuiKey_RightArrow, viewport_id) || ImGui::IsKeyDown(ImGuiKey_D, viewport_id)))
    strifeXFuture = -1;
  else
    strifeXFuture = 0;
}


void CCameraElem::handleMouseWheel(int delta)
{
  const real speedChangeMultiplier = clamp(config->speedChangeMultiplier, 0.0001f, 10.0f);
  const real multiplier = delta > 0 ? speedChangeMultiplier : (1.0f / speedChangeMultiplier);

  if (wingw::is_key_pressed(VK_SHIFT))
    config->controlMultiplier = clamp(config->controlMultiplier * multiplier, 1.0f / 16.0f, 256.0f);
  else
    config->moveStep = clamp(config->moveStep * multiplier, 1.0f / 16.0f, 256.0f);

  // For non-MAX cameras the strife speed is always the same as the move speed. (See FreeCameraTab::onOk.)
  config->strifeStep = config->moveStep;
}


bool CCameraElem::canPutCapsule(const Point3 &pt)
{
  Point3 cpt, wpt;
  FpsCameraConfig *fpsConf = (FpsCameraConfig *)config;
  if (!fpsConf)
    return false;
  const real fpsHeight = bow ? fpsConf->halfHeight : fpsConf->height;
  const real fromEyesToCrown = 0.10; // 10 centimeters

  real bottom = pt.y - fpsHeight + fpsConf->radius + 0.01;
  real top = pt.y + fromEyesToCrown - fpsConf->radius;

  if (bottom >= top)
  {
    bottom = pt.y;
    top = pt.y;
  }

  Capsule cap;
  cap.set(Point3(pt.x, bottom, pt.z), Point3(pt.x, top, pt.z), fpsConf->radius);
  return IEditorCoreEngine::get()->clipCapsuleStatic(cap, cpt, wpt) >= -1e-3f;
}


Point3 CCameraElem::getSurfPos(const Point3 &pos)
{
  Point3 rez(pos);
  rez.y = 0;

  FpsCameraConfig *fpsConf = (FpsCameraConfig *)config;
  if (!fpsConf)
    return rez;

  const real fpsHeight = bow ? fpsConf->halfHeight : fpsConf->height;
  const real clambStep = bow ? fpsConf->stepHalfHeight : fpsConf->stepHeight;
  rez.y = fpsHeight;

  for (int i = 0; i < 100; i++)
  {
    real dist = 1000;
    Point3 startPos = pos + Point3(0, i * clambStep, 0);
    if (IEditorCoreEngine::get()->traceRay(startPos, Point3(0, -1, 0), dist))
    {
      rez.y = startPos.y - dist + fpsHeight;
      break;
    }
  }
  return rez;
}


void CCameraElem::setAboveSurf()
{
  if (!aboveSurface || !vpw)
    return;

  TMatrix camera;
  vpw->getCameraTransform(camera);

  Point3 endPos = getSurfPos(camera.getcol(3));
  if (!canPutCapsule(endPos))
    return;

  vpw->setCameraPos(endPos);
}


void CCameraElem::rotate(real deltaX, real deltaY, bool multiplySencetive, bool aroundSelection)
{
  if (!vpw)
    return;

  deltaX *= config->rotationStep * PI / 180.0;
  deltaY *= config->rotationStep * PI / 180.0;

  if (multiplySencetive && multiply)
  {
    deltaX *= config->controlMultiplier;
    deltaY *= config->controlMultiplier;
  }

  TMatrix cameraTm;
  vpw->getCameraTransform(cameraTm);

  BBox3 box;
  bool selection = IEditorCoreEngine::get()->getSelectionBox(box);
  Point3 rotationCenter = (selection && aroundSelection) ? box.center() : cameraTm.getcol(3);

  TMatrix rotationSpace;
  rotationSpace.identity();
  rotationSpace.setcol(3, rotationCenter);

  TMatrix cameraRotation = cameraTm;
  cameraRotation.setcol(3, Point3(0.f, 0.f, 0.f));

  TMatrix rotation = rotyTM(deltaX) * cameraRotation * rotxTM(deltaY) * inverse(cameraRotation);
  TMatrix newCameraTm = rotationSpace * rotation * inverse(rotationSpace) * cameraTm;

  // Do not allow rolling over the limit, but allow coming out of a roll that is over the limit. The
  // latter can happen by clicking the Y label on the viewport axis gizmo, or using the top/bottom
  // ortho camera view.
  const real PINCH_MIN_LIMIT = 0.1;
  const real oldRoll = ::fabs(cameraTm.getcol(2).y);
  const real newRoll = ::fabs(newCameraTm.getcol(2).y);
  if ((newRoll > cos(PINCH_MIN_LIMIT) && newRoll >= oldRoll) || newCameraTm.getcol(1).y < 0)
    newCameraTm = rotationSpace * rotyTM(deltaX) * inverse(rotationSpace) * cameraTm;

  vpw->setCameraTransform(newCameraTm);
}


void CCameraElem::moveForward(real deltaZ, bool multiplySencetive, IGenViewportWnd *wnd)
{
  if (!wnd)
    wnd = vpw;

  if (!vpw)
    return;

  deltaZ *= config->moveStep;

  if (multiplySencetive && multiply)
    deltaZ *= config->controlMultiplier;

  if (aboveSurface)
  {
    TMatrix camera;
    vpw->getCameraTransform(camera);

    const Point3 endPos = camera.getcol(3) + camera.getcol(2) * deltaZ;
    const Point3 camPos = getSurfPos(endPos);

    if (!canPutCapsule(camPos))
      return;

    vpw->setCameraPos(camPos);
    return;
  }

  if (!wnd->isOrthogonal())
  {
    TMatrix camera;
    wnd->getCameraTransform(camera);
    wnd->setCameraPos(camera.getcol(3) + camera.getcol(2) * deltaZ);
    wnd->getCameraTransform(camera);
  }
  else
    wnd->setOrthogonalZoom(wnd->getOrthogonalZoom() * (1 + deltaZ / 10));
}


void CCameraElem::moveUp(real deltaY, bool multiplySencetive)
{
  if (!vpw || aboveSurface)
    return;

  deltaY *= config->strifeStep;
  if (multiplySencetive && multiply)
    deltaY *= config->controlMultiplier;

  TMatrix camera;
  vpw->getCameraTransform(camera);
  vpw->setCameraPos(camera.getcol(3) + Point3(0, deltaY, 0));
}


void CCameraElem::strife(real dx, real dy, bool multiply_sencetive, bool config_sencetive)
{
  if (!vpw)
    return;

  if (config_sencetive)
  {
    dx *= config->strifeStep;
    dy *= config->strifeStep;
  }
  if (multiply_sencetive && multiply)
  {
    dx *= config->controlMultiplier;
    dy *= config->controlMultiplier;
  }

  TMatrix camera;
  vpw->getCameraTransform(camera);

  if (aboveSurface)
  {
    Point3 endPos = camera.getcol(3) - camera.getcol(0) * dx + camera.getcol(1) * dy;

    if (!canPutCapsule(endPos))
      return;

    vpw->setCameraPos(getSurfPos(endPos));
  }
  else
    vpw->setCameraPos(camera.getcol(3) - camera.getcol(0) * dx + camera.getcol(1) * dy);
}


void CCameraElem::showConfigDlg(void *parent, CameraConfig *max_cc, CameraConfig *free_cc, CameraConfig *fps_cc, CameraConfig *tps_cc)
{
  CamerasConfigDlg dlg(parent, max_cc, free_cc, fps_cc, tps_cc);

  // dlg.execute();
  dlg.showDialog();
}


void CCameraElem::switchCamera(bool ctrl_pressed, bool shift_pressed)
{
  int new_cam = MAX_CAMERA;
  if (ctrl_pressed && shift_pressed)
    new_cam = TPS_CAMERA;
  else if (ctrl_pressed)
    new_cam = FPS_CAMERA;
  else if (shift_pressed)
    new_cam = CAR_CAMERA;

  switch (getCamera())
  {
    case FREE_CAMERA:
    case FPS_CAMERA:
    case TPS_CAMERA:
    case CAR_CAMERA:
      if (getCamera() != new_cam)
      {
        setCamera(new_cam);
        debug("switch to type#%d camera", new_cam);
        break;
      }

      setCamera(MAX_CAMERA);
      debug("switch to max camera");
      SetCursorPos(freeCamEnterPos.x, freeCamEnterPos.y);
      break;

    case MAX_CAMERA:
      POINT restore_pos;
      GetCursorPos(&restore_pos);

      freeCamEnterPos = IPoint2(restore_pos.x, restore_pos.y);
      if (new_cam != MAX_CAMERA)
      {
        setCamera(new_cam);
        debug("switch to type#%d camera", new_cam);
      }
      else
      {
        setCamera(FREE_CAMERA);
        debug("switch to free camera");
      }
      break;
  }
}

void CCameraElem::setAboveSurface(bool above_surface)
{
  aboveSurface = above_surface;
  setAboveSurfFuture = above_surface;
}

//=====================================================================================
//
//=====================================================================================
FpsCameraElem::FpsCameraElem() : CCameraElem(FPS_CAMERA), speed(0, 0, 0), accelerate(0, 0, 0), mayJump(false), prevPos(0, 0, 0)
{
  del_it(config);
  config = new FpsCameraConfig();
}


//=====================================================================================
void FpsCameraElem::actInternal()
{
  FpsCameraConfig *fpsConf = (FpsCameraConfig *)config;
  if (!fpsConf)
    return;

  if (initPosition)
  {
    TMatrix camera;
    vpw->getCameraTransform(camera);
    pos = camera.getcol(3);
    speed = Point3(0, 0, 0);
    initPosition = false;
    return;
  }

  accelerate = Point3(0, -fpsConf->gravity, 0);

  moveOn(speed * ::dagor_game_act_time);
  speed += accelerate * ::dagor_game_act_time;

  if (pos != prevPos)
    vpw->setCameraPos(pos);
  prevPos = pos;

  CCameraElem::actInternal();
}

//=====================================================================================
struct EnableCollidersGuard
{
  EnableCollidersGuard(const BBox3 &box) { IEditorCoreEngine::get()->setupColliderParams(1, box); }
  EnableCollidersGuard() { IEditorCoreEngine::get()->setupColliderParams(1, BBox3()); }
  ~EnableCollidersGuard() { IEditorCoreEngine::get()->setupColliderParams(0, BBox3()); }
};


//=====================================================================================
void FpsCameraElem::moveOn(const Point3 &dpos)
{
  TMatrix camera;
  vpw->getCameraTransform(camera);

  if (length(dpos) < 0.001)
    return;

  Point3 pt = pos + dpos;
  FpsCameraConfig *fpsConf = (FpsCameraConfig *)config;
  if (!fpsConf)
    return;

  // ladders prefs
  const real footstepH = bow ? fpsConf->stepHalfHeight : fpsConf->stepHeight;
  const real fpsHeight = bow ? fpsConf->halfHeight : fpsConf->height;
  const real fromEyesToCrown = 0.10; // 10 centimeters

  real top = pt.y + fromEyesToCrown;
  real bottom = top - fpsHeight + footstepH;
  Point3 cpt, wpt;

  // clip big capsule
  Capsule cap;
  cap.set(Point3(pt.x, bottom + fpsConf->radius, pt.z), Point3(pt.x, top - fpsConf->radius, pt.z), fpsConf->radius);

  EnableCollidersGuard enableColliders(cap.getBoundingBoxScalar());
  real dist = IEditorCoreEngine::get()->clipCapsuleStatic(cap, cpt, wpt);
  if (dist < -0.001)
  {
    pt -= cpt - wpt;
    speed -= normalize(speed) * (speed * (cpt - wpt));
  }
  else
    pt += dpos;

  dist = 1000;
  if (!IEditorCoreEngine::get()->traceRay(pt, Point3(0, -1, 0), dist))
  {
    pt.y = fpsHeight - fromEyesToCrown;
    speed.y = 0;
  }
  else if (dist < fpsHeight - fromEyesToCrown)
  {
    pt.y += fpsHeight - fromEyesToCrown - dist;
    speed.y = 0;
    mayJump = true;
  }
  else
    mayJump = false;

  pos = pt;
}


//=====================================================================================
void FpsCameraElem::clear()
{
  CCameraElem::clear();

  speed = Point3(0, 0, 0);
  accelerate = Point3(0, 0, 0);
}


//=====================================================================================
void FpsCameraElem::handleKeyboardInput(unsigned viewport_id)
{
  CCameraElem::handleKeyboardInput(viewport_id);

  if (ImGui::IsKeyReleased(ImGuiKey_Tab, viewport_id) && mayJump)
  {
    FpsCameraConfig *fpsConf = (FpsCameraConfig *)config;
    if (fpsConf)
      speed.y += fpsConf->jumpSpeed;
  }
}


//=====================================================================================
void FpsCameraElem::moveForward(real deltaZ, bool multiplySencetive, IGenViewportWnd *wnd)
{
  if (!wnd)
    wnd = vpw;

  if (!vpw)
    return;

  deltaZ *= config->moveStep;

  if (multiplySencetive && multiply)
    deltaZ *= config->controlMultiplier;

  TMatrix camera;
  vpw->getCameraTransform(camera);
  Point3 dpos = camera.getcol(2) * deltaZ;
  dpos.y = 0;
  moveOn(dpos);
}


//=====================================================================================
void FpsCameraElem::strife(real dx, real dy, bool multiply_sencetive, bool config_sencetive)
{
  if (!vpw)
    return;

  if (config_sencetive)
  {
    dx *= config->strifeStep;
    dy *= config->strifeStep;
  }
  if (multiply_sencetive && multiply)
  {
    dx *= config->controlMultiplier;
    dy *= config->controlMultiplier;
  }

  TMatrix camera;
  vpw->getCameraTransform(camera);
  Point3 dpos = -camera.getcol(0) * dx + camera.getcol(1) * dy;
  dpos.y = 0;
  moveOn(dpos);
}


//=====================================================================================
//
//=====================================================================================
TpsCameraElem::TpsCameraElem() : CCameraElem(TPS_CAMERA)
{
  memset(&target, 0, sizeof(target));
  memset(&cam, 0, sizeof(cam));

  del_it(config);

  TpsCameraConfig *tpsConf = new TpsCameraConfig();
  config = tpsConf;
  maxDist = tpsConf->maxMaxDist;
  cam.curDist = maxDist;
}


TpsCameraElem::TpsCameraElem(int cam_type) : CCameraElem(cam_type)
{
  memset(&target, 0, sizeof(target));
  memset(&cam, 0, sizeof(cam));

  del_it(config);

  TpsCameraConfig *tpsConf = new TpsCameraConfig();
  config = tpsConf;
  maxDist = tpsConf->maxMaxDist;
  cam.curDist = maxDist;
}

//=====================================================================================
void TpsCameraElem::actInternal()
{
  TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;
  if (!tpsConf)
    return;

  if (initPosition)
  {
    TMatrix camera;

    vpw->getCameraTransform(camera);
    target.pos = camera.getcol(3);
    target.vel = Point3(0, 0, 0);
    initPosition = false;
    return;
  }

  target.acc = Point3(0, -tpsConf->gravity, 0);

  moveOn(target.vel * ::dagor_game_act_time);
  target.vel += target.acc * ::dagor_game_act_time;

  if (target.pos != target.prevPos || cam.curDist < maxDist)
    cam.changed = true;

  target.prevPos = target.pos;

  CCameraElem::actInternal();

  if (cam.changed)
  {
    if (cam.curDist < maxDist)
      cam.curDist += tpsConf->maxOutSpeed * ::dagor_game_act_time;

    if (cam.curDist > maxDist)
      cam.curDist = maxDist;


    Point3 look_dir = ::sph_ang_to_dir(Point2(cam.ang));
    Point3 c_pos = target.pos;
    real dist = cam.curDist + tpsConf->radius;

    EnableCollidersGuard enableColliders;
    if (IEditorCoreEngine::get()->traceRay(c_pos, -look_dir, dist, NULL, false))
    {
      cam.curDist = dist - tpsConf->radius;
      if (cam.curDist < tpsConf->minDist)
        ; // no-op
      else
        c_pos -= look_dir * cam.curDist;
    }
    else
      c_pos -= look_dir * cam.curDist;

    TMatrix4 tm = matrix_look_at_lh(c_pos, c_pos + look_dir, Point3(0, 1, 0));
    vpw->setCameraTransform(inverse(tmatrix(tm)));
    vpw->invalidateCache();
    cam.changed = false;
  }
}

void TpsCameraElem::render()
{
  if (getCamera() != thisCamType)
    return;

  TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;
  if (!tpsConf)
    return;

  const real footstepH = bow ? tpsConf->stepHalfHeight : tpsConf->stepHeight;
  const real fpsHeight = bow ? tpsConf->halfHeight : tpsConf->height;
  const real fromEyesToCrown = 0.10; // 10 centimeters
  const real rad = tpsConf->radius;
  const int segs = 24;
  const E3DCOLOR col = E3DCOLOR(255, 255, 0, 255);

  real top = target.pos.y + fromEyesToCrown;
  real bottom = top - fpsHeight;
  Point3 cp0 = Point3(target.pos.x, bottom + rad, target.pos.z), cp1 = Point3(target.pos.x, top - rad, target.pos.z);

  ::begin_draw_cached_debug_lines();
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);
  ::draw_cached_debug_sphere(cp0, rad, col, segs);
  ::draw_cached_debug_sphere(cp1, rad, col, segs);


  Point2 pt[64];

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = Point2(cosf(a), sinf(a)) * rad;
  }

  for (int i = 0; i < segs; ++i)
    ::draw_cached_debug_line(Point3(pt[i].x, 0, pt[i].y) + cp0, Point3(pt[i].x, 0, pt[i].y) + cp1, col);

  ::end_draw_cached_debug_lines();
}

//=====================================================================================
void TpsCameraElem::moveOn(const Point3 &dpos)
{
  TMatrix camera;
  vpw->getCameraTransform(camera);

  if (length(dpos) < 0.01)
    return;

  Point3 pt = target.pos + dpos;
  TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;
  if (!tpsConf)
    return;

  // ladders prefs
  const real footstepH = bow ? tpsConf->stepHalfHeight : tpsConf->stepHeight;
  const real fpsHeight = bow ? tpsConf->halfHeight : tpsConf->height;
  const real fromEyesToCrown = 0.10; // 10 centimeters

  real top = pt.y + fromEyesToCrown;
  real bottom = top - fpsHeight + footstepH;
  Point3 cpt, wpt;

  // clip big capsule
  Capsule cap;
  cap.set(Point3(pt.x, bottom + tpsConf->radius, pt.z), Point3(pt.x, top - tpsConf->radius, pt.z), tpsConf->radius);

  EnableCollidersGuard enableColliders(cap.getBoundingBoxScalar());
  real dist = IEditorCoreEngine::get()->clipCapsuleStatic(cap, cpt, wpt);
  if (dist < -0.001)
  {
    pt -= cpt - wpt;
    target.vel -= normalize(target.vel) * (target.vel * (cpt - wpt));
  }
  else
    pt += dpos;

  dist = 1000;
  if (!IEditorCoreEngine::get()->traceRay(pt, Point3(0, -1, 0), dist))
  {
    pt.y = fpsHeight - fromEyesToCrown;
    target.vel.y = 0;
  }
  else if (dist < fpsHeight - fromEyesToCrown)
  {
    pt.y += fpsHeight - fromEyesToCrown - dist;
    target.vel.y = 0;
    target.mayJump = true;
  }
  else
    target.mayJump = false;

  target.pos = pt;
}


//=====================================================================================
void TpsCameraElem::clear()
{
  CCameraElem::clear();

  target.vel = Point3(0, 0, 0);
  target.acc = Point3(0, 0, 0);
}


//=====================================================================================
void TpsCameraElem::handleKeyboardInput(unsigned viewport_id)
{
  CCameraElem::handleKeyboardInput(viewport_id);

  if (ImGui::IsKeyReleased(ImGuiKey_Tab, viewport_id) && target.mayJump)
  {
    TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;
    if (tpsConf)
      target.vel.y += tpsConf->jumpSpeed;
  }
}


//=====================================================================================
void TpsCameraElem::handleMouseWheel(int dz)
{
  TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;
  float new_md = maxDist;

  if (dz < 0)
    new_md += dz * 0.2;
  else
    new_md += dz * 0.5;

  if (new_md < tpsConf->minDist)
    new_md = tpsConf->minDist;
  else if (new_md > tpsConf->maxMaxDist)
    new_md = tpsConf->maxMaxDist;
  maxDist = new_md;
  cam.changed = true;
}

//=====================================================================================
void TpsCameraElem::rotate(real dX, real dY, bool multiplySencetive, bool aroundSelection)
{
  TpsCameraConfig *tpsConf = (TpsCameraConfig *)config;

  cam.ang.x = norm_s_ang(cam.ang.x - dX * config->rotationStep * PI / 180.0);
  cam.ang.y += dY * config->rotationStep * PI / 180.0;
  if (cam.ang.y > tpsConf->maxVAng)
    cam.ang.y = tpsConf->maxVAng;
  else if (cam.ang.y < tpsConf->minVAng)
    cam.ang.y = tpsConf->minVAng;
  cam.changed = true;
}
//=====================================================================================
void TpsCameraElem::moveForward(real deltaZ, bool multiply_sensitive, IGenViewportWnd *wnd)
{
  if (!wnd)
    wnd = vpw;

  if (!vpw)
    return;

  deltaZ *= config->moveStep;

  if (multiply_sensitive && multiply)
    deltaZ *= config->controlMultiplier;

  TMatrix camera;
  vpw->getCameraTransform(camera);
  Point3 dpos = camera.getcol(2);
  dpos.y = 0;
  moveOn(normalize(dpos) * deltaZ);
}


//=====================================================================================
void TpsCameraElem::strife(real dx, real dy, bool multiply_sensitive, bool config_sensitive)
{
  if (!vpw)
    return;

  if (config_sensitive)
  {
    dx *= config->strifeStep;
    dy *= config->strifeStep;
  }
  if (multiply_sensitive && multiply)
  {
    dx *= config->controlMultiplier;
    dy *= config->controlMultiplier;
  }

  TMatrix camera;
  vpw->getCameraTransform(camera);
  Point3 dpos = -camera.getcol(0) * dx + camera.getcol(1) * dy;
  dpos.y = 0;
  moveOn(dpos);
}

InitOnDemand<FreeCameraElem> ec_camera_elem::freeCameraElem;
InitOnDemand<MaxCameraElem> ec_camera_elem::maxCameraElem;
InitOnDemand<FpsCameraElem> ec_camera_elem::fpsCameraElem;
InitOnDemand<TpsCameraElem> ec_camera_elem::tpsCameraElem;
InitOnDemand<CarCameraElem> ec_camera_elem::carCameraElem;
