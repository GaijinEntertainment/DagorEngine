#include <image/dag_tga.h>
#include <image/dag_texPixel.h>

static double g0(double x, double y, double p) { return (x - y * tan(p)) / cos(p); }

static double gp(double x, double y, double p) { return -y * pow(1. / cos(p), 3.) + (1. / cos(p)) * tan(p) * (x - y * tan(p)); }

static double kelvin(double x, double y)
{
  int numIter = 1000;
  double h = PI / (2. * numIter);
  double p = 0.0001;
  double sum = 0.;
  for (int i = 0; i < numIter; i++)
  {
    double pheda = -(PI / 2) + (2 * i + 1) * h;
    double g1 = g0(x, y, pheda);
    double gp1 = gp(x, y, pheda);
    double cg1 = cos(g1);
    double sgp1 = sin(gp1 * h);
    double t1 = p * 2 * h * cg1;
    double t2 = 2 * gp1 * cg1 * sgp1;
    sum = sum + (t1 + t2) / (p + gp1 * gp1);
  }
  return sum;
}

void generate_wake()
{
  const int sz = 1280;

  float *hm = new float[sz * sz];
  memset(hm, 0, sizeof(float) * sz * sz);

  TexImage32 *im = load_tga32("e:\\Temp\\water\\src.tga", tmpmem);
  TexPixel32 *px = im->getPixels();

#if 0
  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
      hm[y * sz + x] = -((float)px[y * sz + x].r - 163);

  for (int x = 0; x < 155; x++)
    for (int y = 0; y < sz; y++)
      if (hm[y * sz + x] < 0.f)
        hm[y * sz + x] = 0.f;

  for (int x = 1150; x < 1152; x++)
    for (int y = 0; y < sz; y++)
      if (hm[y * sz + x] < 0.f)
        hm[y * sz + x] = 0.f;

  for (int x = 0; x < sz; x++)
    for (int y = 128; y < 130; y++)
      if (hm[y * sz + x] < 0.f)
        hm[y * sz + x] = 0.f;

  for (int x = 0; x < sz; x++)
    for (int y = 1150; y < 1152; y++)
      if (hm[y * sz + x] < 0.f)
        hm[y * sz + x] = 0.f;
#endif

  int time = get_time_msec();
  float xyScale = 0.035f;
  for (int y = 0; y < 1024; y++)
  {
    debug("y=%d", y);
    for (int x = 0; x < 1024; x++)
      hm[(y + 128) * sz + (x + 128)] = -50.f * kelvin(x * xyScale, (y - 512.f) * xyScale);
  }
  debug("%dms", get_time_msec() - time);


  // Kill sharp wave around bow.

  for (int x = 0; x < 200; x++)
    for (int y = 0; y < sz; y++)
      if (hm[y * sz + x] < 0.f)
        hm[y * sz + x] = 0.f;


  // Linear fade out at stern.

  int wd = 100;
  for (int x = 1152 - wd; x < 1152; x++)
    for (int y = 0; y < sz; y++)
      hm[y * sz + x] *= (1152 - x - 1) / (float)wd;


  // Circular fade out.

  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
    {
      float r = sqrtf((x - 160.f) * (x - 160.f) + (y - sz / 2.f) * (y - sz / 2.f));
      hm[y * sz + x] *= cvt(r, 500.f, 1100.f, 1.f, 0.f);
    }


  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
    {
      px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b = clamp((int)(1.f * hm[y * sz + (sz - x - 1)] + 127.5f), 0, 255);
      px[y * sz + x].a = 255;
    }
  save_tga32("e:\\Temp\\water\\test.tga", im);


  float tgaBaseScale = 0.5f;
  float tgaDiffScale = 10.f;

  const int steps = 4;
  float sigmas[steps] = {3, 5, 10, 20};

  for (int step = 0; step < steps; step++)
  {
    float sigma = sigmas[step];

    const int hkSz = 20;
    const int kSz = 2 * hkSz + 1;


    // Build gaussian kernel.

    float *kernel = new float[kSz * kSz];
    for (int kx = -hkSz; kx <= hkSz; kx++)
      for (int ky = -hkSz; ky <= hkSz; ky++)
        kernel[(ky + hkSz) * kSz + kx + hkSz] =
          (1.f / sqrtf(2.f * PI * sigma * sigma)) * expf(-(kx * kx + ky * ky) / (2.f * sigma * sigma));

    float sum = 0.f;
    for (int k = 0; k < kSz * kSz; k++)
      sum += kernel[k];
    for (int k = 0; k < kSz * kSz; k++)
      kernel[k] /= sum;

    debug("kernel at corner = %g", kernel[0]);


    // Blur.

    float *blur = new float[sz * sz];
    for (int x = 0; x < sz; x++)
      for (int y = 0; y < sz; y++)
      {
        float b = 0;
        for (int xx = x - hkSz; xx <= x + hkSz; xx++)
          for (int yy = y - hkSz; yy <= y + hkSz; yy++)
          {
            float v = (xx >= 0 && xx < sz && yy >= 0 && yy < sz) ? hm[yy * sz + xx] : 0.f;
            b += v * kernel[(yy - y + hkSz) * kSz + xx - x + hkSz];
          }
        blur[y * sz + x] = b;
      }


    // diff = sharp - blurred.

    float *diff = new float[sz * sz];
    for (int x = 0; x < sz; x++)
      for (int y = 0; y < sz; y++)
        diff[y * sz + x] = hm[y * sz + x] - blur[y * sz + x];


    // Kill negative spectral components of bow wave.

    int filtWd = 270;
    for (int x = 0; x < filtWd; x++)
      for (int y = 0; y < sz; y++)
      {
        float lim = cvt(x, filtWd, 205, -10, 0);
        if (diff[y * sz + x] < lim)
          diff[y * sz + x] = lim;
      }

    for (int x = 0; x < sz - 1055; x++)
    {
      for (int y = 0; y < 615; y++)
        if (diff[y * sz + x] < 0)
          diff[y * sz + x] = 0;
      for (int y = sz - 615; y < sz; y++)
        if (diff[y * sz + x] < 0)
          diff[y * sz + x] = 0;
    }


    // Save sharp wave for reference.

    for (int x = 0; x < sz; x++)
      for (int y = 0; y < sz; y++)
      {
        px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b =
          clamp((int)(tgaBaseScale * hm[y * sz + (sz - x - 1)] + 127.5f), 0, 255);
        px[y * sz + x].a = 255;
      }
    save_tga32(String(0, "e:\\Temp\\water\\blur%d.tga", step), im);


    // Next sharp = blurred.

    for (int x = 0; x < sz; x++)
      for (int y = 0; y < sz; y++)
        hm[y * sz + x] = blur[y * sz + x];


    // Save laplacian pyramid slice.

    debug("step=%d", step);
    for (int x = 0; x < sz; x++)
      for (int y = 0; y < sz; y++)
      {
        px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b =
          clamp((int)(tgaDiffScale * diff[y * sz + (sz - x - 1)] + 127.5f), 0, 255);
        px[y * sz + x].a = 255;
      }
    save_tga32(String(0, "e:\\Temp\\water\\step%d.tga", step), im);
  }


  // Last slice of laplacian pyramid.

  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
    {
      px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b = clamp((int)(1.f * hm[y * sz + (sz - x - 1)] + 127.5f), 0, 255);
      px[y * sz + x].a = 255;
    }
  save_tga32("e:\\Temp\\water\\base.tga", im);


  ExitProcess(0);
}


