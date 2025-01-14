//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_materialData.h>
#include <math/dag_color.h>
#include <util/dag_stdint.h>

struct ShaderMatData
{
  union VarValue
  {
    float c[4];
    real r;
    int i;
    TEXTUREID texId;
    d3d::SamplerHandle samplerHnd;

    Color4 &c4() { return reinterpret_cast<Color4 &>(c); }
    const Color4 &c4() const { return const_cast<VarValue *>(this)->c4(); }

    VarValue() { memset(c, 0, sizeof(c)); }
    bool operator==(const VarValue &val) const { return memcmp(c, val.c, sizeof(c)) == 0; }
    bool operator!=(const VarValue &val) const { return !(*this == val); }
  };

  D3dMaterial d3dm;
  const char *sclassName;

  TEXTUREID textureId[MAXMATTEXNUM];

  dag::Span<VarValue> stvar;

  int16_t matflags;
};
