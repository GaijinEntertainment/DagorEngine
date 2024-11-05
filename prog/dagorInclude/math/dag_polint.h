//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_math.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <util/dag_globDef.h>
#include <float.h>

/*
INPUT:  xa[n], ya[n], n - samples
        x
TEMP:   c[n], d[n]
OUTPUT: y = value at point x of polynomial of degree n-1,
  going through ya[i] at xa[i]
        dy = error estimate

*/

template <class YTYPE>
inline void polint_inplace(const real *xa, const YTYPE *ya, int n, YTYPE *c, YTYPE *d, real x, YTYPE &y, YTYPE &dy)
{
  int i, m, ns = 0;
  real den, dif, dift, ho, hp;
  YTYPE w;

  dif = fabs(x - xa[0]);
  c[0] = ya[0];
  d[0] = ya[0];

  for (int i = 1; i < n; i++)
  {
    if ((dift = fabs(x - xa[i])) < dif)
    {
      ns = i;
      dif = dift;
    }
    c[i] = ya[i];
    d[i] = ya[i];
  }

  y = ya[ns];
  ns--;
  for (m = 1; m < n; m++)
  {
    for (i = 0; i < n - m; i++)
    {
      ho = xa[i] - x;
      hp = xa[i + m] - x;
      w = c[i + 1] - d[i];
      den = ho - hp;
      G_ASSERT(den != 0.0 && "Error in polint: two identical points in xa");
      w /= den;
      d[i] = w * hp;
      c[i] = w * ho;
    }
    y += (dy = (2 * (ns + 1) < (n - m) ? c[ns + 1] : d[ns--]));
  }
}

/*
SIDE EFFECT:
        uses memory allocation
*/

template <class YTYPE>
inline void polint(const real *xa, const YTYPE *ya, int n, real x, YTYPE &y, YTYPE &dy)
{
  YTYPE *c = (YTYPE *)memalloc(sizeof(YTYPE) * n, tmpmem);
  YTYPE *d = (YTYPE *)memalloc(sizeof(YTYPE) * n, tmpmem);

  polint_inplace<YTYPE>(xa, ya, n, c, d, x, y, dy);

  memfree(c, tmpmem);
  memfree(d, tmpmem);
}
