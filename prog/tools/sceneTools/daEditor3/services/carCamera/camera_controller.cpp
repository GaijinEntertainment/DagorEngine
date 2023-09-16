#include "camera_controller.h"

#include "../../de_appwnd.h"

#include <oldEditor/de_workspace.h>
#include <oldEditor/de_clipping.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <scene/dag_frtdump.h>

#define USE_BULLET_PHYSICS 1
#include <phys/dag_vehicle.h>
#include <vehiclePhys/physCar.h>
#undef USE_BULLET_PHYSICS


//--------------------------------------------

inline void DecToSph(const Point3 &dec, Point3 &sph)
{
  /* radius */ sph.x = dec.length();
  /* teta   */ sph.y = atan2f(dec.y, sqrtf(dec.x * dec.x + dec.z * dec.z));
  /* psi    */ sph.z = atan2f(dec.z, dec.x);
}


inline void SphToDec(const Point3 &sph, Point3 &dec)
{
  float sp = sinf(sph.z), cp = cosf(sph.z), st = sinf(sph.y), ct = cosf(sph.y);
  dec = Point3(sph.x * cp * ct, sph.x * st, sph.x * sp * ct);
}


inline Point3 cwmul(const Point3 &a, const Point3 &b) { return Point3(a.x * b.x, a.y * b.y, a.z * b.z); }


inline void cwsymclamp(Point3 &v, const Point3 &m)
{
  if (v.x > m.x)
    v.x = m.x;
  else if (v.x < -m.x)
    v.x = -m.x;
  if (v.y > m.y)
    v.y = m.y;
  else if (v.y < -m.y)
    v.y = -m.y;
  if (v.z > m.z)
    v.z = m.z;
  else if (v.z < -m.z)
    v.z = -m.z;
}


Point3 clip_camera(const Point3 &pos, const Point3 &tpos, IPhysCar *target)
{
  float rad = 0.25f;
  Point3 camerapos, cameratarg;

  Point3 p = pos, tp = tpos;

  Point3 dir = p - tp;
  float t = 1;
  // debug ( "%d: &pmid=%p", __LINE__, &pmid );

  if (IEditorCoreEngine::get()->traceRay(tp, dir, t))
  // if ( adr2_ssh.getRtDump().traceray ( tp, dir, t ))
  {
    p += (t - 1) * dir;
    p -= normalize(dir) * (rad * .5f);
  }

  dir.normalize();
  Capsule cap;
  /*
  for (int iter=0; iter<5; ++iter)
  {
    cap.set(p,p,rad);
    Point3 cp1,cp2;
    if (!game_phys_world->clipCapsule(cap, cp1, cp2, target))
    //IEditorCoreEngine::get()->clipCapsuleStatic(cap, cpt, wpt)
      break;
    p+=((cp2-cp1)*dir)*dir;
  }
  */

  camerapos = p;
  cameratarg = tp;
  return camerapos;
}


String get_car_common_fn()
{
  DataBlock appBlk(DAGORED2->getWorkspace().getAppPath());
  const char *cpBlkPath = appBlk.getBlockByNameEx("game")->getStr("car_params", "/game/config/car_params.blk");

  char buf[260];
  char *path = dd_get_fname_location(buf, cpBlkPath);
  return String(260, "%s%s%s", DAGORED2->getWorkspace().getAppDir(), path, "car_common.blk");
}

//--------------------------------------------

