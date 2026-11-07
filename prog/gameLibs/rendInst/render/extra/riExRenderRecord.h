// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/drawOrder.h"

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderState.h>
#include <shaders/dag_renderStateId.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_shaderVarsUtils.h>


// fields ordered by alignment, so no padding between them: records are copied and sorted in bulk
struct RIExRenderRecord
{
  const ShaderElement *curShader;
  uint32_t voxelSurfaceId;
  uint32_t voxelDataOffset;
  shaders::CombinedDynVariantState dvState;
  IPoint2 ofsAndCnt;
  int si, sv, numv, numf, bv;
  uint16_t poolOrder;
  uint16_t vstride;
  PackedDrawOrder drawOrder_stage;
  uint8_t vbIdx;
  uint8_t primitive;
  uint8_t elemOrder;
  uint8_t texLevel; // <= TexStreamingContext::MAX_TEX_LEVEL
  uint8_t isTree : 1, isTessellated : 1, isSWVertexFetch : 1, disableOptimization : 1;
  uint8_t lod;
  RIExRenderRecord(const ShaderElement *curShader, const shaders::CombinedDynVariantState &dv_state, uint16_t poolOrder,
    uint16_t vstride, uint8_t vbIdx, PackedDrawOrder drawOrder_stage, uint8_t elem_order, uint8_t primitive, IPoint2 ofsAndCnt, int si,
    int sv, int numv, int numf, int bv, int texLevel, int isTree, int isTessellated, bool disable_optimization, uint8_t lod) :
    curShader(curShader),
    voxelSurfaceId(~0u),
    voxelDataOffset(0),
    dvState(dv_state),
    ofsAndCnt(ofsAndCnt),
    si(si),
    sv(sv),
    numv(numv),
    numf(numf),
    bv(bv),
    poolOrder(poolOrder),
    vstride(vstride),
    drawOrder_stage(drawOrder_stage),
    vbIdx(vbIdx),
    primitive(primitive),
    elemOrder(elem_order),
    texLevel(texLevel),
    isTree(isTree),
    isTessellated(isTessellated),
    isSWVertexFetch(false),
    disableOptimization(disable_optimization),
    lod(lod)
  {}
};
