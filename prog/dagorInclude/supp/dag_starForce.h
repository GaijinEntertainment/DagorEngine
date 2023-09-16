//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#ifndef STARFORCEEXP

#ifdef STARFORCE_PROTECT
#define STARFORCEEXP __declspec(dllexport)
#else
#define STARFORCEEXP
#endif

#if _TARGET_STATIC_LIB && (STARFORCE_PROTECT)
#define STARFORCEEXPCORE __declspec(dllexport)
#else
#define STARFORCEEXPCORE
#endif

#endif

//
// decorate function name for automatic Star-Force protection
//
// "ftype" can be:
// * SFINIT<v> - for callbacks functions (called before any execution of main code including static ctors)
// <v> is ordinal number specifying order of function calls
// * SFLB - loopback functions (usual protected function)
// * SFPROT - public functions (left in import table after protection, generally you don't need this)
//
// "speed" - function execution speed, can be in range [0..4] - [very slow..very fast]
//
// Note: decoration works only for global functions (i.e. no members or namespaces)
//
#ifdef STARFORCE_PROTECT
#define STARFORCE_DECORATE(ftype, speed, name) ftype##_##speed##name
#else
#define STARFORCE_DECORATE(ftype, speed, name) name
#endif