CameraController::CameraController(IPhysCar *_car) :
  car(_car),
  shakeTbl(midmem),
  chaseValid(false),
  lastGoodPos(1, 0, 0),
  lastPosSph(0, 0, 0),
  afterSwitch(true),
  itm(TMatrix::IDENT),
  back(false)
{
  camera_noise_init();

  smWVelX.init(5);
  smWVelY.init(5);

  vOffsTargetFullScreen.set(0, 1.5, -1.5);
  vOffsCameraFullScreen.set(0, 1.2, -5.5);
  vFovOffsCameraFullScreen.set(0, 1.5, -1.5);
  vOffsTargetSplitScreen.set(0, 1.5, -1.5);
  vOffsCameraSplitScreen.set(0, 1.2, -5.5);
  vFovOffsCameraSplitScreen.set(0, 1.5, -1.5);

  bClip = true;

  velDirWeight = 0.9;
  minUsableVel = 15 / 3.6f;
  maxUsableVel = 30 / 3.6f;

  fov = 75;
  nitroFov = 90;

  vSpringEffect.set(150, 200, 200);
  vExpSlowdown.set(0.5, 0.7, 0.7);

  maxWVel = Point3(TWOPI, TWOPI, 0);
  maxCosTeta = cosf(45.0f * DEG_TO_RAD);
  maxVel2ForRotY = real_sq(20 / 3.6);
  testTimeMax = 0.6;

  sphVelMax = Point3(3500 * 3.6, 1800 * DEG_TO_RAD, 3600 * DEG_TO_RAD);
  sphAccMax = Point3(400, 600 * DEG_TO_RAD, 1200 * DEG_TO_RAD);

  shakePhase = 0;

  setShake(10, 0.0, 0.0, 0.0, 10.0);
  setShake(25, 0.120, 0.007, 0.01, 25.0);
  setShake(90, 0.240, 0.015, 0.04, 90.0);

  reset();
}


CameraController::~CameraController() { car = NULL; }


void CameraController::setShake(float shakeFactor, float vertAmp, float gammaAmp, float rotAmp, float noiseFreq)
{
  ShakeTbl st;
  st.shakeFactor = shakeFactor;
  st.vertAmp = vertAmp;
  st.gammaAmp = gammaAmp;
  st.rotAmp = rotAmp;
  st.noiseFreq = noiseFreq;
  shakeTbl.push_back(st);
}


void CameraController::reset()
{
  chaseValid = true;
  afterSwitch = true;
  tgtLookTurnK.zero();
  curLookTurnK.zero();

  smWVelX.fill(0);
  smWVelY.fill(0);

  sphVel.zero();

  shakeFactor = 0;
  testTimeLeft = 0;

  current_tilt = 0;
  fovFactor = 0;
  idleInertiaFactor = 0;
}


void CameraController::setParamsPreset(int n_preset)
{
  DataBlock blk(get_car_common_fn());

  const DataBlock *b = blk.getBlockByNameEx("ChaseCamera")->getBlock(n_preset);
  if (!b)
    fatal("Invalid preset %d", n_preset);
  else
  {
    vOffsTargetFullScreen = b->getPoint3("vOffsTargetFullScreen", Point3(0, 1.8, 0));
    vOffsCameraFullScreen = b->getPoint3("vOffsCameraFullScreen", Point3(0, 1.6, -6.2));
    vFovOffsCameraFullScreen = b->getPoint3("vFovOffsCameraFullScreen", vOffsCameraFullScreen);
    vOffsTargetSplitScreen = b->getPoint3("vOffsTargetSplitScreen", Point3(0, 0.6, 0));
    vOffsCameraSplitScreen = b->getPoint3("vOffsCameraSplitScreen", Point3(0, 1.5, -6.2));
    vFovOffsCameraSplitScreen = b->getPoint3("vFovOffsCameraSplitScreen", vOffsCameraSplitScreen);
    minUsableVel = b->getReal("minUsableVel", 15.0f) / 3.6f;
    maxUsableVel = b->getReal("maxUsableVel", 30.0f) / 3.6f;
    maxCosTeta = ::cosf(60 * DEG_TO_RAD);
    fov = b->getReal("fov", fov);
    nitroFov = b->getReal("nitroFov", nitroFov);
  }
}

#define CANCEL_LOOK_BACK (2)

