//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <spirv.hpp11>

namespace spirv
{
using namespace spv;
inline Id string_length(const char *a, Id max_c)
{
  for (Id i = 0; i < max_c; ++i)
    if ('\0' == a[i])
      return i;
  return max_c;
}
inline bool string_equals(const char *a, const char *b, Id max_c)
{
  for (Id i = 0; i < max_c; ++i)
    if (a[i] != b[i])
      return false;
  return true;
}

struct FileHeader
{
  Id magic;
  Id version;
  Id toolId;
  Id idBounds;
  Id reserved;
};

template <typename T>
struct TypeTraits;

namespace detail
{
template <typename T, typename TAG>
struct TaggedType
{
  T value;
  TaggedType() = default;
  ~TaggedType() = default;
  TaggedType(const TaggedType &) = default;
  TaggedType &operator=(const TaggedType &) = default;
  TaggedType(TaggedType &&) = default;
  TaggedType &operator=(TaggedType &&) = default;
  TaggedType(const T &v) : value{v} {};

  friend inline bool operator==(TaggedType l, TaggedType r) { return r.value == l.value; }
  friend inline bool operator!=(TaggedType l, TaggedType r) { return !(r == l); }
};
template <typename A, typename B>
struct CompositeType
{
  A first;
  B second;
  CompositeType() = default;
  ~CompositeType() = default;
  CompositeType(const CompositeType &) = default;
  CompositeType &operator=(const CompositeType &) = default;
  CompositeType(CompositeType &&) = default;
  CompositeType &operator=(CompositeType &&) = default;
  CompositeType(const A &a, const B &b) : first{a}, second{b} {}
};
struct Operands
{
  const Id *from;
  const Id *to;
  Id literalContextSize;
  bool error = false;
  Id errorOnParam = 0;
  Operands() = default;
  ~Operands() = default;
  Operands(const Operands &) = default;
  Operands &operator=(const Operands &) = default;
  Operands(const Id *w, Id c) : from{w}, to{w + c} {}
  template <typename T>
  auto read()
  {
    decltype(TypeTraits<T>::read(from, to, literalContextSize, error)) r;
    if (!error)
    {
      r = TypeTraits<T>::read(from, to, literalContextSize, error);
      ++errorOnParam;
    }
    return r;
  }
  // called after all operands have been read, if any are missing ore if some are left over, we
  // found an error and return false and report it with on error
  template <typename ECB>
  bool finishOperands(ECB on_error, const char *op_name)
  {
    if (error)
      return !on_error(op_name, "Error while reading parameters");
    else if (from != to)
      return !on_error(op_name, "Unexpected left over words on operation");
    return true;
  }
  void setLiteralContextSize(Id size) { literalContextSize = size; }
  Id count() const { return to - from; }
};
template <typename T>
struct BasicIdTypeTraits
{
  typedef T ReadType;
  static ReadType read(const Id *&from, const Id *to, Id cds, bool &error)
  {
    if (to - from < 1)
    {
      error = true;
      return {};
    }
    auto value = *from++;
    return {value};
  }
};
} // namespace detail
struct LiteralInteger
{
  Id value;
  LiteralInteger() = default;
  ~LiteralInteger() = default;
  LiteralInteger(const LiteralInteger &) = default;
  LiteralInteger &operator=(const LiteralInteger &) = default;
  LiteralInteger(LiteralInteger &&) = default;
  LiteralInteger &operator=(LiteralInteger &&) = default;
  LiteralInteger(const Id &v) : value{v} {}
};
// Size usually depends on the type that is associated with this value
// in most cases this is just one word for 32 or less bits constant values
// currently this can not be larger than 64 bits
struct LiteralContextDependentNumber
{
  // make it possible to store 64 bits of data in any case
  unsigned long long value;
  LiteralContextDependentNumber() = default;
  ~LiteralContextDependentNumber() = default;
  LiteralContextDependentNumber(const LiteralContextDependentNumber &) = default;
  LiteralContextDependentNumber &operator=(const LiteralContextDependentNumber &) = default;
  LiteralContextDependentNumber(LiteralContextDependentNumber &&) = default;
  LiteralContextDependentNumber &operator=(LiteralContextDependentNumber &&) = default;
  LiteralContextDependentNumber(unsigned long long v) : value{v} {}
};
// utf8 string, just computing the length on assignment for convenience
struct LiteralString
{
  const Id *value;
  Id length;
  LiteralString() = default;
  ~LiteralString() = default;
  LiteralString(const LiteralString &) = default;
  LiteralString &operator=(const LiteralString &) = default;
  LiteralString(LiteralString &&) = default;
  LiteralString &operator=(LiteralString &&) = default;
  LiteralString(Id max_cnt, const Id &v) : value{&v}, length{string_length(reinterpret_cast<const char *>(&v), max_cnt * sizeof(Id))}
  {}
  LiteralString(Id max_cnt, const Id *v) : value{v}, length{string_length(reinterpret_cast<const char *>(v), max_cnt * sizeof(Id))} {}
  Id words() const { return (length + 1 + sizeof(Id) - 1) / sizeof(Id); }
  const char *asString() const { return reinterpret_cast<const char *>(value); }

