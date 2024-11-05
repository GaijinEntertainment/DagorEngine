// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/carDynamics/carModel.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_graphStat.h>
#include <math/dag_mathUtils.h>

// #include <debug/dag_debug.h>

// Clutch must be smoothed out a lot when applying starts
#define DEFAULT_CLUTCH_LINEARITY 0.98f

// Threshold at which force an unlock of engine and gearbox.
// This can happen if the driver just throws in a gear,
// without clutching (and if autoclutch is off).
// We should penalize this with a gear scratch sound and
// damage.
#define DELTA_VEL_THRESHOLD 2.0f

// static int gid_clutch_app = -1, gid_eng_rpm = -1, gid_diff_rpm = -1;

CarDrivelineElement::CarDrivelineElement(CarDriveline *dl)
{
  // Init member variables
  parent = NULL;
  child[0] = child[1] = NULL;
  driveline = dl;

  inertia = 0.01f;
  ratio = invRatio = 1;
  effectiveInertiaDownStream = 0;
  cumulativeRatio = 0;
  tReaction = tBraking = 0;
  rotV = rotA = 0;
}

CarDrivelineElement::~CarDrivelineElement() {}

// Set a parent for this component. Mostly, this is done implicitly
// using addChild().
void CarDrivelineElement::setParent(CarDrivelineElement *_parent) { parent = _parent; }

void CarDrivelineElement::setRatio(float r)
{
  if (fabs(r) < 1e-5f)
    r = 1;
  ratio = r;

  // Precalculate inverse ratio for faster calculations later
  invRatio = 1.0f / r;
}

// Reset variables for a fresh start (like when Shift-R is used)
void CarDrivelineElement::resetData() { rotV = rotA = 0; }

CarDrivelineElement *CarDrivelineElement::getChild(int n)
{
  G_ASSERT(n == 0 || n == 1);
  return child[n];
}

void CarDrivelineElement::addChild(CarDrivelineElement *comp)
{
  if (!child[0])
    child[0] = comp;
  else if (!child[1])
    child[1] = comp;
  else
    // DAG_FATAL("component already has 2 children");
    G_ASSERT(!"component already has 2 children");

  // Declare us as parent of the child
  comp->setParent(this);
}


/*****************
 * Precalculation *
 *****************/

// Calculate the total effective inertia for this node plus the children
void CarDrivelineElement::calcEffInertia()
{
  effectiveInertiaDownStream = inertia * cumulativeRatio * cumulativeRatio;
  if (child[0])
  {
    child[0]->calcEffInertia();
    effectiveInertiaDownStream += child[0]->getEffInertia();
  }
  if (child[1])
  {
    child[1]->calcEffInertia();
    effectiveInertiaDownStream += child[1]->getEffInertia();
  }
}

// Calculate the cumulative ratio for this component and entire tree
void CarDrivelineElement::calcFullRatio()
{
  cumulativeRatio = ratio;
  if (child[0])
    child[0]->calcFullRatio();
  if (child[1])
    child[1]->calcFullRatio();

  // Take ratio if child[0] only, since only singular links are supported
  // (like engine->gear->driveshaft)
  if (child[0])
    cumulativeRatio *= child[0]->getFullRatio();
  // cumulativeRatio*=child[0]->getRatio();
}

// Calculate reaction forces from the leaves up
void CarDrivelineElement::calcReactionForces()
{
  tReaction = tBraking = 0;
  if (child[0])
  {
    child[0]->calcReactionForces();
    tReaction += child[0]->getReactionTorque();
    tBraking += child[0]->getBrakingTorque();
  }
  if (child[1])
  {
    child[1]->calcReactionForces();
    tReaction += child[1]->getReactionTorque();
    tBraking += child[1]->getBrakingTorque();
  }
  // Multiply by the inverse ratio for upstream
  tReaction *= invRatio;
  tBraking *= invRatio;
}

// Calculate the velocity that should be in this component, by
// following the tree downward
float CarDrivelineElement::calcRotVel()
{
  float r;

  if (child[0])
  {
    if (child[1]) // Average velocity of both children (diff probably)
      r = (child[0]->calcRotVel() + child[1]->calcRotVel()) / 2;
    else
      r = child[0]->calcRotVel();
  }
  else
  {
    // Leaf; must be a wheel. The leaf defines the actual velocity
    // that is expected.
    r = rotV;
  }
  // Return velocity, take care of ratio
  return r * ratio;
  // return r;
}

void CarDrivelineElement::calcForces() {}

