#pragma once

#define DEOBFUSCATE_ZSTD_DATA obfusc_vrom_data
#define OBFUSCATE_ZSTD_DATA   obfusc_vrom_data
static inline void obfusc_vrom_data(void *data, int data_sz)
{
  static const unsigned zstd_xor_p[4] = {0xAA55AA55, 0xF00FF00F, 0xAA55AA55, 0x12481248};

  if (data_sz < 16)
    return;
  unsigned *w = (unsigned *)data;
  w[0] ^= zstd_xor_p[0];
  w[1] ^= zstd_xor_p[1];
  w[2] ^= zstd_xor_p[2];
  w[3] ^= zstd_xor_p[3];
  if (data_sz >= 32)
  {
    w += (data_sz / 4) - 4;
    w[0] ^= zstd_xor_p[3];
    w[1] ^= zstd_xor_p[2];
    w[2] ^= zstd_xor_p[1];
    w[3] ^= zstd_xor_p[0];
  }
}
