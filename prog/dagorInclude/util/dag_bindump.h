//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_hash.h"
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <EASTL/type_traits.h>
#include <EASTL/vector.h>
#include <EASTL/stack.h>
#include <EASTL/map.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>

// The `bindump` library is designed for easy serialization and deserialization of binary dumps
// This library has the following features:
// * Quick start. All you need is just to include this header file and start using
// * The dump layout is described once and is used by both the writer and the reader
// * There is no need to patch anything manually or write in a certain order, everything is done automatically
// * There is a built-in check of the dump format. That is, the reader will check that the dump format matches the layout

// Usage example:
// ```
//   BINDUMP_BEGIN_LAYOUT(SimpleLayout)
//     uint64_t value = 123;
//     List<std::array<double, 4>> colors;
//     BINDUMP_BEGIN_LIST(InnerLayout)
//       int data;
//       BINDUMP_BEGIN_LIST(InnerInnerLayout)
//         List<int> indices;
//       BINDUMP_END_LIST()
//     BINDUMP_END_LIST()
//   BINDUMP_END_LAYOUT()
//
//   bindump::MemoryWriter data;
//   bindump::Master<SimpleLayout> write_dump;
//   write_dump.InnerLayout.resize(10);
//   write_dump.InnerLayout[5].InnerInnerLayout.resize(10);
//   write_dump.InnerLayout[5].InnerInnerLayout[6].indices.resize(10);
//   write_dump.InnerLayout[5].InnerInnerLayout[6].indices[7] = 42;
//   write_dump.write(data);
//
//   const bindump::Mapper<SimpleLayout> *read_dump = bindump::Mapper<SimpleLayout>::create(data.data.data());
//   G_ASSERT(read_dump->InnerLayout[5].InnerInnerLayout[6].indices[7] == 42);
// ```

// The layout description starts with `BEGIN_LAYOUT(LayoutName)` and ends with `END_LAYOUT()`
// you can also extend the layout using inheritance `BEGIN_EXTEND_LAYOUT(LayoutName, SomeBaseLayout)`

// Inside the layout, you can use the following types
// * Any POD-type as is
// * Field - stores a user-defined wrapper type. For example, `Field<bindump::Vector<Type>> my_vector;`
// * Address - stores the address to any internal layout element
// * List - array of elements of POD type or wrapper type
// * Layout - a different layout
// * Ptr - pointer to layout, maybe empty
// * LayoutList - array of elements of a different layout type

// The `bindump` has the following targets:
// Master - used for layout population/serialization/deserialization
// Mapper - has the fastest deserialization possible. Does not allocate additional memory
// Hasher - to calculate the layout hash

// There is also a streamer here. It is not compatible with the master and mapper dump.
// But unlike them, it requires less boiler-plate code.
// With the stream API, you can convert any type into a serializable type, for example like this:
// ```
//   template<typename Type>
//   class SerializableDynArray : public DynArray<Type>
//   {
//     void serialize() const
//     {
//       uint32_t count = this->size();
//       stream::writeAux(count);
//       stream::write(this->data(), count);
//     }
//     void deserialize()
//     {
//       uint32_t count = 0;
//       stream::readAux(count);
//       this->resize(count);
//       stream::read(this->data(), count);
//     }
//   public:
//     BINDUMP_ENABLE_STREAMING(SerializableDynArray, DynArray<Type>, DynArray);
//   };
//
//   struct MyLayout
//   {
//     int value;
//     SerializableDynArray<float> vertices;
//     SerializableDynArray<SerializableDynArray<double>> matrices;
//   };
//
//   Mylayout layout = populate_with_some_data();
//   bindump::MemoryWriter data;
//   bindump::streamWrite(layout, data);
// ```

// Implementation details:
// The secret ingredient is based on the assignment operator. If you declare a variable of a some class,
// and assign it another variable of the same class, then the compiler will carefully call similar operators
// for all members of this class in the order of their declaration. This is what we need to implement this library.
// All wrapper types have this operator

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced local function has been removed

namespace bindump
{
struct IWriter
{
  virtual ~IWriter() = default;
  virtual void begin() = 0;
  virtual void end() = 0;
  // Adds uninitialized data at the end and returns offset before adding
  virtual uint64_t append(uint64_t size) = 0;
  virtual void write(uint64_t offset, const void *src_data, uint64_t src_size) = 0;
};

struct IReader
{
  virtual ~IReader() = default;
  virtual void begin() = 0;
  virtual void end() = 0;
  virtual void read(uint64_t offset, void *dest_data, uint64_t dest_size) = 0;
};

namespace detail
{
enum Target
{
  MASTER,
  MAPPER,
  HASHER,
};

namespace traits
{
template <typename Type, enum detail::Target target>
constexpr auto getType(...) -> Type;

template <typename Type, enum detail::Target target, typename dest = typename Type::template MyLayout<target>>
constexpr auto getType(const Type &) -> dest;

template <typename Type, enum detail::Target target>
using RetargetType = decltype(getType<Type, target>(Type{}));

template <typename Type>
constexpr uint64_t sizeOf()
{
  return sizeof(RetargetType<Type, MAPPER>);
}
} // namespace traits

namespace master
{
struct DeferredAction
{
  virtual void deferredWrite() const = 0;
  virtual void deferredRead() = 0;
};

template <typename WorkerType, typename WrapperAddressType>
struct Context
{
  struct Props
  {
    uint64_t stream_offset = 0;
    WrapperAddressType wrapper_address = nullptr;
  };

  using DeferredActionType =
    eastl::conditional_t<eastl::is_const_v<eastl::remove_pointer_t<WrapperAddressType>>, const DeferredAction *, DeferredAction *>;

