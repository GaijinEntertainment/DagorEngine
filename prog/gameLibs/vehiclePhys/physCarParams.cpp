// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/physCarParams.h>

#include <ioSys/dag_dataBlock.h>

void PhysCarSuspensionParams::userSave(DataBlock &b) const
{
#define PREAL(n) b.setReal(#n, n)
  PREAL(fixPtOfs);
  PREAL(upTravel);
  PREAL(maxTravel);
  PREAL(springK);
  PREAL(springKdUp);
  PREAL(springKdDown);
  PREAL(arbK);
#undef PREAL
}

void PhysCarSuspensionParams::userLoad(const DataBlock &b)
{
#define PREAL(n) n = b.getReal(#n, n)
  PREAL(fixPtOfs);
  PREAL(upTravel);
  PREAL(maxTravel);
  PREAL(springK);
  PREAL(springKdUp);
  PREAL(springKdDown);
  PREAL(arbK);
#undef PREAL
}


void PhysCarParams::setDefaultValues(const PhysCarSuspensionParams &front_susp, const PhysCarSuspensionParams &rear_susp)
{
  maxWheelAng = PI / 4;

  frontBrakesTorque = 3000;
  rearBrakesTorque = 2500;
  handBrakesTorque = 5000;
  handBrakesTorque2 = 15000;
  brakeForceAdjustK = 0;

  ackermanAngle = 0;
  toeIn = 0;

  minWheelSelfBrakeTorque = 0;
  longForcePtInOfs = 0;
  longForcePtDownOfs = 0;

  frontSusp = front_susp;
  rearSusp = rear_susp;
}

void PhysCarParams::load(const DataBlock *b, const DataBlock &cars_blk)
{
  maxWheelAng = b->getReal("maxWheelAng", maxWheelAng * RAD_TO_DEG) * DEG_TO_RAD;
  longForcePtInOfs = b->getReal("longForcePtInOfs", 0);
  longForcePtDownOfs = b->getReal("longForcePtDownOfs", 0);
  ackermanAngle = b->getReal("ackermanAng", 10.0);
  toeIn = b->getReal("toeIn", 0);

  const DataBlock *brBlk = cars_blk.getBlockByNameEx("brakes")->getBlockByName(b->getStr("brakes", NULL));
  if (!brBlk)
    DAG_FATAL("can't load brakes preset <%s>", b->getStr("brakes", NULL));
  frontBrakesTorque = brBlk->getReal("frontBrakesTorque", 0);
  rearBrakesTorque = brBlk->getReal("rearBrakesTorque", 0);
  handBrakesTorque = brBlk->getReal("handBrakesTorque", 0);
  handBrakesTorque2 = brBlk->getReal("handBrakesTorque2", handBrakesTorque);
  brakeForceAdjustK = brBlk->getReal("brakeForceAdjustK", 0);
  minWheelSelfBrakeTorque = brBlk->getReal("minWheelSelfBrakeTorque", 25.0);

  const DataBlock *defSuspBlk = b->getBlockByNameEx("suspension");
  const DataBlock *frontSuspBlk = defSuspBlk->getBlockByName("front");
  const DataBlock *rearSuspBlk = defSuspBlk->getBlockByName("rear");

  loadSuspensionParams(frontSusp, frontSuspBlk ? *frontSuspBlk : *defSuspBlk);
  loadSuspensionParams(rearSusp, rearSuspBlk ? *rearSuspBlk : *defSuspBlk);
}


void PhysCarParams::loadSuspensionParams(PhysCarSuspensionParams &s, const DataBlock &b)
{
  s.upperPt = b.getPoint3("upperPt", s.upperPt);
  s.springAxis = b.getPoint3("springAxis", s.springAxis);
  s.fixPtOfs = b.getReal("fixPtOfs", s.fixPtOfs);
  s.upTravel = b.getReal("upTravel", s.upTravel);
  s.maxTravel = b.getReal("maxTravel", s.maxTravel);

  float w_nat = b.getReal("spring_w_nat", 0);
  float k = b.getReal("spring_k", 0);
  if (w_nat > 0)
  {
    s.springK = w_nat;
    s.setSpringKAsW0(true);
  }
  else if (k > 0)
  {
    s.springK = k;
    s.setSpringKAsW0(false);
  }

  s.springKdUp = b.getReal("spring_kd_up", s.springKdUp);
  s.springKdDown = b.getReal("spring_kd_down", s.springKdDown);
  s.arbK = b.getReal("anti_roll_bar_k", s.arbK);
  s.setPowered(b.getBool("powered", s.powered()));
  s.setControlled(b.getBool("steered", s.controlled()));
  s.setRigidAxle(b.getBool("rigid_axle", s.rigidAxle()));

  s.wRad = b.getReal("wheelRad", s.wRad);
  s.wMass = b.getReal("wheelMass", s.wMass);
}

void PhysCarParams::userSave(DataBlock &b) const
{
#define PREAL(n) b.setReal(#n, n)
  PREAL(frontBrakesTorque);
  PREAL(rearBrakesTorque);
  PREAL(handBrakesTorque);
  PREAL(handBrakesTorque2);
  PREAL(brakeForceAdjustK);
  PREAL(minWheelSelfBrakeTorque);
  PREAL(ackermanAngle);
  PREAL(toeIn);

  PREAL(longForcePtInOfs);
  PREAL(longForcePtDownOfs);
  PREAL(maxWheelAng);
#undef PREAL

  frontSusp.userSave(*b.addBlock("frontSusp"));
  rearSusp.userSave(*b.addBlock("rearSusp"));
}

void PhysCarParams::userLoad(const DataBlock &b)
{
#define PREAL(n) n = b.getReal(#n, n)
  PREAL(frontBrakesTorque);
  PREAL(rearBrakesTorque);
  PREAL(handBrakesTorque);
  PREAL(handBrakesTorque2);
  PREAL(brakeForceAdjustK);
  PREAL(minWheelSelfBrakeTorque);
  PREAL(ackermanAngle);
  PREAL(toeIn);

  PREAL(longForcePtInOfs);
  PREAL(longForcePtDownOfs);
  PREAL(maxWheelAng);
#undef PREAL

  frontSusp.userLoad(*b.getBlockByNameEx("frontSusp"));
  rearSusp.userLoad(*b.getBlockByNameEx("rearSusp"));
}
