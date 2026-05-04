// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_sdf_edt.h>
#include <memory/dag_framemem.h>
#include <math.h>

// 1D squared distance transform (in-place on f, length n).
// On input, f[i] = 0 for feature pixels, HUGE for non-feature.
// On output, f[i] = squared distance to nearest feature pixel.
void sdf::dt_1d(float *f, int n, float *tmp_d, int *tmp_v, float *tmp_z)
{
  int k = 0;
  tmp_v[0] = 0;
  tmp_z[0] = -1e20f;
  tmp_z[1] = 1e20f;

  for (int q = 1; q < n; q++)
  {
    float s;
    for (;;)
    {
      int vk = tmp_v[k];
      s = ((f[q] + (float)(q * q)) - (f[vk] + (float)(vk * vk))) / (float)(2 * q - 2 * vk);
      if (s > tmp_z[k])
        break;
      k--;
    }
    k++;
    tmp_v[k] = q;
    tmp_z[k] = s;
    tmp_z[k + 1] = 1e20f;
  }

  k = 0;
  for (int q = 0; q < n; q++)
  {
    while (tmp_z[k + 1] < (float)q)
      k++;
    int vk = tmp_v[k];
    tmp_d[q] = (float)((q - vk) * (q - vk)) + f[vk];
  }

  for (int q = 0; q < n; q++)
    f[q] = tmp_d[q];
}

// In-place blocked transpose of a w*w matrix for cache-friendly column access.
static void transpose(float *__restrict data, int w)
{
  const int BLOCK = 64;
  for (int by = 0; by < w; by += BLOCK)
    for (int bx = by; bx < w; bx += BLOCK)
    {
      int ye = by + BLOCK < w ? by + BLOCK : w;
      int xe = bx + BLOCK < w ? bx + BLOCK : w;
      if (by == bx)
      {
        for (int y = by; y < ye; y++)
          for (int x = y + 1; x < xe; x++)
          {
            float tmp = data[y * w + x];
            data[y * w + x] = data[x * w + y];
            data[x * w + y] = tmp;
          }
      }
      else
      {
        for (int y = by; y < ye; y++)
          for (int x = bx; x < xe; x++)
          {
            float tmp = data[y * w + x];
            data[y * w + x] = data[x * w + y];
            data[x * w + y] = tmp;
          }
      }
    }
}

// 2D EDT: rows, transpose, rows, transpose back.
// tmp layout: [tmp_d: w floats][tmp_z: (w+1) floats][tmp_v: w ints]
static void edt_2d(float *grid, int w)
{
  int tmp_bytes = w * sizeof(float) + w * sizeof(int) + (w + 1) * sizeof(float);
  char *tmp = (char *)framemem_ptr()->alloc(tmp_bytes);
  float *tmp_d = (float *)tmp, *tmp_z = tmp_d + w;
  int *tmp_v = (int *)(tmp_z + w + 1);

  for (int y = 0; y < w; y++)
    sdf::dt_1d(grid + y * w, w, tmp_d, tmp_v, tmp_z);

  transpose(grid, w);
  for (int x = 0; x < w; x++)
    sdf::dt_1d(grid + x * w, w, tmp_d, tmp_v, tmp_z);
  transpose(grid, w);

  framemem_ptr()->free(tmp);
}

void sdf::build_edt(const uint32_t *mask, int w, float *out_sdf)
{
  const uint32_t *__restrict mdata = mask;

  // Initialize: 0 for unset bits, huge for set bits
  for (int i = 0; i < w * w; i++)
    out_sdf[i] = ((mdata[i >> 5] >> (i & 31)) & 1) ? 1e20f : 0.0f;

  // EDT gives squared distances
  edt_2d(out_sdf, w);

  // Convert to actual distance
  for (int i = 0; i < w * w; i++)
    out_sdf[i] = sqrtf(out_sdf[i]);
}
