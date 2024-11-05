// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>


static inline uint32_t le2be32(uint32_t v) { return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24); }

inline uint16_t le2be16(uint16_t v) { return (v >> 8) | ((v & 0xFF) << 8); }

static inline uint64_t le2be64(uint64_t v)
{
  return (((uint64_t)le2be32(uint32_t(v & 0xFFFFFFFF))) << 32) | (uint64_t)le2be32(uint32_t(v >> 32));
}

#if _TARGET_CPU_BE
// big endian host

static inline uint64_t readInt64(IGenLoad &crd, bool le)
{
  uint64_t v = crd.readInt64();
  return le ? le2be64(v) : v;
}

static inline void writeInt64(IGenSave &cwr, uint64_t v, bool le) { cwr.writeInt64(le ? le2be64(v) : v); }

static inline uint32_t readInt32(IGenLoad &crd, bool le)
{
  uint32_t v = crd.readInt();
  return le ? le2be32(v) : v;
}

static inline void writeInt32(IGenSave &cwr, uint32_t v, bool le) { cwr.writeInt(le ? le2be32(v) : v); }

static inline uint16_t readInt16(IGenLoad &crd, bool le)
{
  uint32_t v = crd.readIntP<2>();
  return le ? le2be16(v) : v;
}

static inline void writeInt16(IGenSave &cwr, uint16_t v, bool le) { cwr.writeIntP<2>(le ? le2be16(v) : v); }

static inline float readReal32(IGenLoad &crd, bool le)
{
  if (!le)
    return crd.readReal();
  else
  {
    union
    {
      int i;
      float f;
    } u;
    u.i = le2be32(crd.readInt());
    return u.f;
  }
}

static inline void writeReal32(IGenSave &cwr, float v, bool le)
{
  if (!le)
    cwr.writeReal(v);
  else
  {
    union
    {
      int i;
      float f;
    } u;
    u.f = v;
    cwr.writeInt(le2be32(u.i));
  }
}

#else
// little endian host

static inline uint64_t readInt64(IGenLoad &crd, bool le)
{
  uint64_t v = crd.readInt64();
  return le ? v : le2be64(v);
}

static inline void writeInt64(IGenSave &cwr, uint64_t v, bool le) { cwr.writeInt64(le ? v : le2be64(v)); }

static inline uint32_t readInt32(IGenLoad &crd, bool le)
{
  uint32_t v = crd.readInt();
  return le ? v : le2be32(v);
}

static inline void writeInt32(IGenSave &cwr, uint32_t v, bool le) { cwr.writeInt(le ? v : le2be32(v)); }

static inline uint16_t readInt16(IGenLoad &crd, bool le)
{
  uint32_t v = crd.readIntP<2>();
  return le ? v : le2be16(v);
}

static inline void writeInt16(IGenSave &cwr, uint16_t v, bool le) { cwr.writeIntP<2>(le ? v : le2be16(v)); }

static inline float readReal32(IGenLoad &crd, bool le)
{
  if (le)
    return crd.readReal();
  else
  {
    union
    {
      int i;
      float f;
    } u;
    u.i = le2be32(crd.readInt());
    return u.f;
  }
}

static inline void writeReal32(IGenSave &cwr, float v, bool le)
{
  if (le)
    cwr.writeReal(v);
  else
  {
    union
    {
      int i;
      float f;
    } u;
    u.f = v;
    cwr.writeInt(le2be32(u.i));
  }
}

#endif

static inline int encode_len_sz(unsigned v)
{
  if (v < 0x80)
    return 1;
  if (v < 0x4000)
    return 2;
  if (v < 0x300000)
    return 3;
  return 1;
}
static inline int encode_len(unsigned v, char dest[4])
{
  if (v < 0x80)
  {
    dest[0] = v;
    return 1;
  }
  if (v < 0x4000)
  {
    dest[0] = 0x80 | (v >> 8);
    dest[1] = 0xFF & v;
    return 2;
  }
  if (v < 0x300000)
  {
    dest[0] = 0xC0 | (v >> 16);
    dest[1] = 0xFF & (v >> 8);
    dest[2] = 0xFF & v;
    return 3;
  }
  G_ASSERT(v < 0x300000);
  dest[0] = 0;
  return 1;
}
static inline bool decode_len(const char *buf, int &out_inc, int &out_len, int sz_avail)
{
  if (sz_avail < 1)
    return false;
  const unsigned char *b = (const unsigned char *)buf;
  if (unsigned(b[0]) < 0x80)
  {
    out_inc = 1;
    out_len = b[0];
    return true;
  }

  if (sz_avail < 2)
    return false;
  if ((b[0] & 0xC0) == 0x80)
  {
    out_inc = 2;
    out_len = ((b[0] & 0x3F) << 8) | b[1];
    return true;
  }

  if (sz_avail < 3)
    return false;
  out_inc = 3;
  out_len = ((b[0] & 0x3F) << 16) | (b[1] << 8) | b[2];
  return true;
}
static inline int decode_len(IGenLoad &crd, unsigned &inout_total_sz)
{
  unsigned char c[4];
  crd.read(c, 1);
  if (c[0] < 0x80)
  {
    inout_total_sz++;
    return c[0];
  }

  if ((c[0] & 0xC0) == 0x80)
  {
    crd.read(c + 1, 1);
    inout_total_sz += 2;
    return ((c[0] & 0x3F) << 8) | c[1];
  }

  crd.read(c + 1, 2);
  inout_total_sz += 3;
  return ((c[0] & 0x3F) << 16) | (c[1] << 8) | c[2];
}

static inline int get_count_storage_type(int cnt)
{
  if (cnt >= 0x10000)
    return 3;
  if (cnt >= 0x100)
    return 2;
  return cnt > 0 ? 1 : 0;
}
static inline int get_storage_type_sz(int type) { return type ? 1 << (type - 1) : 0; }
static inline void write_count_compact(IGenSave &cwr, bool le, int type, int cnt)
{
  if (type == 3)
    writeInt32(cwr, cnt, le);
  else if (type == 2)
    writeInt16(cwr, cnt, le);
  else if (type == 1)
    cwr.writeIntP<1>(cnt);
}
static inline int read_count_compact(IGenLoad &crd, bool le, int type)
{
  if (type == 3)
    return readInt32(crd, le);
  else if (type == 2)
    return readInt16(crd, le);
  else if (type == 1)
    return crd.readIntP<1>();
  return 0;
}
