// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "math/dag_check_nan.h"
#include "debug/dag_assert.h"
#include "util/dag_globDef.h"
#include <cstdint>
#include <cfloat>
#include <vecmath/dag_vecMath.h>

static bool verify_impl(bool cond, const char *error_text, const char *text_cond)
{
  if (!cond)
    logerr(error_text, text_cond);
  return cond;
}

#define verify(x, t) verify_impl(x, t, #x)

DAGOR_NOINLINE void verify_nan_finite_checks(uint32_t inan32, uint64_t inan64, float finf32)
{
  G_UNUSED(inan64);
  bool nanCheckPassed = true;
  bool finiteCheckPassed = true;
  static float p1[4] = {1.f, 0.f, 2.f, 3.f};
  static float p2[4] = {1.f, 2.f, 3.f, 0.f};
  static float p3[4] = {finf32, 1.f, 2.f, 3.f};
  memcpy(&p1[1], (void *)&inan32, sizeof(inan32));
  memcpy(&p2[3], (void *)&inan32, sizeof(inan32));
#if !defined(_M_IX86_FP) || _M_IX86_FP == 0
  float fnan32;
  double dnan64;
  memcpy(&fnan32, (void *)&inan32, sizeof(fnan32));
  memcpy(&dnan64, (void *)&inan64, sizeof(dnan64));
  nanCheckPassed &= verify(!check_nan(1.f), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(!check_nan(1.0), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(fnan32), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(-fnan32), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(dnan64), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(-dnan64), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(NAN), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(sqrtf(-1.0f)), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(check_nan(sqrt(-1.0)), "NaN check is broken on that platform/compiler: %s");

  finiteCheckPassed &= verify(!v_test_xyz_finite(v_ldu(p1)), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(v_test_xyz_finite(v_ldu(p2)), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(fnan32), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(-fnan32), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(dnan64), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(-dnan64), "Finite check is broken on that platform/compiler: %s");
#endif
  nanCheckPassed &= verify(v_test_xyz_nan(v_ldu(p1)), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(!v_test_xyz_nan(v_ldu(p2)), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(v_test_xyzw_nan(v_ldu(p2)), "NaN check is broken on that platform/compiler: %s");
  nanCheckPassed &= verify(!v_test_xyzw_nan(v_ldu(p3)), "NaN check is broken on that platform/compiler: %s");

  finiteCheckPassed &= verify(check_finite(1.0f), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(check_finite(1.0), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(finf32), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(-finf32), "Finite check is broken on that platform/compiler: %s");
  volatile float zero32 = 0.f;
  volatile double zero64 = 0.0;
  finiteCheckPassed &= verify(!check_finite(1.f / zero32), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!check_finite(1.0 / zero64), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(v_test_xyzw_finite(V_C_ONE), "Finite check is broken on that platform/compiler: %s");
  finiteCheckPassed &= verify(!v_test_xyzw_finite(v_ldu(p3)), "Finite check is broken on that platform/compiler: %s");
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(nanCheckPassed);
  G_ASSERT(finiteCheckPassed);
#endif
}

DAGOR_NOINLINE void verify_nan_finite_checks()
{
#if defined(__clang__) || defined(__GNUC__)
  volatile float finf32 = __builtin_inff();
#else
  volatile float finf32 = INFINITY;
#endif
  verify_nan_finite_checks(0x7fc00000, 0x7ff8000000000000ULL, finf32);
}