void CameraController::update(float dt)
{
  if (!car)
    return;

  // if (slomo_timer == 0)
  updateValues(dt);

  TMatrix tmJeep;
  car->getVisualTm(tmJeep);
  const Point3 cvel = car->getVel();
  const Point3 wvel = car->getAngVel();

  Point3 targetPt = getCarTargetPt(&tmJeep);

  // test for default chase camera usage is valid
  testChaseCameraValid(dt, tmJeep, cvel, wvel);

  Point3 vTarget;
  Point3 vDestination;

  Point3 &vOffsCamera1 = vOffsCameraFullScreen;
  Point3 &vOffsCamera2 = vFovOffsCameraFullScreen;
  Point3 vOffsTarget = vOffsTargetFullScreen;
  Point3 vOffsCamera = ::lerp(vOffsCamera1, vOffsCamera2, fovFactor);

  // got too far from vehicle
  const float maxDistSq = real_sq(80.0f);
  if (back || lengthSq(itm.getcol(3) - targetPt) > maxDistSq)
  {
    if (back == CANCEL_LOOK_BACK)
      back = false;

    reset();

    itm.identity();
    itm.setcol(3, getVirtualPos(vOffsCamera, targetPt, tmJeep.getcol(2), cvel));
  }

  if (chaseValid)
    vDestination = getVirtualPos(vOffsCamera, targetPt, tmJeep.getcol(2), cvel);
  else
    vDestination = lastGoodPos + targetPt;

  {
    Point3 dir = targetPt - vDestination;
    dir -= dir * dir.y; //== WTF?
    dir.normalize();
    if (!back)
      vTarget = targetPt - dir * vOffsTarget.z + Point3(0, vOffsTarget.y, 0);
    else
      vTarget = targetPt + dir * vOffsTarget.z + Point3(0, vOffsTarget.y, 0);
  }

  {
    const float idleDistSq = real_sq(0.3f);
    float speed = length(cvel);
    const Point2 speedRange(3.0f / 3.6f, 15.0f / 3.6f);
    if (lengthSq(tgtLookTurnK) > real_sq(0.05f))
      idleInertiaFactor = 0;
    else if ((speed < speedRange[1]) && (lengthSq(vDestination - itm.getcol(3)) < idleDistSq))
    {
      float speedK = ::min((speedRange[1] - speed) / (speedRange[1] - speedRange[0]), 1.0f);
      idleInertiaFactor = ::min(idleInertiaFactor + 2.0f * speedK * dt, 1.0f);
    }
    else
      idleInertiaFactor = ::max(idleInertiaFactor - 0.2f * dt, 0.0f);
  }

  if (idleInertiaFactor > 0.0f)
  {
    float th = 0.8f;
    if (idleInertiaFactor > th)
      vDestination = itm.getcol(3);
    else
    {
      vDestination = lerp(vDestination, itm.getcol(3), sqrtf(idleInertiaFactor / th));
    }
  }

  if (bClip)
    vDestination = clip_camera(vDestination, vTarget, car);

  vDestination -= targetPt;
  move(vDestination, dt);
  vDestination += targetPt;

  if (bClip)
    vDestination = clip_camera(vDestination, vTarget, car);

  itm.setcol(3, vDestination);
  {
    Point3 dir = normalize(targetPt - vDestination);
    vTarget = targetPt - dir * vOffsTarget.z + Point3(0, 1, 0) * vOffsTarget.y;
  }

  if (chaseValid)
    lastGoodPos = vDestination - targetPt;

  updateTilt(dt, vDestination, cvel);

  float freq = car->getVel().length() / 80.0;
  calctm(vDestination, vTarget, current_tilt, itm);
  afterSwitch = false;

  for (int i = 0; i < 2; ++i)
  {
    curLookTurnK[i] = ::approach(curLookTurnK[i], tgtLookTurnK[i], dt, 0.05f);
    if (fabsf(curLookTurnK[i]) < 0.001f)
      curLookTurnK[i] = 0.0f;
  }
}


