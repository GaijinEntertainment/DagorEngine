#include <vehiclePhys/carDynamics/carModel.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>

CarDynamicsModel::CarDynamicsModel(int num_wheels)
{
  G_ASSERT(num_wheels <= NUM_WHEELS);

  // Init member variables
  diffCount = 0;
  curSpeedKmh = 0;

  // Parts
  // Note the dependencies:
  // - driveline before engine (or any other driveline component)
  // - engine before body
  driveline = new CarDriveline(this);
  engine = new CarEngineModel(this);
  gearbox = new CarGearBox(this);

  // Defaults
  numWheels = num_wheels;
  memset(wheel, 0, sizeof(CarWheelState *) * NUM_WHEELS);
  for (int i = 0; i < num_wheels; i++)
    wheel[i] = new CarWheelState();
  memset(&espSens, 0, sizeof(espSens));
}

CarDynamicsModel::~CarDynamicsModel()
{
  del_it(engine);
  del_it(gearbox);
  for (int i = 0; i < NUM_WHEELS; i++)
    del_it(wheel[i]);
  for (int i = 0; i < diffCount; i++)
    del_it(diff[i]);
  del_it(driveline);
}


bool CarDynamicsModel::load(const DataBlock *blk_eng, const PhysCarGearBoxParams &gbox_params)
{
  // Read engine (BEFORE body; that adds the engine mass)
  engine->load(blk_eng);

  // Read gearbox
  gearbox->setParams(gbox_params);

  // Read driveline (assumes wheels and engine have been loaded)
  // (reads clutch/handbrakes/differentials)
  driveline->load(blk_eng);

  //
  // Build driveline tree; always start with engine and gearbox
  //
  driveline->setEngine(engine);
  engine->addChild(gearbox);
  driveline->setGearbox(gearbox);

  // Now that the driveline is complete, precalculate some things.
  // Note that the preClutchInertia needs to be calculated only once,
  // since it is the engine's inertia, which never changes.
  driveline->calcInertiaBeforeClutch();
  // Note that setting the gear (called shortly) recalculates all
  // effectives inertiae and ratios.

  gearbox->setGear(CarGearBox::GEAR_1);

  return true;
}

void CarDynamicsModel::addWheelDifferential(int wheel0, int wheel1, const PhysCarDifferentialParams &params)
{
  G_ASSERT(diffCount < NUM_DIFFERENTIALS);

  CarDifferential *d = new CarDifferential(driveline);
  diff[diffCount] = d;

  d->setParams(params);

  d->wheel[0] = wheel[wheel0];
  wheel[wheel0]->setDiff(d, 0);
  d->wheel[1] = wheel[wheel1];
  wheel[wheel1]->setDiff(d, 1);
  gearbox->addChild(d);

  //  diff->addChild(diff->wheel[0]);
  //  diff->addChild(diff->wheel[1]);

  diffCount++;
}

// Precalculate some data on the driveline.
// Call this function again when the gearing changes, since this
// affects the effective inertia and ratios.
// Assumes the driveline is completed (all components in the tree)
void CarDynamicsModel::prepareDriveline()
{
  driveline->calcFullRatios();
  driveline->calcEffInertia();
  driveline->calcInertiaAfterClutch();
}

void CarDynamicsModel::update(float dt)
{
  engine->calcForces();

  // Do the driveline, after engine torque and wheel reaction forces are known
  driveline->calcForces(dt);

  // Calculate the resulting accelerations (some may have been calculated
  // already in calcForces() though).
  driveline->calcAcc();

  driveline->integrate(dt);
}

// Update the car in areas where physics integration is not a concern
// This is a place where you put things that need to be done only once
// every graphics frame, like polling controllers, settings sample
// frequencies etc.
void CarDynamicsModel::updateAfterSimulate(float at_time) { gearbox->updateAfterSimulate(at_time); }
