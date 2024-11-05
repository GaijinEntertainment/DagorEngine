//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_baseDef.h>
#include <util/dag_globDef.h>

static inline bool dagor_target_code_valid(unsigned tc)
{
  switch (tc)
  {
    case _MAKE4C('PC'):
    case _MAKE4C('iOS'):
    case _MAKE4C('and'):
    case _MAKE4C('PS4'): return true;
  }
  return false;
}

static inline bool dagor_target_code_be(unsigned) { return false; }

static inline bool dagor_target_code_store_packed(unsigned tc)
{
  switch (tc)
  {
    case _MAKE4C('PC'):
    case _MAKE4C('iOS'):
    case _MAKE4C('and'):
    case _MAKE4C('PS4'): return true;
  }
  return false;
}

static inline bool dagor_target_code_supports_tex_diff(unsigned tc)
{
  switch (tc)
  {
    case _MAKE4C('PC'): return true;
  }
  return false;
}
