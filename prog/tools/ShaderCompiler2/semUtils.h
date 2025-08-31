// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  some semantic funtions
/************************************************************************/
#ifndef __SEMUTILS_H
#define __SEMUTILS_H

#include <util/dag_globDef.h>
#include "shsyn.h"

namespace semutils
{
// convert from string to integer, assert if failed
int int_number(const char *s);

// convert from string to real, assert if failed
double real_number(const char *s);

// convert from string to integer
bool try_int_number(const char *s, int &out);
// convert from string to real
bool try_real_number(const char *s, double &out);

// convert from real decl to real
double real_number(ShaderTerminal::signed_real *s);
// convert from real decl to int (given that the literal denotes an int)
int int_number(ShaderTerminal::signed_real *s);
} // namespace semutils

#endif //__SEMUTILS_H