void CarDrivelineElement::calcAcc() {}

void CarDrivelineElement::integrate(float dt)
{
  rotV += rotA * dt;
  rotA = 0;
}

/************
 * Driveline *
 ************/
CarDriveline::CarDriveline(CarDynamicsModel *_car)
{
  // Init members
  car = _car;
  root = NULL;
  gearbox = nullptr;
  autoClutch = false;
  preClutchInertia = postClutchInertia = totalInertia = 0;
  prepostLocked = true;
  centerDiffPowerRatio = 0.5f;
  centerDiffTorque = 0;

  //  gid_clutch_app = Stat3D::addGraphic("clutch_app", "clutch_app", E3DCOLOR(255,0,0), 1.1, -0.1);
  //  gid_eng_rpm = Stat3D::addGraphic("eng_rpm", "eng_rpm", E3DCOLOR(255,0,255), 6000, -0.1);
  //  gid_diff_rpm = Stat3D::addGraphic("diff_rpm", "diff_rpm", E3DCOLOR(0,0,255), 6000, -0.1);

  // Clutch defaults
  clutchMaxTorque = 100;
  clutchLinearity = DEFAULT_CLUTCH_LINEARITY;
  forcedClutchAppValue = -1;
  setClutchApp(1);
  tClutch = 0;
}

CarDriveline::~CarDriveline() {}

void CarDriveline::setEngine(CarEngineModel *comp) { root = comp; }

bool CarDriveline::load(const DataBlock *blk)
{
  clutchMaxTorque = blk->getReal("clutchMaxTorque", 0);
  return true;
}

void CarDriveline::loadCenterDiff(const DataBlock &blk)
{
  centerDiff.load(blk);
  centerDiffPowerRatio = centerDiff.fixedRatio;
  centerDiffTorque = 0;
}

void CarDriveline::loadCenterDiff(const PhysCarCenterDiffParams &src)
{
  centerDiff = src;
  centerDiffPowerRatio = centerDiff.fixedRatio;
  centerDiffTorque = 0;
}

void CarDriveline::resetData()
{
  autoClutch = false;
  prepostLocked = false;
  tClutch = 0;
  clutchCurrentTorque = 0;
  clutchApplication = 0;
  clutchApplicationLinear = 0;
}

/*****************
 * Precalculation *
 *****************/
// Calculate the resulting spin factors (ratios) throughout the driveline
void CarDriveline::calcFullRatios()
{
  if (root)
    root->calcFullRatio();
}

// Calculate all effective inertia in the driveline
void CarDriveline::calcEffInertia()
{
  if (root)
    root->calcEffInertia();
}

// Calculate all inertia before the clutch (engine)
void CarDriveline::calcInertiaBeforeClutch()
{
  // Engine should be the root
  if (!root)
    return;
  // Note that we take the inertia without adding the ratios, since
  // this will be used when the engine rotates at a different speed
  // from the rest of the drivetrain.
  preClutchInertia = root->getInertia();
}

// Calculate all inertia after the clutch (gearbox, diffCount, wheels)
void CarDriveline::calcInertiaAfterClutch()
{
  // Check for a driveline to be present
  if (!root)
    return;
  postClutchInertia = root->getChild(0)->getEffInertia();

  // Also calculate total inertia
  totalInertia = preClutchInertia + postClutchInertia;
}

void CarDriveline::setClutchApp(float app)
{
  if (forcedClutchAppValue >= 0)
    return;

  if (app < 0)
    app = 0;
  else if (app > 1.0f)
    app = 1.0f;

  // Store linear clutch as well. The clutch flow is:
  // controls -> clutch -> engine (autoclutch if <autoClutchRPM) -> gearbox
  // For the gearbox to correct the clutch, the linear untouched value is used.
  clutchApplicationLinear = app;

  // Make it undergo severe non-linearity; I couldn't get
  // a smooth take-off without stalling the engine.
  app = clutchLinearity * app + (1.0f - clutchLinearity) * (app * app);
  clutchApplication = app;

#ifdef ND
  // Calculate clutch torque; for neutral gear, the clutch never generates torque.
  if (car->getGearbox() && car->getGearbox()->isNeutral())
    clutchCurrentTorque = 0;
  else
#endif
    // Calculate clutch torque
    clutchCurrentTorque = app * clutchMaxTorque;
}

