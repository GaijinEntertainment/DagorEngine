//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_bindump_ext.h>
#include <ioSys/dag_chainedMemIo.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <libTools/util/targetClass.h>
#include <util/dag_simpleString.h>

namespace mkbindump
{
/// Simple reference to data, holds 'start'=index/offset and 'count' of items
struct Ref
{
  int start, count;

  int end() const { return start + count - 1; }

  void zero()
  {
    start = 0;
    count = 0;
  }
  void set(int s, int c)
  {
    start = s;
    count = c;
  }
};

/// Fast namemap builder with crossmap (old nameId -> ordinal nameId)
///   Ordinal nameId scheme means that index of string in sorted table of strings is nameId
///   Thus, one can store only table of strings to get nameId->name resolution (fast lookup)
///   and name->nameId resolution (fast binsearch)
class GatherNameMap : public FastNameMap
{
public:
  GatherNameMap() = default;

  /// builds crossmap to convert old nameId -> ordinal nameId
  void prepareXmap()
  {
    clear_and_resize(xmap, nameCount());
    mem_set_ff(xmap);

    gather_ids_in_lexical_order(sNid, static_cast<const FastNameMap &>(*this));
    for (int i = 0; i < sNid.size(); i++)
      xmap[sNid[i]] = i;
  }

  /// return name by index in lexical order
  const char *getOrdinalName(unsigned ord_idx) const { return ord_idx < sNid.size() ? getName(sNid[ord_idx]) : nullptr; }

  /// name IDs sorted in lexical order; valid after call to prepareXmap()
  SmallTab<int, TmpmemAlloc> sNid;
  /// crossmap to convert old nameId -> ordinal nameId; valid after call to prepareXmap()
  SmallTab<int, TmpmemAlloc> xmap;
};


/// Read-only Shared storage capable of reusing existing data
/// References to storage data can point to the same data or even partiall overlap
template <class T>
struct SharedStorage
{
  SharedStorage() : data(tmpmem), pos(tmpmem), base(0) {}

  /// Searches through storage to find the same data as pointed by ('val','count')
  ///   * 'opt_thresh' defines maximum data count for which full search will be conducted;
  ///     index when 'count' is greater than 'opt_thresh', storage will search only from origins
  ///     previously added data
  ///   * 'div' can be used to obtain aligment greater that 1 unit
  /// Returns true if data found and fills 'dest_start' with start index in storage data
  /// Return false if data not found, adds new data to storage and fills 'dest_start' with start
  bool getRef(int &dest_start, const T *val, int count, int opt_thres = 4, int div = 1)
  {
    if (count < opt_thres)
    {
      for (int i = 0; i <= (int)data.size() - count; i += div)
        if (memcmp(val, &data[i], sizeof(T) * count) == 0) //-V1014
        {
          dest_start = i;
          return true;
        }
    }
    else
    {
      for (int i = 0; i < pos.size(); i++)
        if (pos[i] + count <= data.size() && memcmp(val, &data[pos[i]], sizeof(T) * count) == 0) //-V1014
        {
          dest_start = pos[i];
          return true;
        }
    }
    if (div > 1 && (data.size() % div) != 0)
    {
      int add = div - (data.size() % div);
      int idx = append_items(data, add);
      mem_set_0(make_span(data).subspan(idx));
    }

    for (int l = (div > 1 ? ((count - 1) / div) * div : count - 1), i = data.size() - l; l > 0; i += div, l -= div)
      if (i >= 0 && mem_eq(make_span(data).subspan(i, l), val))
      {
        dest_start = i;
        append_items(data, count - l, val + l);
        return pos.size() && dest_start <= pos.back();
      }

    dest_start = append_items(data, count, val);
    return false;
  }
  bool getRef(int64_t &dest_start, const T *val, int count, int opt_thres = 4, int div = 1)
  {
    int dest32;
    bool ret = getRef(dest32, val, count, opt_thres, div);
    dest_start = dest32;
    return ret;
  }

