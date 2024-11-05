// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/utils.h>
#include <daNet/bitStream.h>
#include <math.h>

bool gamephys::readHistory32(const danet::BitStream &bs, uint32_t &data)
{
  bool isRaw = false;
  bool isOk = bs.Read(isRaw);
  if (isRaw)
  {
    isOk = isOk && bs.Read(data);
    return isOk;
  }
  bool isFilledWith = false;
  isOk = isOk && bs.Read(isFilledWith);
  data = isFilledWith ? uint32_t(-1) : 0;
  return isOk;
}

void gamephys::writeHistory32(danet::BitStream &bs, uint32_t data)
{
  if (data != 0 && data != uint32_t(-1))
  {
    bs.Write(true);
    bs.Write(data);
    return;
  }
  bs.Write(false);
  bs.Write(data != 0);
}

double gamephys::alignTime(double time, float fixed_dt) { return (double)fixed_dt * floor(time / (double)fixed_dt); }
