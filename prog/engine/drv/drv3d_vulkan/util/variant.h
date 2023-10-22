#pragma once

#include <EASTL/unique_ptr.h>
#include "type_lists.h"

// simple variant, as eastl variant has some problems
template <typename... Types>
class Variant
{
  typedef Variant<Types...> ThisType;
  typedef TypePack<Types...> ThisTypesPack;
  typedef typename TypeFromIndex<0, ThisTypesPack>::Type DefaultType;
  typedef typename eastl::aligned_storage<SizeOfLargestType<ThisTypesPack>::Value>::type StorageType;
  uint32_t typeIndex = 0;
  StorageType store{};

  typedef void (ThisType::*DestructorType)();
  typedef void (ThisType::*CopyType)(const void *);
  typedef void (ThisType::*MoveType)(void *);

  template <typename T>
  void destructOp()
  {
    reinterpret_cast<T *>(&store)->~T();
  }
  template <typename T>
  void copyOp(const void *src)
  {
    ::new (&store) T(*reinterpret_cast<const T *>(src));
  }
  template <typename T>
  void moveOp(void *src)
  {
    ::new (&store) T(eastl::move(*reinterpret_cast<T *>(src)));
  }
  template <typename U, typename T>
  void callOp(U &&call)
  {
    call(*reinterpret_cast<T *>(&store));
  }
  void defaultInit()
  {
    typeIndex = 0;
    ::new (&store) DefaultType;
  }
  void tidy()
  {
    static const DestructorType destructList[] = {&ThisType::destructOp<Types>...};
    (this->*(destructList[typeIndex]))();
  }
  // assumes caller did call tidy first
  void copyFromOther(const Variant &other)
  {
    static const CopyType copyList[] = {&ThisType::copyOp<Types>...};
    typeIndex = other.typeIndex;
    (this->*(copyList[typeIndex]))(&other.store);
  }
  // assumes caller did call tidy first
  void moveFromOther(Variant &other)
  {
    static const MoveType moveList[] = {&ThisType::moveOp<Types>...};
    typeIndex = other.typeIndex;
    (this->*(moveList[typeIndex]))(&other.store);
  }
  template <typename T>
  void copyFrom(const T &other)
  {
    typedef typename eastl::decay<T>::type DecayedT;
    typeIndex = TypeIndexOf<DecayedT, ThisTypesPack>::value;
    ::new (&store) DecayedT(other);
  }
  template <typename T>
  void moveFrom(T &&other)
  {
    typedef typename eastl::decay<T>::type DecayedT;
    typeIndex = TypeIndexOf<DecayedT, ThisTypesPack>::value;
    ::new (&store) DecayedT(eastl::move(other));
  }

public:
  Variant() { defaultInit(); }
  ~Variant() { tidy(); }

  Variant(const Variant &other) { copyFromOther(other); }
  Variant &operator=(const Variant &other)
  {
    tidy();
    copyFromOther(other);
    return *this;
  }
  Variant(Variant &&other) { moveFromOther(other); }
  Variant &operator=(Variant &&other)
  {
    tidy();
    moveFromOther(other);
    return *this;
  }

  template <typename T>
  Variant(const T &value)
  {
    copyFrom(value);
  }
  template <typename T>
  Variant(T &&value)
  {
    moveFrom(eastl::forward<T>(value));
  }

  // remove from overload set if t is this type to avoid conflicts
  template <typename T>
  typename eastl::enable_if<!eastl::is_same<T, Variant>::value, Variant &>::type operator=(const T &value)
  {
    tidy();
    copyFrom(value);
    return *this;
  }
  // remove from overload set if t is this type to avoid conflicts
  template <typename T>
  typename eastl::enable_if<!eastl::is_same<T, Variant>::value, Variant &>::type operator=(T &&value)
  {
    tidy();
    moveFrom(eastl::forward<T>(value));
    return *this;
  }

  template <typename T>
  void visit(T &&clb)
  {
    typedef void (ThisType::*CallType)(T &&);
    static const CallType callList[] = {&ThisType::callOp<T, Types>...};
    (this->*(callList[typeIndex]))(eastl::forward<T>(clb));
  }
};

template <typename T, typename... U>
inline void visit(T &&clb, Variant<U...> &v)
{
  v.visit(eastl::forward<T>(clb));
}