  bool enabled = false;
  WorkerType *worker = nullptr;
  Props current_props;
  eastl::stack<Props> props_stack;
  eastl::vector<DeferredActionType> deferred_actions;

  bool isEnabled() const { return enabled; }
  void enable()
  {
    G_ASSERT(!enabled);
    enabled = true;
  }
  void disable()
  {
    G_ASSERT(enabled);
    enabled = false;
  }

  void begin(WorkerType &wk)
  {
    G_ASSERT(!current_props.stream_offset && !current_props.wrapper_address && props_stack.empty());
    G_ASSERT(!worker && deferred_actions.empty());
    worker = &wk;
    worker->begin();
    enable();
  }

  virtual void deferredAction(DeferredActionType) {}

  void end()
  {
    G_ASSERT(props_stack.empty());
    for (DeferredActionType action : deferred_actions)
      deferredAction(action);
    disable();
    close();
  }

  void close()
  {
    eastl::stack<Props>{}.swap(props_stack);
    if (worker)
      worker->end();
    enabled = false;
    worker = nullptr;
    current_props = Props{};
    deferred_actions.clear();
  }

  template <typename LayoutType>
  void beginLayout(LayoutType &layout, uint64_t stream_offset)
  {
    props_stack.push(current_props);
    current_props.wrapper_address = (WrapperAddressType)&layout;
    current_props.stream_offset = stream_offset;
    props_stack.push(current_props);
  }

  template <typename LayoutType>
  void endLayout()
  {
    uint64_t begin_layout_offset = props_stack.top().stream_offset;
    uint64_t end_layout_offset = current_props.stream_offset;
    endLayoutImpl<LayoutType>(begin_layout_offset, end_layout_offset);
  }

  template <typename LayoutType>
  void endLayoutImpl(uint64_t begin_layout_offset, uint64_t end_layout_offset)
  {
    uint64_t expected_offset = begin_layout_offset + traits::sizeOf<LayoutType>();
    streamUnmanagedData(expected_offset, end_layout_offset);

    props_stack.pop();
    current_props = props_stack.top();
    props_stack.pop();
  }

  virtual void stream(WrapperAddressType address, uint64_t size) = 0;

  void streamUnmanagedData(uint64_t expected_offset, uint64_t actual_offset)
  {
    G_ASSERT(expected_offset >= actual_offset);
    uint64_t delta = expected_offset - actual_offset;
    if (delta)
      stream(current_props.wrapper_address, delta);
  }

  template <typename WrapperType>
  void preStreamData(const WrapperType &wrapper)
  {
    const uint8_t *address = (const uint8_t *)&wrapper;
    streamUnmanagedData((uint64_t)address, (uint64_t)current_props.wrapper_address);
    current_props.wrapper_address = (WrapperAddressType)address;
  }

  template <typename WrapperType>
  void postStreamData(const WrapperType &wrapper)
  {
    current_props.wrapper_address = (WrapperAddressType)&wrapper + sizeof(WrapperType);
  }
};
} // namespace master

namespace hasher
{
struct Hasher
{
  HashVal<64> mHash = 0;

  void begin() { mHash = FNV1Params<64>::offset_basis; }
  void end() {}
  void step(uint32_t value) { mHash = fnv1_step<64>(value, mHash); }
};

struct HasherContext : master::Context<Hasher, const uint8_t *>
{
  void stream(const uint8_t *, uint64_t size) override { this->worker->step((uint32_t)size); }

  template <typename LayoutType>
  void endLayout()
  {
    uint64_t begin_layout_address = (uint64_t)props_stack.top().wrapper_address;
    uint64_t end_layout_address = (uint64_t)current_props.wrapper_address;
    this->template endLayoutImpl<LayoutType>(begin_layout_address, end_layout_address);
  }
};

inline HasherContext &context()
{
  thread_local HasherContext ctx;
  return ctx;
}

template <typename LayoutType>
static void hash()
{
  if (eastl::is_trivially_copy_assignable_v<LayoutType>)
  {
    context().worker->step(sizeof(LayoutType));
    return;
  }

  LayoutType src{};
  LayoutType dest;
  context().beginLayout<LayoutType>(src, 0);
  dest = src;
  context().endLayout<LayoutType>();
}

template <typename UserDataType, uint32_t random_number>
struct DataHasher
{
  // A dummy byte is added to suppress empty base optimization,
  // because it is necessary that the sizeof(DataHasher) be the same regardless of whether it is base type or not
  uint8_t __dummy = 0;
  DataHasher &operator=(const DataHasher &other)
  {
    context().preStreamData(other);
    hash<UserDataType>();
    context().worker->step(random_number);
    context().postStreamData(other);
    return *this;
  }
};
} // namespace hasher
} // namespace detail

template <typename LayoutType>
HashVal<64> getHash()
{
  detail::hasher::Hasher hasher;
  detail::hasher::context().begin(hasher);
  detail::hasher::hash<LayoutType>();
  detail::hasher::context().end();
  return hasher.mHash;
}

namespace detail
{
namespace master
{
namespace writer
{
struct WriterContext : Context<IWriter, const uint8_t *>
{
  void stream(const uint8_t *address, uint64_t size) override
  {
    worker->write(current_props.stream_offset, address, size);
    current_props.stream_offset += size;
  }