  eastl::string asStringObj() const { return {asString(), length}; }
};
// stores op code for OpExtInst, can never be larger than 32 bits
struct LiteralExtInstInteger
{
  Id value;
  LiteralExtInstInteger() = default;
  ~LiteralExtInstInteger() = default;
  LiteralExtInstInteger(const LiteralExtInstInteger &) = default;
  LiteralExtInstInteger &operator=(const LiteralExtInstInteger &) = default;
  LiteralExtInstInteger(LiteralExtInstInteger &&) = default;
  LiteralExtInstInteger &operator=(LiteralExtInstInteger &&) = default;
  LiteralExtInstInteger(Id v) : value{v} {}
};
// stores an opcode for OpSpecConstantOp, can never be larger than 16 bits
struct LiteralSpecConstantOpInteger
{
  Id value;
  LiteralSpecConstantOpInteger() = default;
  ~LiteralSpecConstantOpInteger() = default;
  LiteralSpecConstantOpInteger(const LiteralSpecConstantOpInteger &) = default;
  LiteralSpecConstantOpInteger &operator=(const LiteralSpecConstantOpInteger &) = default;
  LiteralSpecConstantOpInteger(LiteralSpecConstantOpInteger &&) = default;
  LiteralSpecConstantOpInteger &operator=(LiteralSpecConstantOpInteger &&) = default;
  LiteralSpecConstantOpInteger(Id v) : value{v} {}
};
template <typename T>
struct Optional
{
  T value;
  bool valid = false;
  Optional() = default;
  ~Optional() = default;
  Optional(const Optional &) = default;
  Optional &operator=(const Optional &) = default;
  Optional(Optional &&) = default;
  Optional &operator=(Optional &&) = default;
  Optional(T v) : value{v}, valid{true} {}
  const T &get_or(const T &dfault)
  {
    if (value)
      return value;
    return dfault;
  }
};

template <typename T>
struct Multiple
{
  const Id *from;
  const Id *to;
  Id count = 0;
  Id cds = 0;
  Multiple() = default;
  ~Multiple() = default;
  Multiple(const Multiple &) = default;
  Multiple &operator=(const Multiple &) = default;
  Multiple(Multiple &&) = default;
  Multiple &operator=(Multiple &&) = default;
  // init with count 0 to allow unconditional create even if no space is left for any element
  Multiple(const Id &v) : from{&v}, to{&v} {}
  Multiple(const Id *v) : from{v}, to{v} {}
  bool empty() const { return count == 0; }
  // awkward but necessary to avoid allocating memory
  T consume()
  {
    if (empty())
      return {};

    bool error = false;
    T r = TypeTraits<T>::read(from, to, cds, error);
    --count;
    return r;
  }
};

template <typename T>
struct TypeTraits<Multiple<T>>
{
  static auto read(const Id *&from, const Id *to, Id cds, bool &error) -> Multiple<decltype(TypeTraits<T>::read(from, to, cds, error))>
  {
    // starts from 0
    Multiple<decltype(TypeTraits<T>::read(from, to, cds, error))> result{from};
    result.cds = cds;
    bool ec = false;
    for (;;)
    {
      // dummy read until we can no longer read any more
      TypeTraits<T>::read(from, to, cds, ec);
      if (ec)
        break;
      result.count++;
    }
    result.to = from;
    return result;
  }
};

template <typename T>
struct TypeTraits<Optional<T>>
{
  static auto read(const Id *&from, const Id *to, Id cds, bool &error) -> Optional<decltype(TypeTraits<T>::read(from, to, cds, error))>
  {
    // simply try to read from stream, if fail, just create empty object
    bool ec = false;
    auto result = TypeTraits<T>::read(from, to, cds, ec);
    if (!ec)
      return {result};
    return {};
  }
};

template <typename T>
class CastableUniquePointer
{
  // because of eastl unique_ptr misbehaving until v 3.13
  // this wrapper is needed, as reset()/destructor of
  // empty unique_ptr objects will always call the deleter
  // which will crash if not checked by a wrapper type
  struct CallWrapper
  {
    void (*clb)(T *) = nullptr;
    void operator()(T *v)
    {
      // crash if clb is null, then we would hit a mem leak
      if (v)
        clb(v);
    }