  /// The same as previous version of getRef(), but works with Ref destination, filling
  /// both 'dest.start' and 'dest.count' fields of 'dest'
  /// This method also records origins of added data (to perform more effective searches)
  bool getRef(Ref &dest, const T *val, int count, int opt_thres = 4, int div = 1)
  {
    dest.count = count;
    if (!count)
    {
      dest.start = 0;
      return true;
    }

    if (getRef(dest.start, val, count, opt_thres, div))
      return true;
    pos.push_back(dest.start);
    return false;
  }

  bool getRef(bindump::Span<T> &dest, const T *val, int count, int opt_thres = 4, int div = 1)
  {
    dest.setCount(count);
    if (!count)
    {
      dest = nullptr;
      return true;
    }

    int start = 0;
    bool ret = getRef(start, val, count, opt_thres, div);
    dest = vecHolder.getElementAddress(start);
    if (ret)
      return true;
    pos.push_back(start);
    return false;
  }

  /// returns offset in dump for given index in storage (using current base)
  int indexToOffset(int idx) const { return base + elem_size(data) * idx; }

  /// converts Ref.start meaning index in storage to offset in dump (using current base)
  void refIndexToOffset(Ref &ref) const
  {
    if (ref.count > 0)
      ref.start = indexToOffset(ref.start);
    else
      ref.zero();
  }

  /// released all used memory and reinit storage
  void clear()
  {
    clear_and_shrink(data);
    clear_and_shrink(pos);
    base = 0;
  }

  Tab<T> data;
  Tab<int> pos;
  /// user accessible base offset of storage in dump
  int base;

  bindump::VecHolder<T> &getVecHolder()
  {
    vecHolder = make_span(data);
    return vecHolder;
  }

private:
  bindump::VecHolder<T> vecHolder;
};

/// convert integer32 from Little endian to Big endian
inline uint32_t le2be32(uint32_t v) { return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24); }
inline uint16_t le2be16(uint16_t v) { return (v >> 8) | ((v & 0xFF) << 8); }
inline void le2be32_s(void *dest, const void *src, size_t dw_num)
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
inline void le2be16_s(void *dest, const void *src, size_t w_num)
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

// helpers to perform conditional conversion (to be used instead of 'read_be ? le2be32(val) : val' pattern)
inline uint32_t le2be32_cond(uint32_t v, bool be) { return be ? le2be32(v) : v; }
inline uint16_t le2be16_cond(uint16_t v, bool be) { return be ? le2be16(v) : v; }
inline void le2be32_s_cond(void *dest, const void *src, size_t dw_num, bool be)
{
  if (be)
    le2be32_s(dest, src, dw_num);
}
inline void le2be16_s_cond(void *dest, const void *src, size_t w_num, bool be)
{
  if (be)
    le2be16_s(dest, src, w_num);
}

/// Special IGenSave-clone interface to provide LE/BE writing
/// when BINDUMP_TARGET_BE is defined (0 or 1), compile-time constant target is used
/// when BINDUMP_TARGET_BE is not defined, target is specified in ctor
class BinDumpSaveCB
{
public:
  enum
  {
    PTR_SZ = 8,
    TAB_SZ = 16
  };

#ifdef BINDUMP_TARGET_BE
  static const int WRITE_BE = BINDUMP_TARGET_BE;

  BinDumpSaveCB(int max_sz, unsigned target) : cwr(max_sz < (128 << 10) ? max_sz : (128 << 10)), blocks(midmem), origin(midmem)
  {
    targetCode = target;
    memset(zero, 0, sizeof(zero));
    curOrigin = 0;
    cwr.setMcdMinMax(128 << 10, 8 << 20);
  }
#else
  bool WRITE_BE;

  BinDumpSaveCB(int max_sz, unsigned target, bool big_endian) :
    cwr(max_sz < (128 << 10) ? max_sz : (128 << 10)), blocks(midmem), origin(midmem), WRITE_BE(big_endian)
  {
    targetCode = target;
    memset(zero, 0, sizeof(zero));
    memset(cvtBuf, 0, sizeof(cvtBuf));
    curOrigin = 0;
    cwr.setMcdMinMax(128 << 10, 8 << 20);
  }
#endif

