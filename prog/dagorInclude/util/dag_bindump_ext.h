//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_bindump.h"
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <generic/dag_span.h>
#include <EASTL/string_view.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>

namespace bindump
{
struct SizeMetter : IWriter
{
  uint64_t mSize = 0;

  void begin() override { mSize = 0; }
  void end() override {}
  uint64_t append(uint64_t size) override
  {
    uint64_t offset = mSize;
    mSize += size;
    return offset;
  }
  void write(uint64_t, const void *, uint64_t) override {}
};

template <typename Cont>
struct MemoryWriterBase : IWriter
{
  Cont mData;

  void begin() override { mData.clear(); }
  void end() override {}
  uint64_t append(uint64_t size) override
  {
    uint64_t offset = mData.size();
    mData.resize(offset + size);
    return offset;
  }
  void write(uint64_t offset, const void *src_data, uint64_t src_size) override
  {
    G_ASSERT(offset + src_size <= mData.size());
    const uint8_t *data_start = (const uint8_t *)src_data;
    eastl::copy(data_start, data_start + src_size, mData.begin() + offset);
  }
};

// TODO: Should be removed as soon as we switch to `dag::Vector` in all places where the `MemoryWriter` is used
using MemoryWriter = MemoryWriterBase<eastl::vector<uint8_t>>;
using VectorWriter = MemoryWriterBase<dag::Vector<uint8_t>>;

class FileWriter : public IWriter
{
  FullFileSaveCB mSaver;

public:
  FileWriter(const char *filename) : mSaver(filename) {}

  explicit operator bool() const { return mSaver.fileHandle != nullptr; }

  void begin() override { mSaver.seekto(0); }
  void end() override { mSaver.close(); }

  uint64_t append(uint64_t size) override
  {
    uint64_t offset = mSaver.tell();
    eastl::vector<char> empty_data(size);
    mSaver.write(empty_data.data(), empty_data.size());
    return offset;
  }
  void write(uint64_t offset, const void *src_data, uint64_t src_size) override
  {
    int size = mSaver.tell();
    G_ASSERT(offset + src_size <= size);
    mSaver.seekto((int)offset);
    mSaver.write(src_data, (int)src_size);
    mSaver.seekto(size);
  }
};

class MemoryReader : public IReader
{
  const uint8_t *mData = nullptr;
  const uint64_t mSize;

public:
  MemoryReader(const uint8_t *data, uint64_t size) : mData(data), mSize(size) {}

  void begin() override {}
  void end() override {}

  void read(uint64_t offset, void *dest_data, uint64_t dest_size) override
  {
    G_ASSERT(offset + dest_size <= mSize);
    eastl::copy(mData + offset, mData + offset + dest_size, (uint8_t *)dest_data);
  }
};

class FileReader : public IReader
{
  FullFileLoadCB mLoader;

public:
  FileReader(const char *filename) : mLoader(filename) {}

  explicit operator bool() const { return mLoader.fileHandle != nullptr; }

  void begin() override { mLoader.seekto(0); }
  void end() override { mLoader.close(); }

  void read(uint64_t offset, void *dest_data, uint64_t dest_size) override
  {
    mLoader.seekto(offset);
    mLoader.read(dest_data, (int)dest_size);
  }
};

template <typename LayoutType>
bool writeToFileFast(const LayoutType &layout, const char *filename)
{
  FullFileSaveCB saver(filename);
  if (!saver.fileHandle)
    return false;

  SizeMetter size_metter;
  streamWrite(layout, size_metter);

  MemoryWriter mem_writer;
  mem_writer.mData.reserve(size_metter.mSize);
  streamWrite(layout, mem_writer);

  saver.write(mem_writer.mData.data(), mem_writer.mData.size());
  return true;
}

namespace detail
{
template <enum detail::Target target>
struct StrHolderBase : public traits::List<char, 4, target>
{
  bool empty() const { return this->size() == 0; }
  uint64_t length() const { return !empty() ? this->size() - 1 : 0; }
  const char *c_str() const { return !empty() ? this->getDataPtr() : ""; }
  operator eastl::string_view() const { return eastl::string_view(c_str(), length()); }
  explicit operator const char *() const { return c_str(); }
};

template <typename UserDataType, uint64_t alignment, enum detail::Target target>
struct VecHolderBase : public traits::List<UserDataType, alignment, target>
{
  bool empty() const { return this->size() == 0; }

  operator dag::ConstSpan<UserDataType>() const { return {data(), (intptr_t)this->size()}; }
  operator dag::Span<UserDataType>() { return {data(), (intptr_t)this->size()}; }