void CameraController::updateValues(float dt)
{
  float speed = car->getVel().length();
  float speedFovFactor = clamp((speed - 20.0f) / 30.0f, 0.0f, 1.0f);

  float accelerator = 0.0;

  /*
  CarPlayer *player = race_car->getCarPlayer();

  if (player)
  {
    A2BaseDevice *device = player->getDevice(CarPlayer::DEVICE_ACCELERATOR);
    if (device && device->isActive())
      accelerator=1.0;
  }
  */

  float dFov = speedFovFactor - fovFactor;
  float dUpMax = (0.2f + accelerator * 0.3f) * dt;
  fovFactor += clamp(dFov, -0.6f * dt, dUpMax);

  float tgtShake = 0.0f;
  if (accelerator > 0)
  {
    const float maxShakeSpeed = 150.0f / 3.6f;
    if (speed < maxShakeSpeed)
      tgtShake = real_sq(sinf(speed / maxShakeSpeed * PI));
    else
      tgtShake = 0.5f;
  }
  const Point2 shakeSpeedRange(90.0f / 3.6f, 220.0f / 3.6f);
  float speedShakeK = (speed - shakeSpeedRange[0]) / (shakeSpeedRange[1] - shakeSpeedRange[0]);
  tgtShake = ::clamp(tgtShake, speedShakeK, 1.0f);
  shakeFactor = ::approach(shakeFactor, tgtShake, dt, 0.03f);
}


Point3 CameraController::getCarTargetPt(const TMatrix *tm_)
{
  BBox3 bbox = car->getLocalBBox();
  const TMatrix *tm = tm_;
  TMatrix carTm;
  if (!tm)
  {
    car->getVisualTm(carTm);
    tm = &carTm;
  }
  Point3 pt(0, bbox.lim[1].y - 1.0f, bbox.center().z);
  return (*tm) * pt;
}


void CameraController::testChaseCameraValid(float dt, const TMatrix &cartm, const Point3 &cvel, const Point3 &wvel)
{
  bool valid = true;

  for (float dt1 = dt; dt1 > 0; dt1 -= 0.01)
  {
    smWVelX.inputVal(wvel.x);
    smWVelY.inputVal(wvel.y);
  }

  if (fabs(smWVelX.outputVal()) > maxWVel.x)
    valid = false;
  else if (fabs(smWVelY.outputVal()) > maxWVel.y && lengthSq(cvel) < maxVel2ForRotY)
    valid = false;
  else if (fabs(cartm.getcol(2).y) > maxCosTeta)
    valid = false;

  if (!valid)
  {
    testTimeLeft = testTimeMax;
    chaseValid = false;
  }
  else if (valid && !chaseValid)
  {
    testTimeLeft -= dt;
    if (testTimeLeft <= 0)
      chaseValid = true;
  }
}


Point3 CameraController::getVirtualPos(const Point3 &vOffs, const Point3 &target_pt, const Point3 &car_dir, const Point3 &vel)
{
  float velocity = length(vel);
  float velWt = velDirWeight * ::clamp((velocity - minUsableVel) / (maxUsableVel - minUsableVel), 0.0f, 1.0f);

  if (car->getMachinery()->getGearStep() < 0)
    velWt *= ::max(vel * car_dir, 0.0f);

  Point3 vBackDir = car_dir;

  if (back)
    vBackDir = -vBackDir;
  else if (velWt > 0)
  {
    vBackDir = vBackDir * (1 - velWt) + vel * (velWt / velocity);
    vBackDir.normalize();
  }

  Point3 vXDir = Point3(0, 1, 0) % vBackDir;
  Point3 vUpDir;

  if (vXDir.lengthSq() < 1.0e-3f)
  {
    vUpDir = Point3(0, 0, 1);
    vXDir = normalize(vUpDir % vBackDir);
  }
  else
  {
    vXDir.normalize();
    vUpDir = normalize(vBackDir % vXDir);
  }

  Point3 vp = vOffs;

  //==
  BBox3 bbox = car->getLocalBBox();
  float carXZRad = sqrtf(::real_sq(bbox.lim[0].x - bbox.lim[1].x) + ::real_sq(bbox.lim[0].z - bbox.lim[1].z)) / 2;
  float zCorrect = -max((carXZRad - 2.5f) * 1.8f, 0.0f);

  Point3 relPos = vXDir * vp.x + vUpDir * vp.y + vBackDir * (vp.z + zCorrect);

  if (curLookTurnK.x != 0.0f || curLookTurnK.y != 0.0f)
  {
    Point3 sphPos;
    DecToSph(relPos, sphPos);
    float angUp = 35.0f * DEG_TO_RAD, angDown = 10.0f * DEG_TO_RAD;

    Point2 dAng = curLookTurnK;

    if (dAng.y > 0)
      sphPos.y = min(sphPos.y + dAng.y * angUp, angUp + 10 * DEG_TO_RAD);
    else
      sphPos.y = max(sphPos.y + dAng.y * angDown, -angDown - 10 * DEG_TO_RAD);
    sphPos.z = norm_s_ang(sphPos.z + dAng.x * PI);
    SphToDec(sphPos, relPos);
  }

  return target_pt + relPos;
}


