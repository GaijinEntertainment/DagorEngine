// inspired by https://www.activision.com/cdn/research/siggraph_2018_opt.pdf
#pragma once
#include <math.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>

static constexpr bool useMicroSurface = false;

float AlphaToMicroNormalLengthF(float alpha)
{
  if (alpha == 0.0f)
    return 1.0f;
  if (alpha == 1.0f)
    return 0.5f;

  float a = sqrtf(1.0f - alpha * alpha);

  return a / (a + (1 - a * a) * atanhf(a));
}

float AlphaToMacroNormalLengthF(float alpha)
{
  if (alpha == 0.0f)
    return 1.0f;
  if (alpha == 1.0f)
    return 2.0f / 3.0f;

  float a = sqrtf(1.0f - alpha * alpha);

  return (a - (alpha * alpha) * atanhf(a)) / (a * a * a);
}

double AlphaToMicroNormalLength(double alpha)
{
  if (alpha == 0.0)
    return 1.0;
  if (alpha == 1.0)
    return 0.5;

  double a = sqrt(1.0 - alpha * alpha);

  return a / (a + (1 - a * a) * atanh(a));
}

double AlphaToMacroNormalLength(double alpha)
{
  if (alpha == 0.0)
    return 1.0;
  if (alpha == 1.0)
    return 2.0f / 3.0f;

  double a = sqrt(1.0f - alpha * alpha);

  return (a - (alpha * alpha) * atanh(a)) / (a * a * a);
}

