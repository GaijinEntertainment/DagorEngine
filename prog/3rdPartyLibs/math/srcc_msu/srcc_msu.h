#pragma once


extern "C"
{
  #include "f2c.h"

  int is01r_c(real *x, real *f, real *df, int *nx, real *sm, real *y,
    real *c__, real *rab, int *ierr);
}
