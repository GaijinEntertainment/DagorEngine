//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

struct HmapDetLayerProps
{
  struct Param
  {
    enum
    {
      PT_tex,
      PT_int,
      PT_float,
      PT_color,
      PT_bool,
      PT_float2
    };
    SimpleString name;
    SimpleString texType;
    int type;
    float pmin, pmax;
    union
    {
      float defValF[4];
      int defValI[4];
    };
    int regIdx;
  };

  SimpleString name;
  SimpleString hlslCode;
  Tab<Param> param;
  int vs, ps, vd, prog;

  bool canOutSplatting;
  bool needsMask;

  ~HmapDetLayerProps();
};
DAG_DECLARE_RELOCATABLE(HmapDetLayerProps);