  //! returns target code assigned in ctor
  unsigned getTarget() const { return targetCode; }

  //! assigns profile string
  void setProfile(const char *str) { profileName = str; }
  //! returns profile string assigned with setProfile()
  const char *getProfile() const { return profileName.empty() ? NULL : profileName.str(); }

  //! fully resets writer
  void reset(int new_size)
  {
    clear_and_shrink(blocks);
    clear_and_shrink(origin);

    if (new_size > (128 << 10))
      new_size = (128 << 10);
    else if (new_size < (4 << 10))
      new_size = (4 << 10);
    cwr.deleteMem();
    cwr.setMem(MemoryChainedData::create(new_size, NULL), true);
    cwr.setMcdMinMax(128 << 10, 8 << 20);
  }

  // base writer interface
  int tell() { return cwr.tell() - curOrigin; }
  void seekto(int ofs) { return cwr.seekto(ofs + curOrigin); }
  void seekToEnd() { return cwr.seektoend(0); }

  int getBlockLevel() { return blocks.size(); }
  void beginBlock()
  {
    cwr.writeInt(0);
    blocks.push_back(tell());
  }
  void beginTaggedBlock(int fourcc)
  {
    beginBlock();
    writeFourCC(fourcc);
  }
  int endBlock(unsigned block_flags = 0)
  {
    G_ASSERTF(block_flags <= 0x3, "block_flags=%08X", block_flags); // 2 bits max
    if (blocks.size() <= 0)
      DAGOR_THROW(IGenSave::SaveException("block not begun", cwr.tell()));

    int ofs = blocks.back();
    int o = tell();
    seekto(ofs - sizeof(int));
    G_ASSERTF(o >= ofs && !((o - ofs) & 0xC0000000), "o=%08X ofs=%08X o-ofs=%08X", o, ofs, o - ofs);
    writeInt32e(unsigned(o - ofs) | (block_flags << 30));

    seekto(o);
    blocks.pop_back();
    return o - ofs;
  }

  // origin management
  // dangerous - changes tell() and seekto() reference point
  void setOrigin()
  {
    origin.push_back(curOrigin);
    curOrigin = cwr.tell();
  }
  void popOrigin()
  {
    if (origin.size() <= 0)
      DAGOR_THROW(IGenSave::SaveException("origin not begun", cwr.tell()));
    curOrigin = origin.back();
    origin.pop_back();
  }

  // additional helper routines
  void writeRaw(const void *p, int sz) { cwr.write(p, sz); }
  void write16ex(const void *p, int sz)
  {
    G_ASSERT((sz & 1) == 0);
    if (WRITE_BE)
    {
      if (sz < sizeof(cvtBuf))
      {
        le2be16_s(cvtBuf, p, sz / 2);
        cwr.write(cvtBuf, sz);
      }
      else
      {
        uint16_t *data = (uint16_t *)memalloc(sz, tmpmem);
        le2be16_s(data, p, sz / 2);
        cwr.write(data, sz);
        memfree(data, tmpmem);
      }
    }
    else
      cwr.write(p, sz);
  }
  void write32ex(const void *p, int sz)
  {
    G_ASSERT((sz & 3) == 0);
    if (WRITE_BE)
    {
      if (sz < sizeof(cvtBuf))
      {
        le2be32_s(cvtBuf, p, sz / 4);
        cwr.write(cvtBuf, sz);
      }
      else
      {
        uint32_t *data = (uint32_t *)memalloc(sz, tmpmem);
        le2be32_s(data, p, sz / 4);
        cwr.write(data, sz);
        memfree(data, tmpmem);
      }
    }
    else
      cwr.write(p, sz);
  }
  void writeZeroes(size_t cnt)
  {
    while (cnt >= sizeof(zero))
    {
      cwr.write(zero, sizeof(zero));
      cnt -= sizeof(zero);
    }
    if (cnt > 0)
      cwr.write(zero, int(cnt));
  }
  void align4()
  {
    int rest = (tell() & 3);
    if (rest)
      cwr.write(zero, 4 - rest);
  }
  void align8()
  {
    int rest = (tell() & 7);
    if (rest)
      cwr.write(zero, 8 - rest);
  }
  void align16()
  {
    int rest = (tell() & 15);
    if (rest)
      cwr.write(zero, 16 - rest);
  }
  void align32()
  {
    int rest = (tell() & 31);
    if (rest)
      cwr.write(zero, 32 - rest);
  }

