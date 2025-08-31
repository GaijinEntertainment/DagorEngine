//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !_MSC_VER || __clang__ || defined(__ATOMIC_SEQ_CST)
#error This file is for MSVC only! Not even clang-cl!
#endif

#if defined(_M_IX86) || defined(_M_X64)
#include <supp/dag_msvc_atomics_x86.h>
#elif defined(_M_ARM64)
#include <supp/dag_msvc_atomics_arm.h>
#else
#error "Unsupported architecture!"
#endif
