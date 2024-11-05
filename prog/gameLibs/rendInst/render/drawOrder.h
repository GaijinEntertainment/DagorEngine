// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mathUtils.h>
#include <shaders/dag_shaderMesh.h>

struct PackedDrawOrder
{
  struct Values
  {
    uint8_t order : 2;
    uint8_t stage : 6;
  };
  union
  {
    Values v;
    uint8_t r;
  };

  PackedDrawOrder() { r = 0; }

  PackedDrawOrder(const ShaderMesh::RElem &elem, const unsigned int stage, const int draw_order_var_id)
  {
    int drawOrder = 0;
    elem.mat->getIntVariable(draw_order_var_id, drawOrder);

    // draw_order values : -1, 0, 1
    v.order = sign(drawOrder) + 1;
    v.stage = stage;
  }

  operator uint8_t() const { return r; }
  const Values *operator->() const { return &v; }
};