  void deferredAction(const DeferredAction *action) override { action->deferredWrite(); }
};

inline WriterContext &context()
{
  thread_local WriterContext ctx;
  return ctx;
}

template <typename Type>
static uint64_t append(const Type *data, uint64_t count)
{
  uint64_t offset = context().worker->append(count * sizeof(Type));
  context().worker->write(offset, data, count * sizeof(Type));
  return offset;
}

static uint64_t append(uint64_t size) { return context().worker->append(size); }

static void writeOffset(uint64_t offset)
{
  int32_t delta = 0;
  if (offset)
  {
    int64_t delta64 = offset - context().current_props.stream_offset;
    G_ASSERT(delta64 >= eastl::numeric_limits<int32_t>::min() && delta64 <= eastl::numeric_limits<int32_t>::max());
    delta = (int32_t)delta64;
  }
  context().stream((const uint8_t *)&delta, sizeof(delta));
}

template <typename Type, uint64_t alignment, bool should_write_count = true>
static uint64_t writeList(const Type *data, uint32_t count)
{
  uint64_t offset = 0;
  if (count)
  {
    uint64_t current_offset = append(0);
    append((alignment - current_offset % alignment) % alignment);
    if (data)
      offset = append(data, count);
    else
      offset = append(count * sizeof(Type));
  }
  writeOffset(offset);
  if (should_write_count)
    context().stream((const uint8_t *)&count, sizeof(count));
  return offset;
}

template <typename LayoutType>
static void begin(const LayoutType &layout, IWriter &writer)
{
  context().begin(writer);
  context().beginLayout(layout, 0);
  append(traits::sizeOf<LayoutType>());
}

template <typename LayoutType>
static void end()
{
  context().endLayout<LayoutType>();
  context().end();
}
} // namespace writer

namespace reader
{
struct ReaderContext : Context<IReader, uint8_t *>
{
  void stream(uint8_t *address, uint64_t size) override
  {
    worker->read(current_props.stream_offset, address, size);
    current_props.stream_offset += size;
  }

  void deferredAction(DeferredAction *action) override { action->deferredRead(); }

  // stream offset -> List content address
  eastl::map<uint64_t, uint8_t *> offset_to_lists;
};

inline ReaderContext &context()
{
  thread_local ReaderContext ctx;
  return ctx;
}

static uint64_t readOffset()
{
  uint64_t old_offset = context().current_props.stream_offset;
  int32_t offset = 0;
  context().stream((uint8_t *)&offset, sizeof(offset));
  if (offset)
    offset += old_offset;
  return offset;
}

static eastl::pair<uint64_t, uint64_t> readList(bool should_read_count)
{
  uint64_t offset_to_list = readOffset();
  uint32_t count = offset_to_list ? 1 : 0;
  if (should_read_count)
    context().stream((uint8_t *)&count, sizeof(count));
  return {offset_to_list, count};
}

template <typename LayoutType>
static void begin(const LayoutType &layout, IReader &reader)
{
  G_ASSERT(context().offset_to_lists.empty());
  context().begin(reader);
  context().beginLayout(layout, 0);
}

template <typename LayoutType>
static void end()
{
  context().endLayout<LayoutType>();
  context().end();
  context().offset_to_lists.clear();
}
} // namespace reader

// All wrapped types have an assignment operator.
// If the read/write mode is activated now, then data is being reading/writing,
// otherwise it works like a common assignment operator
#define DECLARE_ASSIGNMENT_OPERATOR_BASE(Type, AssignCode) \
  Type &operator=(const Type &other)                       \
  {                                                        \
    if (writer::context().worker)                          \
      other.write();                                       \
    else if (reader::context().worker)                     \
      read();                                              \
    else                                                   \
      AssignCode;                                          \
    return *this;                                          \
  }

#define DECLARE_ASSIGNMENT_OPERATOR(Type, BaseType) DECLARE_ASSIGNMENT_OPERATOR_BASE(Type, BaseType::operator=(other))

template <typename LayoutType>
struct HashWrapper
{
  HashVal<64> hash;
  operator HashVal<64>() const { return hash; }

  HashWrapper() { hash = getHash<traits::RetargetType<LayoutType, HASHER>>(); }
};

template <typename Type>
class ListWrapper;

template <typename Type>
struct ListElementAddress
{
  ListWrapper<Type> *mList = nullptr;
  uint64_t mIndex = 0;
  eastl::shared_ptr<uint64_t> mListOffset;

  ListElementAddress() = default;
  ListElementAddress(ListWrapper<Type> &list, uint64_t index) : mList(&list), mIndex(index), mListOffset(list.mListOffset) {}

  uint64_t getElementOffset() const
  {
    if (!mListOffset)
      return 0;

    G_ASSERT(*mListOffset);

    // Here we calculate the offset of the i-th element on the mapper side.
    // For this we need to know the size of the element on the mapper side,
    // for POD-types this is just a `sizeof(Type)`.
    // But another layout can also be an element,
    // you can't just take a sizeof for it, so we use an utility function.
    return *mListOffset + mIndex * traits::sizeOf<Type>();
  }
  Type *getDataPtr()
  {
    G_ASSERT(mList);
    return mList->getDataPtr(mIndex);
  }
  const Type *getDataPtr() const
  {
    G_ASSERT(mList);
    return mList->getDataPtr(mIndex);
  }
};

template <typename Type>
class ListWrapper
{
protected:
  using data_t = Type;
  friend ListElementAddress<data_t>;
  eastl::vector<data_t> mData;
  eastl::shared_ptr<uint64_t> mListOffset;

  data_t *getDataPtr(uint64_t no = 0) { return mData.begin() + no; }
  const data_t *getDataPtr(uint64_t no = 0) const { return mData.begin() + no; }
  uint64_t getCount() const
  {
    G_ASSERT(mData.size() <= eastl::numeric_limits<int32_t>::max());
    return mData.size();
  }

public:
  ListElementAddress<data_t> getElementAddress(uint64_t index)
  {
    if (!mListOffset)
      mListOffset = eastl::make_shared<uint64_t>();
    return {*this, index};
  }
  void resize(uint64_t count) { mData.resize(count); }
};

template <typename UserDataType, uint64_t alignment>
class ListMaster : public ListWrapper<UserDataType>
{
  static_assert(eastl::is_trivially_destructible<UserDataType>::value,
    "It looks like this type owns the resource, so we can't properly serialized it");

