#pragma once


#include <supp/dag_math.h>
#include <math/dag_color.h>
#include <math/dag_e3dColor.h>

/* standard conversion from float pixels to rgbe pixels */
/* note: you can remove the "inline"s if your compiler complains about it */
inline void float2rgbe(unsigned char *rgbe, float red, float green, float blue)
{
  float v;
  int e;

  v = red;
  if (green > v)
    v = green;
  if (blue > v)
    v = blue;
  if (v < 1e-32)
  {
    rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
  }
  else
  {
    v = frexp(v, &e) * 256.0 / v;
    rgbe[0] = (unsigned char)(red * v);
    rgbe[1] = (unsigned char)(green * v);
    rgbe[2] = (unsigned char)(blue * v);
    rgbe[3] = (unsigned char)(e + 128);
  }
}

/* standard conversion from rgbe to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */
inline void rgbe2float(float *red, float *green, float *blue, unsigned char *rgbe)
{
  float f;

  if (rgbe[3]) /*nonzero pixel*/
  {
    f = ldexp(1.0, rgbe[3] - (int)(128 + 8));
    *red = rgbe[0] * f;
    *green = rgbe[1] * f;
    *blue = rgbe[2] * f;
  }
  else
    *red = *green = *blue = 0.0;
}

/* RGBE (0xFF,0xFF,0xFF,0xFF) we treat as NAN (not a number) */
/* this function checks for exponent=+128, which is true for NANs */
/* note: if rgbeIsNan() returned true on rgbe value, this value MUST NOT be */
/*       converted to float via rgbe2float() because result will be invalid */
inline bool rgbeIsNan(unsigned char *rgbe) { return rgbe[3] == 0xFF; }

inline E3DCOLOR float2rgbe(const Color3 &src)
{
  unsigned char rgbe[4];
  float2rgbe(rgbe, src.r, src.g, src.b);
  return E3DCOLOR(rgbe[0], rgbe[1], rgbe[2], rgbe[3]);
}

inline Color3 rgbe2float(E3DCOLOR src)
{
  Color3 dst;
  unsigned char rgbe[4] = {src.r, src.g, src.b, src.a};
  rgbe2float(&dst.r, &dst.g, &dst.b, rgbe);
  return dst;
}
