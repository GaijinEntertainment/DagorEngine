// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/drawOrder.h"

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderState.h>
#include <shaders/dag_renderStateId.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_shaderVarsUtils.h>


struct RIExRenderRecord
{
  const ShaderElement *curShader;
  uint32_t prog;
  shaders::RenderStateId rstate;
  ShaderStateBlockId state;
  shaders::TexStateIdx tstate;
  shaders::ConstStateIdx cstate;
  uint16_t cv;
  uint16_t poolOrder;
  uint16_t vstride;
  uint8_t vbIdx;
  PackedDrawOrder drawOrder_stage;
  uint8_t primitive;
  uint8_t elemOrder;
  IPoint2 ofsAndCnt;
  int si, sv, numv, numf, bv;
  uint16_t texLevel;
  uint8_t isTree : 1, isTessellated : 1, isSWVertexFetch : 1, disableOptimization : 1;
#if DAGOR_DBGLEVEL > 0
  uint8_t lod;
#else
  static constexpr uint8_t lod = 0;
#endif
  RIExRenderRecord(const ShaderElement *curShader, int cv, uint32_t prog, ShaderStateBlockId state_, shaders::RenderStateId rstate,
    shaders::TexStateIdx tstate, shaders::ConstStateIdx cstate, uint16_t poolOrder, uint16_t vstride, uint8_t vbIdx,
    PackedDrawOrder drawOrder_stage, uint8_t elem_order, uint8_t primitive, IPoint2 ofsAndCnt, int si, int sv, int numv, int numf,
    int bv, int texLevel, int isTree, int isTessellated, bool disable_optimization
#if DAGOR_DBGLEVEL > 0
    ,
    uint8_t lod
#endif
    ) :
    curShader(curShader),
    cv(cv < 0 ? ~0 : cv),
    prog(prog),
    state(state_),
    rstate(rstate),
    tstate(tstate),
    cstate(cstate),
    poolOrder(poolOrder),
    vstride(vstride),
    vbIdx(vbIdx),
    drawOrder_stage(drawOrder_stage),
    primitive(primitive),
    ofsAndCnt(ofsAndCnt),
    si(si),
    sv(sv),
    numv(numv),
    numf(numf),
    bv(bv),
    texLevel(texLevel),
    isTree(isTree),
    isTessellated(isTessellated),
    isSWVertexFetch(false),
    disableOptimization(disable_optimization),
    elemOrder(elem_order)
#if DAGOR_DBGLEVEL > 0
    ,
    lod(lod)
#endif
  {}
};