#include "normal_len_to_alpha.cpp.inc"
struct NormalLengthToAlpha
{
  float *lut = NULL;
  int N = 768;
  float lenScale = 2, lenBias = -1;
  bool owned = false;
  bool micro = false;
  ~NormalLengthToAlpha()
  {
    if (owned)
      delete[] lut;
  }
  void calc()
  {
    owned = true;
    lut = new float[N + 1];

    float *errors = new float[N];
    memset(lut, 0, sizeof(float) * (N + 1));
    for (uint32_t i = 0; i < N; ++i)
      errors[i] = 1e6f;
    const float minLen = micro ? AlphaToMicroNormalLength(1) : AlphaToMacroNormalLength(1); // or use macro?
    lenScale = 1. / (1. - minLen);
    lenBias = -minLen * lenScale;

    lut[N] = 0.f;
    int n = N;
    int prevLutI = -1;
    for (uint32_t i = 0; i < n; ++i)
    {
      const float alpha = i / float(n - 1);
      const float len = micro ? AlphaToMicroNormalLength(alpha) : AlphaToMacroNormalLength(alpha);
      const float lutF = saturate(len * lenScale + lenBias) * N;
      const float err = lutF - floor(lutF);
      int lutI = int(lutF);
      if (lutI >= N || lutI <= 0)
        continue;
      // printf("%f: len %f lutF %f err %f, was %f, lutI = %d\n", alpha, len, lutF, err, errors[lutI], lutI);
      if (errors[lutI] > err)
      {
        lut[lutI] = alpha;
        errors[lutI] = err;
      }
      if (prevLutI > lutI)
      {
        float alphaStart = (i - 1) / float(n - 1), alphaEnd = alpha;
        float lenStart = micro ? AlphaToMicroNormalLength(alphaStart) : AlphaToMacroNormalLength(alphaStart), lenEnd = len;
        for (int bi = 0; bi < 12; ++bi)
        {
          const float alphaH = (alphaStart + alphaEnd) * 0.5f;
          const float lenH = micro ? AlphaToMicroNormalLength(alphaH) : AlphaToMacroNormalLength(alphaH);
          const float lutH = saturate(lenH * lenScale + lenBias) * N;
          const float errH = lutH - floor(lutH);
          const int lutHI = int(lutH);
          if (errors[lutHI] > errH)
          {
            // printf("bil %f: len %f lutF %f err %f, was %f, lutI = %d\n", alphaH, lenH, lutH, err, errors[lutHI], lutHI);
            lut[lutHI] = alphaH;
            errors[lutHI] = errH;
          }
          if (lutHI == prevLutI)
            alphaStart = alphaH;
          else
            alphaEnd = alphaH;
        }
      }
      prevLutI = lutI;
    }
    lut[0] = 1.f;
    for (uint32_t i = 1; i < N; ++i)
    {
      if (errors[i] >= 1e2)
        logerr("incorrect normal to alpha lut %d:%f", i, lut[i]);
    }
    delete[] errors;
  }
  NormalLengthToAlpha(bool micro) : micro(micro)
  {
#if NORMAL_LEN_TO_ALPHA_MACRO
    if (!micro)
    {
      lenScale = normal_len_scale_macro;
      lenBias = normal_len_bias_macro;
      N = normal_len_to_alpha_lut_size_macro;
      lut = normal_len_to_alpha_lut_macro;
      return;
    }
#endif
#if NORMAL_LEN_TO_ALPHA_MICRO
    if (micro) //-V547
    {
      lenScale = normal_len_scale_micro;
      lenBias = normal_len_bias_micro;
      N = normal_len_to_alpha_lut_size_micro;
      lut = normal_len_to_alpha_lut_micro;
      return;
    }
#endif
    calc(); //-V779
  }
  void print()
  {
    printf("//NormalLengthToAlpha::print\n");
    printf("#define NORMAL_LEN_TO_ALPHA_%s 1\n", micro ? "MICRO" : "MACRO");
    printf("float normal_len_scale_%s = %.7f;\n", micro ? "micro" : "macro", lenScale);
    printf("float normal_len_bias_%s = %.7f;\n", micro ? "micro" : "macro", lenBias);
    printf("uint32_t normal_len_to_alpha_lut_size_%s = %d;\n", micro ? "micro" : "macro", N);
    printf("float normal_len_to_alpha_lut_%s[%d] =\n{\n", micro ? "micro" : "macro", N + 1);
    for (int i = 0; i < N; ++i)
      printf("%.*f,%s", FLT_DIG + 2, lut[i], (i & 7) == 7 ? "\n" : " ");
    printf("%f\n", lut[N]);
    printf("};\n");
  }
  // rational fits found in mathematica, error of <0.3%
  float lenToAlphaR1(float x) const
  {
    return (1 + x * (-1.95812 + 0.958152 * x)) / (-0.202196 + x * (1.49241 + x * (-1.96739 + x * 0.681675)));
  }
  // rational fit found in mathematica, error of 0.3%
  float alphaToLenR1(float x) const
  {
    return (1.0002663457579501 + x * (4.457287753851844 + x * (-0.7699221992262305 + x * 0.1827258598017819))) /
           (1 + x * (4.501750055256134 + 1.8032494712366847 * x));
  }
  // rational fit found in mathematica, error of 0.8%
  float lenToAlphaR2(float x) const { return (1 + x * (-1.9104 + 0.910538 * x)) / (0.0916413 + x * (0.334935 + x * -0.413151)); }
  // rational fit found in mathematica, error of <0.8%
  float alphaToLenR2(float x) const // x^2
  {
    return (1.00048 + x * (3.89521 - 0.0976433 * x)) / (1 + x * (3.95333 + x * 2.2447));
  }
  float lenToAlpha(float normal_len) const
  {
    // linear filtering
    const float param = saturate(normal_len * lenScale + lenBias);
    const float li = param * N;
    const float flooredLi = floorf(li);
    const int sampleLi = int(flooredLi);
    return lerp(lut[clamp(sampleLi, 0, N + 1)], lut[clamp(sampleLi + 1, 0, N + 1)], li - flooredLi);
    // point filtering
    return lut[clamp(int(N * (normal_len * lenScale + lenBias)), 0, N)];
  }
};
