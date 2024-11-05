// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/carDynamics/differentialModel.h>
#include <vehiclePhys/carDynamics/wheelModel.h>
#include <vehiclePhys/carDynamics/carModel.h>
#include <ioSys/dag_dataBlock.h>

#include <vehiclePhys/physCarParams.h>

#include <debug/dag_debug.h>

const char *const PhysCarDifferentialParams::diff_type_names[PhysCarDifferentialParams::NUM_TYPES] = {
  "free", "viscous", "salisbury", "eLSD"};

PhysCarDifferentialParams::PhysCarDifferentialParams()
{
  type = FREE;
  memset(this, 0, sizeof(*this));
}

void PhysCarDifferentialParams::load(const DataBlock &blk)
{
  const char *typeStr = blk.getStr("typeStr", NULL);
  if (typeStr)
  {
    type = -1;
    for (int i = 0; i < NUM_TYPES; i++)
      if (strcmp(diff_type_names[i], typeStr) == 0)
      {
        type = i;
        break;
      }
  }
  else
    type = blk.getInt("type", FREE);

  G_ASSERT(type >= 0 && type < NUM_TYPES);

  powerRatio = 0.5 - 1.0 / (blk.getReal("powerRatio", 2) + 1.0);
  coastRatio = 0.5 - 1.0 / (blk.getReal("coastRatio", 1) + 1.0);

  // Type-specific parameters
  if (type == VISCOUS)
    lockingCoeff = blk.getReal("locking_coeff", 0);
  else if (type == SALISBURY)
    preload = blk.getReal("preload", 0);
  else if (type == ELSD)
  {
    yawAngleWt = blk.getReal("yawAngleWt", 4 * 100.0);
    steerAngleWt = blk.getReal("steerAngleWt", 0.5 * 100.0);
    yawRateWt = blk.getReal("yawRateWt", 0);
  }
}

void PhysCarDifferentialParams::save(DataBlock &blk) const
{
  blk.setStr("typeStr", diff_type_names[type]);
  blk.setInt("type", type);

  blk.setReal("powerRatio", 1.0 / (0.5 - powerRatio) - 1.0);
  blk.setReal("coastRatio", 1.0 / (0.5 - coastRatio) - 1.0);

  if (type == VISCOUS)
    blk.setReal("locking_coeff", lockingCoeff);
  else if (type == SALISBURY)
    blk.setReal("preload", preload);
  else if (type == ELSD)
  {
    blk.setReal("yawAngleWt", yawAngleWt);
    blk.setReal("steerAngleWt", steerAngleWt);
    blk.setReal("yawRateWt", yawRateWt);
  }
}

const char *const PhysCarCenterDiffParams::diff_type_names[PhysCarCenterDiffParams::NUM_TYPES] = {"free", "viscous", "eLSD"};

PhysCarCenterDiffParams::PhysCarCenterDiffParams()
{
  fixedRatio = 0.5;
  viscousK = 0;
  powerRatio = 0.5;
  coastRatio = 0.5;
  type = FREE;
}

void PhysCarCenterDiffParams::load(const DataBlock &blk)
{
  const char *typeName = blk.getStr("typeStr", diff_type_names[FREE]);

  bool found = false;
  for (int i = 0; !found && i < NUM_TYPES; ++i)
  {
    if (strcmp(typeName, diff_type_names[i]) == 0)
    {
      type = i;
      found = true;
    }
  }

  if (!found)
    DAG_FATAL("unknown center diff type: %s", typeName);

  fixedRatio = blk.getReal("fixedRatio", 0.5);
  viscousK = blk.getReal("viscousK", 4);
  powerRatio = 0.5 - 1.0 / (blk.getReal("powerRatio", 3) + 1.0);
  coastRatio = 0.5 - 1.0 / (blk.getReal("coastRatio", 1.5) + 1.0);
}

void PhysCarCenterDiffParams::save(DataBlock &blk) const
{
  blk.setStr("typeStr", diff_type_names[type]);
  blk.setReal("fixedRatio", fixedRatio);
  blk.setReal("viscousK", viscousK);
  blk.setReal("powerRatio", 1.0 / (0.5 - powerRatio) - 1.0);
  blk.setReal("coastRatio", 1.0 / (0.5 - coastRatio) - 1.0);
}


CarDifferential::CarDifferential(CarDriveline *dl) : CarDrivelineElement(dl)
{
  wheel[0] = wheel[1] = NULL;
  params = new PhysCarDifferentialParams();

  resetData();
}

