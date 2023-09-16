//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <libTools/util/targetClass.h>
#include <util/dag_string.h>


class BinDumpReader
{
public:
  BinDumpReader(IGenLoad *_crd, unsigned target, bool big_endian) : crd(_crd), READ_BE(big_endian), blocks(tmpmem), targetCode(target)
  {}

  unsigned getTarget() const { return targetCode; }
  bool readBE() const { return READ_BE; }

  int readFourCC() { return crd->readInt(); }
  int readInt32e()
  {
    unsigned v = crd->readInt();
    return READ_BE ? le2be32(v) : v;
  }

  void read32ex(void *dest, int len)
  {
    G_ASSERT((len & 3) == 0);
    crd->read(dest, len);
    if (READ_BE)
      le2be32_s(dest, dest, len / 4);
  }

  void readDwString(String &s)
  {
    int len = readInt32e();
    s.resize(len + 1);
    crd->read(s.data(), len);
    s[len] = 0;
    if (len & 3)
      crd->seekrel(4 - (len & 3));
  }

  int tell() { return crd->tell(); }
  void seekto(int ofs) { crd->seekto(ofs); }
  void seekrel(int ofs) { crd->seekrel(ofs); }

  int beginBlock(unsigned *out_block_flags = nullptr)
  {
    int l = readInt32e();
    if (out_block_flags)
      *out_block_flags = l >> 30;
    l &= 0x3FFFFFFF;

    blocks.push_back().set(tell(), l);
    return l;
  }

  int beginTaggedBlock(unsigned *out_block_flags = nullptr)
  {
    beginBlock(out_block_flags);
    return readFourCC();
  }

  void endBlock()
  {
    if (blocks.size() <= 0)
      DAGOR_THROW(IGenLoad::LoadException("endBlock without beginBlock", tell()));

    Block &b = blocks.back();
    seekto(b.ofs + b.len);
    blocks.pop_back();
  }

  int getBlockLength() { return blocks.size() ? blocks.back().len : 0; }
  int getBlockRest() { return blocks.size() ? blocks.back().end() - tell() : 0; }
  int getBlockLevel() { return blocks.size(); }

  IGenLoad &getRawReader() { return *crd; }

  void convert32(void *p, int sz)
  {
    G_ASSERT((sz & 3) == 0);
    if (READ_BE)
      le2be32_s(p, p, sz / 4);
  }

  void convert16(void *p, int sz)
  {
    G_ASSERT((sz & 1) == 0);
    if (READ_BE)
      le2be16_s(p, p, sz / 2);
  }

  int convert32(int v) { return READ_BE ? le2be32(v) : v; }
  int convert16(int v) { return READ_BE ? le2be16(v) : v; }

  template <class T>
  void convert32(dag::Span<T> s)
  {
    convert32(s.data(), data_size(s));
  }
  template <class T>
  void convert16(dag::Span<T> s)
  {
    convert16(s.data(), data_size(s));
  }

public:
  /// convert integer32 from Little endian to Big endian and vice versa
  static inline uint32_t le2be32(uint32_t v) { return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24); }
  static inline uint16_t le2be16(uint16_t v) { return (v >> 8) | ((v & 0xFF) << 8); }
  static inline void le2be32_s(void *dest, const void *src, int dw_num)
  {
    uint32_t *d = (uint32_t *)dest;
    const uint32_t *s = (uint32_t *)src;
    while (dw_num > 0)
    {
      *d = le2be32(*s);
      d++;
      s++;
      dw_num--;
    }
  }
  static inline void le2be16_s(void *dest, const void *src, int w_num)
  {
    uint16_t *d = (uint16_t *)dest;
    const uint16_t *s = (uint16_t *)src;
    while (w_num > 0)
    {
      *d = le2be16(*s);
      d++;
      s++;
      w_num--;
    }
  }

protected:
  IGenLoad *crd;
  bool READ_BE;
  unsigned targetCode;

  struct Block
  {
    int ofs, len;

    int end() { return ofs + len; }
    void set(int o, int l)
    {
      ofs = o;
      len = l;
    }
  };
  Tab<Block> blocks;
};
