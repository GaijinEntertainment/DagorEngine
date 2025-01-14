#pragma once

#define DEOBFUSCATE_ZSTD_DATA obfusc_vrom_data
#define OBFUSCATE_ZSTD_DATA   obfusc_vrom_data

#define DECL_ZSTD_XOR_KEY static const unsigned zstd_xor_p[4] = {0xAA55AA55, 0xF00FF00F, 0xAA55AA55, 0x12481248}

static inline void obfusc_vrom_data(void *data, int data_sz)
{
  DECL_ZSTD_XOR_KEY;

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


#define DEOBFUSCATE_ZSTD_DATA_PARTIAL obfusc_vrom_data_partial
#define OBFUSCATE_ZSTD_DATA_PARTIAL   obfusc_vrom_data_partial

static inline void obfusc_vrom_data_partial(void *data, int data_sz, int data_ofs, int data_total_sz)
{
  DECL_ZSTD_XOR_KEY;

  if (data_total_sz < 16)
    return;

  auto move_part = [&](void *dst, int dstOfs, int dstSize, void *src, int srcOfs, int srcSize) {
    int dstEnd = dstOfs + dstSize;
    int srcEnd = srcOfs + srcSize;

    int commonOfs = max(srcOfs, dstOfs);
    int commonEnd = min(srcEnd, dstEnd);
    int commonSize = commonEnd - commonOfs;
    if (commonSize <= 0)
      return false;

    int srcOffset = commonOfs - srcOfs;
    int dstOffset = commonOfs - dstOfs;
    memcpy((char *)dst + dstOffset, (char *)src + srcOffset, commonSize);
    return true;
  };

  alignas(16) unsigned char buf[16];
  unsigned *w = reinterpret_cast<unsigned *>(buf);
  if (move_part(buf, 0, sizeof(buf), data, data_ofs, data_sz))
  {
    w[0] ^= zstd_xor_p[0];
    w[1] ^= zstd_xor_p[1];
    w[2] ^= zstd_xor_p[2];
    w[3] ^= zstd_xor_p[3];
    move_part(data, data_ofs, data_sz, buf, 0, sizeof(buf));
  }

  if (data_total_sz >= 32)
  {
    int bufOfs = sizeof(unsigned) * ((data_total_sz / 4) - 4);
    if (move_part(buf, bufOfs, sizeof(buf), data, data_ofs, data_sz))
    {
      w[0] ^= zstd_xor_p[3];
      w[1] ^= zstd_xor_p[2];
      w[2] ^= zstd_xor_p[1];
      w[3] ^= zstd_xor_p[0];
      move_part(data, data_ofs, data_sz, buf, bufOfs, sizeof(buf));
    }
  }
}

#undef DECL_ZSTD_XOR_KEY
