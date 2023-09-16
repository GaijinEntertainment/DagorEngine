// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGOR_FX_BAKEDFX_H
#define _GAIJIN_DAGOR_FX_BAKEDFX_H
#pragma once


#include <generic/dag_smallTab.h>
#include <fx/dag_fxInterface.h>


enum
{
  BAKED_FX_FLG_SHADER = 0x000000FF,

  BAKED_FX_FLG_FACING_MASK = 0x00000300,
  BAKED_FX_FLG_FREE = 0x00000000,
  BAKED_FX_FLG_FACING = 0x00000100,
  BAKED_FX_FLG_AXIS_FACING = 0x00000200,
};


enum
{
  BAKED_FX_NUM_TEX_SLOTS = 4,
};


class BakedFxMesh
{
public:
  SmallTab<EffectsInterface::StdParticleVertex, MidmemAlloc> verts;
  SmallTab<int, MidmemAlloc> indices;
};


#endif
