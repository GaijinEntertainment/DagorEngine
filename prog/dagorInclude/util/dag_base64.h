//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>
#include <util/dag_stdint.h>

class Base64
{
public:
  Base64() { init(); }
  Base64(const char *s)
  {
    init();
    *this = s;
  }

  ~Base64() { release(); }

  void encode(const uint8_t *from, int size);
  int decodeLength() const;      // minimum size in bytes of destination buffer for decoding
  int decode(uint8_t *to) const; // does not append a \0 - needs a decodeLength() bytes buffer

  // void encode( const String& src );
  void decode(String &dest) const;

  void operator=(const char *s);

  inline const char *c_str() const { return (const char *)data; }

private:
  uint8_t *data;
  int len;
  int alloced;

  inline void init()
  {
    len = 0;
    alloced = 0;
    data = NULL;
  }

  inline void release()
  {
    if (data)
      delete[] data;
    init();
  }

  inline void ensureAlloced(int size)
  {
    if (size > alloced)
      release();
    data = new uint8_t[size];
    alloced = size;
  }
};

//! convert binary data to string in Base-64 format
template <typename S>
static inline const char *data_to_str_b64(S &dest, const void *src_data, size_t src_data_len)
{
  Base64 b64;
  b64.encode((uint8_t *)src_data, (int)src_data_len);
  dest = b64.c_str();
  return dest.str();
}
static inline const char *data_to_str_b64_buf(char *dest, size_t dest_len, const void *src_data, size_t src_data_len)
{
  Base64 b64;
  b64.encode((uint8_t *)src_data, (int)src_data_len);
  size_t len = strlen(b64.c_str()) + 1;
  if (len > dest_len)
    return NULL;
  memcpy(dest, b64.c_str(), len);
  return dest;
}

//! convert string in Base-64 format to binary data
template <typename T>
static inline const void *str_b64_to_data(T &dest, const char *s)
{
  Base64 dec(s);
  size_t len = dec.decodeLength();
  if (len % elem_size(dest))
    return NULL;
  dest.resize(len / elem_size(dest));
  mem_set_0(dest);
  dec.decode((uint8_t *)dest.data());
  return dest.data();
}
static inline const void *str_b64_to_data_buf(void *dest_buf, size_t dest_buf_len, const char *s, size_t *out_len = NULL)
{
  Base64 dec(s);
  size_t l = dec.decodeLength();
  if (out_len)
    *out_len = l;
  if (l > dest_buf_len)
    return NULL;
  memset(dest_buf, 0, l);
  dec.decode((uint8_t *)dest_buf);
  return dest_buf;
}
