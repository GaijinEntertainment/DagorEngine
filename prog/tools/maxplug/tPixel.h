#ifndef TPIXEL_H
#define TPIXEL_H


typedef struct rgba_t
{
  unsigned char rgba[4];
} rgba_t;


union tPixel
{
  unsigned long u;
  rgba_t c;
  float fp;
};

#define FP_B 2
#define FP_G 1
#define FP_R 0
#define FP_A 3

struct fpPixel
{
  float fp[4];
  // R 0  to match fp texture format
  // G 1
  // B 2
  // A 3
};

enum PixelFormat
{
  PF_DWORD,
  PF_fp32x4
};

class TCImage
{
public:
  int width;
  int height;
  int byte_pitch;
  int nPlanes;
  int nPlanesInFile;

  PixelFormat pf;
  union
  {
    tPixel *pixels;
    fpPixel *fppixels;
  };
  TCImage()
  {
    pixels = 0;
    pf = PF_DWORD;
  };
};


#endif