// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderMesh.h>
#include <math/dag_mathBase.h>
#include <debug/dag_log.h>

inline constexpr unsigned MATERIAL_WRITE_BITS = 1;
inline constexpr unsigned ORDER_BITS = 10;
inline constexpr unsigned STAGE_BITS = 3;
inline constexpr int ORDER_BIAS = 1 << (ORDER_BITS - 1);

G_STATIC_ASSERT(MATERIAL_WRITE_BITS + ORDER_BITS + STAGE_BITS <= 16);
G_STATIC_ASSERT((1 << STAGE_BITS) >= ShaderMesh::STG_COUNT);

struct PackedDrawOrder
{
  struct Values
  {
    uint16_t order : ORDER_BITS;
    uint16_t materialWrite : MATERIAL_WRITE_BITS;
    uint16_t stage : STAGE_BITS; // In reality we only have 6 stages supported
  };
  union
  {
    Values v;
    uint16_t r;
  };

  PackedDrawOrder() { r = 0; }

  PackedDrawOrder(const ShaderMesh::RElem &elem, const unsigned int stage, const int draw_order_var_id,
    const int material_write_var_id)
  {
    r = 0;
    int drawOrder = 0;
    elem.mat->getIntVariable(draw_order_var_id, drawOrder);
    if (DAGOR_UNLIKELY(drawOrder < -ORDER_BIAS || drawOrder >= ORDER_BIAS))
    {
      LOGERR_ONCE("draw_order=%d of shader '%s' is outside [%d, %d], clamped", drawOrder, elem.mat->getShaderClassName(), //-V1096
        -ORDER_BIAS, ORDER_BIAS - 1);
      drawOrder = clamp(drawOrder, -ORDER_BIAS, ORDER_BIAS - 1);
    }

    int materialWrite = 0;
    elem.mat->getIntVariable(material_write_var_id, materialWrite);
    if (DAGOR_UNLIKELY(materialWrite < 0 || materialWrite > 1))
    {
      LOGERR_ONCE("material_write_no_edit=%d of shader '%s' is not 0 or 1, clamped", materialWrite, //-V1096
        elem.mat->getShaderClassName());
      materialWrite = clamp(materialWrite, 0, 1);
    }

    v.materialWrite = materialWrite;
    v.order = drawOrder + ORDER_BIAS;
    v.stage = stage;
  }

  operator uint16_t() const { return r; }
  const Values *operator->() const { return &v; }
};