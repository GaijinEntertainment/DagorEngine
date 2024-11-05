// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

typedef struct
{
  float x;
  float y;
  float z;
} Vector;


// Standard Perlin 'optimised' noise functions
void Noise2(Vector vec1, float *result);
void Noise3(Vector vec1, float *result);


// Intel SIMD (PIII or CeleronII) optimised versions
void Noise24(Vector vec1, Vector vec2, Vector vec3, Vector vec4, float *result);
void Noise34(Vector vec1, Vector vec2, Vector vec3, Vector vec4, float *result);

// to initialise the 'noise' lookup arrays
void PInitNoise();