    CallWrapper() = default;
    ~CallWrapper() = default;

    CallWrapper(const CallWrapper &) = default;
    CallWrapper &operator=(const CallWrapper &) = default;

    CallWrapper(void (*c)(T *)) : clb{c} {}

    template <typename U>
    CallWrapper(U u) : clb(u)
    {}

    CallWrapper &operator=(void (*c)(T *))
    {
      clb = c;
      return *this;
    }
  };
  typedef eastl::unique_ptr<T, CallWrapper> PtrType;
  PtrType mem;

public:
  CastableUniquePointer() = default;
  ~CastableUniquePointer() = default;

  CastableUniquePointer(const CastableUniquePointer &) = default;
  CastableUniquePointer &operator=(const CastableUniquePointer &) = default;

  CastableUniquePointer(CastableUniquePointer &&) = default;
  CastableUniquePointer &operator=(CastableUniquePointer &&) = default;

  explicit CastableUniquePointer(T *v) : mem{v, [](T *v) { delete v; }} {}

  template <typename U>
  explicit CastableUniquePointer(U *v)
  {
    *this = CastableUniquePointer<U>(v);
  }

  CastableUniquePointer &operator=(T *v)
  {
    mem = PtrType{v, [](T *v) { delete v; }};
    return *this;
  }

  template <typename U>
  CastableUniquePointer &operator=(U *v)
  {
    *this = CastableUniquePointer<U>(v);
    return *this;
  }

  template <typename U>
  CastableUniquePointer(CastableUniquePointer<U> &&other)
  {
    mem = PtrType{reinterpret_cast<T *>(other.release()), [](T *v) { delete reinterpret_cast<U *>(v); }};
  }

  template <typename U>
  CastableUniquePointer &operator=(CastableUniquePointer<U> &&other)
  {
    mem = PtrType{reinterpret_cast<T *>(other.release()), [](T *v) { delete reinterpret_cast<U *>(v); }};
    return *this;
  }

  T *release() { return mem.release(); }

  T *get() { return mem.get(); }
  const T *get() const { return mem.get(); }

  T *operator->() { return mem.get(); }
  const T *operator->() const { return mem.get(); }

  explicit operator bool() const { return !!mem; }
  bool operator!() const { return !mem; }

  void clear() { mem.reset(); }
  void reset(T *value)
  {
    mem = PtrType{value, [](T *v) { delete v; }};
  }
  template <typename U>
  void reset(U *value)
  {
    *this = CastableUniquePointer<U>(value);
  }
  void reset() { mem.reset(); }

  friend bool operator==(const CastableUniquePointer &l, const CastableUniquePointer &r) { return l.get() == r.get(); }
  friend bool operator!=(const CastableUniquePointer &l, const CastableUniquePointer &r) { return l.get() != r.get(); }
};

template <typename T, typename U>
inline T *as(CastableUniquePointer<U> &v)
{
  return reinterpret_cast<T *>(v.get());
}
template <typename T, typename U>
inline const T *as(const CastableUniquePointer<U> &v)
{
  return reinterpret_cast<const T *>(v.get());
}

template <typename T, typename U>
inline bool is(const CastableUniquePointer<U> &v)
{
  return T::is(v.get());
}

// Simple wrapper that can automatically handle type casting in any direction
// It uses the opcodes as a guide to validate valid cast target types
template <typename T>
class NodePointer
{
  T *mem = nullptr;

public:
  NodePointer() = default;
  ~NodePointer() = default;

