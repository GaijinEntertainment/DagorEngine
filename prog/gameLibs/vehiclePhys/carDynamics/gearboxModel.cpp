#include <vehiclePhys/carDynamics/carModel.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>

#include <vehiclePhys/physCarParams.h>

#define DEBUG_EXPORT_GEAR_GLOBALS 0

/**********************************************************************/

PhysCarGearBoxParams::PhysCarGearBoxParams()
{
  numGears = 0;
  memset(ratio, 0, sizeof(ratio));
  memset(inertia, 0, sizeof(inertia));

  ratio[CarGearBox::GEAR_NEUTRAL] = 1;
  inertia[CarGearBox::GEAR_NEUTRAL] = 0.01f;
  ratioMul = 1.0f;

  timeToDeclutch = 0;
  timeToClutch = 0;
  minTimeToSwitch = 0;
  maxTargetRPM = 100000;
}

void PhysCarGearBoxParams::load(const DataBlock *blk)
{
  minTimeToSwitch = (int)blk->getReal("minTimeToSwitch", 500);
  timeToClutch = (int)blk->getReal("timeToClutch", 500);
  timeToDeclutch = (int)blk->getReal("timeToDeclutch", 500);

  maxTargetRPM = blk->getReal("maxTargetRPM", 100000);

  numGears = blk->getInt("numGears", 0) + 2; //+ R and N
  G_ASSERT(numGears > 2);
  G_ASSERT(numGears < NUM_GEARS_MAX);

  ratioMul = blk->getReal("ratioMul", 1.0f);

  G_ASSERT(CarGearBox::GEAR_REVERSE == 1);
  const DataBlock *b = blk->getBlockByName("gearRev");
  G_ASSERT(b);

  ratio[CarGearBox::GEAR_REVERSE] = b->getReal("ratio", 1.0f);
  inertia[CarGearBox::GEAR_REVERSE] = b->getReal("inertia", 0.01f);

  for (int gear = CarGearBox::GEAR_1; gear < numGears; gear++)
  {
    b = blk->getBlockByName(String(32, "gear%d", gear - 1));
    G_ASSERT(b);
    ratio[gear] = b->getReal("ratio", 1.0f);
    inertia[gear] = b->getReal("inertia", 0.01f);
  }
}

float PhysCarGearBoxParams::getRatio(int gear)
{
  G_ASSERT(gear >= 0);
  G_ASSERT(gear < numGears);
  return ratio[gear] * ratioMul;
}

float PhysCarGearBoxParams::getInertia(int gear)
{
  G_ASSERT(gear >= 0);
  G_ASSERT(gear < numGears);
  return inertia[gear];
}

void PhysCarGearBoxParams::userSave(DataBlock &b) const
{
  b.setInt("numGears", numGears);
  b.setReal("ratioMul", ratioMul);
  for (int i = 0; i < numGears; ++i)
  {
    b.setReal(String(32, "ratio%d", i), ratio[i]);
    b.setReal(String(32, "inertia%d", i), inertia[i]);
  }
}

void PhysCarGearBoxParams::userLoad(const DataBlock &b)
{
  numGears = b.getInt("numGears", numGears);
  ratioMul = b.getReal("ratioMul", ratioMul);
  G_ASSERT(numGears > 0 && numGears < NUM_GEARS_MAX);
  G_ASSERT(ratioMul > 0 && ratioMul < 100);

  for (int i = 0; i < numGears; ++i)
  {
    ratio[i] = b.getReal(String(32, "ratio%d", i), ratio[i]);
    inertia[i] = b.getReal(String(32, "inertia%d", i), inertia[i]);
  }
}

/**********************************************************************/

CarGearBox::CarGearBox(CarDynamicsModel *_car) : CarDrivelineElement(_car->getDriveline())
{
  car = _car;

  flags = AUTOMATIC;
  autoShiftStart = 0;
  timeToShift = 0;
  curGear = futureGear = 0;
  shiftUpEnabled = true;

  params = new PhysCarGearBoxParams();
}

CarGearBox::~CarGearBox() { del_it(params); }

void CarGearBox::setParams(const PhysCarGearBoxParams &gbox_params)
{
  *params = gbox_params;
  resetData();
}

void CarGearBox::getParams(PhysCarGearBoxParams &gbox_params) { gbox_params = *params; }

void CarGearBox::resetData()
{
  CarDrivelineElement::resetData();
  flags &= ~FORCE_REVERSE;
  setGear(GEAR_1);
  timeToShift = autoShiftStart = 0;
  shiftUpEnabled = true;
}


