//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>
#include <stddef.h>
#include <generic/dag_span.h>
#include <EASTL/type_traits.h>
#include <EASTL/utility.h>
#include <debug/dag_assert.h>
#include <util/dag_globDef.h>

EA_DISABLE_VC_WARNING(4146) // unary minus operator applied to unsigned type, result still unsigned

class DataBlock;

namespace danet
{
class BitStream;
namespace internal
{
#define DECL_HAS_MEMBER(name, mbr)                                                 \
  template <class T, typename Enable = void>                                       \
  struct name : public eastl::false_type                                           \
  {};                                                                              \
  template <class T>                                                               \
  struct name<T, typename eastl::enable_if<eastl::is_class<T>::value>::type>       \
  {                                                                                \
  private:                                                                         \
    struct Fallback                                                                \
    {                                                                              \
      int mbr;                                                                     \
    };                                                                             \
    struct Derived : T, Fallback                                                   \
    {};                                                                            \
    template <class U>                                                             \
    static eastl::yes_type test(...);                                              \
    template <class U>                                                             \
    static eastl::no_type test(decltype(U::mbr) *);                                \
                                                                                   \
  public:                                                                          \
    static const bool value = sizeof(test<Derived>(0)) == sizeof(eastl::yes_type); \
  }
DECL_HAS_MEMBER(IsString, c_str);
DECL_HAS_MEMBER(HaveAllocBuffer, allocBuffer);
DECL_HAS_MEMBER(IsContainer, resize);
DECL_HAS_MEMBER(IsOptional, has_value);
#undef DECL_HAS_MEMBER
template <typename T>
inline typename eastl::enable_if<HaveAllocBuffer<T>::value>::type resize_str(T &str, uint32_t sz)
{
  str.allocBuffer(sz + 1);
}
template <typename T>
inline typename eastl::disable_if<HaveAllocBuffer<T>::value>::type resize_str(T &str, uint32_t sz)
{
  str.resize(sz);
}

template <typename T, typename = decltype(write_type(eastl::declval<BitStream &>(), eastl::declval<T>()))>
eastl::true_type supports_write_type_test(const T &);
eastl::false_type supports_write_type_test(...);
template <typename T>
using supports_write_type = decltype(supports_write_type_test(eastl::declval<T>()));

template <typename T, typename = decltype(read_type(eastl::declval<const BitStream &>(), eastl::declval<T &>()))>
eastl::true_type supports_read_type_test(const T &);
eastl::false_type supports_read_type_test(...);
template <typename T>
using supports_read_type = decltype(supports_read_type_test(eastl::declval<T>()));
}; // namespace internal

class BitStream
{
public:
  BitStream(IMemAlloc *a = defaultmem) :
    bitsUsed(0),
    dataOwner(0),
    bitsAllocated(0),
    readOffset(0),
    data(nullptr),
    allocator(a)
  {}
  BitStream(const uint8_t *_data, size_t lenInBytes, bool copy, IMemAlloc *a = defaultmem) :
    bitsUsed((uint32_t)bytes2bits(lenInBytes)), dataOwner((uint32_t)copy ? 1 : 0), readOffset(0), allocator(a)
  {
    if (copy)
    {
      size_t bytesAllocated = 0;
      data = (uint8_t *)allocator->alloc(lenInBytes, &bytesAllocated);
      G_ASSERT(bytesAllocated >= lenInBytes);
      memcpy(data, _data, lenInBytes);
      bitsAllocated = (uint32_t)bytes2bits(bytesAllocated);
    }
    else
    {
      data = (uint8_t *)_data;
      bitsAllocated = (uint32_t)bytes2bits(lenInBytes);
    }
  }
  BitStream(uint32_t reserveBytes, IMemAlloc *a = defaultmem) : bitsUsed(0), dataOwner(1), readOffset(0), allocator(a)
  {
    size_t bytesAllocated = 0;
    data = (uint8_t *)allocator->alloc(reserveBytes, &bytesAllocated);
    G_ASSERT(bytesAllocated >= reserveBytes);
    bitsAllocated = (uint32_t)bytes2bits(bytesAllocated);
    memset(data, 0, bytesAllocated);
  }