void generate_foam()
{
  const int sz = 1200;

  float *hm = new float[sz * sz];
  memset(hm, 0, sizeof(float) * sz * sz);

  TexImage32 *im = load_tga32("e:\\Temp\\water\\src.tga", tmpmem);
  TexPixel32 *px = im->getPixels();

  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
      hm[y * sz + x] = (float)px[y * sz + x].r - 127;


  float reqWaveHeight = 0.f;
  float hmToCont = 4.f;

  float *cont = new float[sz * sz];
  memset(cont, 0, sizeof(float) * sz * sz);
  for (int y = 0; y < sz; y++)
    for (int x = 1; x < sz; x++)
    {
      if (hm[y * sz + x] > reqWaveHeight && hm[y * sz + x] > hm[y * sz + x - 1])
      {
        for (int xx = x + 2; xx < sz; xx++)
          if (hm[y * sz + xx] < hm[y * sz + x])
          {
            cont[y * sz + (xx + x) / 2] = hmToCont * hm[y * sz + x];
            break;
          }
          else if (hm[y * sz + xx] > hm[y * sz + x])
            break;
      }
    }


  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
    {
      px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b = clamp((int)(cont[y * sz + x] + 0.5f), 0, 255);
      px[y * sz + x].a = 255;
    }
  save_tga32("e:\\Temp\\water\\test_contour.tga", im);


  int blurL = 400;
  float frontL = 0.05f;
  float tailAmp = 0.7f;

  float *blur = new float[sz * sz];
  memcpy(blur, cont, sizeof(float) * sz * sz);

  for (int b = 1; b <= blurL; b++)
    for (int y = 0; y < sz; y++)
    {
      for (int x = 0; x + b < sz; x++)
      {
        if (cont[y * sz + x] == 0.f)
          continue;

        float t = (float)b / blurL;
        t = clamp(t / clamp(cont[y * sz + x] / 255.f, 0.01f, 1.f), 0.f, 1.f);

        float mul = 0.f;
        if (t < frontL)
          mul = 1.f;
        else if (t < 2.f * frontL)
          mul = cvt(t, frontL, 2.f * frontL, 1.f, tailAmp);
        else
          mul = cvt(t, 2.f * frontL, 1.f, tailAmp, 0.f);

        float bl = cont[y * sz + x] * mul;
        blur[y * sz + x + b] = max(blur[y * sz + x + b], bl);
      }
    }

  for (int x = 0; x < sz; x++)
    for (int y = 0; y < sz; y++)
    {
      px[y * sz + x].r = px[y * sz + x].g = px[y * sz + x].b = clamp((int)(blur[y * sz + sz - x - 1] + 0.5f), 0, 255);
      px[y * sz + x].a = 255;
    }
  save_tga32("e:\\Temp\\water\\out_foam.tga", im);


  ExitProcess(0);
}
