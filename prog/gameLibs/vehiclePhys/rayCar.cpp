// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rayCar.h"
#include "physCarData.h"
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <vehiclePhys/carDynamics/carModel.h>
#include <phys/dag_physics.h>
#include <scene/dag_physMat.h>
#include <workCycle/dag_workCycle.h>
#include <generic/dag_tabSort.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>

#ifndef NO_3D_GFX
#include <debug/dag_debug3d.h>
#include <perfMon/dag_graphStat.h>
#include <gui/dag_visualLog.h>
#endif

#include <math/random/dag_random.h>
#include <math/dag_mathBase.h>

#include "rayCarDamping.h"

#define ACTIVE_CAR_BODY_INTERACT_MASK   1
#define INACTIVE_CAR_BODY_INTERACT_MASK 0
#define DEBUG_CAR_PHYS_MODE             0
#define DEBUG_WHEEL_SLIP                0

#if DEBUG_WHEEL_SLIP
static int gid_slip0 = -1, gid_slip1 = -1;
#endif

#define SLIP_START_TIME 2.0f

using namespace physcarprivate;

static const real MAX_ANGULAR_VELOCITY = 10.0f;

struct CarAndPhBodyPair
{
  RayCar *car;
  PhysBody *body;
  struct Lambda
  {
    static int order(const CarAndPhBodyPair &a, const CarAndPhBodyPair &b)
    {
      if (intptr_t diff = intptr_t(a.body) - intptr_t(b.body))
        return diff > 0 ? 1 : -1;
      return 0;
    }
  };
};

static TabSortedInline<CarAndPhBodyPair, CarAndPhBodyPair::Lambda> car_phys_bodies(midmem_ptr());


// delayed operations for separate simulate/flush
struct RayCarDelayedOp
{
  RayCar *car;
  int index : 24;
  int op : 8;

  enum
  {
    OP_SET_ACTIVE = IPhysCar::CPM_ACTIVE_PHYS,
    OP_SET_INACTIVE = IPhysCar::CPM_INACTIVE_PHYS,
    OP_SET_GHOST = IPhysCar::CPM_GHOST,
    OP_SET_DISABLED = IPhysCar::CPM_DISABLED,
    OP_ADD_IMPULSE,
    OP_SET_DIRECT_SPEED,
    OP_SET_TM,
    OP_SET_VEL,
    OP_SET_ANG_VEL,
    OP_POST_RESET,
  };
};
static Tab<RayCarDelayedOp> dcar_op(tmpmem);
static Tab<Point3> dcar_op_data(tmpmem);
static bool dcar_op_processing = false;

RayCar::RayCar(const char *car_name, PhysBody *body_, const TMatrix &phys_to_logic_tm, const BBox3 &bbox_, const BSphere3 &bsphere_,
  bool simple_phys, const PhysCarSuspensionParams &frontSusp, const PhysCarSuspensionParams &rearSusp) :
  wheels(midmem),
  phBody(body_),
  driver(NULL),
  carCollisionCb(NULL),
  collisionFxCb(NULL),
  carName(car_name),
  bbox(bbox_),
  bsphere(bsphere_),
  virtualCarTm(TMatrix::IDENT),
  virtualCarVel(0, 0, 0),
  idle(false),
  quasiIdle(false),
  isSimpleCar(simple_phys),
  resultingBrakeFactor(0),
  curWheelAng(0),
  wasBodyCollided(false),
  collidedDuringUpdate(false),
  externalDraw(false),
  userData(NULL),
  cachedDirectSpeed(0.0f),
  planarNorm(1.0f, 1.0f, 1.0f, 1.0f),
  lastSteerEnabled(true)
{
  usedAssistsWord = 0;
  usedAssistsDuringUpdateWord = 0;
  enabledAssistsWord = 0;

  tcdVelAng = tcdCarDevAng = 0;
  rdVelAng = rdCarAng = 0;
  tcdDirWt = tcdVelWt = 0;

  fakeBodyEngineFeedbackAngle = 0;
  fakeShakeTm = simple_phys ? NULL : new TMatrix(1);

#if defined(USE_BULLET_PHYSICS)
  vehicle = IPhysVehicle::createRayCarBullet(phBody, ::dagor_game_act_rate < 100 ? 4 : 2);

#elif defined(USE_JOLT_PHYSICS)
  vehicle = IPhysVehicle::createRayCarJolt(phBody, ::dagor_game_act_rate < 100 ? 4 : 2);

#else
  DAG_FATAL("unsupported physics!");
#endif

  vehicle->setCwaa(this);

  phBody->setGroupAndLayerMask(ACTIVE_CAR_BODY_INTERACT_MASK, ACTIVE_CAR_BODY_INTERACT_MASK);
  carPhysMode = CPM_ACTIVE_PHYS;

  rcar = new CarDynamicsModel(4);

  const DataBlock *b, &carsBlk = loadCarDataBlock(b), *eng_blk, *gb_blk;

  eng_blk = carsBlk.getBlockByNameEx("engines")->getBlockByName(b->getStr("engine", "standard"));
  gb_blk = carsBlk.getBlockByNameEx("gearboxes")->getBlockByName(b->getStr("gearbox", "standard"));
  if (!eng_blk)
    DAG_FATAL("can't load engine preset <%s>", b->getStr("engine", "standard"));
  if (!gb_blk)
    DAG_FATAL("can't load gearbox preset <%s>", b->getStr("gearbox", "standard"));

  PhysCarGearBoxParams gboxParams;
  gboxParams.load(gb_blk);
  rcar->load(eng_blk, gboxParams);

  steerPreset = b->getStr("steer", "default");

  float customMass = b->getReal("mass", -1);
  if (customMass > 0)
    setMass(customMass);

  tmPhys2Logic = phys_to_logic_tm;
  tmLogic2Phys = inverse(tmPhys2Logic);

  cachedTm = TMatrix::IDENT;

  enableComputeSteerMz = false;
  steerMz = 0;
  steerMzEqAngle = 0;

  damping = new RayCarDamping();

  params.setDefaultValues(frontSusp, rearSusp);
  loadParamsFromBlk(b, carsBlk);

  calculateBoundingCapsule();

  phBody->setCollisionCallback(this);

  CarAndPhBodyPair pair = {this, phBody};
  car_phys_bodies.insert(pair, 128);

  wheelPhysMatId = PhysMat::getMaterialId("rubber");

#ifndef NO_3D_GFX

#if DEBUG_WHEEL_SLIP
  gid_slip0 = Stat3D::addGraphic("slip0", "slip0", E3DCOLOR(255, 200, 100), 1);
  gid_slip1 = Stat3D::addGraphic("slip1", "slip1", E3DCOLOR(200, 255, 100), 1);
#endif

#endif // NO_3D_GFX

  carMass = getMass();

  calcCached();
  physDisable(true);
  physInactivate(true);
  targetCarPhysMode = carPhysMode = CPM_GHOST;

  vinylTexId = BAD_TEXTUREID;
  planarVinylTexId = BAD_TEXTUREID;
  vinylTexManualDelete = false;
  planarVinylTexManualDelete = false;

  // compute weight distribution
  Point3 cg = tmLogic2Phys.getcol(3);
  float long_wk = (cg.z - params.rearSusp.upperPt.z) / (params.frontSusp.upperPt.z - params.rearSusp.upperPt.z);
  float lat_fwk = -(cg.x - params.frontSusp.upperPt.x) / params.frontSusp.upperPt.x / 2;
  float lat_rwk = -(cg.x - params.rearSusp.upperPt.x) / params.rearSusp.upperPt.x / 2;

  debug("carMass=%.3f " FMT_P3 ", weight distr: %.1f/%.1f (F/R), %.1f:%.1f/%.1f:%.1f", carMass, P3D(cg), long_wk * 100,
    (1 - long_wk) * 100, long_wk * 100 * lat_fwk, long_wk * 100 * (1 - lat_fwk), (1 - long_wk) * 100 * lat_rwk,
    (1 - long_wk) * 100 * (1 - lat_rwk));

#if 0
  if (fabs(lat_fwk-0.5) > 0.01 || fabs(lat_rwk-0.5) > 0.01)
    DAG_FATAL("shifted Cg: %.1f:%.1f/%.1f:%.1f  due to lat_fwk=%.4f lat_rwk=%.4f",
      long_wk*100*lat_fwk, long_wk*100*(1-lat_fwk),
      (1-long_wk)*100*lat_rwk, (1-long_wk)*100*(1-lat_rwk),
      lat_fwk, lat_rwk);
#endif

  // create wheels
  Point3 wAxis = tmPhys2Logic % Point3(1, 0, 0);
  int wPmid = PhysMat::getMaterialId("rubber");
  vehicle->addWheel(NULL, params.frontSusp.wRad, wAxis, wPmid);
  vehicle->addWheel(NULL, params.frontSusp.wRad, wAxis, wPmid);
  vehicle->addWheel(NULL, params.rearSusp.wRad, wAxis, wPmid);
  vehicle->addWheel(NULL, params.rearSusp.wRad, wAxis, wPmid);

  wheels.resize(4);
  wheels[0].reset(true, true, carMass * long_wk * lat_fwk);
  wheels[1].reset(true, false, carMass * long_wk * (1 - lat_fwk));
  wheels[2].reset(false, true, carMass * (1 - long_wk) * lat_rwk);
  wheels[3].reset(false, false, carMass * (1 - long_wk) * (1 - lat_rwk));
  if (!isSimpleCar)
  {
    vehicle->setCwcf(0xF, this);
    for (int i = 0; i < wheels.size(); i++)
      vehicle->setWheelContactCb(i, this);
  }

  for (int i = 0; i < wheels.size(); i++)
    setupWheel(i);
  setWheelParamsPreset(frontWheelPreset, rearWheelPreset);

  if (!simple_phys)
  {
    enableAssist(CA_ABS, true);
    if (wheels[2].powered && !wheels[0].powered)
      enableAssist(CA_TRACTION, true);
    //    enableAssist(CA_ESP, true);
  }
}

