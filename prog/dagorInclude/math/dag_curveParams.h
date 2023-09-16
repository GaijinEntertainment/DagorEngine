//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <util/dag_stdint.h>
#include <string.h>
#if DAGOR_DBGLEVEL > 0
#include <util/dag_globDef.h>
#endif
#include <generic/dag_carray.h>

class CubicCurveSampler
{
public:
  int type;
  carray<real, 4> coef;
  real *splineCoef;
  static void *(*mem_allocator)(size_t);
  static void (*mem_free)(void *);

  CubicCurveSampler()
  {
    coef[0] = 0;
    coef[1] = 1;
    coef[2] = 0;
    coef[3] = 0;
    type = 1;
    splineCoef = NULL;
  }
  CubicCurveSampler(const CubicCurveSampler &cs) : splineCoef(NULL) { *this = cs; }
  ~CubicCurveSampler()
  {
    if (splineCoef)
    {
      if (mem_free)
        mem_free(splineCoef);
      else
        delete[] splineCoef;
      splineCoef = NULL;
    }
    type = 1;
  }

  CubicCurveSampler &operator=(const CubicCurveSampler &cs)
  {
    if (this == &cs)
      return *this;
    if (splineCoef)
    {
      if (mem_free)
        mem_free(splineCoef);
      else
        delete[] splineCoef;
      splineCoef = NULL;
    }
    type = cs.type;
    mem_copy_from(coef, cs.coef.data());
    if (cs.type > 1)
    {
      splineCoef = mem_allocator ? (real *)mem_allocator(sizeof(float) * 4 * type) : new real[4 * type];
      memcpy(splineCoef, cs.splineCoef, 4 * type * sizeof(float));
    }
    return *this;
  }

  void load(const char *&ptr, int &len)
  {
    const real *rp = (const real *)ptr;
    const int *ip = (const int *)ptr;
    if (ip[0] == 0xFFFFFFFF)
    {
      mem_set_0(coef);

      type = ip[1];
      if (type == 0)
      {
        coef[0] = rp[2];
        ptr += sizeof(real) * 3;
        len -= sizeof(real) * 3;
      }
      else
      {
        splineCoef = mem_allocator ? (real *)mem_allocator(sizeof(float) * 4 * type) : new real[4 * type];
        memcpy(coef.data(), rp + 2, (type - 1) * sizeof(float));
        memcpy(splineCoef, rp + 2 + type - 1, type * 4 * sizeof(float));
        ptr += sizeof(real) * (2 + (type - 1) + type * 4);
        len -= sizeof(real) * (2 + (type - 1) + type * 4);
      }
    }
    else
    {
      coef[0] = rp[0];
      coef[1] = rp[1];
      coef[2] = rp[2];
      coef[3] = rp[3];
      type = 1;

      ptr += sizeof(real) * 4;
      len -= sizeof(real) * 4;
    }
#if DAGOR_DBGLEVEL > 0
    for (int i = 0; i < coef.size(); ++i)
      G_ASSERT(!check_nan(coef[i]));
    if (splineCoef)
      for (int i = 0; i < type * 4; ++i)
        G_ASSERT(!check_nan(splineCoef[i]));
#endif
  }


  void setCoef(real k0, real k1, real k2, real k3)
  {
    coef[0] = k0;
    coef[1] = k1;
    coef[2] = k2;
    coef[3] = k3;
    type = 1;
    if (splineCoef)
    {
      if (mem_free)
        mem_free(splineCoef);
      else
        delete[] splineCoef;
      splineCoef = NULL;
    }
  }


  real sample(real x)
  {
    if (type == 1)
      return ((coef[3] * x + coef[2]) * x + coef[1]) * x + coef[0];
    if (type == 0)
      return coef[0];

    const real *c = splineCoef;
    if (x <= coef[0])
      goto compute;

    for (int i = 1, ie = type < 5 ? type : 4; i < ie; i++)
      if (x < coef[i])
      {
        x -= coef[i - 1];
        c += i * 4;
        goto compute;
      }

    x -= coef[type - 2];
    c += (type - 1) * 4;

  compute:
    return ((c[3] * x + c[2]) * x + c[1]) * x + c[0];
  }
};