  void write() const
  {
    writer::context().preStreamData(*this);
    uint64_t offset = writer::writeList<UserDataType, alignment>(this->getDataPtr(), this->getCount());
    if (this->mListOffset)
      *this->mListOffset = offset;
    writer::context().postStreamData(*this);
  }

  void read()
  {
    reader::context().preStreamData(*this);
    auto offset_count = reader::readList(true);
    uint64_t offset = offset_count.first;
    this->resize(offset_count.second);
    reader::context().offset_to_lists[offset] = (uint8_t *)this->mData.data();
    reader::context().worker->read(offset, this->getDataPtr(), this->getCount() * sizeof(UserDataType));
    reader::context().postStreamData(*this);
  }

public:
  DECLARE_ASSIGNMENT_OPERATOR(ListMaster, ListWrapper<UserDataType>)
};

template <typename LayoutType, bool is_layout_list>
class LayoutsMaster : public ListWrapper<LayoutType>
{
  using LayoutMapper = traits::RetargetType<LayoutType, MAPPER>;

  void write() const
  {
    writer::context().preStreamData(*this);
    uint64_t offset = writer::template writeList<LayoutMapper, 1, is_layout_list>(nullptr, this->getCount());
    if (this->mListOffset)
      *this->mListOffset = offset;

    LayoutType layout;
    for (uint64_t i = 0; i < this->getCount(); i++)
    {
      writer::context().beginLayout<const LayoutType>(*this->getDataPtr(i), offset + i * sizeof(LayoutMapper));
      layout = *this->getDataPtr(i);
      writer::context().endLayout<const LayoutType>();
    }
    writer::context().postStreamData(*this);
  }

  void read()
  {
    reader::context().preStreamData(*this);
    auto offset_count = reader::readList(is_layout_list);
    uint64_t offset = offset_count.first;
    this->resize(offset_count.second);
    reader::context().offset_to_lists[offset] = (uint8_t *)this->mData.data();

    LayoutType layout{};
    for (uint64_t i = 0; i < this->getCount(); i++)
    {
      reader::context().beginLayout<LayoutType>(*this->getDataPtr(i), offset + i * sizeof(LayoutMapper));
      *this->getDataPtr(i) = layout;
      reader::context().endLayout<LayoutType>();
    }
    reader::context().postStreamData(*this);
  }

public:
  DECLARE_ASSIGNMENT_OPERATOR(LayoutsMaster, ListWrapper<LayoutType>)
};

template <typename LayoutType>
struct LayoutMaster
  // Protected inheritance is used here so that public methods from the `LayoutsMaster` class
  // do not become public for `LayoutMaster` and the `PtrMaster` classes,
  // because `resize()` and `getElementAddress()` doesn't make sense for these classes
  : protected LayoutsMaster<LayoutType, false>
{
  LayoutMaster() { this->mData.resize(1); }
};

template <typename LayoutType>
struct PtrMaster : protected LayoutsMaster<LayoutType, false>
{
  void create() { this->mData.resize(1); }
  explicit operator bool() const { return this->getCount() != 0; }
};

template <typename UserDataType>
class AddressMaster : public DeferredAction
{
  ListElementAddress<UserDataType> mAddress;
  mutable uint64_t mOffset = 0;
  UserDataType *mDataPtr = nullptr;

  void write() const
  {
    // At the moment, we may not know the exact offset for the List element on the mapper side,
    // because, for example, this List will be serialized after us.
    // Therefore, we will just reserve a place for offset,
    // and we will write the actual offset in the `apply()` method, which will be called at the end
    writer::context().preStreamData(*this);
    mOffset = writer::context().current_props.stream_offset;
    writer::writeOffset(mOffset);
    writer::context().deferred_actions.emplace_back(this);
    writer::context().postStreamData(*this);
  }

  void read()
  {
    reader::context().preStreamData(*this);
    mAddress = {};
    mDataPtr = nullptr;
    mOffset = reader::readOffset();
    if (mOffset)
      reader::context().deferred_actions.emplace_back(this);
    reader::context().postStreamData(*this);
  }

  void deferredWrite() const override
  {
    int32_t layout = 0;
    writer::context().beginLayout<int32_t>(layout, mOffset);
    writer::writeOffset(mAddress.getElementOffset());
    writer::context().endLayout<int32_t>();
  }

  void deferredRead() override
  {
    auto found = reader::context().offset_to_lists.upper_bound(mOffset);
    --found;
    uint64_t delta = mOffset - found->first;
    G_ASSERT(delta % traits::sizeOf<UserDataType>() == 0);
    uint64_t index = delta / traits::sizeOf<UserDataType>();
    mDataPtr = (UserDataType *)found->second + index;
  }

protected:
  using data_t = UserDataType;
  data_t *getDataPtr() { return mDataPtr ? mDataPtr : mAddress.getDataPtr(); }
  const data_t *getDataPtr() const { return mDataPtr ? mDataPtr : mAddress.getDataPtr(); }

public:
  DECLARE_ASSIGNMENT_OPERATOR_BASE(AddressMaster, mAddress = other.mAddress)

  AddressMaster &operator=(const ListElementAddress<data_t> &address)
  {
    mAddress = address;
    return *this;
  }
  AddressMaster &operator=(std::nullptr_t)
  {
    mAddress = {};
    return *this;
  }
  explicit operator bool() const { return getDataPtr() != nullptr; }
};

class CurrentSizeMaster
{
  void write() const { size = writer::append(0); }

  void read() {}

public:
  DECLARE_ASSIGNMENT_OPERATOR_BASE(CurrentSizeMaster, size = other.size)
  mutable uint64_t size;
};

#undef DECLARE_ASSIGNMENT_OPERATOR
#undef DECLARE_ASSIGNMENT_OPERATOR_BASE
} // namespace master

namespace mapper
{
template <typename Type>
class PtrMapper
{
  int32_t mOffset;

protected:
  using data_t = Type;