CarDifferential::~CarDifferential() { del_it(params); }


void CarDifferential::resetData()
{
  accIn = 0;
  accASymmetric = 0;
  accOut[0] = accOut[1] = 0;

  velASymmetric = 0;

  CarDrivelineElement::resetData();
}

void CarDifferential::setParams(const PhysCarDifferentialParams &p)
{
  *params = p;
  //  setRatio(params.ratio);
  //  setInertia(params.inertia);
  setRatio(0);
  setInertia(0);
}

// Calculates the locking torque of the differential
float CarDifferential::calcLockTorque(float torqueOut[], float mIn, float mt)
{
  float m0, powerRatio, coastRatio;
  switch (params->type)
  {
    case PhysCarDifferentialParams::FREE:
      // No locking; both wheels run free
      return 0;

    case PhysCarDifferentialParams::VISCOUS:
      velASymmetric = wheel[1]->getRotV() - wheel[0]->getRotV();
      m0 = -params->lockingCoeff * velASymmetric;
      powerRatio = 0.5f;
      coastRatio = 0.5f;
      break;

    case PhysCarDifferentialParams::SALISBURY:
    {
      float t0 = fabs(torqueOut[0]), t1 = fabs(torqueOut[1]);
      m0 = (t0 > 0.1 || t1 > 0.1) ? mIn * (t1 - t0) / (t1 + t0) : 0;
      powerRatio = params->powerRatio;
      coastRatio = params->coastRatio;
    }
    break;

    case PhysCarDifferentialParams::ELSD:
    {
      const EspSensors &esp = getDriveline()->getCar()->espSens;
      m0 = (esp.yawAngle * params->yawAngleWt + esp.steerAngle * params->steerAngleWt + esp.yawRate * params->yawRateWt) *
           (mt > 0 ? -1 : 1);
      powerRatio = params->powerRatio;
      coastRatio = params->coastRatio;
    }
    break;

    default: debug("WARN: CarDifferential:calcLockTorque(); unknown diff type"); return 0;
  }

  float power_max_m0 = mt * powerRatio;
  float coast_max_m0 = -mt * coastRatio;
  if (mt > 0 && fabs(m0) > power_max_m0)
    m0 = m0 > 0 ? power_max_m0 : -power_max_m0;
  else if (mt < 0 && fabs(m0) > coast_max_m0)
    m0 = m0 > 0 ? coast_max_m0 : -coast_max_m0;

  return m0;
}

void CarDifferential::lock(int diff_side) { locked |= 1 << diff_side; }


// Special version in case there is only 1 differential.
// Differences with regular operating:
// - 'torqueIn' is directly passed in from the driveline root (engine).
// - 'inertiaIn' is directly passed from the driveline (engine's eff. inertia)
// Calculates accelerations of the 3 sides of the differential
// based on incoming torques and inertia's.
// Also unlocks sides (only the wheels can be locked for now)
// if the engine torques exceeds the reaction/braking torque
// (if that side was locked, see lock())