  BitStream(const BitStream &bs)
  {
    allocator = defaultmem;
    DuplicateBitStream(bs);
  }

  BitStream(BitStream &&bs)
  {
    memcpy(this, &bs, sizeof(*this));
    memset(&bs, 0, offsetof(BitStream, allocator));
  }

  BitStream &operator=(const BitStream &bs)
  {
    if (dataOwner)
      memfree(GetData(), allocator);
    DuplicateBitStream(bs);
    return *this;
  }

  BitStream &operator=(BitStream &&bs)
  {
    if (&bs != this)
    {
      this->~BitStream();
      memcpy(this, &bs, sizeof(*this));
      memset(&bs, 0, offsetof(BitStream, allocator));
    }
    return *this;
  }

  ~BitStream()
  {
    if (dataOwner)
      memfree(GetData(), allocator);
  }

  void Clear() { *this = BitStream(allocator); }

  //
  // RakNet compatible interface
  //

  void Reset() { bitsUsed = readOffset = 0u; }
  uint32_t GetNumberOfBitsUsed() const { return GetWriteOffset(); }
  uint32_t GetWriteOffset() const { return bitsUsed; }
  uint32_t GetNumberOfBytesUsed(void) const { return bits2bytes(bitsUsed); }
  uint32_t GetReadOffset() const { return readOffset; }
  void SetReadOffset(uint32_t newReadOffset) const { readOffset = newReadOffset; }
  uint32_t GetNumberOfUnreadBits() const { return bitsUsed - readOffset; }
  uint32_t GetNumberOfAllocatedBits() const { return bitsAllocated; }
  uint8_t *GetData() { return data; }
  const uint8_t *GetData() const { return data; }
  dag::ConstSpan<uint8_t> getSlice() const { return dag::ConstSpan<uint8_t>(data, GetNumberOfBytesUsed()); }
  void IgnoreBits(uint32_t bits) const { readOffset += min(bits, bitsUsed - readOffset); }
  void IgnoreBytes(uint32_t bytes) const { IgnoreBits(bytes2bits(bytes)); }
  void SetWriteOffset(uint32_t offs) { bitsUsed = offs; }
  void SetUnalignedWriteOffset(uint32_t offs) { bitsUsed = offs; }
  void AlignWriteToByteBoundary()
  {
    if (bitsUsed)
      bitsUsed += 8 - (((bitsUsed - 1) & 7) + 1);
  }
  void AlignReadToByteBoundary() const
  {
    if (readOffset)
      readOffset += 8 - (((readOffset - 1) & 7) + 1);
  }
  void ResetWritePointer() { bitsUsed = 0; }
  void ResetReadPointer() const { readOffset = 0; }

  // actual read/write implementation
  void WriteBits(const uint8_t *input, uint32_t bits)
  {
    if (!input || !bits)
      return;
    reserveBits(bits);

    uint32_t bitsUsedMod8 = bitsUsed & 7;
    uint32_t bitsMod8 = bits & 7;
    const uint8_t *srcPtr = input;
    uint8_t *destPtr = GetData() + (bitsUsed >> 3);

    if (!bitsUsedMod8 && !bitsMod8)
    {
      memcpy(destPtr, srcPtr, bits >> 3);
      bitsUsed += bits;
      return;
    }

    uint8_t upShift = 8 - bitsUsedMod8; // also how many remaining free bits in byte left
    uint8_t srcByte;
    uint8_t destByte = *destPtr & (0xff << upShift); // clear low bits

    bitsUsed += bits;

    for (; bits >= 8; bits -= 8)
    {
      srcByte = *srcPtr++;
      *destPtr++ = destByte | (srcByte >> bitsUsedMod8);
      destByte = srcByte << upShift;
    }
    if (!bits)
    {
      *destPtr = destByte | (*destPtr & (0xff >> bitsUsedMod8));
      return;
    }
    srcByte = *srcPtr & ((1 << bits) - 1);
    int bitsDiff = (int)bits - (int)upShift;
    if (bitsDiff <= 0) // enough space left in byte to fit remaining bits?
    {
      *destPtr = destByte | (*destPtr & (0xff >> (bits + bitsUsedMod8))) | (srcByte << -bitsDiff);
      return;
    }
    *destPtr++ = destByte | (srcByte >> bitsDiff);
    bits -= upShift;
    *destPtr = (*destPtr & (0xff >> bits)) | ((srcByte & ((1 << bits) - 1)) << (8 - bits));
  }