RayCar::~RayCar()
{
  applyCarPhysModeChanges();

  vehicle->destroyVehicle();

  del_it(damping);

  for (int i = 0; i < car_phys_bodies.size(); i++)
  {
    if (car_phys_bodies[i].car == this)
    {
      erase_items(car_phys_bodies, i, 1);
      break;
    }
  }

  freeVinylTex();
  freePlanarVinylTex();

  del_it(fakeShakeTm);
  del_it(rcar);
}

void RayCar::calculateBoundingCapsule()
{
  // make bounding capsule
  Point3 wd = bbox.width();
  int md = 0;
  if (wd[1] > wd[md])
    md = 1;
  if (wd[2] > wd[md])
    md = 2;

  real r = 0;
  for (int i = 0; i < 3; i++)
    if (i != md)
      r += wd[i] * wd[i] * 0.25f;
  r = sqrt(r);
  Point3 dl(0, 0, 0);
  dl[md] = wd[md] * .5f - r;
  Point3 c = bbox.center();
  capsule.set(c - dl, c + dl, r);
  cachedCapsule = capsule;
}

void RayCar::setupWheel(int wid)
{
  RayCarWheel &w = wheels[wid];
  Point3 fix_pt, spring;
  PhysCarSuspensionParams &s = w.front ? params.frontSusp : params.rearSusp;

  float k = s.springKAsW0() ? w.m0 * s.springK * s.springK : s.springK;

  // spring length with load of balanced part of car mass
  float loadedStableLen = s.wRad + s.upTravel - s.fixPtOfs;

  // spring length with no load
  float freeLen = loadedStableLen + w.m0 * 9.81 / k;

  float lmin = s.wRad - s.fixPtOfs;
  float lmax = s.wRad + s.maxTravel - s.fixPtOfs;

  spring = normalize(tmPhys2Logic % s.springAxis);
  fix_pt = s.upperPt + normalize(s.springAxis) * s.fixPtOfs;
  if (w.left)
    fix_pt.x = -fix_pt.x;
  fix_pt = tmPhys2Logic * fix_pt;

  w.powered = s.powered();
  w.controlled = s.controlled();
  w.resetDiffEq();

  debug("setup wheel %c%c: fixPt=" FMT_P3 " lmin=%.3f lmax=%.3f, l0=%.3f, rest=%.3f k=%.0f", w.front ? 'F' : 'R', w.left ? 'L' : 'R',
    P3D(fix_pt), lmin, min(lmax, freeLen), freeLen, loadedStableLen, k);

  vehicle->setWheelParams(wid, s.wMass, 1.0);
  vehicle->setSpringPoints(wid, fix_pt, spring, lmin, min(lmax, freeLen), freeLen, 0, loadedStableLen, params.longForcePtInOfs,
    params.longForcePtDownOfs);
  vehicle->setSpringHardness(wid, k, s.springKdUp, s.springKdDown);

  rcar->getWheelState(wid)->setRad(s.wRad);
  rcar->getWheelState(wid)->setMass(s.wMass);
}

void RayCar::detachWheel(int which, DynamicRenderableSceneLodsResource **model, TMatrix &pos)
{
  *model = NULL;
  pos = TMatrix::IDENT;

  G_ASSERT((which >= 0) && (which < wheels.size()));
  wheels[which].visible = false;

#ifndef NO_3D_GFX
  if (!wheels[which].rendObj)
    return;

  *model = wheels[which].rendObj->getLodsResource();
  pos = wheels[which].rendObj->getNodeWtm(0);
#endif
}

inline double rungekutt(double x, double x1, double y, int n, double a, double b)
{
  double result;
  int i;
  double h;
  double y1;
  double k1;
  double k2;
  double k3;
#define F(x, y) (a * y + b)

  h = (x1 - x) / n;
  y1 = y;
  i = 1;
  do
  {
    k1 = h * F(x, y);
    x = x + h / 2;
    y = y1 + k1 / 2;
    k2 = F(x, y) * h;
    y = y1 + k2 / 2;
    k3 = F(x, y) * h;
    x = x + h / 2;
    y = y1 + k3;
    y = y1 + (k1 + 2 * k2 + 2 * k3 + F(x, y) * h) / 6;
    y1 = y;
    i = i + 1;
  } while (i <= n);
  result = y;
  return result;

#undef F
}

void RayCar::calcWheelContactForces(int wid, float norm_force, float load_rad, const Point3 wheel_basis[3],
  const Point3 &ground_normal, float wheel_ang_vel, const Point3 &ground_vel, float cpt_friction, float &out_lat_force,
  float &out_long_force)
{
  float slip_a_star = 0.0f;
  float slip_ratio = 0.0f;

  if (norm_force < 0)
  {
    LOGERR_CTX("norm_force = %f", norm_force);
    norm_force = 0;
  }

  if (norm_force > carMass * 9.81)
    norm_force = carMass * 9.81;

  float dt = wheels[wid].dt;
  float gnd_spd_fwd = ground_vel * wheel_basis[B_FWD];
  float gnd_spd_side = ground_vel * wheel_basis[B_SIDE];
  float abs_vcx = fabs(gnd_spd_fwd);
  Point2 vs(gnd_spd_fwd - wheel_ang_vel * load_rad, gnd_spd_side);
  float vc = sqrtf(gnd_spd_fwd * gnd_spd_fwd + gnd_spd_side * gnd_spd_side);

  if (wheels[wid].psiDt > 0)
  {
    wheels[wid].psiP = -asinf(wheel_basis[B_SIDE] * wheels[wid].lastFwdDir) / wheels[wid].psiDt;
    wheels[wid].lastFwdDir = wheel_basis[B_FWD];
    wheels[wid].psiDt = 0;
  }

  if (dt > 0)
  {
    float x = abs_vcx;
    int signu = gnd_spd_fwd >= 0.0 ? +1 : -1;
    float sigma_a = 0.91, sigma_k = 0.09;

    wheels[wid].lastSlipLatVel = gnd_spd_side;

    slip_a_star = rungekutt(0, dt, wheels[wid].lastSlipAstar, 4, -x / sigma_a, -gnd_spd_side / sigma_a);

    if (fabs(gnd_spd_side) < 0.15)
      slip_a_star *= 0.2;

    if (fabs(gnd_spd_side) > 0.15 || x > 2.0)
      wheels[wid].dampingOn = 0;
    else if (!wheels[wid].dampingOn && signu != wheels[wid].lastSignU)
      wheels[wid].dampingOn = 1;

    if (signu != wheels[wid].lastSignU)
    {
      wheels[wid].lastS = -wheels[wid].lastS;
      wheels[wid].lastSignU = signu;
    }
    wheels[wid].lastS = rungekutt(0, dt, wheels[wid].lastS, 4, -x / sigma_k, (wheel_ang_vel * load_rad * signu - x) / sigma_k);

    if (wheels[wid].lastS > 50.0 || check_nan(wheels[wid].lastS))
      wheels[wid].lastS = 50;
    else if (wheels[wid].lastS < -50.0)
      wheels[wid].lastS = -50;

    if (wheels[wid].dampingOn)
      wheels[wid].lastS *= 0.8;

    slip_ratio = wheels[wid].lastSlipRatio = wheels[wid].lastS * signu;

    wheels[wid].lastSlipAstar = slip_a_star;
    wheels[wid].dt = 0;
  }
  else
  {
    slip_a_star = wheels[wid].lastSlipAstar;
    slip_ratio = wheels[wid].lastSlipRatio;
    wheels[wid].lastFwdDir = wheel_basis[B_FWD];
    wheels[wid].psiP = 0;
  }

  /*
  if ( wid == 0 )
    debug ( "---" );
  debug ( "%d: dt=%.3f slip_angle=%.3f slip_ratio=%.3f signu=%d angvel=%.3f, DAMP=%d gnd_vel=" FMT_P3 "",
    wid, dt, slip_a_star, slip_ratio, wheels[wid].lastSignU, wheel_ang_vel,
    wheels[wid].dampingOn, P3D(ground_vel));
  */

#ifndef NO_3D_GFX

#if DEBUG_WHEEL_SLIP
  if (wid == 2)
    Stat3D::setGraphicValue(gid_slip0, slip_ratio);
  else
    Stat3D::setGraphicValue(gid_slip1, slip_ratio);
#endif

#endif // NO_3D_GFX

  float gamma_s = (wheel_basis[B_SIDE] * wheel_basis[B_UP]), inclination;
  if (gamma_s > 1)
    inclination = PI / 2;
  else if (gamma_s < -1)
    inclination = -PI / 2;
  else
    inclination = asin(gamma_s);

  TireForceModel2 &p = rcar->getWheelState(wid)->pacejka;
  bool wheel_lock = fabs(slip_ratio + 1) < 0.02;
  p.setContactVel(length(vs), abs_vcx, gnd_spd_fwd > 0.0 ? +1 : (gnd_spd_fwd < 0.0 ? -1 : 0));
  p.setCosAquote(gnd_spd_fwd / (vc + 0.1));
  p.setPhy(-wheels[wid].psiP / (vc + 0.1), -(wheels[wid].psiP - (1 - 0.7) * wheel_ang_vel * gamma_s) / (vc + 0.1));

  p.setFriction(cpt_friction * globalFrictionMul, cpt_friction * globalFrictionMul, globalFrictionVDecay);
  p.setCamber(inclination);

  if (wheel_lock)
  {
    p.setSlipAngle(0);
    p.setSlipRatio(slip_ratio);
  }
  else
  {
    p.setSlipAstar(slip_a_star);
    p.setSlipRatio(slip_ratio);
  }
  p.setNormalForce(fabs(norm_force));
  p.computeForces(enableComputeSteerMz && wheels[wid].front);


  float fx = p.getFx(), fy = p.getFy();

  if (wheel_lock)
  {
    float vsl = length(vs);
    fx = vsl > 0 ? fx * vs.x / vsl : 0;
    fy = vsl > 0 ? -fx * vs.y / vsl : 0;
  }
  // debug ( "cpt_friction=%.3f", cpt_friction );

  out_lat_force = -fy;

  if (slip_ratio < 0 && params.brakeForceAdjustK != 0.0 && rcar->curSpeedKmh > 2.0)
    fx *= (1.0 + params.brakeForceAdjustK);

  out_long_force = fx;
  wheels[wid].normForce = norm_force;
  wheels[wid].vs = vs;
}