  data_t *patchPtr(int32_t offset, uint64_t no) const
  {
    if (!offset)
      return nullptr;
    return (data_t *)((uint8_t *)this + offset) + no;
  }

  data_t *getDataPtr(uint64_t no = 0) { return patchPtr(mOffset, no); }
  const data_t *getDataPtr(uint64_t no = 0) const { return patchPtr(mOffset, no); }

public:
  explicit operator bool() const { return mOffset != 0; }
};

template <typename Type>
class ListMapper : protected PtrMapper<Type>
{
protected:
  uint32_t mCount;
  uint64_t getCount() const { return mCount; }
};
} // namespace mapper

namespace streamer
{
template <typename StreamerType, typename WrapperAddressType>
struct Context : master::Context<StreamerType, WrapperAddressType>
{
  eastl::map<const uint8_t *, uint64_t> address_to_offset;
  eastl::map<uint64_t, uint8_t *> offset_to_address;
  uint64_t mOffset = 0;

  template <typename LayoutType>
  void endLayout()
  {
    uint64_t begin_layout_address = (uint64_t)this->props_stack.top().wrapper_address;
    uint64_t end_layout_address = (uint64_t)this->current_props.wrapper_address;
    this->template endLayoutImpl<LayoutType>(begin_layout_address, end_layout_address);
  }

  template <typename LayoutType>
  void begin(const LayoutType &layout, StreamerType &streamer)
  {
    G_ASSERT(address_to_offset.empty() && offset_to_address.empty() && mOffset == 0);
    master::Context<StreamerType, WrapperAddressType>::begin(streamer);
    this->beginLayout(layout, 0);
  }

  template <typename LayoutType>
  void end()
  {
    endLayout<LayoutType>();
    master::Context<StreamerType, WrapperAddressType>::end();
    close();
  }

  void close()
  {
    master::Context<StreamerType, WrapperAddressType>::close();
    address_to_offset.clear();
    offset_to_address.clear();
    mOffset = 0;
  }

  template <typename Type, typename KeyType, typename ValueType>
  ValueType remap(const eastl::map<KeyType, ValueType> &map, KeyType key)
  {
    auto found = map.upper_bound(key);
    --found;
    G_ASSERT(key >= found->first);
    uint64_t delta = key - found->first;
    G_ASSERT(delta % sizeof(Type) == 0);
    return found->second + delta;
  }
};

namespace writer
{
struct WriterContext : Context<IWriter, const uint8_t *>
{
  void stream(const uint8_t *address, uint64_t size) override
  {
    uint64_t offset = worker->append(size);
    worker->write(offset, address, size);
  }

  void deferredAction(const master::DeferredAction *action) override { action->deferredWrite(); }

  template <typename Type>
  void addAddressTo(const Type *element)
  {
    this->address_to_offset[(const uint8_t *)element] = worker->append(0);
  }

  template <typename Type>
  uint64_t translateAddressToOffset(const Type *element)
  {
    return this->remap<Type>(this->address_to_offset, (const uint8_t *)element);
  }
};

inline WriterContext &context()
{
  thread_local WriterContext ctx;
  return ctx;
}
} // namespace writer

namespace reader
{
struct ReaderContext : Context<IReader, uint8_t *>
{
  void stream(uint8_t *address, uint64_t size) override
  {
    worker->read(mOffset, address, size);
    mOffset += size;
  }

  void deferredAction(master::DeferredAction *action) override { action->deferredRead(); }

  template <typename Type>
  void addOffsetTo(Type *element)
  {
    this->offset_to_address[mOffset] = (uint8_t *)element;
  }