  const UserDataType *begin() const { return this->getDataPtr(); }
  const UserDataType *end() const { return this->getDataPtr() + this->size(); }
  UserDataType *begin() { return this->getDataPtr(); }
  UserDataType *end() { return this->getDataPtr() + this->size(); }
  const UserDataType *data() const { return this->getDataPtr(); }
  UserDataType *data() { return this->getDataPtr(); }
};

template <typename UserDataType, enum detail::Target target>
class SpanBase : public traits::Address<UserDataType, target>
{
protected:
  uint32_t mCount = 0;

public:
  uint32_t size() const { return mCount; }
  const UserDataType &operator[](uint64_t no) const { return *(data() + no); }
  UserDataType &operator[](uint64_t no) { return *(data() + no); }
  operator dag::ConstSpan<UserDataType>() const { return {data(), (intptr_t)size()}; }
  explicit operator const UserDataType *() const { return data(); }

  const UserDataType &back() const { return *(end() - 1); }
  const UserDataType *begin() const { return data(); }
  const UserDataType *end() const { return data() + size(); }
  const UserDataType *data() const { return size() ? this->getDataPtr() : nullptr; }
  UserDataType *begin() { return data(); }
  UserDataType *end() { return data() + size(); }
  UserDataType *data() { return size() ? this->getDataPtr() : nullptr; }

  UserDataType &get() = delete;
  const UserDataType &get() const = delete;
};
} // namespace detail

template <enum detail::Target target = detail::MASTER>
struct StrHolder : public detail::StrHolderBase<target>
{
  template <enum detail::Target t>
  using MyLayout = StrHolder<t>;
};

template <>
struct StrHolder<detail::MASTER> : public detail::StrHolderBase<detail::MASTER>
{
  template <enum detail::Target t>
  using MyLayout = StrHolder<t>;

  StrHolder &operator=(eastl::string_view str)
  {
    this->resize(str.size() + 1);
    eastl::copy(str.begin(), str.end(), this->mData.begin());
    this->mData[str.size()] = 0;
    return *this;
  }
};

template <typename UserDataType, uint64_t alignment = 4, enum detail::Target target = detail::MASTER>
struct VecHolder : public detail::VecHolderBase<detail::traits::RetargetType<UserDataType, target>, alignment, target>
{
  template <enum detail::Target t>
  using MyLayout = VecHolder<UserDataType, alignment, t>;
};

template <typename UserDataType, uint64_t alignment>
struct VecHolder<UserDataType, alignment, detail::MASTER> : public detail::VecHolderBase<UserDataType, alignment, detail::MASTER>
{
  template <enum detail::Target t>
  using MyLayout = VecHolder<UserDataType, alignment, t>;

  VecHolder() = default;
  VecHolder(const VecHolder &) = default;
  VecHolder &operator=(const VecHolder &) = default;
  VecHolder &operator=(VecHolder &&other) noexcept
  {
    this->mData = eastl::move(other.mData);
    this->mListOffset = eastl::move(other.mListOffset);
    return *this;
  }

  VecHolder(const detail::traits::List<UserDataType, alignment, detail::MASTER> &vec)
  {
    detail::traits::List<UserDataType, alignment, detail::MASTER>::operator=(vec);
  }
  VecHolder(const eastl::vector<UserDataType> &vec) { *this = vec; }
  VecHolder(dag::Span<UserDataType> slice) { *this = slice; }

  VecHolder &operator=(const eastl::vector<UserDataType> &vec)
  {
    this->mData = vec;
    return *this;
  }

  VecHolder &operator=(eastl::vector<UserDataType> &&vec) noexcept
  {
    this->mData = eastl::move(vec);
    return *this;
  }

  VecHolder &operator=(dag::Span<UserDataType> slice)
  {
    this->mData.assign(slice.begin(), slice.end());
    return *this;
  }

  VecHolder &operator=(dag::ConstSpan<UserDataType> slice)
  {
    this->mData.assign(slice.begin(), slice.end());
    return *this;
  }

  eastl::vector<UserDataType> &content() { return this->mData; }

  operator eastl::vector<UserDataType>() && { return eastl::move(this->mData); }
};

template <typename UserDataType, enum detail::Target target = detail::MASTER>
struct Span : public detail::SpanBase<detail::traits::RetargetType<UserDataType, target>, target>
{
  template <enum detail::Target t>
  using MyLayout = Span<UserDataType, t>;
};

template <typename UserDataType>
struct Span<UserDataType, detail::MASTER> : public detail::SpanBase<UserDataType, detail::MASTER>
{
  template <enum detail::Target t>
  using MyLayout = Span<UserDataType, t>;