void CarDifferential::calc1DiffForces(float dt, float torqueIn, float inertiaIn)
{
  // Gregor Veble's naming convention
  float j1, j2, j3, // Inertia of output1/output2/input
    jw,             // Total inertia of wheels
    jt;             // Total inertia of all attached components
                    // jd;        // Difference of wheel inertia's
  float m0,         // Locking torque
    m1,             // Output1 torque
    m2,             // Output2 torque
    m3;             // Input torque (from the engine probably)
  float mt,         // Total net torque
    md;             // Asymmetric torque (?)

  // Check that everything is ok
  G_ASSERT(wheel[0] && wheel[1]);

  float torqueBrakingOut[2];
  float torqueOut[2];

  // Note that no ratios are used here; the input and outputs
  // are geared 1:1:1. This makes the formulas easier. To add
  // a differential ratio, the other functions for the input torques
  // take care of this.

  // Retrieve current effective inertia
  // The base is the driveshaft; the accelerations and torques are
  // related to its position in the drivetrain.
  // inertiaIn=engine->GetInertiaAtDifferential();
  float inertiaOut0 = wheel[0]->getInertia();
  float inertiaOut1 = wheel[1]->getInertia();

  // Retrieve potential braking torque; if bigger than the reaction
  // torque, the output will become unlocked. If not, the output is
  // locked.
  // Note that the braking torque already points in the opposite
  // direction of the output (mostly a wheel) rotation. This is contrary
  // to Gregor Veble's approach, which only calculates the direction
  // in the formulae below.
  torqueBrakingOut[0] = wheel[0]->getBrakingTorque();
  torqueBrakingOut[1] = wheel[1]->getBrakingTorque();

  // Retrieve torques at all ends
  // Notice that inside the diff, there can be a ratio. If this is 2 for
  // example, the driveshaft will rotate twice as fast as the wheel axles.
  // torqueIn=engine->GetTorqueAtDifferential();
  torqueOut[0] = wheel[0]->getFeedbackTorque();
  torqueOut[1] = wheel[1]->getFeedbackTorque();

  m1 = torqueOut[0] + torqueBrakingOut[0];
  m2 = torqueOut[1] + torqueBrakingOut[1];
  m3 = torqueIn; // Entails engine braking already
  mt = m1 + m2 + m3;

  // Proceed to Gregor's naming convention and algorithm
  // Determine locking
  m0 = calcLockTorque(torqueOut, torqueIn, mt);

  j1 = inertiaOut0;
  j2 = inertiaOut1;
  j3 = inertiaIn;
  jw = j1 + j2;
  jt = jw + j3;

  if (locked)
  {
    if (locked == 3) // both locked
      m1 = m2 = -m3 * 0.5f;
    else
    {
      if (locked & 1)
        m1 = (m2 * j3 - 2.0f * m3 * j2 - m0 * (2.0f * j2 + j3)) / (4.0f * j2 + j3);
      if (locked & 2)
        m2 = (m1 * j3 - 2.0f * m3 * j1 + m0 * (2.0f * j1 + j3)) / (4.0f * j1 + j3);
    }
    if (locked & 1 && fabs(m1 - torqueOut[0]) > fabs(torqueBrakingOut[0]))
      locked ^= 1;
    if (locked & 2 && fabs(m2 - torqueOut[1]) > fabs(torqueBrakingOut[1]))
      locked ^= 2;
  }

  //  jd = j1-j2;  // Inertia difference (of outputs)
  //  // Calculate determinant of 2x2 matrix
  //  float det=4.0f*j1*j2+j3*jw;

  mt = m1 + m2 + m3;
  md = m2 - m1 + m0;

  // Calculate asymmetric acceleration
  accASymmetric = md / jw;

  // Calculate total acceleration based on all torques
  // (which is in fact the driveshaft rotational acceleration)
  accIn = mt / jt;

  // Derive from these the acceleration of the 2 output parts
  accOut[1] = accIn + accASymmetric;
  accOut[0] = accIn - accASymmetric;

  float w0r = wheel[0]->rotationV;
  float w1r = wheel[1]->rotationV;
  if ((w0r + accOut[0] * dt) * w0r < 0.f && torqueIn * accOut[0] <= 0.f) // wheel changes rot direction
  {
    accOut[0] = -w0r / dt; // dampen
    locked |= 1;
  }
  if ((w1r + accOut[1] * dt) * w1r < 0.f && torqueIn * accOut[1] <= 0.f) // wheel changes rot direction
  {
    accOut[1] = -w1r / dt; // dampen
    locked |= 2;
  }

  /*
    //-- Experimental braking code start
    if ( (fabsf(torqueBrakingOut[0]) > fabsf(accOut[0]+wheel[0]->rotationV/dt))
      || (fabsf(torqueBrakingOut[1]) > fabsf(accOut[1]+wheel[1]->rotationV/dt)))
    {
      accOut[0] = -wheel[0]->getRotV()/dt;
      accOut[1] = -wheel[1]->getRotV()/dt;
      accIn = (accOut[0] + accOut[1])*0.5f;
      accASymmetric = accIn-accOut[0];
      return;
    }
    //-- Experimental braking code end
  */

  // prevent huge speeds for free wheel while another one has contact
  const float maxWheelRotV = 500.0f;
  bool needAdjustRecalc = false;
  for (int i = 0; i < 2; i++)
  {
    if ((wheel[i]->rotationV > maxWheelRotV && accOut[i] > 0) || (wheel[i]->rotationV < -maxWheelRotV && accOut[i] < 0))
    {
      accOut[i] = 0;
      needAdjustRecalc = true;
    }
  }
  if (needAdjustRecalc)
  {
    accIn = (accOut[0] + accOut[1]) * 0.5f;
    accASymmetric = accIn - accOut[0];
  }

  // DEBUG_CTX("acc = %f +/- %f", accIn, accASymmetric);
}

void CarDifferential::integrate(float dt) { rotV = (wheel[0]->rotationV + wheel[1]->rotationV) / 2; }