  bool ReadBits(uint8_t *output, uint32_t bits) const
  {
    if (!bits)
      return true;
    else if ((readOffset + bits) > bitsUsed)
    {
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
      if (output)
        memset(output, 0x7e, bits2bytes(bits));
#endif
      return false;
    }

    const uint8_t *dataPtr = GetData();
    uint32_t readmod8 = readOffset & 7;
    if (readmod8 == 0 && (bits & 7) == 0) // fast path - everything byte aligned
    {
      memcpy(output, dataPtr + bits2bytes(readOffset), bits2bytes(bits));
      readOffset += bits;
      return true;
    }

    memset(output, 0, bits2bytes(bits));

    uint32_t offs = 0;
    while (bits > 0)
    {
      *(output + offs) |= *(dataPtr + (readOffset >> 3)) << readmod8;

      if (readmod8 > 0 && bits > (8 - readmod8))
        *(output + offs) |= *(dataPtr + (readOffset >> 3) + 1) >> (8 - readmod8);

      if (bits >= 8)
      {
        bits -= 8;
        readOffset += 8;
        offs++;
      }
      else
      {
        *(output + offs) >>= 8 - bits;
        readOffset += bits;
        break;
      }
    }
    return true;
  }

  void Write(const char *input, uint32_t lenInBytes) { WriteBits((const uint8_t *)input, bytes2bits(lenInBytes)); }
  bool Read(char *output, uint32_t lenInBytes) const { return ReadBits((uint8_t *)output, bytes2bits(lenInBytes)); }

  bool Read(BitStream &bs) const
  {
    uint32_t numberOfBitsUsed;
    if (!ReadCompressed(numberOfBitsUsed))
      return false;
    AlignReadToByteBoundary();
    if (readOffset + numberOfBitsUsed > bitsUsed)
      return false;
    bs.bitsUsed = numberOfBitsUsed;
    uint32_t bytesToCopy = bits2bytes(numberOfBitsUsed);
    bs.reserveBits(bytes2bits(bytesToCopy));
    memcpy(bs.GetData(), GetData() + bits2bytes(GetReadOffset()), bytesToCopy);
    SetReadOffset(GetReadOffset() + bytes2bits(bytesToCopy));
    return true;
  }

  void Write(const BitStream &bs)
  {
    uint32_t numberOfBitsUsed = bs.GetNumberOfBitsUsed();
    WriteCompressed(numberOfBitsUsed);
    // If someone ever will try to remove alignment at start/end, be sure to check everything with
    // 1 2 3 4 5 6 and 7 bits of data in front of the bitstream!
    AlignWriteToByteBoundary();
    uint32_t bytesToCopy = bits2bytes(numberOfBitsUsed);
    reserveBits(bytes2bits(bytesToCopy));
    memcpy(GetData() + bits2bytes(GetWriteOffset()), bs.GetData(), bytesToCopy);
    SetWriteOffset(GetWriteOffset() + bytes2bits(bytesToCopy));
  }

  template <typename T>
  typename eastl::disable_if_t<!eastl::is_trivially_copyable_v<T> || eastl::is_same_v<T, bool> ||
                               internal::supports_write_type<T>::value>
  Write(const T &t)
  {
    static_assert(!eastl::is_pointer_v<T>);
    WriteBits((const uint8_t *)&t, bytes2bits(sizeof(T)));
  }
  template <typename T>
  typename eastl::disable_if_t<
    !eastl::is_trivially_copyable_v<T> || eastl::is_same_v<T, bool> || internal::supports_read_type<T>::value, bool>
  Read(T &t) const
  {
    static_assert(!eastl::is_pointer_v<T> && !eastl::is_const_v<T>);
    return ReadBits((uint8_t *)&t, bytes2bits(sizeof(T)));
  }