  template <typename Type>
  Type *translateOffsetToAddress(uint64_t offset)
  {
    return (Type *)this->remap<Type>(this->offset_to_address, offset);
  }
};

inline ReaderContext &context()
{
  thread_local ReaderContext ctx;
  return ctx;
}
} // namespace reader

struct BeginNonSerializable
{
  BeginNonSerializable &operator=(const BeginNonSerializable &other)
  {
    if (hasher::context().worker)
      hasher::context().preStreamData(other);
    else if (writer::context().worker)
      writer::context().preStreamData(other);
    else if (reader::context().worker)
      reader::context().preStreamData(*this);
    return *this;
  }
};

struct EndNonSerializable
{
  // This is added so that there are no extra paddings after this type
  uint64_t __dummy = 0;
  EndNonSerializable &operator=(const EndNonSerializable &other)
  {
    if (hasher::context().worker)
      hasher::context().postStreamData(other);
    else if (writer::context().worker)
      writer::context().postStreamData(other);
    else if (reader::context().worker)
      reader::context().postStreamData(*this);
    return *this;
  }
};

template <typename Type>
void write(const Type *data, uint32_t count, bool need_to_add_address)
{
  if (eastl::is_trivially_copy_assignable_v<Type>)
  {
    if (need_to_add_address)
      writer::context().addAddressTo(data);
    writer::context().stream((const uint8_t *)data, count * sizeof(Type));
    return;
  }
  Type layout;
  for (uint32_t i = 0; i < count; i++)
  {
    writer::context().beginLayout<const Type>(data[i], 0);
    writer::context().addAddressTo(&data[i]);
    layout = data[i];
    writer::context().endLayout<const Type>();
  }
}

template <typename Type>
void read(Type *data, uint32_t count, bool need_to_add_offset)
{
  if (eastl::is_trivially_copy_assignable_v<Type>)
  {
    if (need_to_add_offset)
      reader::context().addOffsetTo(data);
    reader::context().stream((uint8_t *)data, count * sizeof(Type));
    return;
  }
  for (uint32_t i = 0; i < count; i++)
  {
    reader::context().disable();
    Type layout = data[i];
    reader::context().enable();
    reader::context().beginLayout<Type>(data[i], 0);
    reader::context().addOffsetTo(&data[i]);
    data[i] = layout;
    reader::context().endLayout<Type>();
  }
}
} // namespace streamer

template <typename WrapperType, typename UserDataType, int wrap>
class Wrapper;

template <typename ListType, typename UserDataType, int wrap>
class List;

namespace traits
{
template <typename UserDataType>
struct EmptyWrapper : public UserDataType
{
  using data_t = UserDataType;
  data_t *getDataPtr() { return nullptr; }
  const data_t *getDataPtr() const { return nullptr; }
};

template <typename UserDataType, enum detail::Target>
struct WrapHolder;

template <typename UserDataType>
struct WrapHolder<UserDataType, MASTER>
{
  using Hash = master::HashWrapper<UserDataType>;
  using Address = master::AddressMaster<UserDataType>;
  template <uint64_t alignment>
  using List = master::ListMaster<UserDataType, alignment>;
  using LayoutList = master::LayoutsMaster<UserDataType, true>;
  using Layout = master::LayoutMaster<UserDataType>;
  using Ptr = master::PtrMaster<UserDataType>;
  using CurrentSize = master::CurrentSizeMaster;
};

template <typename UserDataType>
struct WrapHolder<UserDataType, MAPPER>
{
  using Hash = HashVal<64>;
  using Address = mapper::PtrMapper<UserDataType>;
  template <uint64_t>
  using List = mapper::ListMapper<UserDataType>;
  using LayoutList = List<0>;
  using Layout = Address;
  using Ptr = Address;
  using CurrentSize = uint64_t;
};

template <typename UserDataType>
struct WrapHolder<UserDataType, HASHER>
{
  using Hash = EmptyWrapper<hasher::DataHasher<HashVal<64>, 29>>;
  using Address = EmptyWrapper<hasher::DataHasher<uint64_t, 16777619>>;
  template <uint64_t>
  using List = EmptyWrapper<hasher::DataHasher<UserDataType, 7757>>;
  using LayoutList = EmptyWrapper<hasher::DataHasher<UserDataType, 331>>;
  using Layout = EmptyWrapper<hasher::DataHasher<UserDataType, 97>>;
  using Ptr = EmptyWrapper<hasher::DataHasher<UserDataType, 17>>;
  using CurrentSize = EmptyWrapper<hasher::DataHasher<uint64_t, 131>>;
};

enum WrapType
{
  AddressType,
  ListType,
  LayoutListType,
  LayoutType,
  PtrType
};

template <typename UserDataType, enum detail::Target, int>
struct WrapTypeGetter {};

#define DECLARE_WRAP_TYPE_GETTER(WrapName)                            \
  template <typename UserDataType, enum detail::Target target>        \
  struct WrapTypeGetter<UserDataType, target, WrapName##Type>         \
  {                                                                   \
    using Wrap = typename WrapHolder<UserDataType, target>::WrapName; \
  }

DECLARE_WRAP_TYPE_GETTER(Address);
DECLARE_WRAP_TYPE_GETTER(List);
DECLARE_WRAP_TYPE_GETTER(LayoutList);
DECLARE_WRAP_TYPE_GETTER(Layout);
DECLARE_WRAP_TYPE_GETTER(Ptr);
#undef DECLARE_WRAP_TYPE_GETTER

template <typename Type, uint64_t alignment, enum detail::Target target>
constexpr auto getList(...) -> detail::List<typename WrapHolder<Type, target>::template List<alignment>, Type, ListType>;

template <typename Type, uint64_t, enum detail::Target target, typename dest = typename Type::template MyLayout<target>>
constexpr auto getList(const Type &) -> detail::List<typename WrapHolder<dest, target>::LayoutList, Type, LayoutType>;

template <typename Type, enum detail::Target target>
using Address = detail::Wrapper<typename WrapHolder<Type, target>::Address, Type, AddressType>;

template <typename Type, uint64_t alignment, enum detail::Target target>
using List = decltype(getList<Type, alignment, target>(eastl::declval<Type>()));

template <typename Type, enum detail::Target target>
using LayoutList = detail::List<typename WrapHolder<Type, target>::LayoutList, Type, LayoutType>;

template <typename Type, enum detail::Target target>
using Layout = detail::Wrapper<typename WrapHolder<Type, target>::Layout, Type, LayoutType>;

template <typename Type, enum detail::Target target>
using Ptr = detail::Wrapper<typename WrapHolder<Type, target>::Ptr, Type, PtrType>;
} // namespace traits

template <typename WrapperType, typename UserDataType = void, int wrap = 0>
class Wrapper : public WrapperType
{
public:
  template <enum detail::Target t>
  using MyLayout = Wrapper<typename traits::WrapTypeGetter<UserDataType, t, wrap>::Wrap, UserDataType, wrap>;

  using WrapperType::operator=;
  typename WrapperType::data_t *operator->() { return this->getDataPtr(); }
  const typename WrapperType::data_t *operator->() const { return this->getDataPtr(); }
  typename WrapperType::data_t &operator*() { return *this->getDataPtr(); }
  const typename WrapperType::data_t &operator*() const { return *this->getDataPtr(); }
  typename WrapperType::data_t &get() { return *this->getDataPtr(); }
  const typename WrapperType::data_t &get() const { return *this->getDataPtr(); }
};

template <typename ListType, typename UserDataType, int wrap>
class List : public ListType
{
public:
  template <enum detail::Target t>
  using MyLayout = List<typename traits::WrapTypeGetter<UserDataType, t, wrap>::Wrap, UserDataType, wrap>;

