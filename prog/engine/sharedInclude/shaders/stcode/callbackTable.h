// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "scalarTypes.h"

namespace stcode::cpp
{

typedef void (*SetResourceCallback)(int, uint, unsigned int); // stage id, res id, reg
typedef void (*SetResourceDirectCallback)(int, uint, void *); // stage id, res id, handle/pointer to mem
typedef void (*GetGlobalMatrixCallback)(float4x4 *);          // dst
typedef void (*GetGlobalVecCallback)(int, float4 *);          // component, dst

typedef real (*GetTimeCallback)(real, real); // period, offset

typedef float4 (*GetTexDimCallback)(unsigned int, int); // id, mip
typedef int (*GetBufSizeCallback)(unsigned int);        // id
typedef float4 (*GetViewportCallback)();
typedef uint64_t (*RequestSamplerCallback)(int, float4, int, int); // smp id, args.. -> sampler handle

typedef int (*CheckResourceExistanceCallback)(unsigned int); // id, returns 1 if exists, 0 otherwise

typedef void (*SetConstCallback)(int, unsigned int, float4 *, int); // stage, dst, val, cnt

typedef void *(*GetShadervarPtrCallback)(int); // var_id

struct CallbackTable
{
  SetResourceCallback setTex;
  SetResourceCallback setBuf;
  SetResourceCallback setConstBuf;
  SetResourceDirectCallback setSampler;
  SetResourceCallback setRwtex;
  SetResourceCallback setRwbuf;
  SetResourceDirectCallback setTlas;
  GetGlobalMatrixCallback getGlobTm;
  GetGlobalMatrixCallback getProjTm;
  GetGlobalMatrixCallback getViewProjTm;
  GetGlobalVecCallback getLocalViewComponent;
  GetGlobalVecCallback getLocalWorldComponent;

  GetTimeCallback getShaderGlobalTimePhase;
  GetTexDimCallback getTexDim;
  GetBufSizeCallback getBufSize;
  GetViewportCallback getViewport;
  CheckResourceExistanceCallback texExists;
  CheckResourceExistanceCallback bufExists;
  RequestSamplerCallback requestSampler;

  SetConstCallback setConst;

  GetShadervarPtrCallback getShadervarPtrFromDump;
};

} // namespace stcode::cpp
