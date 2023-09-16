//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif


  inline float halton_sequence(unsigned int index, unsigned int base)
  {
    float result = 0.0f;
    float invBase = 1.0f / base;
    float fraction = invBase;
    while (index > 0)
    {
      result += (index % base) * fraction;
      index /= base;
      fraction *= invBase;
    }
    return result;
  }

#ifdef __cplusplus
}
#endif