float RayCar::calcWheelBrakingTorque(int wid, float brake_factor)
{
  G_ASSERT(wid >= 0 && wid < wheels.size());
  ICarController *ctrl = (driver && driver->getController()) ? driver->getController() : NULL;

  float brakeTorque;

  float handbrake = ctrl ? ctrl->getHandbrakeState() : -1.0f;
  if (!wheels[wid].front && ctrl && (handbrake >= 0))
  {
    brakeTorque = params.handBrakesTorque + handbrake * (params.handBrakesTorque2 - params.handBrakesTorque);
  }
  else
  {
    if (!idle && (enabledAssistsWord & (1 << CA_TRACTION)) && wheels[wid].powered && fabs(wheels[wid].lastSlipRatio) > 0.1 &&
        brake_factor < 1 && fabs(wheels[wid].lastSlipAstar) < 1.2 && rcar->getWheelState(wid)->rotationV > 5)
    {
      float t = fabs(wheels[wid].lastSlipRatio) - 0.1;
      brake_factor += 10 * t * (1 - 0.5 * fabs(curWheelAng) / params.maxWheelAng);
      if (brake_factor > 1)
        brake_factor = 1;
      usedAssistsDuringUpdateWord |= (1 << CA_TRACTION);
    }
    else if (!idle && (enabledAssistsWord & (1 << CA_ABS)) && wheels[wid].lastSlipRatio < -0.2 && brake_factor > 0)
    {
      float t = fabs(wheels[wid].lastSlipRatio) - 0.2;
      brake_factor -= 25 * t * t;
      if (brake_factor < 0)
        brake_factor = 0;
      usedAssistsDuringUpdateWord |= (1 << CA_ABS);
    }

    brakeTorque = brake_factor * (wheels[wid].front ? params.frontBrakesTorque : params.rearBrakesTorque);
  }

  if (brakeTorque < params.minWheelSelfBrakeTorque)
    brakeTorque = params.minWheelSelfBrakeTorque;

  return brakeTorque;
}

unsigned RayCar::calcWheelsAcc(float wheel_acc[], int wheel_num, unsigned wheel_need_mask, float dt)
{
  if (carPhysMode != CPM_ACTIVE_PHYS)
  {
    //== TODO: calc from car vel
    memset(wheel_acc, 0, wheel_num * sizeof(wheel_acc[0]));
    return wheel_need_mask;
  }

  unsigned out = 0;

  for (int i = 0; i < wheels.size(); i++)
  {
    CarWheelState &w = *rcar->getWheelState(i);

    if (!vehicle->getWheelContact(i))
      wheels[i].resetDiffEq();
    else
      wheels[i].dt += dt;

    float brakeTorque = calcWheelBrakingTorque(i, resultingBrakeFactor);
    w.rotationV = vehicle->getWheelAngularVelocity(i);
    w.torqueFeedbackTC = vehicle->getWheelFeedbackTorque(i);
    w.torqueBrakingTC = (w.rotationV >= 0) ? -brakeTorque : brakeTorque;
    // debug( "Wheel[%d].torqueFeedbackTC=%.3f", i, w.torqueFeedbackTC );
  }

  rcar->update(dt);

  if (!rcar->getDriveline()->isClutchLocked() && rcar->getDriveline()->getClutchApp() < 0.005)
  {
    if (idle)
      for (int i = 0; i < wheel_num; i++)
      {
        wheel_acc[i] = 0;
        out |= (1 << i);
      }
  }
  else
  {
    for (int i = 0; i < wheel_num; i++)
    {
      if (!(wheel_need_mask & (1 << i)))
        continue;

      CarWheelState &w = *rcar->getWheelState(i);
      if (idle)
      {
        wheel_acc[i] = 0;
        out |= (1 << i);
      }
      else if (w.diff)
      {
        out |= (1 << i);
        // Use differential to determine acceleration
        wheel_acc[i] = w.diff->getAccOut(w.diffSide);
      }

      // debug("wheel[%d].acc=%.3f (vel=%.3f)", i, wheel_acc[i], vehicle->getWheelAngularVelocity(i));
    }
    // debug("----");
  }

  return out;
}
void RayCar::postCalcWheelsAcc(float wheel_acc[], int wheel_num, float dt)
{
  for (int i = 0; i < wheel_num; i++)
  {
    CarWheelState &w = *rcar->getWheelState(i);
    float predicted_w = wheel_acc[i] * dt + w.rotationV;

    if (check_nan(predicted_w) || fabs(predicted_w) > 800)
    {
      debug("++++ predicted_w=%.5f, w.rotationV=%.3f wheel_acc[i]=%.3f", predicted_w, w.rotationV, wheel_acc[i]);

      debug("==== performing soft car reset");
      memset(wheel_acc, 0, wheel_num * sizeof(wheel_acc[0]));
      postResetImmediate(false);
      return;
    }
  }
}

void RayCar::checkIdle()
{
  CarEngineModel *eng = rcar->getEngine();
  ICarController *ctrl = driver ? driver->getController() : NULL;
  if ((ctrl && (ctrl->getGasFactor() > 1e-3f /*|| ctrl->getBrakeFactor()>1e-3f*/)) ||
      (eng->getRPM() > eng->getIdleRPM() * 0.95f + eng->getMaxRPM() * 0.05f) ||
      (lengthSq(getPhysBody()->getVelocity()) > 1.0f / 3.6f) || (lengthSq(getPhysBody()->getAngularVelocity()) > 0.1f))
  {
    idle = false;
  }
  else
  {
    TMatrix tm;
    getTm(tm);
    if (tm.getcol(1).y < cosf(15.0f * DEG_TO_RAD))
      idle = false;
    else
    {
      float load = 0, loadSum = 0.0f;
      for (int i = 0; i < wheels.size(); i++)
      {
        if (!vehicle->getWheelContact(i))
        {
          idle = false;
          return;
        }
        else
        {
          const float f = wheels[i].normForce;
          load += f;
          loadSum += fabsf(f);
        }
      }

      if (!idle)
      {
        if (loadSum < 1e-6f)
          idle = true;
        else
          idle = fabsf(carMass - load / 9.81f) < 4;
      }
      // if ( !idle )
      // debug ( "carMass=%.3f load=%.3f", carMass, load/9.81 );
    }
  }
}
void RayCar::checkQuasiIdle()
{
  if (idle)
  {
    quasiIdle = true;
    return;
  }
  if (rcar->getDriveline()->getClutchApp() > 0.001 || lengthSq(getVel()) > (3 * 3 / 3.6 / 3.6))
  {
    quasiIdle = false;
    return;
  }

  float load = 0;
  for (int i = 0; i < wheels.size(); i++)
    if (!vehicle->getWheelContact(i))
    {
      quasiIdle = false;
      return;
    }
    else
      load += wheels[i].normForce;

  if (!quasiIdle)
    quasiIdle = fabs(load / 9.81 / carMass - cachedTm.getcol(1).y) < 0.002;
}

void RayCar::updateBeforeSimulate(float dt)
{
  wasBodyCollided = collidedDuringUpdate;
  collidedDuringUpdate = false;
  usedAssistsWord = usedAssistsDuringUpdateWord;
  usedAssistsDuringUpdateWord = 0;
}

