#include <image/dag_texPixel.h>
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>

static int normalize(int val, int max_val)
{
  if (val < 0)
    return val + max_val;
  if (val >= max_val)
    return val - max_val;
  return val;
}
typedef unsigned char TexPixel8;

//==============================================================================
void generate_texture_alpha(TexPixel8 *source, int srcW, int srcH, int srcPitch, TexPixel8 *dest, int dstW, int dstH, int dstPitch,
  int filter, int atest, bool wrapU, bool wrapV)
{
  float blockSizeX = (srcW / (float)dstW);
  float blockSizeY = (srcH / (float)dstH);

  float offsetX = (blockSizeX - 1) * 0.5f;
  float offsetY = (blockSizeY - 1) * 0.5f;
  for (int y = 0; y < dstH; ++y)
  {
    // int yOrig = y*srcH/dstH;
    float orig_y = ((float)(srcH - blockSizeY) * (float)y / (float)((dstH > 1 ? dstH : 2) - 1)) + offsetY;
    int yOrig = int(orig_y);
    for (int x = 0; x < dstW; ++x, ++dest)
    {
      // int xOrig = x*srcW/dstW;
      float orig_x = ((float)(srcW - blockSizeX) * (float)x / (float)((dstW > 1 ? dstW : 2) - 1)) + offsetX;
      int xOrig = int(orig_x);
      bool isTransp = source[xOrig + yOrig * srcPitch] <= atest;
      int nearestDist2 = filter * filter;
      for (int fi = 1; fi <= filter; ++fi)
      {
        int lj = yOrig - fi, hj = yOrig + fi;
        int li = xOrig - fi, hi = xOrig + fi;
        if (!wrapV)
        {
          lj = max(0, lj), hj = min(srcH - 1, hj);
        }
        if (!wrapU)
        {
          li = max(0, li), hi = min(srcW - 1, hi);
        }
        int nhj = normalize(hj, srcH), nlj = normalize(lj, srcH);
        int nhi = normalize(hi, srcW), nli = normalize(li, srcW);
#define BODY(xi, yj, dxi, dyj)                                                    \
  {                                                                               \
    if ((source[xi + yj * srcPitch] <= atest) != isTransp)                        \
    {                                                                             \
      int distSq = (dxi - xOrig) * (dxi - xOrig) + (dyj - yOrig) * (dyj - yOrig); \
      if (distSq < nearestDist2)                                                  \
        nearestDist2 = distSq;                                                    \
    }                                                                             \
  }

        for (int j = lj; j <= hj; ++j)
        {
          int nj = normalize(j, srcH);
          BODY(nli, nj, li, j)
          BODY(nhi, nj, hi, j)
        }
        for (int i = li + 1; i <= hi - 1; ++i)
        {
          int ni = normalize(i, srcW);
          BODY(ni, nlj, i, lj)
          BODY(ni, nhj, i, hj)
        }
        if (nearestDist2 < fi * fi)
          break;
      }
      float dist = sqrtf(nearestDist2) / filter;
      if (isTransp)
        dist = -dist;
      int distI = 127 + dist * 127;
      if (distI < 0)
        distI = 0;
      else if (distI > 255)
        distI = 255;
      *dest = distI;
    }
    dest += dstPitch - dstW;
  }
}
