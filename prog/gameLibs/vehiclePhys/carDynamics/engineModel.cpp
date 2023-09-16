#include <vehiclePhys/carDynamics/carModel.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <math/dag_mathUtils.h>

#define DEF_MAXRPM 5000
#define DEF_TORQUE 100 // In case no curve is present

CarEngineModel::CarEngineModel(CarDynamicsModel *_car) : CarDrivelineElement(_car->getDriveline())
{
  car = _car;
  init();
}

CarEngineModel::~CarEngineModel() {}

void CarEngineModel::init()
// Init all member variables
{
  flags = 0;
  maxRPM = DEF_MAXRPM;
  idleRPM = 500;
  stallRPM = 400;
  startRPM = 500;
  autoClutchRPM = idleRPM * 1.5f;
  // autoClutch=0;
  // starterTorque=50;
  idleThrottle = 0.1f;
  throttleRange = 1000.0 - idleThrottle * 1000.0;
  brakingCoeff = 0;

  curveTorqueMul = 0;
  throttle = 0;

  curNitroPower = 0.0f;
  targetNitroPower = 0.0f;
  nitroChangeSpeed = 2.0f;

  resetData();
}

// Reset engine before usage (after Shift-R for example)
void CarEngineModel::resetData()
{
  CarDrivelineElement::resetData();

  tEngine = 0;
  targetNitroPower = 0.0f;
  curNitroPower = 0.0f;

  // No automatic clutch engaged
  // flags&=~AUTOCLUTCH_ACTIVE;

  // Start engine (or not) and make it stable; AFTER RDLC:resetData(), since that
  // sets the rotational velocity to 0.
  // Pre-start engine
  setRPM(idleRPM);

  setThrottle(0);
}

void CarEngineModel::setRPM(float rpm)
{
  // Convert to radians and seconds
  rotV = rpm * TWOPI / 60.0f;
}


// Precalculate known numbers to speed up calculations during gameplay
void CarEngineModel::prepare()
{
  // Calculate idle throttle based on desired idle RPM
  float maxT = getMaxTorque(idleRPM);
  float minT = getMinTorque(idleRPM);
  if (fabsf(minT) < 1e-5f)
  {
    // There's no engine braking, hm
    idleThrottle = 0;
  }
  else
  {
    // Calculate throttle at which resulting engine torque would be 0
    idleThrottle = -minT / (maxT - minT);
  }

  // Calculate effective throttle range so that the throttle axis
  // can be more quickly converted to a 0..1 number.
  // This also includes converting from the controllers integer 0..1000
  // range to 0..1 floating point.
  throttleRange = (1.0 - idleThrottle) / 1000.0;

  // Reset engine
  resetData();
}

bool CarEngineModel::load(const DataBlock *engBlk)
{
  G_ASSERT(engBlk);
  maxRPM = engBlk->getReal("maxRPM", DEF_MAXRPM);
  idleRPM = engBlk->getReal("idleRPM", 1000);

  curveTorqueMul = engBlk->getReal("torqueMul", 1.0f);

  const DataBlock *tblk = engBlk->getBlockByName("torque");
  G_ASSERT(tblk);
  crvTorque.load(*tblk);

  // Make sure it ends at 0 (assuming no engine will rev above 100,000 rpm)
  if (fabs(crvTorque.getValue(100000)) > 1e-5f)
  {
    debug("WARN: The torque curve needs to end at 0 torque (is now %.2f)", crvTorque.getValue(100000));
  }

  setInertia(engBlk->getReal("inertia", 0.1));
  brakingCoeff = engBlk->getReal("brakingCoeff", 0);

  nitroChangeSpeed = engBlk->getReal("nitroChangeSpeed", 1.0);

  autoClutchRPM = engBlk->getReal("autoClutchRPM", 1500);

  // Calculate static facts
  prepare();

  // Check the facts
  G_ASSERT(stallRPM < idleRPM);
  G_ASSERT(stallRPM < startRPM);
  return true;
}