int CarGearBox::getNumGears() { return params->numGears; }

void CarGearBox::setGear(int gear)
{
  G_ASSERT(gear >= GEAR_NEUTRAL);
  G_ASSERT(gear < params->numGears);

  futureGear = curGear = gear;

  // Calculate resulting ratio
  setRatio(params->getRatio(curGear));
  setInertia(params->getInertia(curGear));

  // Recalculate driveline inertias and ratios
  car->prepareDriveline();
}

void CarGearBox::requestGear(int n, float at_time, bool ignore_cur_shift_timeout)
{
  if ((autoShiftStart == 0) || ignore_cur_shift_timeout) //< No shift in progress
  {
    autoShiftStart = at_time;
    timeToShift = autoShiftStart + float(params->minTimeToSwitch) * 0.001f;
    futureGear = n;
  }
}

void CarGearBox::requestGear(int n, bool ignore_cur_shift_timeout)
{
  requestGear(n, float(::get_time_msec_qpc()) * 0.001f, ignore_cur_shift_timeout);
}

// Based on current input values, adjust the engine
void CarGearBox::integrate(float dt)
{
  if (child[0])
    child[0]->integrate(dt);
  if (child[1])
    child[1]->integrate(dt);

  float target_vel = calcRotVel() / getRatio();
  CarDrivelineElement::integrate(dt);
  if (fabsf(getRotVel() - target_vel) > 5)
  {
    // debug("%d: gearbox sync: getRotVel()=%.3f calcRotVel()=%.3f",
    //   get_time_msec(), getRotVel(), target_vel);
    setRotVel(target_vel);
  }
}

void CarGearBox::performGearSwitch(float at_time)
{
  // bool ac=true;//RMGR->IsEnabled(RManager::ASSIST_AUTOCLUTCH);
  float maxClutch = 0, desiredClutch = 0;

  // The shifting process
  if (autoShiftStart)
  {
    float t = (at_time - autoShiftStart) * 1000.f;
    {
      // Shift up/down path
      // Check for engine autoclutch, which preceeded the gearbox
      if (car->getDriveline()->isAutoClutchActive())
      {
        // Never apply more clutch than the engine autoclutch dictates
        maxClutch = car->getDriveline()->getClutchAppLinear();
      }
      else
      {
        // Gearbox dictates auto-clutch fully
        maxClutch = 1.0f;
        // if (ac)
        car->getDriveline()->enableAutoClutch(true);
      }

      // We are in a shifting operation
      if (curGear != futureGear)
      {
        // We are in the pre-shift phase; apply the clutch
        if (t >= params->timeToDeclutch)
        {
          // Declutch is ready, change gear
          setGear(futureGear);
          desiredClutch = 0.0f;
          // if (ac)car->getDriveline()->setClutchApp(1.0f);
        }
        else
        {
          // Change clutch over time (depress pedal means clutch goes from 1->0
          // if (ac)
          desiredClutch = 1.0f - (t * 1000 / params->timeToDeclutch) / 1000.0f;
          // car->getDriveline()->setClutchApp((t*1000/timeToDeclutch)/1000.0f);
        }
      }
      else
      {
        // We are in the post-shift phase
        if (t >= params->timeToClutch + params->timeToDeclutch)
        {
          // Clutch is ready, end shifting process
          // Conclude the shifting process
          autoShiftStart = 0;
          // if (ac)
          {
            // Full clutch again; the engine will disable autoclutch
            // on the next sim step if the rpm is above autoClutchRPM
            // (in other words: don't turn it off here & now)
            desiredClutch = 1.0f;
            // car->getDriveline()->setClutchApp(0.0f);
            // car->getDriveline()->enableAutoClutch(false);
          }
        }
        else
        {
          // Change clutch
          // if (ac)
          desiredClutch = ((t - params->timeToClutch) * 1000 / params->timeToDeclutch) / 1000.0f;
        }
      }

      // if (ac)
      {
        // Set actual clutch application; the engine was done
        // before this, so mind the engine's autoclutch!
        if (desiredClutch > maxClutch)
          desiredClutch = maxClutch;
        car->getDriveline()->setClutchApp(desiredClutch);
      }
    }
  }
}


#if DEBUG_EXPORT_GEAR_GLOBALS
float cd_rpm = 0, cd_torque = 0, cd_torque_p1 = 0, cd_torque_m1 = 0;
int cd_gear = 0;
#endif