void RayCar::updateSteerAssist(float dt)
{
  const bool updateSteer = cachedTm.getcol(1).y > 0.5f;
  const bool lastSteer = lastSteerEnabled;
  lastSteerEnabled = updateSteer;
  if (!updateSteer)
    return;
  else if (!lastSteer)
    resetTargetCtrlDir(true, 0);

  // compute real carAng/velAng
  float x = cachedTm.getcol(2).x * 0.95 + getVel().x * 0.1;
  float z = cachedTm.getcol(2).z * 0.95 + getVel().z * 0.1;
  if (x * x + z * z < 1e-4)
    rdVelAng = 0;
  else
    rdVelAng = atan2f(z, x);

  x = cachedTm.getcol(2).x;
  z = cachedTm.getcol(2).z;
  if (x * x + z * z < 1e-4)
    rdCarAng = 0;
  else
    rdCarAng = atan2f(z, x);

  // help car rotate
  if (tcdDirWt > 0)
  {
    float sa = sinf(tcdVelAng + tcdCarDevAng), ca = cosf(tcdVelAng + tcdCarDevAng);
    Point3 targetCtrlDir(ca, 0, sa);

    float tcdx = targetCtrlDir * cachedTm.getcol(2);
    float tcdz = targetCtrlDir * cachedTm.getcol(0);
    float dl = sqrt(tcdx * tcdx + tcdz * tcdz);

    if (dl > 1e-4)
    {
      float wt = tcdDirWt, iwt = 1 - wt;
      Point2 new_dir_xz = normalize(Point2(tcdx / dl * wt + 1 * iwt, tcdz / dl * tcdDirWt + 0 * iwt));

      Point3 new_dir = cachedTm.getcol(2) * new_dir_xz.x + cachedTm.getcol(0) * new_dir_xz.y;
      cachedTm.setcol(2, new_dir);
      cachedTm.setcol(0, cachedTm.getcol(1) % new_dir);
      getPhysBody()->setTm(cachedTm * tmLogic2Phys);
      calcCached(false);
      usedAssistsWord |= 1 << CA_STEER;
    }
  }

  // help velocity steer
  if (tcdVelWt > 0)
  {
    float sa = sinf(tcdVelAng), ca = cosf(tcdVelAng);
    Point3 targetCtrlDir(ca, 0, sa);
    float tcdx = fsel(cachedDirectSpeed, targetCtrlDir.x, -targetCtrlDir.x);
    float tcdz = fsel(cachedDirectSpeed, targetCtrlDir.z, -targetCtrlDir.z);
    Point3 v = getPhysBody()->getVelocity();
    float vl = sqrt(v.x * v.x + v.z * v.z);
    if (vl > 1.0)
    {
      float wt = tcdDirWt, iwt = 1 - wt;
      Point2 new_vel_xz = normalize(Point2(tcdx * wt + v.x / vl * iwt, tcdz * wt + v.z / vl * iwt));
      vl = fabs(new_vel_xz * Point2(v.x, v.z));
      getPhysBody()->setVelocity(Point3(new_vel_xz.x * vl, v.y, new_vel_xz.y * vl));
      usedAssistsWord |= 1 << CA_STEER;
    }
  }
}

void RayCar::updateAfterSimulate(float dt, float at_time)
{
  TMatrix oldTm;
  if (carPhysMode == CPM_ACTIVE_PHYS)
    oldTm = cachedTm;

  calcCached();

  // update cached values
  cachedDirectSpeed = getVel() * cachedTm.getcol(2);

  if (carPhysMode == CPM_DISABLED)
    return;

  if (carPhysMode == CPM_ACTIVE_PHYS)
  {
    if (!idle && quasiIdle)
    {
      // try to eliminate side velocity
      Point3 vel = getPhysBody()->getVelocity();
      float side_vel = vel * cachedTm.getcol(0);
      vel -= side_vel * cachedTm.getcol(0);
      getPhysBody()->setVelocity(vel);
      // getPhysBody()->setAngularVelocity(Point3(0,0,0));
    }

    if (getPhysBody()->isActive())
      vehicle->update(dt);

    checkIdle();
    checkQuasiIdle();

    if (!idle && !getPhysBody()->isActive())
    {
      getPhysBody()->activateBody(true);
      vehicle->update(dt);
    }
    if (!idle && quasiIdle && lengthSq(getVel()) < 0.0025)
    {
      // velocity is sufficiently small, eliminate ANY motion
      if (lengthSq(getVel()) < 0.0025)
        setTm(oldTm);
    }
  }
  else
    idle = false;

  if (carPhysMode == CPM_ACTIVE_PHYS)
  {
    if (!driver || !driver->getController())
    {
      rcar->getEngine()->setThrottle(0);
      for (int i = 0; i < wheels.size(); i++)
      {
        vehicle->setWheelBrakeTorque(i, calcWheelBrakingTorque(i, 1.0f));

        if (wheels[i].controlled)
          vehicle->setWheelSteering(i, 0);
      }
    }
    else
    {
      updateGearShiftEnable();

      float resultingGasFactor = driver->getController()->getGasFactor();
      resultingBrakeFactor = driver->getController()->getBrakeFactor();

      /* this task is better handled by differential
      if (cachedDirectSpeed > 0.95*length(getVel()))
        for (int i=0; i<wheels.size(); i++)
          if (wheels[i].powered)
            if (wheels[i].lastSlipRatio*rcar->curSpeedKmh*resultingGasFactor > 30 )
              resultingGasFactor = wheels[i].lastSlipRatio*rcar->curSpeedKmh > 60
                ? 0 : 2-wheels[i].lastSlipRatio*rcar->curSpeedKmh/30;
      */

      rcar->getEngine()->setThrottle(int(resultingGasFactor * 1000));

      float wheelAng = ::clamp(driver->getController()->getSteeringAngle(), -params.maxWheelAng, params.maxWheelAng);

      TMatrix tm;
      phBody->getTm(tm);
      Point3 vel = phBody->getVelocity();
      // Point3 v = vel-(vel*tm.getcol(1))*tm.getcol(1);

      EspSensors &espSens = rcar->espSens;
      espSens.steerAngle = wheelAng;
      espSens.yawRate = ((phBody->getPointVelocity(tm * Point3(1, 0, 0)) - vel) % tm.getcol(0)) * tm.getcol(1);
      const float lsq = lengthSq(vel);
      espSens.yawAngle = lsq < 1e-4f ? 0.0f : ((vel / sqrtf(lsq)) % tm.getcol(0)) * tm.getcol(1);

      curWheelAng = wheelAng;
      for (int i = 0; i < wheels.size(); i++)
      {
        espSens.slipRatio[i] = wheels[i].lastSlipRatio;
        espSens.slipTanA[i] = wheels[i].lastSlipAstar;

        float brakeTorque = calcWheelBrakingTorque(i, resultingBrakeFactor);
        vehicle->setWheelBrakeTorque(i, brakeTorque);

        if (wheels[i].controlled)
        {
          float steer = -wheelAng * RAD_TO_DEG;
          if (params.ackermanAngle)
            if ((wheels[i].left && steer > 0) || (!wheels[i].left && steer < 0))
              steer += (steer * DEG_TO_RAD / params.maxWheelAng) * params.ackermanAngle;

          if (params.toeIn)
          {
            if (wheels[i].left)
              steer -= params.toeIn;
            else
              steer += params.toeIn;
          }

          vehicle->setWheelSteering(i, steer);
        }
      }

      if (!idle && enabledAssistsWord & (1 << CA_STEER))
        updateSteerAssist(dt);
    }

    updateSteerMz();
  }

  if (carPhysMode == CPM_INACTIVE_PHYS || carPhysMode == CPM_GHOST)
  {
    float directSpeed = getDirectSpeed();
    vehicle->updateInactiveVehicle(dt, directSpeed);
    for (int i = 0; i < wheels.size(); i++)
    {
      CarWheelState &w = *rcar->getWheelState(i);
      w.rotationV = directSpeed / w.getRad();
      //== ^ assume CarDynamicsModel and IPhysVehicle wheel rotation velocities are equal!
    }
  }

  if (carPhysMode == CPM_ACTIVE_PHYS)
  {
    if (idle && getPhysBody()->isActive())
      putToSleep(dt);

    if (!idle)
    {
      // limit velocity
      Point3 angVelocity = getPhysBody()->getAngularVelocity();
      if (length(angVelocity) > MAX_ANGULAR_VELOCITY)
        getPhysBody()->setAngularVelocity(normalize(angVelocity) * MAX_ANGULAR_VELOCITY);

      for (int i = 0; i < wheels.size(); i++)
        wheels[i].psiDt = dt;
    }
  }

  rcar->curSpeedKmh = length(getVel()) * 3.6;
  rcar->updateAfterSimulate(at_time);

  damping->update(this, dt);

  if (!isSimpleCar)
    updateFakeBodyFeedbackAngle(dt);

  // visually smooth wheel turns (useful for AI cars)
  float maxWhAngDelta = 600.0f * dt;
  for (int wid = 0, n = wheels.size(); wid < n; ++wid)
  {
    RayCarWheel &w = wheels[wid];
    if (w.controlled)
    {
      float ang = vehicle->getWheelSteering(wid);
      float d = ang - w.visualSteering;
      if (d > maxWhAngDelta)
        d = maxWhAngDelta;
      else if (d < -maxWhAngDelta)
        d = -maxWhAngDelta;
      w.visualSteering += d;
    }
  }
}

void RayCar::updateFakeBodyFeedbackAngle(float dt)
{
  float speedLimit = 40.0f / 3.6f;
  float rpmMin = 0.4f;
  float rpmPeak = 0.8f;
  float speed, speedFactor, rpmRatio, rpmFactor, newAng;

  if (carPhysMode != CPM_ACTIVE_PHYS || !driver)
    goto nofeedback;

  for (int i = 0; i < wheels.size(); i++)
    if (!vehicle->getWheelContact(i))
      goto nofeedback;

  speed = getVel().length();
  if (speed > speedLimit)
    goto nofeedback;

  speedFactor = 1.0f - speed / speedLimit;
  //  float gasFactor = driver->getController()->getGasFactor();
  //  float minGas = 0.7f;
  //  if (gasFactor < minGas)
  //    goto nofeedback;

  // float newAng = gfrnd()*4.0f*DEG_TO_RAD*speedFactor*(gasFactor-minGas)/(1.0f-minGas);

  rpmRatio = rcar->getEngine()->getRPM() / rcar->getEngine()->getMaxRPM();
  if (rpmRatio < rpmMin)
    goto nofeedback;

  rpmFactor = (rpmRatio < rpmPeak) ? ((rpmRatio - rpmMin) / (rpmPeak - rpmMin)) : (0.3f + 0.7f * (1.0f - rpmRatio) / (1.0f - rpmPeak));

  newAng = gfrnd() * 3.0f * DEG_TO_RAD * speedFactor * rpmFactor;
  fakeBodyEngineFeedbackAngle = approach(fakeBodyEngineFeedbackAngle, newAng, dt, 0.2f);
  return;

nofeedback:
  if (fakeBodyEngineFeedbackAngle > 1e-3f)
    fakeBodyEngineFeedbackAngle = approach(fakeBodyEngineFeedbackAngle, 0.0f, dt, 0.1f);
  else
    fakeBodyEngineFeedbackAngle = 0;
}

