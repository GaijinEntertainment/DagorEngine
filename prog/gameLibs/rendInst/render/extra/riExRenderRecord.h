#pragma once

#include <shaders/dag_shaders.h>
#include <shaders/dag_renderStateId.h>
#include <math/integer/dag_IPoint2.h>


struct RIExRenderRecord
{
  const ShaderElement *curShader;
  uint32_t prog;
  shaders::RenderStateId rstate;
  enum : uint32_t
  {
    DISABLE_OPTIMIZATION_BIT_STATE = 0x80000000
  };
  uint32_t state;
  uint32_t tstate;
  uint32_t cstate;
  uint16_t cv;
  uint16_t poolOrder;
  uint16_t vstride;
  uint8_t vbIdx, drawOrder_stage;
  IPoint2 ofsAndCnt;
  int si, numf, bv;
  uint16_t texLevel;
  uint8_t isTree : 1, isTessellated : 1;
  enum
  {
    STAGE_BITS = 6,
    STAGE_MASK = (1 << STAGE_BITS) - 1
  };
#if DAGOR_DBGLEVEL > 0
  uint8_t lod;
#else
  static constexpr uint8_t lod = 0;
#endif
  RIExRenderRecord(const ShaderElement *curShader, int cv, uint32_t prog, uint32_t state_, shaders::RenderStateId rstate,
    uint32_t tstate, uint32_t cstate, uint16_t poolOrder, uint16_t vstride, uint8_t vbIdx, uint16_t drawOrder_stage, IPoint2 ofsAndCnt,
    int si, int numf, int bv, int texLevel, int isTree, int isTessellated
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
    ofsAndCnt(ofsAndCnt),
    si(si),
    numf(numf),
    bv(bv),
    texLevel(texLevel),
    isTree(isTree),
    isTessellated(isTessellated)
#if DAGOR_DBGLEVEL > 0
    ,
    lod(lod)
#endif
  {}
};
