// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "scalarTypes.h"

namespace stcode::cpp
{

struct ShadervarPtrInitInfo
{
  void **dst;
  uint32_t srcId;
  uint32_t pad_;
};

typedef void (*SetResourceCallback)(int, uint, const void *); // stage id, res id, handle/pointer to mem
typedef void (*GetGlobalMatrixCallback)(float4x4 *);          // dst
typedef void (*GetGlobalVecCallback)(int, float4 *);          // component, dst

typedef real (*GetTimeCallback)(real, real); // period, offset

typedef float4 (*GetTexDimCallback)(const void *, int); // tex, mip
typedef int (*GetBufSizeCallback)(const void *);        // id
typedef float4 (*GetViewportCallback)();
typedef uint64_t (*RequestSamplerCallback)(int, float4, int, int); // smp id, args.. -> sampler handle

typedef int (*CheckResourceExistanceCallback)(const void *); // res, returns 1 if exists, 0 otherwise

typedef void (*SetConstCallback)(int, unsigned int, float4 *, int); // stage, dst, val, cnt

typedef void (*GetShadervarPtrsCallback)(const ShadervarPtrInitInfo *, uint32_t); // out_data_infos, ptr_count

// For stblkcode
typedef void (*RegBindlessCallback)(void *, int, void *); // tex_ptr, reg, context
typedef void (*CreateStblkStateCallback)(const uint *, uint16_t, const uint *, uint16_t, const void *, int, bool,
  void *); // vs_tex, vs_tex_range_packed, ps_tex, ps_tex_range_packed, consts, const_cnt, multidraw_cbuf, context

typedef uint (*AcquireTexCallback)(void *); // tex_ptr, returns tid

struct CallbackTable
{
  SetResourceCallback setTex;
  SetResourceCallback setBuf;
  SetResourceCallback setConstBuf;
  SetResourceCallback setSampler;
  SetResourceCallback setRwtex;
  SetResourceCallback setRwbuf;
  SetResourceCallback setTlas;
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

  GetShadervarPtrsCallback getShadervarPtrsFromDump;

  RegBindlessCallback regBindless;

  CreateStblkStateCallback createState;

  AcquireTexCallback acquireTex;
};

} // namespace stcode::cpp