void RayCar::putToSleep(float dt)
{
  // fully put to sleep
  getPhysBody()->setVelocity(Point3(0, 0, 0));
  getPhysBody()->setAngularVelocity(Point3(0, 0, 0));
  for (int i = 0; i < wheels.size(); i++)
  {
    CarWheelState &w = *rcar->getWheelState(i);

    w.rotationV = 0;
    w.torqueFeedbackTC = 0;
    w.torqueBrakingTC = 0;
    wheels[i].resetDiffEq();
  }
  vehicle->updateInactiveVehicle(dt, 0);
  getPhysBody()->activateBody(false);
}

void RayCar::updateSteerMz()
{
  if (enableComputeSteerMz)
  {
    int swnum = 0;

    steerMz = 0;
    steerMzEqAngle = 0;
    for (int i = 0; i < wheels.size(); i++)
    {
      if (wheels[i].controlled && vehicle->getWheelContact(i))
      {
        steerMz += rcar->getWheelState(i)->pacejka.getMz();
        steerMzEqAngle += -atanf(rcar->getWheelState(i)->pacejka.getSlipAstar());
        swnum++;
      }
    }
    if (swnum)
      steerMzEqAngle = steerMzEqAngle / swnum;
    steerMzEqAngle += curWheelAng;

    if (check_nan(steerMzEqAngle))
      steerMzEqAngle = 0;
    if (check_nan(steerMz))
      steerMz = 0;
  }
  else
  {
    steerMz = curWheelAng * 30 * 2 / (4 * DEG_TO_RAD);
    steerMzEqAngle = 0;
  }
}

void RayCar::updateGearShiftEnable()
{
  bool en_shift_up = true;
  for (int i = 0; i < wheels.size(); i++)
  {
    if (wheels[i].powered && rcar->getGearbox()->getCurGear() > CarGearBox::GEAR_1)
    {
      if (fabsf(wheels[i].lastSlipRatio) > 4)
      {
        en_shift_up = false;
        break;
      }
      else
      {
        float lin_vel = rcar->getWheelState(i)->rotationV * rcar->getWheelState(i)->getRad();
        if ((cachedDirectSpeed >= 0 && lin_vel > cachedDirectSpeed * 2) || (cachedDirectSpeed < 0 && lin_vel < cachedDirectSpeed * 2))
        {
          en_shift_up = false;
          break;
        }
      }
    }
  }
  rcar->getGearbox()->enableShiftUp(en_shift_up);
}

Point3 RayCar::getWheelLocalPos(int wid) { return tmLogic2Phys * vehicle->getWheelData(wid)->getLocalPos(); }

Point3 RayCar::getWheelLocalBottomPos(int wid)
{
  return tmLogic2Phys * vehicle->getWheelData(wid)->getLocalPos() +
         Point3(0, -rcar->getWheelState(wid)->getRad(), 0) /*== not so great assumption...*/;
}

void RayCar::enableAssist(CarAssist type, bool en, float /*pwr*/)
{
  if (type == CA_STEER && en && !(enabledAssistsWord & (1 << CA_STEER)))
    resetTargetCtrlDir(true, 0);

  if (en)
    enabledAssistsWord |= (1 << type);
  else
    enabledAssistsWord &= ~(1 << type);
}
bool RayCar::isAssistEnabled(CarAssist type) { return (enabledAssistsWord & (1 << type)) != 0; }
bool RayCar::isAssistUsedLastAct(CarAssist type) { return usedAssistsWord & (1 << type); }

bool RayCar::resetTargetCtrlDir(bool reset_deviation_also, float align_thres_deg)
{
  if (!reset_deviation_also && fabs(norm_s_ang(rdVelAng - rdCarAng)) > align_thres_deg * PI / 180.0)
    return false;
  if (!reset_deviation_also && fabs(tcdCarDevAng) > PI * 2 / 180)
    return false;

  float x = cachedTm.getcol(2).x * 0.95 + getVel().x * 0.1;
  float z = cachedTm.getcol(2).z * 0.95 + getVel().z * 0.1;
  if (x * x + z * z < 1e-4)
    rdVelAng = tcdVelAng = 0;
  else
    rdVelAng = tcdVelAng = atan2f(z, x);

  x = cachedTm.getcol(2).x;
  z = cachedTm.getcol(2).z;
  if (x * x + z * z < 1e-4)
    rdCarAng = 0;
  else
    rdCarAng = atan2f(z, x);

  tcdCarDevAng = 0;
  return true;
}

#define MAX_ANG_DIF (0.44 * PI)
void RayCar::changeTargetCtrlDir(float rot_right_rad, float ret_rate)
{
  tcdVelAng -= rot_right_rad;
  float dif = norm_s_ang(tcdVelAng - rdVelAng);
  if (dif > MAX_ANG_DIF)
    tcdVelAng = norm_s_ang(rdVelAng + MAX_ANG_DIF);
  else if (dif < -MAX_ANG_DIF)
    tcdVelAng = norm_s_ang(rdVelAng - MAX_ANG_DIF);

  if (ret_rate > 0)
  {
    float inc = norm_s_ang(rdCarAng - tcdVelAng) * ret_rate;
    tcdVelAng = norm_s_ang(tcdVelAng + inc);
  }
}
void RayCar::changeTargetCtrlDirDeviation(float rot_right_rad, float return_rate, float align_wt)
{
  tcdCarDevAng = tcdCarDevAng - rot_right_rad;
  float dif = norm_s_ang(tcdVelAng + tcdCarDevAng - rdCarAng);
  if (dif > MAX_ANG_DIF)
    tcdCarDevAng = norm_s_ang(norm_s_ang(rdCarAng + MAX_ANG_DIF) - tcdVelAng);
  else if (dif < -MAX_ANG_DIF)
    tcdCarDevAng = norm_s_ang(norm_s_ang(rdCarAng - MAX_ANG_DIF) - tcdVelAng);

  if (return_rate > 0)
  {
    float inc = tcdCarDevAng * return_rate;
    tcdCarDevAng -= inc;
    RayCar::changeTargetCtrlDir(-inc * (1 - align_wt), 0);
  }
}
#undef MAX_ANG_DIF

void RayCar::changeTargetCtrlDirWt(float wt, float change_strength)
{
  tcdDirWt = wt * change_strength + tcdDirWt * (1 - change_strength);
  tcdDirWt = fsel(tcdDirWt, fsel(tcdDirWt - 1, 1, tcdDirWt), 0);
}
void RayCar::changeTargetCtrlVelWt(float wt, float change_strength)
{
  tcdVelWt = wt * change_strength + tcdVelWt * (1 - change_strength);
  tcdVelWt = fsel(tcdVelWt, fsel(tcdVelWt - 1, 1, tcdVelWt), 0);
}

#ifndef NO_3D_GFX

void RayCar::calcWheelRenderTm(TMatrix &tm, int wid, const TMatrix &car_visual_tm)
{
  RayCarWheel &wheel = wheels[wid];

  IPhysVehicle::IWheelData *wd = vehicle->getWheelData(wid);
  tm = rotyTM(HALFPI) * rotzTM(wd->getAxisRotAng());

  if (!wheel.left)
  {
    tm.setcol(2, -tm.getcol(2));
    tm.setcol(1, -tm.getcol(1));
  }

  if (wheel.controlled)
    tm = rotyTM(wheel.visualSteering * DEG_TO_RAD) * tm;

  tm.setcol(0, -tm.getcol(0));

  tm = car_visual_tm * tm;
  tm.setcol(3, car_visual_tm * tmLogic2Phys * wd->getLocalPos());
}

void RayCar::setWheelRenderTm(int wid, const TMatrix &car_visual_tm)
{
  RayCarWheel &wheel = wheels[wid];

  if (!wheel.rendObj)
    return;

  TMatrix tm;
  calcWheelRenderTm(tm, wid, car_visual_tm);
  wheel.rendObj->setNodeWtm(0, tm);
  if (wheel.brakeId < 0)
    return;

  TMatrix brakeTm = rotyTM(HALFPI);
  IPhysVehicle::IWheelData *wd = vehicle->getWheelData(wid);
  if (!wheel.left)
  {
    brakeTm.setcol(2, -brakeTm.getcol(2));
    brakeTm.setcol(1, -brakeTm.getcol(1));
    if (wheel.front)
      brakeTm *= rotzTM(-PI / 3);
  }
  else
  {
    brakeTm *= rotzTM(PI);
    if (wheel.front)
      brakeTm *= rotzTM(PI / 3);
  }

  if (wheel.controlled)
    brakeTm = rotyTM(wheel.visualSteering * DEG_TO_RAD) * brakeTm;

  brakeTm.setcol(0, -brakeTm.getcol(0));

  brakeTm = car_visual_tm * brakeTm;
  brakeTm.setcol(3, car_visual_tm * tmLogic2Phys * wd->getLocalPos());
  wheel.rendObj->setNodeWtm(wheel.brakeId, brakeTm);
}

void RayCar::getVisualTm(TMatrix &tm)
{
  tm = externalDraw ? externalTm : cachedTm;

  if (!externalDraw && fakeShakeTm && carPhysMode == CPM_ACTIVE_PHYS && fakeBodyEngineFeedbackAngle > 1e-3f)
  {
    tm = tm * rotxTM(-fakeBodyEngineFeedbackAngle); //== probably it's better to do something like tm = *fakeShakeTm * tmPhys2Logic
  }
}