// Determine forces throughout the driveline.
// Assumes the wheel reaction forces are already calculated.
void CarDriveline::calcForces(float dt)
{
  if (!root || !gearbox)
    return;

  // In neutral gear the pre and post parts are always unlocked,
  // and no clutch torque is applied.
  if (car->getGearbox()->isNeutral())
  {
    unlockClutch();
    // tClutch=0;
    clutchCurrentTorque = 0;
  }

  // Calculate current clutch torque (including torque direction)
  if (!isClutchLocked())
  {
    // Engine is running separately from the rest of the driveline.
    // The clutch works to make the velocity of pre-clutch (engine) and
    // post-clutch (rest) equal.
    if (root->getRotVel() > gearbox->getRotVel() * gearbox->getRatio())
    {
      tClutch = root->getEngineTorque() * 1.5 * clutchApplication;
      if (tClutch > clutchCurrentTorque)
        tClutch = clutchCurrentTorque;
      else if (tClutch < clutchCurrentTorque * 0.1 * root->getNitroMul())
        tClutch = clutchCurrentTorque * 0.1 * root->getNitroMul();
    }
    else
      tClutch = -clutchCurrentTorque;
  } // else acceleration will be given by the driveline (engine rotates
    // along with the rest of the driveline as a single assembly)

  // Spread wheel (leaf) reaction forces all the way to the engine (root)
  root->calcReactionForces();

  if (isClutchLocked())
  {
    // Check if pre and post are still locked
    float Te = root->getEngineTorque();
    float Tr = root->getReactionTorque();
    float Tb = root->getBrakingTorque();
    float Tc = clutchCurrentTorque;

    if (fabs(Te - (Tr + Tb)) > Tc)
      unlockClutch();
  } // else it will get locked again when velocity reversal happens
    // of the engine vs. rest of the drivetrain (=gearbox in angular velocity)

  G_ASSERT(car->getDiffCount() == 1 || car->getDiffCount() == 2);

  if (isClutchLocked())
    centerDiffTorque = root->getEngineTorque() * root->getFullRatio();
  else
  {
    // Engine spins at a different rate from the rest of the driveline
    // In this case the clutch fully works on getting the pre- and post-
    // clutch angular velocities equal.
    // Note that if the clutch is fully depressed (engine totally decoupled
    // from the rest of the driveline), this torque will be 0, and the
    // postclutch driveline will just rotate freely.
    centerDiffTorque = getClutchTorque() * root->getFullRatio();
  }

  for (int iDiff = 0; iDiff < car->getDiffCount(); iDiff++)
  {
    CarDifferential *diff = car->getDiff(iDiff);
    G_ASSERT(diff);

    float powerMul = 1.0f;
    if (car->getDiffCount() == 2)
      powerMul = (iDiff == 0) ? (centerDiffPowerRatio) : (1.0f - centerDiffPowerRatio);

    float appliedDiffTorque = centerDiffTorque * powerMul;
    if (!isClutchLocked() && appliedDiffTorque * sign(root->getFullRatio()) < 0.f)
    {
      // Clamp it by the amount of rotation clutch can make
      const float maxDeltaV = root->getRotVel() - gearbox->getRotVel() * gearbox->getRatio();
      const float maxClutchAcc = safediv(maxDeltaV, dt);
      const float maxClutchTorque = maxClutchAcc * root->getInertia();
      const float clutchTorqueSign = sign(maxClutchTorque);
      const float absClutchTorque = maxClutchTorque * clutchTorqueSign;
      if (appliedDiffTorque * clutchTorqueSign > absClutchTorque)
        appliedDiffTorque = maxClutchTorque;
    }
    diff->calc1DiffForces(dt, appliedDiffTorque, root->getEffInertia());
  }
}


// Calculate the accelerations of all the parts in the driveline
void CarDriveline::calcAcc()
{
  G_ASSERT(car->getDiffCount() == 1 || car->getDiffCount() == 2);

  float gearBoxAcc = 0;

  if (!isClutchLocked())
  {
    // Separate pre- and postclutch accelerations
    // First calculate the engine's acceleration.
    root->calcAcc();
  }

  for (int iDiff = 0; iDiff < car->getDiffCount(); iDiff++)
  {
    // Special case with speedier calculations
    CarDifferential *diff = car->getDiff(iDiff);
    float acc = diff->getAccIn();

    for (CarDrivelineElement *comp = diff; comp != gearbox; comp = comp->getParent())
    {
      comp->setRotAcc(acc);
      acc *= comp->getRatio();
    }

    if (car->getDiffCount() == 1)
      gearBoxAcc = acc;
    else if (iDiff == 0) // front
      gearBoxAcc = acc * centerDiffPowerRatio;
    else
      gearBoxAcc += acc * (1.0f - centerDiffPowerRatio);
  }

  float acc = gearBoxAcc;
  CarDrivelineElement *endComp = isClutchLocked() ? NULL : root;
  for (CarDrivelineElement *comp = gearbox; comp != endComp; comp = comp->getParent())
  {
    comp->setRotAcc(acc);
    acc *= comp->getRatio();
  }
}