  template <typename T>
  eastl::enable_if_t<internal::supports_write_type<T>::value> Write(const T &t)
  {
    write_type(*this, t);
  }

  template <typename T>
  eastl::enable_if_t<internal::supports_read_type<T>::value, bool> Read(T &t) const
  {
    return read_type(*this, t);
  }

  template <typename T>
  typename eastl::enable_if<internal::IsContainer<T>::value && !internal::IsString<T>::value>::type Write(const T &cont)
  {
    uint32_t sz = (uint32_t)cont.size(), rsz = 0;
    WriteCompressed(sz);
    for (auto &elem : cont)
    {
      Write(elem);
      ++rsz;
    }
    G_ASSERT(rsz == sz);
    G_UNUSED(sz);
  }
  template <typename T, size_t asz>
  void Write(const T (&cont)[asz])
  {
    for (auto &elem : cont)
      Write(elem);
  }
  template <typename T>
  typename eastl::enable_if<internal::IsContainer<T>::value && !internal::IsString<T>::value, bool>::type Read(T &cont) const
  {
    uint32_t sz = 0;
    if (!ReadCompressed(sz))
      return false;
    cont.resize(sz);
    for (uint32_t i = 0; i < sz; ++i)
      if (!Read(cont[i]))
        return false;
    return true;
  }
  template <typename T, size_t asz>
  bool Read(T (&cont)[asz])
  {
    for (auto &elem : cont)
      if (!Read(elem))
        return false;
    return true;
  }

  template <typename T>
  typename eastl::enable_if<internal::IsOptional<T>::value>::type Write(const T &opt)
  {
    bool hasVal = opt.has_value();
    Write(hasVal);
    if (hasVal)
      Write(opt.value());
  }
  template <typename T>
  typename eastl::enable_if<internal::IsOptional<T>::value, bool>::type Read(T &opt) const
  {
    bool hasVal = false;
    if (!Read(hasVal))
      return false;
    if (!hasVal)
    {
      opt.reset();
      return true;
    }
    typename T::value_type val;
    if (!Read(val))
      return false;
    opt = eastl::move(val);
    return true;
  }

  template <typename B> // Template to avoid implicit types overloads to bool
  typename eastl::enable_if<eastl::is_same<B, bool>::value>::type Write(B f)
  {
    f ? Write1() : Write0();
  }

  //
  // VLQ (https://en.wikipedia.org/wiki/Variable-length_quantity)
  //
  void WriteCompressed(int32_t v) { writeCompressedSignedGeneric(v); }
  bool ReadCompressed(int32_t &v) const { return readCompressedSignedGeneric(v); }
  void WriteCompressed(int16_t v) { writeCompressedSignedGeneric(v); }
  bool ReadCompressed(int16_t &v) const { return readCompressedSignedGeneric(v); }

  void WriteCompressed(uint32_t v) { writeCompressedUnsignedGeneric(v); }
  bool ReadCompressed(uint32_t &v) const { return readCompressedUnsignedGeneric(v); }
  void WriteCompressed(uint16_t v) { writeCompressedUnsignedGeneric(v); }
  bool ReadCompressed(uint16_t &v) const { return readCompressedUnsignedGeneric(v); }

