// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include "mathAng.h"

real square_rec(int iter_no, real val, real *hashed_iter)
{
  if (iter_no == 1)
    return val;
  real mulRez = 1;
  real mul = 1 - val * val;
  for (int i = 0; i < (iter_no - 1) / 2; i++)
    mulRez *= mul;
  if (hashed_iter)
    return (val * mulRez - (1 - iter_no) * (*hashed_iter)) / iter_no;
  else
    return (val * mulRez - (1 - iter_no) * square_rec(iter_no - 2, val, NULL)) / iter_no;
}

int get_cos_power_from_ang(real alfa, real part, real &real_part)
{
  const int maxDegree = 100000;
  static real alfaIters[(maxDegree - 1) / 2], oneIters[(maxDegree - 1) / 2], zeroIters[(maxDegree - 1) / 2];
  int d = 0;
  int k = 0;
  for (d = 1; d < maxDegree; d += 2)
  {
    real zeroIter = 0;
    real oneIter = square_rec(d, 1, d > 1 ? &oneIters[(d - 3) / 2] : NULL);
    real alfaIter = square_rec(d, sin(alfa), d > 1 ? &alfaIters[(d - 3) / 2] : NULL);
    alfaIters[k] = alfaIter;
    oneIters[k] = oneIter;
    zeroIters[k] = zeroIter;
    k++;
    real_part = (alfaIter - zeroIter) / (oneIter - zeroIter);
    if (real_part > part)
      break;
  }
  return d;
}