  // routines for dump writing
  void writeFourCC(int cc) { cwr.writeInt(cc); }
  void writeFloat32e(float v)
  {
    if (WRITE_BE)
      cwr.writeInt(le2be32(*(uint32_t *)&v));
    else
      cwr.write(&v, 4);
  }
  void writeFloat64e(double v)
  {
    if (WRITE_BE)
    {
      uint32_t *i32 = (uint32_t *)&v;
      cwr.writeInt(le2be32(i32[1]));
      cwr.writeInt(le2be32(i32[0]));
    }
    else
      cwr.write(&v, 8);
  }
  void writeReal(float v) { return writeFloat32e(v); }
  void writeInt32e(unsigned v) { cwr.writeInt(WRITE_BE ? le2be32(v) : v); }
  void writeE3dColorE(E3DCOLOR c) { cwr.writeInt(WRITE_BE ? le2be32(c) : (uint32_t)c.u); }
  void writePtr64e(unsigned ptr_ofs)
  {
    writeInt32e(ptr_ofs);
    writeInt32e(0);
  }
  void writePtr64eAt(unsigned ptr_ofs, int ofs)
  {
    int pos = tell();
    seekto(ofs);
    writePtr64e(ptr_ofs);
    seekto(pos);
  }
  void writePtr32eAt(unsigned ptr_ofs, int ofs)
  {
    int pos = tell();
    seekto(ofs);
    writeInt32e(ptr_ofs);
    seekto(pos);
  }
  void writeInt64e(uint64_t v)
  {
    if (WRITE_BE)
    {
      uint32_t *i32 = (uint32_t *)&v;
      cwr.writeInt(le2be32(i32[1]));
      cwr.writeInt(le2be32(i32[0]));
    }
    else
      cwr.write(&v, 8);
  }
  void writeInt16e(unsigned v) { cwr.writeIntP<2>(WRITE_BE ? le2be16(v) : v); }
  void writeInt8e(unsigned v) { cwr.writeIntP<1>(v); }
  void writeInt16eAt(unsigned v, int ofs)
  {
    int pos = tell();
    seekto(ofs);
    cwr.writeIntP<2>(WRITE_BE ? le2be16(v) : v);
    seekto(pos);
  }
  void writeInt32eAt(unsigned v, int ofs)
  {
    int pos = tell();
    seekto(ofs);
    cwr.writeInt(WRITE_BE ? le2be32(v) : v);
    seekto(pos);
  }
  void writeDwString(const char *s)
  {
    int len = s ? i_strlen(s) : 0;
    writeInt32e(len);
    writeRaw(s, len);
    if (len & 3)
      cwr.write(zero, 4 - (len & 3));
  }

  template <class V, typename T = typename V::value_type>
  void writeTabDataRaw(const V &tab)
  {
    cwr.writeTabData(tab);
  }
  template <class V, typename T = typename V::value_type>
  void writeTabDataE(const V &tab, void (*cvt_le2be)(T *data, int cnt))
  {
    if (WRITE_BE)
    {
      T *data = (T *)memalloc(data_size(tab), tmpmem);
      mem_copy_to(tab, data);
      cvt_le2be(data, tab.size());
      cwr.write(data, data_size(tab));
      memfree(data, tmpmem);
    }
    else
      cwr.writeTabData(tab);
  }