  using detail::traits::Address<UserDataType, detail::MASTER>::operator=;
  void setCount(uint32_t count) { this->mCount = count; }

  bool operator!=(const Span &other) const { return !operator==(other); }
  bool operator==(const Span &other) const { return this->begin() == other.begin() && this->size() == other.size(); }
};

enum class BehaviorOnUncompressed
{
  Copy,
  Reference,
};

namespace detail
{
template <typename DataType>
struct Compressed
{
  BINDUMP_BEGIN_LAYOUT(Type)
  private:
    uint32_t decompressedSize = 0;
    VecHolder<uint8_t, 1, target> compressedData;

  public:
    uint32_t getDecompressedSize() const { return decompressedSize; }

    uint32_t getCompressedSize() const { return compressedData.size(); }

    template <typename DataTypeEx = DataType>
    size_t compress(const DataTypeEx &data, int level, ZSTD_CCtx_s *ctx = nullptr, const ZSTD_CDict_s *dict = nullptr)
    {
      static_assert(eastl::is_base_of_v<DataType, DataTypeEx>, "DataTypeEx must be derived from DataType");
      static_assert(target == MASTER, "Unable to call compress() method on Mapper side");
      MemoryWriter writer;
      streamWriteLayout(data, writer);

      if (level < 0)
      {
        compressedData = eastl::move(writer.mData);
        return compressedData.size();
      }

      compressedData.resize(zstd_compress_bound(writer.mData.size()));
      size_t compressed_size = 0;
      if (dict)
        compressed_size =
          zstd_compress_with_dict(ctx, compressedData.data(), compressedData.size(), writer.mData.data(), writer.mData.size(), dict);
      else
        compressed_size = zstd_compress(compressedData.data(), compressedData.size(), writer.mData.data(), writer.mData.size(), level);
      G_ASSERT(compressed_size <= compressedData.size());
      decompressedSize = writer.mData.size();
      compressedData.resize(compressed_size);
      return compressedData.size();
    }
    template <typename Cont>
    DataType *decompress(Cont & out_data, BehaviorOnUncompressed bou, const ZSTD_DDict_s *dict = nullptr) const
    {
      static_assert(target == MAPPER, "Unable to call decompress() method on Master side");
      static_assert(sizeof(typename Cont::value_type) == 1, "The container must consist of single-byte elements");
      if (decompressedSize)
      {
        out_data.resize(decompressedSize);
        size_t decompressed_size = 0;
        if (EASTL_LIKELY(dict))
        {
          ZSTD_DCtx_s *dctx = zstd_create_dctx(/*tmp*/ true); // tmp for framemem
          decompressed_size =
            zstd_decompress_with_dict(dctx, out_data.data(), out_data.size(), compressedData.data(), compressedData.size(), dict);
          zstd_destroy_dctx(dctx);
        }
        else // TODO: use tmp context too
          decompressed_size = zstd_decompress(out_data.data(), out_data.size(), compressedData.data(), compressedData.size());
        if (decompressed_size != decompressedSize)
          return nullptr;
      }
      else if (bou == BehaviorOnUncompressed::Reference)
      {
        out_data.clear();
        return map<DataType>((uint8_t *)compressedData.data());
      }
      else
      {
        out_data.assign(compressedData.begin(), compressedData.end());
      }
      return map<DataType>(out_data.data());
    }
  BINDUMP_END_LAYOUT()
};
} // namespace detail

template <template <enum detail::Target> class LayoutClass>
using CompressedLayout = typename detail::Compressed<detail::EnableHash<LayoutClass, detail::MASTER>>::template Type<detail::MASTER>;
template <typename UserDataType>
using Compressed = typename detail::Compressed<UserDataType>::template Type<detail::MASTER>;

#define BINDUMP_USING_EXTENSION()                                                                                  \
  template <typename UserDataType, uint64_t alignment = 4>                                                         \
  using VecHolder = bindump::VecHolder<UserDataType, alignment, target>;                                           \
  template <typename UserDataType>                                                                                 \
  using Span = bindump::Span<UserDataType, target>;                                                                \
  using StrHolder = bindump::StrHolder<target>;                                                                    \
  template <template <enum bindump::detail::Target> class LayoutClass>                                             \
  using CompressedLayout =                                                                                         \
    typename bindump::detail::Compressed<bindump::detail::EnableHash<LayoutClass, target>>::template Type<target>; \
  template <typename UserDataType>                                                                                 \
  using Compressed = typename bindump::detail::Compressed<UserDataType>::template Type<target>;

template <typename Type>
class vector : public eastl::vector<Type>
{
  void serialize() const
  {
    uint32_t count = this->size();
    stream::writeAux(count);
    stream::write(this->data(), count);
  }
  void deserialize()
  {
    uint32_t count = 0;
    stream::readAux(count);
    this->resize(count);
    stream::read(this->data(), count);
  }
  void hashialize() const { stream::hash<Type>(); }

public:
  BINDUMP_ENABLE_STREAMING(vector, eastl::vector<Type>, vector);
};

class string : public eastl::string
{
  void serialize() const
  {
    uint32_t count = this->length();
    stream::writeAux(count);
    stream::write(this->data(), count);
  }
  void deserialize()
  {
    uint32_t count = 0;
    stream::readAux(count);
    this->resize(count);
    stream::read(this->begin(), count);
  }
  void hashialize() const { stream::hash<char>(); }

public:
  BINDUMP_ENABLE_STREAMING(string, eastl::string, string);
};

template <typename Type>
class Ptr
{
  eastl::unique_ptr<Type> mData;

