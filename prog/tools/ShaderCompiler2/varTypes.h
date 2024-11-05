// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  common variant types
/************************************************************************/
#ifndef __VARTYPES_H
#define __VARTYPES_H

#include <generic/dag_range.h>

// you must recompile compiler, program & recompile all shader-cache files after changing this defile
// #define NO_SHVAR_SPACE_OPT

namespace ShaderVariant
{
// limitations of this variant system:
//////////////////////////////////////////////
// variable flag types              0..255
// (mode id or interval index)      0..65535
// max number of intervals          65534
// num of different interval values 127
// number of static variables       unlimited
// number of dynamic variables      unlimited
#if defined(NO_SHVAR_SPACE_OPT)
typedef int BaseType;
typedef int ExtType;
typedef int ValueType;
#else
typedef unsigned char BaseType;
typedef unsigned short ExtType;
typedef signed char ValueType;
#endif
typedef Range<ValueType> ValueRange;


/************************************************************************/
/* variant types
/************************************************************************/
enum VarType : BaseType
{
  VARTYPE_MODE = (BaseType)1,
  VARTYPE_INTERVAL = (BaseType)2,
  VARTYPE_GLOBAL_INTERVAL = (BaseType)3,
  VARTYPE_GL_OVERRIDE_INTERVAL = (BaseType)4,
};


/************************************************************************/
/* variant subtype for VARTYPE_MODE (must be < __VARTYPE_BASE)
/************************************************************************/
enum VarTypeModeID
{
  TWO_SIDED = (ExtType)5,
  REAL_TWO_SIDED = (ExtType)6,
};
} // namespace ShaderVariant

/************************************************************************/
/* variant subtype for VARTYPE_INTERVAL (must be < __VARTYPE_BASE)
/************************************************************************/
enum VarTypeIntervalID
{
  INTERVAL_NOT_INIT = 65535U,
};


#endif //__VARTYPES_H
