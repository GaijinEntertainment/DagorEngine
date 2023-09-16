#include <libTools/util/colorUtil.h>


E3DCOLOR normalize_color4(const Color4 &color, real &bright)
{
  real max = -1;
  for (int i = 0; i < 3; ++i)
    if (color[i] > max)
      max = color[i];

  if (max > 1)
  {
    const real mul = 1.0 / max;

    bright *= max;

    return ::e3dcolor(Color4(color * mul));
  }

  return ::e3dcolor(color);
}