  void writeTabData16e(dag::ConstSpan<uint16_t> tab) { writeTabDataE(tab, cvt_uint16_le2be); }
  void writeTabData32e(dag::ConstSpan<uint32_t> tab) { writeTabDataE(tab, cvt_uint32_le2be); }
  void writeTabData16e(dag::ConstSpan<int16_t> tab) { writeTabDataE((dag::ConstSpan<uint16_t> &)tab, cvt_uint16_le2be); }
  void writeTabData32e(dag::ConstSpan<int32_t> tab) { writeTabDataE((dag::ConstSpan<uint32_t> &)tab, cvt_uint32_le2be); }

  template <class V, typename T = typename V::value_type>
  void writeTabData16ex(const V &tab)
  {
    G_ASSERT((sizeof(T) & 1) == 0);
    if (WRITE_BE)
    {
      uint16_t *data = (uint16_t *)memalloc(data_size(tab), tmpmem);
      mem_copy_to(tab, data);
      cvt_uint16_le2be(data, int(data_size(tab) / sizeof(*data)));
      cwr.write(data, data_size(tab));
      memfree(data, tmpmem);
    }
    else
      cwr.writeTabData(tab);
  }
  template <class V, typename T = typename V::value_type>
  void writeTabData32ex(const V &tab)
  {
    G_ASSERT((sizeof(T) & 3) == 0);
    if (WRITE_BE)
    {
      uint32_t *data = (uint32_t *)memalloc(data_size(tab), tmpmem);
      mem_copy_to(tab, data);
      cvt_uint32_le2be(data, int(data_size(tab) / sizeof(*data)));
      cwr.write(data, data_size(tab));
      memfree(data, tmpmem);
    }
    else
      cwr.writeTabData(tab);
  }

  template <class T>
  void writeStorage(SharedStorage<T> &ss)
  {
    ss.base = tell();
    cwr.writeTabData(ss.data);
  }
  template <class T>
  void writeStorageE(SharedStorage<T> &ss, void (*cvt_le2be)(T *data, int cnt))
  {
    ss.base = tell();
    writeTabDataE(make_span_const(ss.data), cvt_le2be);
  }
  void writeStorage16e(SharedStorage<uint16_t> &ss) { writeStorageE(ss, cvt_uint16_le2be); }
  void writeStorage32e(SharedStorage<uint32_t> &ss) { writeStorageE(ss, cvt_uint32_le2be); }

  template <class T>
  void writeStorage16ex(SharedStorage<T> &ss)
  {
    ss.base = tell();
    writeTabData16ex(ss.data);
  }
  template <class T>
  void writeStorage32ex(SharedStorage<T> &ss)
  {
    ss.base = tell();
    writeTabData32ex(make_span_const(ss.data));
  }

  void writeRef(const Ref &ref)
  {
    writeInt32e(ref.start);
    writeInt32e(ref.count);
    writeInt32e(0);
    writeInt32e(0);
  }
  void writeRef(int ofs, int count)
  {
    writeInt32e(count ? ofs : 0);
    writeInt32e(count);
    writeInt32e(0);
    writeInt32e(0);
  }

  void copyRaw(IGenLoad &crd, int sz)
  {
    char buf[16384];
    while (sz > 0)
    {
      int bsz = sz < sizeof(buf) ? sz : (int)sizeof(buf);
      crd.read(buf, bsz);
      cwr.write(buf, bsz);
      sz -= bsz;
    }
  }

  //! copies raw data from another BinDumpSaveCB (used as IGenLoad)
  void copyRawFrom(BinDumpSaveCB &crd) { crd.copyDataTo(cwr); }

  //! returns raw underlying writer (useful for direct writing with zlib io, etc.)
  MemorySaveCB &getRawWriter() { return cwr; }

  // get final data/size
  MemoryChainedData *getMem() { return cwr.getMem(); }
  int getSize() { return MemoryChainedData::calcTotalUsedSize(cwr.getMem()); }

  //! copies contents of this writer as raw data to other stream
  void copyDataTo(IGenSave &dest_cwr) { cwr.copyDataTo(dest_cwr); }