  // aligned bytes read/write
  void WriteAlignedBytes(const uint8_t *input, uint32_t bytesLen)
  {
    AlignWriteToByteBoundary();
    Write((const char *)input, bytesLen);
  }
  bool ReadAlignedBytes(uint8_t *output, uint32_t bytesToRead) const
  {
    if (!bytesToRead)
      return true;
    AlignReadToByteBoundary();
    if (readOffset + (bytesToRead << 3) > bitsUsed)
      return false;
    memcpy(output, GetData() + (readOffset >> 3), bytesToRead);
    readOffset += bytesToRead << 3;
    return true;
  }
  void WriteAlignedBytesSafe(const char *input, uint32_t bytesLen, uint32_t maxBytesLen)
  {
    if (!input || !bytesLen)
      WriteCompressed((uint32_t)0);
    else
    {
      WriteCompressed(bytesLen);
      WriteAlignedBytes((const uint8_t *)input, min(bytesLen, maxBytesLen));
    }
  }
  bool ReadAlignedBytesSafe(char *output, uint32_t &inpLen, uint32_t maxLen) const
  {
    if (!ReadCompressed(inpLen))
      return false;
    inpLen = min(inpLen, maxLen);
    if (!inpLen)
      return false;
    return ReadAlignedBytes((uint8_t *)output, inpLen);
  }
  bool ReadAlignedBytesSafeAlloc(char **input, unsigned &ilen, unsigned maxLen) const
  {
    memfree(*input, allocator);
    *input = NULL;
    if (!ReadCompressed(ilen))
      return false;
    ilen = min(ilen, maxLen);
    if (!ilen)
      return false;
    *input = (char *)memalloc(ilen, allocator);
    return ReadAlignedBytes((uint8_t *)*input, (uint32_t)ilen);
  }

  // bools
  void Write0()
  {
    reserveBits(1);
    const uint32_t bitsmod8 = bitsUsed & 7;
    if (!(bitsUsed & 7))
      GetData()[bitsUsed >> 3] = 0;
    else
      GetData()[bitsUsed >> 3] &= ~(0x80 >> bitsmod8);
    bitsUsed++;
  }
  void Write1()
  {
    reserveBits(1);
    const uint32_t bitsmod8 = bitsUsed & 7;
    if (!bitsmod8)
      GetData()[bitsUsed >> 3] = 0x80;
    else
      GetData()[bitsUsed >> 3] |= 0x80 >> bitsmod8;
    bitsUsed++;
  }
  bool ReadBit() const
  {
    G_ASSERT(readOffset < bitsUsed);
    bool r = (GetData()[readOffset >> 3] & (0x80 >> (readOffset & 7))) != 0;
    readOffset++;
    return r;
  }
  bool Read(bool &v) const
  {
    if (readOffset + 1 > bitsUsed)
      return false;
    v = ReadBit();
    return true;
  }

  // other
  uint32_t CopyData(uint8_t **_data) const
  {
    G_ASSERT(_data);
    G_ASSERT(bitsUsed > 0);
    *_data = (uint8_t *)memalloc(bits2bytes(bitsUsed), allocator);
    memcpy(*_data, GetData(), bits2bytes(bitsUsed));
    return bitsUsed;
  }

  //
  // not compatible with RakNet interface
  //
  template <typename T>
  void WriteAt(T t, uint32_t pos)
  {
    const uint32_t curPos = GetWriteOffset();
    SetWriteOffset(pos);
    Write(t);
    SetWriteOffset(curPos);
  }

  template <typename T>
  void WriteArray(T *tptr, size_t cnt)
  {
    WriteBits((const uint8_t *)tptr, (uint32_t)bytes2bits(sizeof(T) * cnt));
  }
  template <typename T>
  inline bool ReadArray(T *tptr, size_t cnt) const
  {
    return ReadBits((uint8_t *)tptr, (uint32_t)bytes2bits(sizeof(T) * cnt));
  }

  void Write(const char *t) { writeString(t, (t && *t) ? strlen(t) : size_t(0)); }
  void Write(char *t) { writeString(t, (t && *t) ? strlen(t) : size_t(0)); }
  template <typename T>
  typename eastl::enable_if<internal::IsString<T>::value>::type Write(const T &str)
  {
    size_t len = (size_t)str.length();
    G_ASSERT(len == strlen(str.c_str()));
    writeString(str.c_str(), len);
  }

  template <typename T>
  typename eastl::enable_if<internal::IsString<T>::value, bool>::type Read(T &t) const
  {
    uint16_t l = 0;
    bool r = ReadCompressed(l);
    if (l && r)
    {
      internal::resize_str(t, l);
      r &= Read((char *)t.c_str(), l);
    }
    if (!r || !l)
      t.clear();
    return r;
  }

