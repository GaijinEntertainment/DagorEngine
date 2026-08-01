// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "color.h"

#include <math.h>


namespace darg
{

// matrices and transfer functions per https://bottosson.github.io/posts/oklab/

static inline float srgb_to_linear(float s) { return s <= 0.04045f ? s * (1.0f / 12.92f) : powf((s + 0.055f) / 1.055f, 2.4f); }


static inline float linear_to_srgb(float l)
{
  if (l <= 0.0f)
    return 0.0f;
  return l <= 0.0031308f ? l * 12.92f : 1.055f * powf(l, 1.0f / 2.4f) - 0.055f;
}


// decoding is per byte, so a table is exact; encoding takes arbitrary floats
struct SrgbDecodeTable
{
  float v[256];

  SrgbDecodeTable()
  {
    for (int i = 0; i < 256; ++i)
      v[i] = srgb_to_linear(i / 255.0f);
  }
};

static const SrgbDecodeTable srgb_decode;


static void linear_to_oklab(float r, float g, float b, float &out_l, float &out_a, float &out_b)
{
  const float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
  const float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
  const float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

  const float l_ = cbrtf(l), m_ = cbrtf(m), s_ = cbrtf(s);

  out_l = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
  out_a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
  out_b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
}


static void oklab_to_linear(float lab_l, float lab_a, float lab_b, float &out_r, float &out_g, float &out_b)
{
  const float l_ = lab_l + 0.3963377774f * lab_a + 0.2158037573f * lab_b;
  const float m_ = lab_l - 0.1055613458f * lab_a - 0.0638541728f * lab_b;
  const float s_ = lab_l - 0.0894841775f * lab_a - 1.2914855480f * lab_b;

  const float l = l_ * l_ * l_;
  const float m = m_ * m_ * m_;
  const float s = s_ * s_ * s_;

  out_r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
  out_g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
  out_b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
}


static inline bool is_in_srgb(float r, float g, float b)
{
  const float eps = 1e-4f;
  return r >= -eps && g >= -eps && b >= -eps && r <= 1.0f + eps && g <= 1.0f + eps && b <= 1.0f + eps;
}


ColorOklcha e3dcolor_to_oklch(E3DCOLOR col)
{
  float lab_l, lab_a, lab_b;
  linear_to_oklab(srgb_decode.v[col.r], srgb_decode.v[col.g], srgb_decode.v[col.b], lab_l, lab_a, lab_b);

  ColorOklcha out;
  out.l = lab_l;
  out.c = sqrtf(lab_a * lab_a + lab_b * lab_b);
  out.h = 0.0f;
  if (out.c >= OKLCH_ACHROMATIC_CHROMA)
  {
    out.h = RadToDeg(atan2f(lab_b, lab_a));
    if (out.h < 0.0f)
      out.h += 360.0f;
  }
  out.a = col.a / 255.0f;
  return out;
}


E3DCOLOR oklch_to_e3dcolor(const ColorOklcha &col)
{
  // no chroma fits outside this lightness range
  const float lab_l = clamp(col.l, 0.0f, 1.0f);
  const float chroma = max(col.c, 0.0f);
  const float hueRad = DegToRad(col.h);
  const float hueCos = cosf(hueRad), hueSin = sinf(hueRad);

  float r, g, b;
  oklab_to_linear(lab_l, chroma * hueCos, chroma * hueSin, r, g, b);

  if (!is_in_srgb(r, g, b))
  {
    // a path between two in-gamut Oklch colors can leave sRGB; bisect for the largest
    // chroma that fits, starting from zero chroma, which always does
    oklab_to_linear(lab_l, 0.0f, 0.0f, r, g, b);

    float lo = 0.0f, hi = chroma;
    for (int i = 0; i < 10; ++i)
    {
      const float mid = 0.5f * (lo + hi);
      float tr, tg, tb;
      oklab_to_linear(lab_l, mid * hueCos, mid * hueSin, tr, tg, tb);
      if (is_in_srgb(tr, tg, tb))
      {
        lo = mid;
        r = tr;
        g = tg;
        b = tb;
      }
      else
        hi = mid;
    }
  }

  return e3dcolor(Color4(linear_to_srgb(r), linear_to_srgb(g), linear_to_srgb(b), col.a));
}

} // namespace darg