void CarGearBox::updateAfterSimulate(float at_time)
{
  performGearSwitch(at_time);

  if (autoShiftStart) //< Shift in progress
    return;

  if (at_time < timeToShift)
    return;

  if (isAutomatic() && (driveline->getForcedClutch() < 0))
  {
    const float idleRPM = car->getEngine()->getIdleRPM();
    float maxRPM = car->getEngine()->getMaxRPM();
    const float shiftdownRPM = lerp(idleRPM, maxRPM, 0.1f);
    if (params->maxTargetRPM > maxRPM)
    {
      debug("maxTargetRPM = %f, maxRPM = %f", params->maxTargetRPM, maxRPM);
      G_ASSERT(!"Invalid maxTargetRPM");
    }

    float rpm = car->getEngine()->getRPM();
    float throttle = car->getEngine()->getThrottleFactor();
    bool wantReverse = (flags & FORCE_REVERSE);

    if (wantReverse)
    {
      if (curGear != GEAR_REVERSE)
        requestGear(GEAR_REVERSE, at_time);
      return;
    }
    if (curGear == GEAR_REVERSE || futureGear == GEAR_REVERSE)
      return;

    float curRatio = params->getRatio(curGear);

    float curT = car->getEngine()->calcTorque(rpm, throttle) * curRatio;
    float prevT = -10000, nextT = -10000;

    const int nextGear = isNeutral() ? GEAR_1 : curGear + 1;
    if (nextGear < params->numGears)
    {
      float nextRatio = params->getRatio(nextGear);
      float nextRpm = isNeutral() ? rpm : (rpm * nextRatio / curRatio);
      nextT = car->getEngine()->calcTorque(nextRpm, throttle) * nextRatio;
    }

    if (curGear > GEAR_1)
    {
      float prevGearRatio = params->getRatio(curGear - 1);
      float target_rpm = rpm * prevGearRatio / curRatio;
      float maxTargetRPM = (params->maxTargetRPM < maxRPM) ? params->maxTargetRPM : maxRPM;
      if (target_rpm < maxTargetRPM)
        prevT = car->getEngine()->calcTorque(target_rpm, throttle) * prevGearRatio;
    }

#if DEBUG_EXPORT_GEAR_GLOBALS
    cd_gear = curGear;
    cd_rpm = rpm;
    cd_torque = curT;
    cd_torque_m1 = prevT;
    cd_torque_p1 = nextT;
#endif

    if (curGear > GEAR_1 && ((throttle > 0.1 && prevT > curT + 100) || rpm < shiftdownRPM))
    {
      autoShiftStart = at_time;
      timeToShift = autoShiftStart + float(params->minTimeToSwitch) * 0.001f;
      switch (curGear)
      {
        case GEAR_1: futureGear = GEAR_NEUTRAL; break;
        default:
          futureGear = curGear - 1;
          if (futureGear > GEAR_1)
          {
            float newRatio = params->getRatio(futureGear - 1);
            float nt = car->getEngine()->calcTorque(rpm * newRatio / curRatio, throttle) * newRatio;
            if (prevT > nt)
              break;

            futureGear--;
            prevT = nt;
          }

#if DEBUG_EXPORT_GEAR_GLOBALS
          debug("%d: shift down: %d->%d, rpm=%.1f nrmp=%.1f t0=%.2f t-1=%.2f", ::get_time_msec(), curGear, futureGear, rpm,
            rpm * params->getRatio(futureGear) / curRatio, curT, prevT);
#endif
          break;
      }
    }
    else if (nextGear < params->numGears && throttle > 0.5 && (nextT > curT + 100 || rpm > maxRPM))
    {
      if (!shiftUpEnabled && curGear >= GEAR_1)
        return;
#if DEBUG_EXPORT_GEAR_GLOBALS
      debug("%d: shift up: %d->%d, rpm=%.1f t0=%.2f t+1=%.2f", ::get_time_msec(), curGear, curGear + 1, rpm, curT, nextT);
#endif

      autoShiftStart = at_time;
      timeToShift = autoShiftStart + float(params->minTimeToSwitch) * 0.001f;
      switch (curGear)
      {
        case GEAR_NEUTRAL: futureGear = GEAR_1; break;
        default: futureGear = curGear + 1; break;
      }
    }
  }
}

void CarGearBox::setAutomatic(bool on)
{
  if (on)
    flags |= AUTOMATIC;
  else
    flags &= ~AUTOMATIC;
}