  void Write(const DataBlock &blk);
  bool Read(DataBlock &blk) const;

  void reserveBits(uint32_t bits)
  {
    if (bits == 0)
      return;

    uint32_t newBitsAllocated = bitsUsed + bits;
    if (!bitsAllocated || bitsAllocated < newBitsAllocated)
    {
      G_ASSERT(!bitsAllocated || dataOwner);
      size_t newBytesAllocated = max(bitsAllocated ? (bitsAllocated >> 3) * 2u : 16u, bits2bytes(newBitsAllocated));
      if (!data)
        data = (uint8_t *)allocator->alloc(newBytesAllocated, &newBytesAllocated);
      else
        data = (uint8_t *)allocator->realloc(data, newBytesAllocated, &newBytesAllocated);
      if (size_t tailBytes = newBytesAllocated - (bitsAllocated >> 3))
        memset(data + (bitsAllocated >> 3), 0, tailBytes);
      dataOwner = 1;
      bitsAllocated = (uint32_t)bytes2bits(newBytesAllocated);
    }
  }

  void swap(BitStream &bs);

protected:
  template <typename T>
  void writeCompressedSignedGeneric(T v)
  {
    G_STATIC_ASSERT(eastl::is_signed<T>::value);
    T vu = (v << 1) ^ (v >> (sizeof(T) * CHAR_BIT - 1));
    writeCompressedUnsignedGeneric(typename eastl::make_unsigned<T>::type(vu));
  }

  template <typename T>
  bool readCompressedSignedGeneric(T &v) const
  {
    G_STATIC_ASSERT(eastl::is_signed<T>::value);
    typename eastl::make_unsigned<T>::type vu;
    if (!readCompressedUnsignedGeneric(vu))
      return false;
    v = static_cast<T>((vu >> 1) ^ (-(vu & 1)));
    return true;
  }

  template <typename T>
  void writeCompressedUnsignedGeneric(T v)
  {
    G_STATIC_ASSERT(eastl::is_unsigned<T>::value);
    for (int i = 0; i < sizeof(v) + 1; ++i)
    {
      uint8_t byte = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
      Write(byte);
      v >>= 7;
      if (!v)
        break;
    }
  }

  template <typename T>
  bool readCompressedUnsignedGeneric(T &v) const
  {
    G_STATIC_ASSERT(eastl::is_unsigned<T>::value);
    v = 0;
    for (int i = 0; i < sizeof(v) + 1; ++i)
    {
      uint8_t byte = 0;
      if (!Read(byte))
        return false;
      v |= T(byte & ~(1 << 7)) << (i * 7);
      if ((byte & (1 << 7)) == 0)
        break;
    }
    return true;
  }

  void writeString(const char *str, size_t str_len)
  {
    G_ASSERT(uint16_t(str_len) == str_len);
    WriteCompressed((uint16_t)str_len);
    if (str_len)
      Write(str, (uint32_t)str_len);
  }

  void DuplicateBitStream(const BitStream &bs)
  {
    bitsUsed = bs.bitsUsed;
    dataOwner = 1;
    readOffset = bs.readOffset;
    size_t bytesAllocated = 0;
    data = bs.bitsAllocated ? (uint8_t *)allocator->alloc(bits2bytes(bs.bitsAllocated), &bytesAllocated) : nullptr;
    memcpy(data, bs.data, bits2bytes(bs.bitsAllocated));
    bitsAllocated = (uint32_t)bytes2bits(bytesAllocated);
  }

  static inline uint32_t bits2bytes(uint32_t bi) { return (bi + 7) >> 3; }
  static inline uint32_t bytes2bits(uint32_t by) { return by << 3; }

  uint32_t bitsUsed : 31;
  uint32_t dataOwner : 1;
  uint32_t bitsAllocated;
  mutable uint32_t readOffset;
  uint8_t *data;
  IMemAlloc *allocator; // TODO: rework to allocator class
};
}; // namespace danet

EA_RESTORE_VC_WARNING()