void RayCar::renderDebug()
{
#if DAGOR_DBGLEVEL > 0
  static E3DCOLOR colors[4] = {E3DCOLOR(255, 255, 255), E3DCOLOR(255, 255, 0), E3DCOLOR(255, 0, 255), E3DCOLOR(255, 0, 0)};
  vehicle->debugRender();
  ::begin_draw_cached_debug_lines(false);
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);
  if (enabledAssistsWord & (1 << CA_STEER))
  {
    Point3 tcdCar(cosf(tcdVelAng + tcdCarDevAng), 0, sinf(tcdVelAng + tcdCarDevAng));
    Point3 tcdVel(cosf(tcdVelAng), 0, sinf(tcdVelAng));
    ::draw_cached_debug_line(getPos(), getPos() + tcdVel * 5, E3DCOLOR(255, 255, 255, 255));
    ::draw_cached_debug_line(getPos(), getPos() + tcdCar * 3, E3DCOLOR(0, 0, 255, 255));
  }

#if DEBUG_CAR_PHYS_MODE
  TMatrix tm;
  getTm(tm);
  BSphere3 bs = tm * bsphere;
  ::draw_cached_debug_sphere(bs.c, bs.r, colors[carPhysMode]);
  //::draw_cached_debug_box(bbox, tm, E3DCOLOR(255,255,0));
#endif
  ::end_draw_cached_debug_lines();
#endif
}

#endif // NO_3D_GFX


Point3 RayCar::getPos()
{
  /*TMatrix tm;
  getTm(tm);
  return tm.getcol(3);*/
  return *(Point3 *)cachedTm[3]; // less accurate in OOP way, but faster
}

Point3 RayCar::getVel()
{
  if (carPhysMode == CPM_ACTIVE_PHYS || carPhysMode == CPM_INACTIVE_PHYS)
    return getPhysBody()->getVelocity();
  else
    return virtualCarVel;
}

Point3 RayCar::getAngVel() { return getPhysBody()->getAngularVelocity(); }

void RayCar::setVel(const Point3 &vel)
{
  if (carPhysMode == CPM_GHOST || carPhysMode == CPM_DISABLED)
    virtualCarVel = vel;
  else
  {
    RayCarDelayedOp &op = dcar_op.push_back();
    op.car = this;
    op.op = RayCarDelayedOp::OP_SET_VEL;
    op.index = dcar_op_data.size();
    dcar_op_data.push_back(vel);
  }
}

void RayCar::setAngVel(const Point3 &wvel)
{
  if (carPhysMode == CPM_GHOST || carPhysMode == CPM_DISABLED)
    ; // no-op
  else
  {
    RayCarDelayedOp &op = dcar_op.push_back();
    op.car = this;
    op.op = RayCarDelayedOp::OP_SET_ANG_VEL;
    op.index = dcar_op_data.size();
    dcar_op_data.push_back(wvel);
  }
}

void RayCar::calcCached(bool updateTm)
{
  if (updateTm)
  {
    if (carPhysMode < CPM_GHOST)
    {
      TMatrix tm;
      getPhysBody()->getTm(tm);
      cachedTm = tm * tmPhys2Logic;
    }
    else
      cachedTm = virtualCarTm;
  }
  if (!isSimpleCar)
  {
    cachedITm = inverse(cachedTm);
    cachedCapsule = capsule;
    cachedCapsule.transform(cachedTm);
    cachedBbox = cachedTm * bbox;
    cachedBsphere = cachedTm * bsphere;
  }
  else
    cachedFlags = 0;
}

void RayCar::getTm(TMatrix &tm) { tm = cachedTm; }

void RayCar::getTm44(TMatrix4 &tm) { tm = cachedTm; }

void RayCar::getITm(TMatrix &itm)
{
  if (isSimpleCar && !(cachedFlags & CF_ITM_VALID))
  {
    cachedITm = inverse(cachedTm);
    cachedFlags |= CF_ITM_VALID;
  }
  itm = cachedITm;
}

void RayCar::setMass(real m)
{
  float ix, iy, iz;
  float curmass;
  phBody->getMassMatrix(curmass, ix, iy, iz);
  phBody->setMassMatrix(m, ix * m / curmass, iy * m / curmass, iz * m / curmass);
  carMass = m;
}

void RayCar::applyPhysParams(const PhysCarParams &p)
{
  //== TEMP, to be replaced with direct copy when all fields will be supported
  DataBlock blk;
  p.userSave(blk);
  params.userLoad(blk);

  for (int wid = 0; wid < wheels.size(); wid++)
    setupWheel(wid);
}

void RayCar::applyGearBoxParams(const PhysCarGearBoxParams &p) { rcar->getGearbox()->setParams(p); }

void RayCar::applyDifferentialParams(int idx, const PhysCarDifferentialParams &p)
{
  G_ASSERT(idx >= 0);
  G_ASSERT(idx < rcar->getDiffCount());
  CarDifferential *diff = rcar->getDiff(idx);
  diff->setParams(p);
  diff->resetData();
}

void RayCar::applyCenterDiffParams(const PhysCarCenterDiffParams &p)
{
  if (params.frontSusp.powered() && params.rearSusp.powered())
    rcar->getDriveline()->loadCenterDiff(p);
}

void RayCar::addImpulse(const Point3 &point, const Point3 &force_x_dt, IPhysCar *from)
{
  setCarPhysMode(IPhysCar::CPM_ACTIVE_PHYS);

  G_ASSERT(!dcar_op_processing);

  TMatrix itm;
  getITm(itm);

  Point3 relPt = itm * point;
  Point3 relImp = itm % force_x_dt;

  RayCarDelayedOp &op = dcar_op.push_back();
  op.car = this;
  op.op = RayCarDelayedOp::OP_ADD_IMPULSE;
  op.index = dcar_op_data.size();
  dcar_op_data.push_back(relPt);
  dcar_op_data.push_back(relImp);

  if (from && from != this)
  {
    // apply damage
    if (carCollisionCb)
      carCollisionCb->onCollideWithCar(this, from, point, normalize(force_x_dt), force_x_dt);
  }
}


int RayCar::wheelsOnGround()
{
  int count = 0;
  for (int i = 0; i < wheels.size(); i++)
    if (vehicle->getWheelContact(i))
      count++;

  return count;
}


int RayCar::wheelsSlide(float delta)
{
  int count = 0;
  for (int i = 0; i < wheels.size(); i++)
  {
    if (!vehicle->getWheelContact(i))
      continue;
    if (/*fabsf(wheels[i].vs.x) > 15.0f || */ fabsf(wheels[i].vs.y) > delta)
      count++;
  }

  return count;
}

real RayCar::wheelNormalForce(int id)
{
  G_ASSERT((id >= 0) && (id < wheels.size()));
  return wheels[id].normForce;
}

void RayCar::setTm(const TMatrix &tm)
{
  if (targetCarPhysMode == CPM_GHOST || targetCarPhysMode == CPM_DISABLED)
    virtualCarTm = tm;
  else
  {
    if (dcar_op_processing)
      getPhysBody()->setTm(tm * tmLogic2Phys);
    else
    {
      RayCarDelayedOp &op = dcar_op.push_back();
      op.car = this;
      op.op = RayCarDelayedOp::OP_SET_TM;
      op.index = append_items(dcar_op_data, 4);
      *(TMatrix *)&dcar_op_data[op.index] = tm * tmLogic2Phys;
    }
  }


  cachedTm = tm;
  calcCached(false);
}

Point3 RayCar::getDir()
{
  TMatrix tm;
  getTm(tm);
  return tm.getcol(2);
}

void RayCar::setNitro(float target_power) { rcar->getEngine()->setNitro(target_power); }

void RayCar::forceClutchApplication(float value) { rcar->getDriveline()->forceClutchApplication(value); }

void RayCar::physInactivate(bool set) { phBody->activateBody(!set); }


void RayCar::physDisable(bool set)
{
  bool alreadyDisabled = (carPhysMode == CPM_GHOST || carPhysMode == CPM_DISABLED);
  if (alreadyDisabled == set)
    return;

  int mask = set ? INACTIVE_CAR_BODY_INTERACT_MASK : ACTIVE_CAR_BODY_INTERACT_MASK;
  getPhysBody()->setGroupAndLayerMask(mask, mask);
  if (set)
    getTm(virtualCarTm);
  else
  {
    setTm(virtualCarTm);
    virtualCarVel.zero();
  }
}

void RayCar::setCarPhysMode(CarPhysMode mode)
{
  if (mode == targetCarPhysMode)
    return;

  G_ASSERT(mode == CPM_ACTIVE_PHYS || mode == CPM_INACTIVE_PHYS || mode == CPM_GHOST || mode == CPM_DISABLED);

  G_ASSERT(!dcar_op_processing);
  RayCarDelayedOp &op = dcar_op.push_back();
  op.car = this;
  op.op = mode;
  targetCarPhysMode = mode;
}