void CameraController::move(Point3 &vDest, float dt)
{
  // apply finite angle velocities
  if (afterSwitch)
  {
    DecToSph(vDest, lastPosSph);
    return;
  }

  Point3 sph, dif;

  // camera pos
  DecToSph(vDest, sph);
  if (sph.x > 0.1f)
  {
    Point3 p0 = vDest;
    Point3 sphAcc;

    dif = sph - lastPosSph;
    dif.y = norm_s_ang(dif.y);
    dif.z = norm_s_ang(dif.z);

    sphAcc = cwmul(dif, vSpringEffect) * dt;
    cwsymclamp(sphAcc, sphAccMax);

    sphVel = cwmul(sphVel, vExpSlowdown) + sphAcc;
    cwsymclamp(sphVel, sphVelMax);

    sph = lastPosSph + sphVel * dt;
    SphToDec(sph, vDest);
  }

  lastPosSph = sph;
}


void CameraController::updateTilt(float dt, const Point3 &dest, const Point3 &vel)
{
  ////
  prevPrevPos.y = 0.0;
  prevPos.y = 0.0;
  Point3 cPos = itm.getcol(3);
  cPos.y = 0.0;
  Point3 tilt = (prevPrevPos - prevPos) % (prevPos - cPos);
  float t = length(prevPos - cPos);
  tilt.y /= (t + 1); //<--- ???

  float speedLength = min(vel.length(), 120.0f / 3.6f);
  const float tiltThreshold = 3.0f * DEG_TO_RAD, maxTilt = 5.0f * DEG_TO_RAD;
  float target_tilt = 1.5f * tilt.y * speedLength;
  if (target_tilt < 0)
    target_tilt = min(target_tilt + tiltThreshold, 0.0f);
  else
    target_tilt = max(target_tilt - tiltThreshold, 0.0f);
  target_tilt = clamp(target_tilt, -maxTilt, maxTilt);
  ///

  float tilt_speed = 60.0f * DEG_TO_RAD;
  float diff = clamp(target_tilt - current_tilt, -tilt_speed * dt, tilt_speed * dt);

  current_tilt = ::approach(current_tilt, current_tilt + diff, dt, 0.05f);

  prevPrevPos = prevPos;
  prevPos = dest; //<--- ???
}


void CameraController::calctm(const Point3 &pos, const Point3 &targ, float cam_ang, TMatrix &out_tm)
{
  out_tm.setcol(3, pos);
  out_tm.setcol(2, normalize(targ - pos));
  Point3 s = Point3(0, 1, 0) % out_tm.getcol(2);
  float sl = length(s);
  if (sl != 0.0)
    s /= sl;
  else
    s = Point3(1, 0, 0);
  out_tm.setcol(0, s);
  out_tm.setcol(1, normalize(out_tm.getcol(2) % s));
  out_tm = out_tm * rotzTM(cam_ang);
}


//----------------------------------------------
// uncnown functions

float CameraController::getFov() { return fov + (nitroFov - fov) * fovFactor; }


void CameraController::setLookTurn(const Point2 &turn_k)
{
  tgtLookTurnK.x = clamp(turn_k.x, -1.0f, 1.0f);
  tgtLookTurnK.y = clamp(turn_k.y, -1.0f, 1.0f);
}


void CameraController::lookBack(bool b)
{
  if ((back == 1) == b)
    return;
  back = b;
  if (!b)
    back = CANCEL_LOOK_BACK;
}