  List &operator=(const List &) = default;
  typename ListType::data_t &operator[](uint64_t no)
  {
    G_ASSERTF(no < size(), "index is %i, size() is %i", no, size());
    return *this->getDataPtr(no);
  }
  const typename ListType::data_t &operator[](uint64_t no) const
  {
    G_ASSERTF(no < size(), "index is %i, size() is %i", no, size());
    return *this->getDataPtr(no);
  }
  uint64_t size() const { return this->getCount(); }
};

#pragma pack(push, 1)
template <template <enum detail::Target> class LayoutClass, enum detail::Target target>
struct EnableHash : public LayoutClass<target>
{
  template <enum detail::Target t>
  using MyLayout = EnableHash<LayoutClass, t>;

  HashVal<64> getLayoutHash() const { return mHash; }

private:
  typename traits::WrapHolder<MyLayout<target>, target>::Hash mHash;
};
#pragma pack(pop)

template <typename LayoutType>
const LayoutType *map_base(const uint8_t *dump)
{
  static_assert(eastl::is_trivially_destructible<LayoutType>::value,
    "It looks like this type owns the resource, so we can't properly serialized it");

  return (const LayoutType *)dump;
}

template <typename LayoutType>
const LayoutType *map(const uint8_t *dump, const LayoutType *)
{
  return map_base<LayoutType>(dump);
}

template <template <enum detail::Target> class LayoutClass>
const EnableHash<LayoutClass, MAPPER> *map(const uint8_t *dump, const EnableHash<LayoutClass, MAPPER> *)
{
  auto mapped_data = map_base<EnableHash<LayoutClass, MAPPER>>(dump);
  using HashableType = EnableHash<LayoutClass, HASHER>;
  if (mapped_data->getLayoutHash() != getHash<HashableType>())
  {
    logwarn_ctx("The layout format does not match the dump format");
    return nullptr;
  }
  return mapped_data;
}

template <typename LayoutType>
const LayoutType *map(const uint8_t *dump)
{
  const LayoutType *null = nullptr;
  return map(dump, null);
}

template <typename LayoutType>
LayoutType *map(uint8_t *dump)
{
  return const_cast<LayoutType *>(map<LayoutType>(static_cast<const uint8_t *>(dump)));
}

template <typename LayoutType>
void streamWriteLayout(const LayoutType &layout, IWriter &writer)
{
  master::writer::begin<LayoutType>(layout, writer);
  LayoutType other;
  other = layout;
  master::writer::end<LayoutType>();
}

template <typename LayoutType>
bool streamReadLayout(LayoutType &layout, IReader &reader)
{
  master::reader::begin<LayoutType>(layout, reader);
  LayoutType other{};
  layout = other;
  master::reader::end<LayoutType>();

  using HashableType = traits::RetargetType<LayoutType, HASHER>;
  bool is_layout_valid = layout.getLayoutHash() == getHash<HashableType>();
  G_ASSERTF(is_layout_valid, "The layout format does not match the dump format");
  return is_layout_valid;
}
} // namespace detail

namespace stream
{
template <typename Type>
void write(const Type *data, uint32_t count)
{
  detail::streamer::write(data, count, true);
}
template <typename Type>
void read(Type *data, uint32_t count)
{
  detail::streamer::read(data, count, true);
}
template <typename Type>
void write(const Type &data)
{
  detail::streamer::write(&data, 1, true);
}
template <typename Type>
void read(Type &data)
{
  detail::streamer::read(&data, 1, true);
}
template <typename Type>
void writeAux(const Type &data)
{
  detail::streamer::write(&data, 1, false);
}
template <typename Type>
void readAux(Type &data)
{
  detail::streamer::read(&data, 1, false);
}

static void hash(uint32_t value) { detail::hasher::context().worker->step(value); }
template <typename Type>
void hash()
{
  detail::hasher::hash<Type>();
}
} // namespace stream

template <template <enum detail::Target> class LayoutClass>
using Master = detail::EnableHash<LayoutClass, detail::MASTER>;

template <template <enum detail::Target> class LayoutClass>
using Mapper = LayoutClass<detail::MAPPER>;

template <template <enum detail::Target> class LayoutClass>
const Mapper<LayoutClass> *map(const uint8_t *dump)
{
  return detail::map<LayoutClass>(dump, nullptr);
}

template <template <enum detail::Target> class LayoutClass>
Mapper<LayoutClass> *map(uint8_t *dump)
{
  return const_cast<Mapper<LayoutClass> *>(map<LayoutClass>(static_cast<const uint8_t *>(dump)));
}

template <typename LayoutType>
void streamWrite(const LayoutType &layout, IWriter &writer)
{
  detail::streamer::writer::context().begin<LayoutType>(layout, writer);
  LayoutType other;
  other = layout;
  detail::streamer::writer::context().end<LayoutType>();
}

template <typename LayoutType>
void streamRead(LayoutType &layout, IReader &reader)
{
  LayoutType other = layout;
  detail::streamer::reader::context().begin<LayoutType>(layout, reader);
  layout = other;
  detail::streamer::reader::context().end<LayoutType>();
}

inline void close()
{
  detail::streamer::reader::context().close();
  detail::streamer::writer::context().close();
}

template <template <enum detail::Target> class LayoutClass>
void streamWrite(const Master<LayoutClass> &layout, IWriter &writer)
{
  detail::streamWriteLayout(layout, writer);
}

template <template <enum detail::Target> class LayoutClass>
bool streamRead(Master<LayoutClass> &layout, IReader &reader)
{
  return detail::streamReadLayout(layout, reader);
}
} // namespace bindump

#pragma warning(pop)