#if defined(USE_BULLET_PHYSICS)
void IPhysCar::applyCarPhysModeChangesBullet()
#elif defined(USE_JOLT_PHYSICS)
void IPhysCar::applyCarPhysModeChangesJolt()
#endif
{
  if (!dcar_op.size())
    return;

  dcar_op_processing = true;
  for (int i = 0; i < dcar_op.size(); i++)
  {
    const RayCarDelayedOp &op = dcar_op[i];

    switch (op.op)
    {
      case RayCarDelayedOp::OP_SET_ACTIVE:
        op.car->calcCached();
        op.car->physDisable(false);
        op.car->physInactivate(false);
        op.car->carPhysMode = op.car->targetCarPhysMode;
        break;

      case RayCarDelayedOp::OP_SET_INACTIVE:
        op.car->calcCached();
        op.car->physDisable(false);
        op.car->physInactivate(true);
        op.car->carPhysMode = op.car->targetCarPhysMode;
        op.car->damping->resetTimer();
        break;

      case RayCarDelayedOp::OP_SET_GHOST:
      case RayCarDelayedOp::OP_SET_DISABLED:
        op.car->physDisable(true);
        op.car->physInactivate(true);
        op.car->carPhysMode = op.car->targetCarPhysMode;
        op.car->damping->resetTimer();
        if (op.op == RayCarDelayedOp::OP_SET_DISABLED)
        {
          // op.car->isVisible = false;
          op.car->resetGears();
        }
        break;

      case RayCarDelayedOp::OP_ADD_IMPULSE:
        op.car->calcCached();
        op.car->getPhysBody()->addImpulse(op.car->cachedTm * dcar_op_data[op.index], op.car->cachedTm % dcar_op_data[op.index + 1]);
        break;

      case RayCarDelayedOp::OP_SET_DIRECT_SPEED:
        op.car->getPhysBody()->setVelocity(dcar_op_data[op.index], false);
        op.car->getPhysBody()->setAngularVelocity(Point3(0, 0, 0), false);
        op.car->vehicle->updateInactiveVehicle(0, length(dcar_op_data[op.index]));
        break;

      case RayCarDelayedOp::OP_SET_TM: op.car->getPhysBody()->setTm(*(TMatrix *)&dcar_op_data[op.index]); break;

      case RayCarDelayedOp::OP_SET_VEL: op.car->getPhysBody()->setVelocity(dcar_op_data[op.index]); break;

      case RayCarDelayedOp::OP_SET_ANG_VEL: op.car->getPhysBody()->setAngularVelocity(dcar_op_data[op.index]); break;

      case RayCarDelayedOp::OP_POST_RESET: op.car->postResetImmediate(); break;
    }
  }

  /*
  debug ( "dcar_op.dataSize=%d (%d max)   dcar_op_data.dataSize=%d (%d max)",
    data_size(dcar_op), dcar_op.capacity()*elem_size(dcar_op),
    data_size(dcar_op_data), dcar_op_data.capacity()*elem_size(dcar_op_data));
  */

  dcar_op.clear();
  dcar_op_data.clear();
  dcar_op_processing = false;
}

const Capsule &RayCar::getBoundCapsule()
{
  if (!isSimpleCar || (cachedFlags & CF_CAPSULE_VALID))
    return cachedCapsule;
  cachedCapsule = capsule;
  cachedCapsule.transform(cachedTm);
  cachedFlags |= CF_CAPSULE_VALID;
  return cachedCapsule;
}

BBox3 RayCar::getBoundBox()
{
  if (!isSimpleCar || (cachedFlags & CF_BBOX_VALID))
    return cachedBbox;
  cachedBbox = cachedTm * bbox;
  cachedFlags |= CF_BBOX_VALID;
  return cachedBbox;
}

BBox3 RayCar::getLocalBBox() { return bbox; }

BSphere3 RayCar::getBoundingSphere()
{
  if (!isSimpleCar || (cachedFlags & CF_BSPHERE_VALID))
    return cachedBsphere;
  cachedBsphere = cachedTm * bsphere;
  cachedFlags |= CF_BSPHERE_VALID;
  return cachedBsphere;
}

Point3 RayCar::getMassCenter()
{
  TMatrix tm;
  phBody->getTm(tm);
  return tm.getcol(3);
}

void RayCar::setDirectSpeed(real speed)
{
  if (carPhysMode == CPM_INACTIVE_PHYS || carPhysMode == CPM_ACTIVE_PHYS)
  {
    G_ASSERT(!dcar_op_processing);
    RayCarDelayedOp &op = dcar_op.push_back();
    op.car = this;
    op.op = RayCarDelayedOp::OP_SET_DIRECT_SPEED;
    op.index = dcar_op_data.size();
    dcar_op_data.push_back(getDir() * speed);
  }
  else
    virtualCarVel = getDir() * speed;
}

void RayCar::disableCollisionDamping(real for_time) { damping->disable(for_time); }

void RayCar::disableCollisionDampingFull(bool set) { damping->disableFull(set); }

void RayCar::setGearStep(int step, bool override_switch_in_progress)
{
  CarGearBox *gbox = rcar->getGearbox();
  if (step < 0)
    gbox->requestGear(CarGearBox::GEAR_REVERSE, override_switch_in_progress);
  else if (step == 0)
    gbox->requestGear(CarGearBox::GEAR_NEUTRAL, override_switch_in_progress);
  else
  {
    int reqGear = ::min(CarGearBox::GEAR_1 + step - 1, gbox->getNumGears() - 1);
    gbox->requestGear(reqGear, override_switch_in_progress);
  }
}

int RayCar::getGearStep()
{
  int step = rcar->getGearbox()->getCurGear();
  if (step == CarGearBox::GEAR_REVERSE)
    return -1;
  if (step == CarGearBox::GEAR_NEUTRAL)
    return 0;
  return step - CarGearBox::GEAR_1 + 1;
}

real RayCar::getEngineRPM() { return rcar->getEngine()->getRPM(); }

real RayCar::getEngineIdleRPM() { return rcar->getEngine()->getIdleRPM(); }

real RayCar::getEngineMaxRPM() { return rcar->getEngine()->getMaxRPM(); }

void RayCar::setAutomaticGearBox(bool set) { rcar->getGearbox()->setAutomatic(set); }

bool RayCar::automaticGearBox() { return rcar->getGearbox()->isAutomatic(); }

void RayCar::onCollision(PhysBody *other, const Point3 &pt, const Point3 &norm, int this_matId, int other_matId,
  const Point3 &norm_imp, const Point3 &friction_imp, const Point3 &this_old_vel, const Point3 &other_old_vel)
{
  //---------------------------------------------------
  // apply damage
  collidedDuringUpdate = true;

  TMatrix itm;
  getITm(itm);

  if ((itm * pt).y > bbox[0].y + 0.05 || cachedTm.getcol(1) * norm < 0.707) // quick hack to prevent car bottom collisions
  {
    damping->onCollision(this, other, norm_imp);
    if (!other)
      resetTargetCtrlDir(false, 1);
  }

  if (carCollisionCb)
  {
    Point3 summaryImp = norm_imp + friction_imp;

    IPhysCar *otherCar = NULL;
    if (other)
    {
      CarAndPhBodyPair key = {NULL, other};
      int idx = ::car_phys_bodies.find(key);
      if (idx != -1)
        otherCar = ::car_phys_bodies[idx].car;
    }

    if (otherCar)
      carCollisionCb->onCollideWithCar(this, otherCar, pt, norm, summaryImp);
    else
      carCollisionCb->onCollideWithPhObj(this, other, pt, norm, summaryImp);
  }

  if (collisionFxCb)
  {
    Point3 vel;
    if (other)
      vel = getPhysBody()->getPointVelocity(pt) - other->getPointVelocity(pt);
    else
      vel = getPhysBody()->getPointVelocity(pt);
    float normVel = vel * norm;
    Point3 tangVel = vel - norm * normVel;

    float tangVelValue = length(tangVel);
    if ((fabsf(normVel) > 0.01f) || (fabsf(tangVelValue) > 0.01f))
      collisionFxCb->spawnCollisionFx(this, -1, pt, norm, tangVel, tangVelValue, normVel, this_matId, other_matId);
  }
}

void RayCar::setCarCollisionCB(ICarCollisionCB *cb) { carCollisionCb = cb; }

void RayCar::setCollisionFxCB(ICollideFxCB *cb) { collisionFxCb = cb; }

void RayCar::reloadCarParams()
{
  del_it(globalCarParamsBlk);

  del_it(rcar);
  rcar = new CarDynamicsModel(4);

  const DataBlock *b, &carsBlk = loadCarDataBlock(b), *eng_blk, *gb_blk;

  eng_blk = carsBlk.getBlockByNameEx("engines")->getBlockByName(b->getStr("engine", "standard"));
  gb_blk = carsBlk.getBlockByNameEx("gearboxes")->getBlockByName(b->getStr("gearbox", "standard"));
  if (!eng_blk)
    DAG_FATAL("can't load engine preset <%s>", b->getStr("engine", "standard"));
  if (!gb_blk)
    DAG_FATAL("can't load gearbox preset <%s>", b->getStr("gearbox", "standard"));

  PhysCarGearBoxParams gboxParams;
  gboxParams.load(gb_blk);
  rcar->load(eng_blk, gboxParams);

  steerPreset = b->getStr("steer", "default");

  postCreateSetup();

  loadParamsFromBlk(b, carsBlk);
  for (int wid = 0; wid < wheels.size(); wid++)
    setupWheel(wid);
  setWheelParamsPreset(frontWheelPreset, rearWheelPreset);
}

void RayCar::postCreateSetup()
{
  const DataBlock *b, &carsBlk = loadCarDataBlock(b);

  vehicle->setAxleParams(0, 1, params.frontSusp.rigidAxle(), params.frontSusp.arbK);
  vehicle->setAxleParams(2, 3, params.rearSusp.rigidAxle(), params.rearSusp.arbK);

  if (params.frontSusp.powered())
  {
    PhysCarDifferentialParams diffParams;
    diffParams.load(*b->getBlockByNameEx("diffFront"));
    rcar->addWheelDifferential(0, 1, diffParams);
  }
  if (params.rearSusp.powered())
  {
    PhysCarDifferentialParams diffParams;
    diffParams.load(*b->getBlockByNameEx("diffRear"));
    rcar->addWheelDifferential(2, 3, diffParams);
  }
  if (params.frontSusp.powered() && params.rearSusp.powered())
    rcar->getDriveline()->loadCenterDiff(*b->getBlockByNameEx("diffCenter"));

  resetGears();
  vehicle->resetWheels();
}

