//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <string.h>


//! convert binary data to string in base-32 (a..z 2..7) format
#define ENCODE_B32(C) ((C) < 26 ? 'a' + (C) : '2' + (C) - 26)
static inline const char *data_to_str_b32_buf(char *dest, size_t dest_len, const void *src_data, size_t src_data_len)
{
  G_ASSERTF_RETURN(src_data_len % 5 == 0, NULL, "src_data_len=%d", src_data_len);
  char *s = dest;
  if (dest_len < (src_data_len / 5) * 8 + 1)
    src_data_len = (dest_len - 1) / 8 * 5;

  for (uint8_t *b8 = (uint8_t *)src_data, b5; src_data_len > 0; b8 += 5, s += 8, src_data_len -= 5)
  {
    b5 = 0x1F & (b8[0] >> 3);
    s[0] = ENCODE_B32(b5);
    b5 = 0x1F & ((b8[0] << 2) | (b8[1] >> 6));
    s[1] = ENCODE_B32(b5);
    b5 = 0x1F & (b8[1] >> 1);
    s[2] = ENCODE_B32(b5);
    b5 = 0x1F & ((b8[1] << 4) | (b8[2] >> 4));
    s[3] = ENCODE_B32(b5);
    b5 = 0x1F & ((b8[2] << 1) | (b8[3] >> 7));
    s[4] = ENCODE_B32(b5);
    b5 = 0x1F & (b8[3] >> 2);
    s[5] = ENCODE_B32(b5);
    b5 = 0x1F & ((b8[3] << 3) | (b8[4] >> 5));
    s[6] = ENCODE_B32(b5);
    b5 = 0x1F & b8[4];
    s[7] = ENCODE_B32(b5);
  }
  *s = '\0';
  return dest;
}
template <typename S>
static inline const char *data_to_str_b32(S &dest, const void *src_data, size_t src_data_len)
{
  G_ASSERTF_RETURN(src_data_len % 5 == 0, NULL, "src_data_len=%d", src_data_len);
  size_t dest_len = src_data_len * 8 / 5 + 1;
  dest.allocBuffer(int(dest_len));
  data_to_str_b32_buf(dest.str(), dest_len, src_data, src_data_len);
  return dest.str();
}
static inline char *uint64_to_str_b32(uint64_t i, char *dest, size_t dest_len)
{
  G_ASSERTF_RETURN(dest_len * 5 - 5 >= 64 || (i >> (dest_len * 5 - 5)) == 0, NULL, "i=%d dest_len=%d (i >> %d)=%d", i, dest_len,
    dest_len * 5 - 5, (i >> (dest_len * 5 - 5)));
  char *p = dest;
  int shift = 60;
  while (shift > 5 && !((i >> shift) & 0x1F))
    shift -= 5;
  for (; shift >= 0; p++, shift -= 5)
  {
    int v = (i >> shift) & 0x1F;
    *p = ENCODE_B32(v);
  }
  G_ASSERTF_RETURN(p - dest < dest_len, NULL, "dest_len=%d used=%d i=%d", dest_len, p - dest, i);
  *p = '\0';
  return dest;
}
#undef ENCODE_B32

//! convert string in base-32 format to binary data
#define DECODE_B32(C) (((C) >= 'a' && (C) < 'a' + 26) ? (C) - 'a' : (((C) >= '2' && (C) < '2' + 6) ? (C) - '2' + 26 : 0))
static inline const void *str_b32_to_data_buf(void *dest_buf, size_t dest_buf_len, const char *s, size_t *out_len = NULL,
  int s_len = -1)
{
  size_t len = s_len >= 0 ? s_len : strlen(s);
  if (len % 8) // we require 8 chars for each of 5 bytes of encoded data
  {
    if (out_len)
      *out_len = 0;
    return NULL;
  }
  len = (len / 8) * 5;
  if (out_len)
    *out_len = len;
  if (len > dest_buf_len)
    len = (dest_buf_len / 5) * 5;

  for (unsigned char *p = (unsigned char *)dest_buf, *pe = p + len; p < pe; p += 5)
  {
    uint8_t b5[8];
    for (int i = 0; i < 8; i++, s++)
      b5[i] = DECODE_B32(*s);
    p[0] = (b5[0] << 3) | (b5[1] >> 2);
    p[1] = (b5[1] << 6) | (b5[2] << 1) | (b5[3] >> 4);
    p[2] = (b5[3] << 4) | (b5[4] >> 1);
    p[3] = (b5[4] << 7) | (b5[5] << 2) | (b5[6] >> 3);
    p[4] = (b5[6] << 5) | b5[7];
  }
  return dest_buf;
}
template <typename T>
static inline const void *str_b32_to_data(T &dest, const char *s)
{
  size_t len = strlen(s);
  if (len % 8) // we require 8 chars for each of 5 bytes of encoded data
    return NULL;
  if ((len / 8 * 5) % elem_size(dest))
    return NULL;
  dest.resize(len / 8 * 5 / elem_size(dest));
  return str_b32_to_data_buf(dest.data(), data_size(dest), s, NULL, (int)len);
}
static inline uint64_t str_b32_to_uint64(const char *b32)
{
  uint64_t res = 0;
  for (; *b32; b32++)
    res = res * 32 + DECODE_B32(*b32);
  return res;
}
#undef DECODE_B32
