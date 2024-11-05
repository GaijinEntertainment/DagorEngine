// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_dxtCompress.h>
#include <debug/dag_fatal.h>

namespace colors
{
struct Color565
{
  unsigned int nBlue : 5, nGreen : 6, nRed : 5;
};
struct Color8888
{
  unsigned char r, g, b, a;
};
} // namespace colors

using namespace colors;
typedef unsigned char byte;

static inline unsigned Unpack565(byte const *packed, byte *colour)
{
  // build the packed value
  unsigned value = (int)packed[0] | ((int)packed[1] << 8);

  // get the components in the stored range
  byte red = (byte)((value >> 11) & 0x1f);
  byte green = (byte)((value >> 5) & 0x3f);
  byte blue = (byte)(value & 0x1f);

  // scale up to 8 bits
  colour[0] = (blue << 3) | (blue >> 2);
  colour[1] = (green << 2) | (green >> 4);
  colour[2] = (red << 3) | (red >> 2);
  colour[3] = 255;

  // return the value
  return value;
}
template <bool isDxt1>
static inline void decompress_dxt1_block(byte *rgba, int stride, const byte *bytes)
{
  // get the block bytes
  // unpack the endpoints
  byte codes[16];
  int a = Unpack565(bytes, codes);
  int b = Unpack565(bytes + 2, codes + 4);
  (void)(a); // to avoid compiler warning
  (void)(b); // to avoid compiler warning

  // generate the midpoints
  for (int i = 0; i < 3; ++i)
  {
    int c = codes[i];
    int d = codes[4 + i];

    if (isDxt1 && a <= b)
    {
      codes[8 + i] = (byte)((c + d) / 2);
      codes[12 + i] = 0;
    }
    else
    {
      codes[8 + i] = (byte)((2 * c + d) / 3);
      codes[12 + i] = (byte)((c + 2 * d) / 3);
    }
  }

  // fill in alpha for the intermediate values
  if (isDxt1)
  {
    codes[8 + 3] = 255;
    codes[12 + 3] = (isDxt1 && a <= b) ? 0 : 255;
  }

  // unpack the indices
  byte indices[16];
  for (int i = 0; i < 4; ++i)
  {
    byte *ind = indices + 4 * i;
    byte packed = bytes[4 + i];

    ind[0] = packed & 0x3;
    ind[1] = (packed >> 2) & 0x3;
    ind[2] = (packed >> 4) & 0x3;
    ind[3] = (packed >> 6) & 0x3;
  }

  // store out the colours
  for (int i = 0, y = 0; y < 4; ++y, rgba += stride - 16)
    for (int x = 0; x < 4; ++x, ++i, rgba += 4)
    {
      byte offset = 4 * indices[i];
      rgba[0] = codes[offset + 0];
      rgba[1] = codes[offset + 1];
      rgba[2] = codes[offset + 2];
      if (isDxt1)
        rgba[3] = codes[offset + 3];
    }
}

void decompress_dxt1(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 8)
      decompress_dxt1_block<true>(decompressedData, row_pitch, src_data);
}

void decompress_dxt5(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      decompress_dxt1_block<false>(decompressedData, row_pitch, src_data + 8);

      unsigned char alphas[8];

      alphas[0] = src_data[0];
      alphas[1] = src_data[1];
      unsigned char *alphamask = src_data + 2;
      if (alphas[0] > alphas[1])
      {
        alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;
        alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;
        alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;
        alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;
        alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;
        alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;
      }
      else
      {
        alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;
        alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;
        alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;
        alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;
        alphas[6] = 0x00;
        alphas[7] = 0xFF;
      }
      unsigned int bits = *((unsigned int *)alphamask);
      for (int j = 0; j < 2; j++)
      {
        for (int i = 0; i < 4; i++)
        {
          unsigned int Offset = (j)*row_pitch + (i)*4 + 3;
          decompressedData[Offset] = alphas[bits & 0x07];
          bits >>= 3;
        }
      }

      bits = *((unsigned int *)&alphamask[3]);
      for (int j = 2; j < 4; j++)
      {
        for (int i = 0; i < 4; i++)
        {
          unsigned int Offset = (j)*row_pitch + (i)*4 + 3;
          decompressedData[Offset] = alphas[bits & 0x07];
          bits >>= 3;
        }
      }
    }
}

void decompress_dxt3(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      decompress_dxt1_block<false>(decompressedData, row_pitch, src_data + 8);

      unsigned offset = 3;
      int id = 0, bitshift = 4;
      for (int j = 0; j < 4; j++, offset += row_pitch - 16)
        for (int i = 0; i < 4; i++, offset += 4, id++, bitshift = (~bitshift) & 4)
          decompressedData[offset] = (src_data[id >> 1] << bitshift);
    }
}

void decompress_dxt(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data, bool is_dxt1)
{
  if (is_dxt1) // src_fmt == D3DFMT_DXT1
    decompress_dxt1(decompressedData, lw, lh, row_pitch, src_data);
  else
    decompress_dxt5(decompressedData, lw, lh, row_pitch, src_data);
}

void decompress_dxt1_downsample4x(unsigned char *, int, int, int, unsigned char *, unsigned char *) { DAG_FATAL("not implemented"); }
void decompress_dxt5_downsample4x(unsigned char *, int, int, int, unsigned char *, unsigned char *) { DAG_FATAL("not implemented"); }
void decompress_dxt3_downsample4x(unsigned char *, int, int, int, unsigned char *, unsigned char *) { DAG_FATAL("not implemented"); }