void RayCar::postReset()
{
  RayCarDelayedOp &op = dcar_op.push_back();
  op.car = this;
  op.op = RayCarDelayedOp::OP_POST_RESET;
}

void RayCar::postResetImmediate(bool full)
{
  if (carPhysMode == IPhysCar::CPM_GHOST || carPhysMode == IPhysCar::CPM_DISABLED)
    return;

  idle = false;

  getPhysBody()->activateBody(true);
  if (full)
  {
    getPhysBody()->setAngularVelocity(Point3(0, 0, 0));
    getPhysBody()->setVelocity(Point3(0, 0, 0));
    if (carPhysMode != CPM_INACTIVE_PHYS && carPhysMode != CPM_ACTIVE_PHYS)
      virtualCarVel = Point3(0, 0, 0);
  }

  resetGears();
  vehicle->resetWheels();
  for (int iWh = 0; iWh < wheels.size(); iWh++)
  {
    wheels[iWh].visible = true;
    wheels[iWh].visualSteering = 0;
  }

  calcCached();
  resetTargetCtrlDir(true, 0);
}

void RayCar::resetGears()
{
  rcar->getEngine()->resetData();
  rcar->getGearbox()->resetData();
  rcar->getDriveline()->resetData();
  for (int i = 0; i < rcar->getDiffCount(); i++)
    rcar->getDiff(i)->resetData();

  for (int i = 0; i < wheels.size(); i++)
  {
    CarWheelState *rw = rcar->getWheelState(i);
    rw->rotationV = 0;
    rw->torqueBrakingTC = 0;
    rw->torqueFeedbackTC = 0;

    wheels[i].resetDiffEq();
  }
  steerMz = 0;
}

void RayCar::calcEngineMaxPower(float &max_hp, float &rpm_max_hp, float &max_torque, float &rpm_max_torque)
{
  rcar->getEngine()->calcMaxPower(max_hp, rpm_max_hp, max_torque, rpm_max_torque);
}

RayCar::WheelDriveType RayCar::getWheelDriveType()
{
  int res = 0;
  G_ASSERT(wheels.size() == 4);
  G_ASSERT(wheels[0].front);
  G_ASSERT(!wheels[2].front);
  if (wheels[0].powered)
    res |= WDT_FRONT;
  if (wheels[2].powered)
    res |= WDT_REAR;
  return WheelDriveType(res);
}

void RayCar::freeVinylTex()
{
  release_managed_tex(vinylTexId); // car reference
  vinylTexId = BAD_TEXTUREID;
}

void RayCar::setVinylTex(TEXTUREID tex_id, bool manual_delete)
{
  if (tex_id == vinylTexId)
  {
    G_ASSERT(manual_delete == vinylTexManualDelete);
    return;
  }

  if (tex_id != BAD_TEXTUREID)
    acquire_managed_tex(tex_id);

  freeVinylTex();

  vinylTexId = tex_id;
  vinylTexManualDelete = manual_delete;
}

void RayCar::freePlanarVinylTex()
{
  release_managed_tex(planarVinylTexId); // referenced for 'tex' var
  planarVinylTexId = BAD_TEXTUREID;
}


void RayCar::setPlanarVinylTex(TEXTUREID tex_id, bool manual_delete)
{
  if (tex_id == planarVinylTexId)
  {
    G_ASSERT(tex_id == BAD_TEXTUREID || manual_delete == planarVinylTexManualDelete);
    return;
  }

  if (tex_id != BAD_TEXTUREID)
    acquire_managed_tex(tex_id);

  freePlanarVinylTex();

  planarVinylTexId = tex_id;
  planarVinylTexManualDelete = manual_delete;
}

const DataBlock &RayCar::loadCarDataBlock(const DataBlock *&car_blk)
{
  return physcarprivate::load_car_params_data_block(carName, car_blk);
}

void RayCar::loadParamsFromBlk(const DataBlock *b, const DataBlock &carsBlk)
{
  const DataBlock *paramsDef = carsBlk.getBlockByNameEx("params_default");

  params.load(b, carsBlk);

  if (b->getBool("preciseAD", false))
  {
    float adsq = b->getReal("airDragSq", 0.1);
    float ad = b->getReal("airDrag", adsq * 30.0);

    float adsq_s = b->getReal("sideAirDragSq", adsq * 4);
    float ad_s = b->getReal("sideAirDrag", adsq_s * 30.0);

    vehicle->setAirDrag(ad, adsq);
    vehicle->setSideAirDrag(ad_s, adsq_s);
  }
  else
  {
    Point3 w = bbox.width();
    float adq = b->getReal("airDragQuality", 1.0);
    float adsq = adq * (w.x * w.y) * 0.116115;
    float adsq_s = adq * (w.y * w.z) * 0.116115;

    vehicle->setAirDrag(adsq * 30, adsq);
    vehicle->setSideAirDrag(adsq_s * 40, adsq_s);
  }

  float alsq = b->getReal("airLiftSq", -0.6);
  float al = b->getReal("airLift", 0.0);
  float alsq_s = b->getReal("sideAirLiftSq", alsq);
  float al_s = b->getReal("sideAirLift", al);
  float allim = b->getReal("airLiftMax", getMass() * 9.8 * 0.3);
  vehicle->setAirLift(al, alsq, allim);
  vehicle->setSideAirLift(al_s, alsq_s);
  vehicle->setSideAirLift(alsq_s * 10, alsq_s);

  damping->loadParams(b->getBlockByNameEx("damping"), paramsDef->getBlockByNameEx("damping"));

  String *wheelPresets[2] = {&frontWheelPreset, &rearWheelPreset};
  get_raywheel_car_def_wheels(carName, wheelPresets);
}

void RayCar::loadWheelsParams(const DataBlock &blk, bool front)
{
  for (int iWheel = 0; iWheel < rcar->getNumWheels(); iWheel++)
    if (wheels[iWheel].front == front)
      rcar->getWheelState(iWheel)->pacejka.load(&blk, wheels[iWheel].left);
}

void RayCar::setWheelParamsPreset(const char *front_tyre, const char *rear_tyre)
{
  if (frontWheelPreset.str() != front_tyre)
    frontWheelPreset = front_tyre;
  if (rearWheelPreset.str() != rear_tyre)
    rearWheelPreset = rear_tyre ? rear_tyre : front_tyre;

  const DataBlock *b, &carsBlk = loadCarDataBlock(b);

  const DataBlock *pacejkaBlk;

  pacejkaBlk = carsBlk.getBlockByNameEx("tires")->getBlockByName(frontWheelPreset);
  if (!pacejkaBlk)
    DAG_FATAL("Pacejka params '%s' not found (front wheels)", frontWheelPreset.str());
  else
    debug("load Pacejka params %s (front wheels)", frontWheelPreset.str());
  loadWheelsParams(*pacejkaBlk, true);

  pacejkaBlk = carsBlk.getBlockByNameEx("tires")->getBlockByName(rearWheelPreset);
  if (!pacejkaBlk)
    DAG_FATAL("Pacejka params '%s' not found (rear wheels)", rearWheelPreset.str());
  else
    debug("load Pacejka params %s (rear wheels)", rearWheelPreset.str());
  loadWheelsParams(*pacejkaBlk, false);
}

const char *RayCar::getWheelParamsPreset(bool front) { return front ? frontWheelPreset : rearWheelPreset; }

bool RayCar::traceRay(const Point3 &p, const Point3 &dir, float &mint, Point3 *normal, int &pmid)
{
  if (!getPhysBody())
    return false;

  if (carPhysMode == CPM_GHOST || carPhysMode == CPM_DISABLED)
  {
    debug("can't trace - car disabled");
    return false;
  }

  return false;
}

void RayCar::onWheelContact(int wheel_id, int pmid, const Point3 &pos, const Point3 &norm, const Point3 &move_dir,
  const Point3 &wheel_dir, float slip_speed)
{
  G_ASSERT(wheel_id >= 0);
  G_ASSERT(wheel_id < wheels.size());


  const PhysMat::InteractProps &interactProps = PhysMat::getInteractProps(pmid, wheelPhysMatId);

#ifndef NO_3D_GFX
  if (wheels[wheel_id].tireEmitter)
  {
    for (int fxId = 0; fxId < interactProps.fx.size(); fxId++)
    {
      PhysMat::FXDesc *desc = interactProps.fx[fxId];
      if (PhysMat::PMFX_TIRES == desc->type)
      {
        float frict = globalFrictionMul * (1.0f - globalFrictionVDecay); //== need explicit road moisture factor?
        if (frict > desc->params.getReal(MAKE_PARAM('fric'), -1))
        {
          int texIndex = desc->params.getInt(MAKE_PARAM('tid '), 0);
          float tireSlipSpeed = fabsf(wheels[wheel_id].lastSlipRatio);
          if (tireSlipSpeed < 0.2)
            tireSlipSpeed = 0;
          else
            tireSlipSpeed = 2.0 + (tireSlipSpeed - 0.2) / 0.4 * 2.0;

          // wheels[wheel_id].tireEmitter->emit(norm, pos, move_dir, wheel_dir,
          //   lighting.getDirLt().shAmb[SPHHARM_00],
          //   tireSlipSpeed + fabs(wheels[wheel_id].lastSlipLatVel)/4.0, texIndex);
        }
      }
    }
  }
#endif

  if (collisionFxCb)
  {
    Point3 moveVel = getPhysBody()->getPointVelocity(pos);
    Point3 tangVel = moveVel - norm * (moveVel * norm);
    collisionFxCb->spawnCollisionFx(this, wheel_id, pos, norm, tangVel, slip_speed, norm * moveVel, wheelPhysMatId, pmid);
  }
}
