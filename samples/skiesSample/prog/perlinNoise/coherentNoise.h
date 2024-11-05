// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct PerlinNoise
{
  enum
  {
    B = 0x100
  };

  int p[B + B + 2];
  float g3[B + B + 2][3];
  float g2[B + B + 2][2];
  float g1[B + B + 2];

  void perlin_init(int seed);
  float perlin_noise3(float x, float y, float z, int PM);
};