void CarDriveline::integrate(float dt)
{
  // Remember difference between engine and gear rotation
  float deltaVel = root->getRotVel() - gearbox->getRotVel() * gearbox->getRatio();
  if (isClutchLocked())
  {
    // Check for the engine and gearbox (ratio'd) to rotate just about equally.
    // If not, the driver may have speedshifted without applying the clutch.
    // Unfortunately, this is possible but should result in damage in the future,
    // since you get a lot of gear noise.
    if (fabs(deltaVel) > DELTA_VEL_THRESHOLD)
    {
      // Unlock pre-post to let things catch up again
      unlockClutch();
      setClutchApp(1.0);
    }
  }

  // Engine
  root->integrate(dt);
  gearbox->integrate(dt);

  if (!isClutchLocked())
  {
    // Check if gearbox is catching up with engine
    float newDeltaVel = root->getRotVel() - gearbox->getRotVel() * gearbox->getRatio();
    if (deltaVel * newDeltaVel <= 0)
    {
      lockClutch();
      // Force engine and gearbox velocity to be the same
      root->setRotVel(gearbox->getRotVel() * gearbox->getRatio() + (newDeltaVel + deltaVel) / 2);
    }
    else if (!isAutoClutchActive())
      setClutchApp(1.0);
  }
  //  Stat3D::setGraphicValue(gid_clutch_app, isClutchLocked()?2:clutchApplication);
  //  Stat3D::setGraphicValue(gid_eng_rpm, root->getRotVel()*60/2/PI);
  //  Stat3D::setGraphicValue(gid_diff_rpm, deltaVel*60/2/PI);

  if (car->getDiffCount() != 2)
    return;

  if (centerDiff.type == PhysCarCenterDiffParams::VISCOUS)
  {
    // viscous diff
    if (fabs(centerDiffTorque) < 1)
      centerDiffPowerRatio = centerDiff.fixedRatio;
    else
    {
      centerDiffPowerRatio =
        centerDiff.fixedRatio - (car->getDiff(0)->getRotVel() - car->getDiff(1)->getRotVel()) * centerDiff.viscousK / centerDiffTorque;

      if (centerDiffPowerRatio < 0)
        centerDiffPowerRatio = 0;
      else if (centerDiffPowerRatio > 1)
        centerDiffPowerRatio = 1;
    }
  }
  else if (centerDiff.type == PhysCarCenterDiffParams::ELSD)
  {
    // eLSD using feedback torques
    float t0 = fabs(car->getDiff(0)->wheel[0]->getFeedbackTorque()) + fabs(car->getDiff(0)->wheel[1]->getFeedbackTorque());
    float t1 = fabs(car->getDiff(1)->wheel[0]->getFeedbackTorque()) + fabs(car->getDiff(1)->wheel[1]->getFeedbackTorque());

    centerDiffPowerRatio = centerDiff.fixedRatio - ((t0 > 0.1 || t1 > 0.1) ? (t1 - t0) / (t1 + t0) : 0);
    float m0 = centerDiffPowerRatio - 0.5;
    if (centerDiffTorque > 0)
    {
      if (m0 < -centerDiff.powerRatio)
        centerDiffPowerRatio = 0.5 - centerDiff.powerRatio;
      else if (m0 > centerDiff.powerRatio)
        centerDiffPowerRatio = 0.5 + centerDiff.powerRatio;
    }
    else
    {
      if (m0 < -centerDiff.coastRatio)
        centerDiffPowerRatio = 0.5 - centerDiff.coastRatio;
      else if (m0 > centerDiff.coastRatio)
        centerDiffPowerRatio = 0.5 + centerDiff.coastRatio;
    }
  }
  /*
  debug("F/R diff: %.3f (%.3f %.3f) R=%.3f",
    (car->getDiff(0)->getRotVel()-car->getDiff(1)->getRotVel())*30/PI,
    car->getDiff(0)->getRotVel()*30/PI, car->getDiff(1)->getRotVel()*30/PI,
    centerDiffPowerRatio);
  */
}