  //! allocates and fill single chunk of memory with this writer's data
  void *makeDataCopy(IMemAlloc *mem = NULL)
  {
    MemoryLoadCB crd(cwr.getMem(), false);
    int sz = getSize();
    char *data = (char *)memalloc(sz, mem ? mem : tmpmem);

    crd.read(data, sz);
    return data;
  }

protected:
  MemorySaveCB cwr;
  Tab<int> blocks;
  Tab<int> origin;
  uint32_t zero[32];
  uint32_t cvtBuf[32];
  unsigned targetCode;
  SimpleString profileName;
  int curOrigin;

  static void cvt_uint16_le2be(uint16_t *data, int cnt)
  {
    while (cnt > 0)
    {
      *data = le2be16(*data);
      data++;
      cnt--;
    }
  }
  static void cvt_uint32_le2be(uint32_t *data, int cnt)
  {
    while (cnt > 0)
    {
      *data = le2be32(*data);
      data++;
      cnt--;
    }
  }
};

/// Helper object for writing PatchableTab data (offset,count) at any place of dump
/// Usage is divided into 3 steps:
///   1: reserve space for PatchableTab in dump and remember this offset
///   2: actually write tab data to dump and remeber offset of data
///   3: get back and fill reserved space in dump with actual PatchableTab
struct PatchTabRef
{
public:
  /// resets patchable tab reference
  void reset()
  {
    base = 0;
    count = 0;
    pos = -1;
    resvDataPos = -1;
  }

  /// Step 1. Insert tab reference to stream and remember this pos
  void reserveTab(BinDumpSaveCB &cwr)
  {
    pos = cwr.tell();
    cwr.writeInt32e(0); // offset
    cwr.writeInt32e(0); // count
    cwr.writeInt32e(0);
    cwr.writeInt32e(0); // reserve for 64-bit PatchableTab
    resvDataPos = -1;
  }

  /// Step 2. Start writing tab data (setup count and base offset)
  void startData(BinDumpSaveCB &cwr, int cnt)
  {
    base = cwr.tell();
    count = cnt;
  }
  /// Step 2. Write tab data (setup count and base offset) using data/cnt pair
  template <class T>
  void writeData(BinDumpSaveCB &cwr, const T *data, int cnt)
  {
    startData(cwr, cnt);
    cwr.writeRaw(data, int(count * sizeof(T)));
  }
  /// Step 2. Write tab data (setup count and base offset) using slice
  template <class V, typename T = typename V::value_type>
  void writeData(BinDumpSaveCB &cwr, const V &data)
  {
    startData(cwr, data.size());
    cwr.writeTabDataRaw(data);
  }
  /// Step 2. Setup count and base offset at relative position (useful for reference tabs)
  void startDataAt(BinDumpSaveCB &cwr, int cnt, int ofs)
  {
    base = cwr.tell() + ofs;
    count = cnt;
  }
  /// Step 2. Reserve space for tab data (setup count and base offset) using cnt/elemsize
  void reserveData(BinDumpSaveCB &cwr, int cnt, int elemsize)
  {
    G_ASSERT(resvDataPos == -1);
    startData(cwr, cnt);
    resvDataPos = base;
    cwr.writeZeroes(elemsize * count);
  }

  /// Step 3. Fix reference written in step 1
  void finishTab(BinDumpSaveCB &cwr, bool preserve_pos = true)
  {
    int wrpos = preserve_pos ? cwr.tell() : -1;
    G_ASSERT(pos != -1);
    cwr.seekto(pos);
    cwr.writeInt32e(count ? base : 0); // offset
    cwr.writeInt32e(count);            // count
    cwr.writeInt32e(0);
    cwr.writeInt32e(0); // reserve for 64-bit PatchableTab

    if (preserve_pos)
      cwr.seekto(wrpos);
  }

public:
  int base, count, pos, resvDataPos;
};

static inline const char *get_target_str(unsigned targetCode)
{
  static unsigned str[2] = {'xxxx', 0};
  if (!targetCode)
    return "????";

  str[0] = targetCode;
  char *p = (char *)&str[0];
  while (!*p)
    p++;
  return p;
}
} // namespace mkbindump
