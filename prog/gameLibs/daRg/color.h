// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>


namespace darg
{

// below this chroma the hue carries no information
static constexpr float OKLCH_ACHROMATIC_CHROMA = 1e-4f;


// Oklab in polar form: lightness, chroma, hue in degrees, alpha
struct ColorOklcha
{
  float l = 0, c = 0, h = 0, a = 0;
};


// E3DCOLOR channels are sRGB-encoded
ColorOklcha e3dcolor_to_oklch(E3DCOLOR c);

// reduces chroma until the color fits sRGB, keeping lightness and hue
E3DCOLOR oklch_to_e3dcolor(const ColorOklcha &c);


} // namespace darg