#ifdef _MSC_VER
#define BINDUMP_PRAGMA_PACK_PUSH __pragma(pack(push, 1))
#define BINDUMP_PRAGMA_PACK_POP  __pragma(pack(pop))
#else
#define BINDUMP_PRAGMA_PACK_PUSH _Pragma("pack(push, 1)")
#define BINDUMP_PRAGMA_PACK_POP  _Pragma("pack(pop)")
#endif

#define BINDUMP_FORWARD_LAYOUT(LayoutName)       \
  template <enum bindump::detail::Target target> \
  struct LayoutName

#define BINDUMP_BEGIN_LAYOUT_BASE(LayoutName, ...)                                                \
  BINDUMP_PRAGMA_PACK_PUSH                                                                        \
  template <enum bindump::detail::Target target = bindump::detail::MASTER>                        \
  struct LayoutName __VA_ARGS__                                                                   \
  {                                                                                               \
    template <enum bindump::detail::Target t>                                                     \
    using MyLayout = LayoutName<t>;                                                               \
                                                                                                  \
  private:                                                                                        \
    template <template <enum bindump::detail::Target> class LayoutClass>                          \
    using Field = LayoutClass<target>;                                                            \
                                                                                                  \
    template <typename UserDataType>                                                              \
    using List = typename bindump::detail::traits::List<UserDataType, 1, target>;                 \
                                                                                                  \
    template <template <enum bindump::detail::Target> class LayoutClass>                          \
    using LayoutList = typename bindump::detail::traits::LayoutList<LayoutClass<target>, target>; \
                                                                                                  \
    template <typename UserDataType>                                                              \
    using Address = typename bindump::detail::traits::Address<UserDataType, target>;              \
                                                                                                  \
    template <template <enum bindump::detail::Target> class LayoutClass>                          \
    using Ptr = typename bindump::detail::traits::Ptr<LayoutClass<target>, target>;               \
                                                                                                  \
    template <template <enum bindump::detail::Target> class LayoutClass>                          \
    using Layout = typename bindump::detail::traits::Layout<LayoutClass<target>, target>;         \
                                                                                                  \
    template <template <enum bindump::detail::Target> class LayoutClass>                          \
    using LayoutAddress = typename bindump::detail::traits::Address<LayoutClass<target>, target>; \
                                                                                                  \
    using CurrentSize = typename bindump::detail::traits::WrapHolder<void, target>::CurrentSize;  \
                                                                                                  \
  public:
#define BINDUMP_END_LAYOUT() \
  }                          \
  ;                          \
  BINDUMP_PRAGMA_PACK_POP

#define BINDUMP_BEGIN_LAYOUT(LayoutName) BINDUMP_BEGIN_LAYOUT_BASE(LayoutName, )
#define BINDUMP_BEGIN_EXTEND_LAYOUT(LayoutName, Base) \
  BINDUMP_BEGIN_LAYOUT_BASE(LayoutName, : bindump::detail::EnableHash<Base, target>)

#define BINDUMP_BEGIN_LIST(Name)              \
  struct __##Name;                            \
  List<__##Name> Name;                        \
  struct __##Name                             \
  {                                           \
    template <enum bindump::detail::Target t> \
    using MyLayout = typename MyLayout<t>::__##Name;
#define BINDUMP_END_LIST() \
  }                        \
  ;

#define BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(Class, AssignCode) \
  Class &operator=(const Class &other)                         \
  {                                                            \
    namespace streamer = bindump::detail::streamer;            \
    namespace hasher = bindump::detail::hasher;                \
    if (hasher::context().isEnabled())                         \
    {                                                          \
      hasher::context().preStreamData(other);                  \
      other.hashialize();                                      \
      hasher::context().postStreamData(other);                 \
    }                                                          \
    else if (streamer::writer::context().isEnabled())          \
    {                                                          \
      streamer::writer::context().preStreamData(other);        \
      other.serialize();                                       \
      streamer::writer::context().postStreamData(other);       \
    }                                                          \
    else if (streamer::reader::context().isEnabled())          \
    {                                                          \
      streamer::reader::context().preStreamData(*this);        \
      deserialize();                                           \
      streamer::reader::context().postStreamData(*this);       \
    }                                                          \
    else                                                       \
      AssignCode;                                              \
    return *this;                                              \
  }

#define BINDUMP_ENABLE_STREAMING(Class, BaseClass, BaseCtor) \
  using BaseClass::BaseCtor;                                 \
  Class() = default;                                         \
  Class(const BaseClass &other) : BaseClass(other)           \
  {}                                                         \
  Class(BaseClass &&other) : BaseClass(eastl::move(other))   \
  {}                                                         \
  Class &operator=(const BaseClass &other)                   \
  {                                                          \
    BaseClass::operator=(other);                             \
    return *this;                                            \
  }                                                          \
  Class &operator=(BaseClass &&other) noexcept               \
  {                                                          \
    BaseClass::operator=(eastl::move(other));                \
    return *this;                                            \
  }                                                          \
  BINDUMP_DECLARE_ASSIGNMENT_OPERATOR(Class, BaseClass::operator=(other))

#define BINDUMP_CONCAT_HELPER(x, y) x##y
#define BINDUMP_CONCAT(x, y)        BINDUMP_CONCAT_HELPER(x, y)

#define BINDUMP_BEGIN_NON_SERIALIZABLE() \
  bindump::detail::streamer::BeginNonSerializable BINDUMP_CONCAT(__begin_non_serializable, __LINE__)
#define BINDUMP_END_NON_SERIALIZABLE() bindump::detail::streamer::EndNonSerializable BINDUMP_CONCAT(__end_non_serializable, __LINE__)
#define BINDUMP_NON_SERIALIZABLE(var_list) \
  BINDUMP_BEGIN_NON_SERIALIZABLE();        \
  var_list;                                \
  BINDUMP_END_NON_SERIALIZABLE()