// Controller input from 0..1000
// Calculates resulting throttle.
void CarEngineModel::setThrottle(int ctlThrottle)
{
  // Convert throttle to 0..1 (mind idle throttle offset)
  throttle = ((float)ctlThrottle) * throttleRange + idleThrottle;
  if (throttle > 1)
    throttle = 1;
  else if (throttle < 0)
    throttle = 0;
}

void CarEngineModel::calcForces()
{
  float rpm = getRPM();

  // Check starter; this section assumes calcForces() is called
  // only once per simulation step. Although this may not be
  // the case in the future, it should not harm that much.
  // Engine is running
  // Calculate minimal torque (if 0% throttle)
  float minTorque = getMinTorque(rpm);

  // Calculate maximum torque (100% throttle situation)
  float maxTorque = getMaxTorque(rpm);

  // The throttle accounts for how much of the torque is actually
  // produced.
  // NOTE: The output power then is 2*PI*rps*outputTorque
  // (rps=rpm/60; rotations per second). Nothing but a statistic now.
  tEngine = (maxTorque - minTorque) * throttle + minTorque;

  // Add nitro factor
  tEngine *= (1.0f + curNitroPower);
}

void CarEngineModel::calcAcc()
{
  if (!driveline->isClutchLocked())
  {
    // Engine moves separately from the rest of the driveline
    rotA = (tEngine - driveline->getClutchTorque()) / getInertia();
  } // else rotA is calculated in the driveline (passed up
    // from the wheels on to the root - the engine)
}


// Step the engine.
// Also looks at the stall state (you can kickstart the engine when driving).
void CarEngineModel::integrate(float dt)
{
  // This is normally done by CarDrivelineElement, but avoid the function call
  // Take care when CarDrivelineElement::integrate() does more than this line.
  if (!driveline->isClutchLocked())
  {
    const float deltaV = child[0]->getRotVel() * child[0]->getRatio() - getRotVel();
    const float deltaSign = sign(deltaV);
    const float absDeltaV = deltaSign * deltaV;
    if (rotA * dt * deltaSign > absDeltaV)
      rotA = safediv(deltaV, dt);
  }
  rotV += rotA * dt;

  float curRpm = getRPM();
  if (curRpm > maxRPM)
    setRPM(maxRPM);
  else if (curRpm < idleRPM)
    setRPM(idleRPM);

  float dNitro = targetNitroPower - curNitroPower;
  float maxDNitro = nitroChangeSpeed * dt;
  if (dNitro > maxDNitro)
    dNitro = maxDNitro;
  else if (dNitro < -maxDNitro)
    dNitro = -maxDNitro;

  curNitroPower += dNitro;

  // Auto-clutch assist
  float rpm = getRPM();
  if (rpm < autoClutchRPM)
  {
    // Engage clutch (is picked up by the driveline)
    car->getDriveline()->enableAutoClutch(true);

    // Calculate clutch amount
    float autoClutch = (rpm - idleRPM) / (autoClutchRPM - idleRPM);
    if (autoClutch < 0)
      autoClutch = 0;
    else if (autoClutch > 1)
      autoClutch = 1;
    car->getDriveline()->setClutchApp(autoClutch);
  }
  else
  {
    // Turn off auto-clutch
    car->getDriveline()->enableAutoClutch(false);
  }
}

void CarEngineModel::calcMaxPower(float &max_hp, float &rpm_max_hp, float &max_torque, float &rpm_max_torque)
{
  max_hp = 0.0f;
  rpm_max_hp = 0.0f;
  max_torque = 0.0f;
  rpm_max_torque = 0.0f;

  for (int i = 0; i < crvTorque.p.size(); i++)
  {
    float torque = crvTorque.p[i].y * curveTorqueMul;
    float rpm = crvTorque.p[i].x;

    if (torque > max_torque)
    {
      max_torque = torque;
      rpm_max_torque = rpm;
    }

    float hp = torque * rpm / 9549 / 0.746;
    if (hp > max_hp)
    {
      max_hp = hp;
      rpm_max_hp = rpm;
    }
  }
}