  NodePointer(const NodePointer &) = default;
  NodePointer &operator=(const NodePointer &) = default;

  explicit NodePointer(T *t) : mem{t} {}

  template <typename U>
  explicit NodePointer(U *u)
  {
    if (u && T::is(u))
      mem = reinterpret_cast<T *>(u);
  }

  template <typename U>
  NodePointer(const NodePointer<U> &other)
  {
    if (T::is(other.get()))
      mem = const_cast<T *>(reinterpret_cast<const T *>(other.get()));
  }

  template <typename U>
  NodePointer &operator=(const NodePointer<U> &other)
  {
    if (other.get() && T::is(other.get()))
      mem = const_cast<T *>(reinterpret_cast<const T *>(other.get()));
    else
      mem = nullptr;
    return *this;
  }

  NodePointer &operator=(T *m)
  {
    mem = m;
    return *this;
  }

  template <typename U>
  NodePointer &operator=(U *m)
  {
    if (m && T::is(m))
      mem = reinterpret_cast<T *>(m);
    else
      mem = nullptr;
    return *this;
  }

  T *get() { return mem; }
  const T *get() const { return mem; }

  explicit operator bool() const { return mem != nullptr; }
  bool isValid() const { return mem != nullptr; }

  auto opCode() const { return mem->opCode; }
  auto extOpCode() const { return mem->extOpCode; }
  auto grammarId() const { return mem->grammarId; }

  T *operator->() { return mem; }
  const T *operator->() const { return mem; }

  void clear() { mem = nullptr; }
  void reset() { mem = nullptr; }
};

template <typename T, typename U>
inline NodePointer<T> as(NodePointer<U> v)
{
  return NodePointer<T>(reinterpret_cast<T *>(v.get()));
}

template <typename T, typename U>
inline bool is(const NodePointer<U> &v)
{
  return v.get() && T::is(v.get());
}

template <typename A, typename B>
inline bool operator==(const NodePointer<A> &l, const NodePointer<B> &r)
{
  return l.get() == as<A>(r).get();
}
template <typename A, typename B>
inline bool operator!=(const NodePointer<A> &l, const NodePointer<B> &r)
{
  return l.get() != as<A>(r).get();
}

template <typename T>
inline NodePointer<T> make_node_pointer(T *n)
{
  return NodePointer<T>{n};
}

// a unique value set, implemented on top of vector with lower_bound and binary_search
template <typename T>
class FlatSet
{
  eastl::vector<T> data;

public:
  FlatSet() = default;
  ~FlatSet() = default;

  FlatSet(const FlatSet &) = default;
  FlatSet &operator=(const FlatSet &) = default;

  FlatSet(FlatSet &&) = default;
  FlatSet &operator=(FlatSet &&) = default;

  void insert(const T &value)
  {
    auto at = eastl::lower_bound(data.begin(), data.end(), value);
    if (at != data.end() && *at == value)
      return;

    data.insert(at, value);
  }
  void remove(const T &value)
  {
    auto at = eastl::lower_bound(data.begin(), data.end(), value);
    if (at != data.end() && *at == value)
      data.erase(at);
  }
  bool has(const T &value) const { return eastl::binary_search(data.begin(), data.end(), value); }

  bool empty() const { return data.empty(); }

  auto begin() { return data.begin(); }
  auto begin() const { return data.begin(); }
  auto cbegin() const { return data.cbegin(); }

  auto end() { return data.end(); }
  auto end() const { return data.end(); }
  auto cend() const { return data.cend(); }

  decltype(auto) front() { return data.front(); }
  decltype(auto) front() const { return data.front(); }
  decltype(auto) back() { return data.back(); }
  decltype(auto) back() const { return data.back(); }
};

namespace detail
{
template <typename T>
inline typename eastl::enable_if<eastl::is_same<void, T>::value>::type make_default()
{}
template <typename T>
inline typename eastl::enable_if<!eastl::is_same<void, T>::value, T>::type make_default()
{
  return T{};
}
} // namespace detail
} // namespace spirv