  void serialize() const
  {
    uint8_t exist = mData ? 1 : 0;
    stream::writeAux(exist);
    if (exist)
      stream::write(*mData);
  }
  void deserialize()
  {
    uint8_t exist = 0;
    stream::readAux(exist);
    if (!exist)
      return;

    create();
    stream::read(*mData);
  }
  void hashialize() const { stream::hash<Type>(); }

public:
  Ptr() = default;
  Ptr(Ptr &&) noexcept = default;
  Ptr &operator=(Ptr &&) = default;
  Ptr(const Ptr &other) { *this = other; }

  BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(Ptr, mData.reset(); if (other.mData) create(*other.mData););

  template <typename... Args>
  Type &create(Args &&...args)
  {
    mData = eastl::make_unique<Type>(eastl::forward<Args>(args)...);
    return *mData;
  }
  operator Type *() { return mData.get(); }
  operator const Type *() const { return mData.get(); }
  explicit operator bool() const { return mData != nullptr; }
  Type *operator->() { return mData.get(); }
  const Type *operator->() const { return mData.get(); }
  Type &operator*() { return *mData; }
  const Type &operator*() const { return *mData; }
  Type *get() { return mData.get(); }
  const Type *get() const { return mData.get(); }
};

template <typename Type>
class Address : public detail::master::DeferredAction
{
  Type *mData = nullptr;
  mutable uint64_t mOffset = 0;

  void serialize() const
  {
    mOffset = detail::streamer::writer::context().worker->append(0);
    stream::writeAux(mOffset);
    detail::streamer::writer::context().deferred_actions.emplace_back(this);
  }
  void deserialize()
  {
    mData = nullptr;
    stream::readAux(mOffset);
    if (mOffset)
      detail::streamer::reader::context().deferred_actions.emplace_back(this);
  }
  void hashialize() const { stream::hash<uint64_t>(); }

  void deferredWrite() const override
  {
    uint64_t offset_to_data = mData ? detail::streamer::writer::context().translateAddressToOffset(mData) : 0;
    detail::streamer::writer::context().worker->write(mOffset, &offset_to_data, sizeof(offset_to_data));
  }

  void deferredRead() override { mData = detail::streamer::reader::context().translateOffsetToAddress<Type>(mOffset); }

public:
  Address() = default;
  Address(Type *address) : mData(address) {}
  BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(Address, mData = other.mData);
  Address &operator=(Type *address)
  {
    mData = address;
    return *this;
  }

  operator Type *() { return mData; }
  operator const Type *() const { return mData; }
  explicit operator bool() const { return mData != nullptr; }
  Type *operator->() { return mData; }
  const Type *operator->() const { return mData; }
  Type &operator*() { return *mData; }
  const Type &operator*() const { return *mData; }
  Type *get() { return mData; }
  const Type *get() const { return mData; }
};

template <typename Type>
class EnableHash
{
  struct EmptyReader : public IReader
  {
    void begin() override {}
    void end() override {}
    void read(uint64_t, void *, uint64_t) override {}
  };

  uint64_t mIsLayoutValid = 1;

  void serialize() const
  {
    HashVal<64> hash = getHash<Type>();
    stream::write(hash);
  }
  void deserialize()
  {
    HashVal<64> hash;
    stream::read(hash);
    mIsLayoutValid = hash == getHash<Type>();
    if (!isLayoutValid())
    {
      static EmptyReader empty_reader;
      detail::streamer::reader::context().worker = &empty_reader;
    }
  }
  void hashialize() const { stream::hash<HashVal<64>>(); }

public:
  BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(EnableHash, ;);
  bool isLayoutValid() const { return mIsLayoutValid != 0; }
};
} // namespace bindump
