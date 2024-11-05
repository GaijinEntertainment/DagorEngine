// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "variant.h"

// NOTE: void is treated as ignored type, so you can use it as some sort of terminator for generated type lists
template <typename... Types>
class VariantVector
{
  typedef VariantVector<Types...> ThisType;
  typedef TypePack<Types...> ThisTypesPack;
#if !_TARGET_ANDROID
  typedef typename TypeSelect<sizeof...(Types) <= 0xFF, uint8_t,
    typename TypeSelect<sizeof...(Types) <= 0xFFFF, uint16_t,
      typename TypeSelect<sizeof...(Types) <= 0xFFFFFFFF, uint32_t, uint64_t>::Type>::Type>::Type
#else
  // had to do this cause arm can't read unaligned int and etc.
  typedef typename TypeSelect<sizeof...(Types) <= 0xFFFFFFFF, uint32_t, uint64_t>::Type
#endif
    AtomicType;
  size_t totalSize = 0;
  size_t fillSize = 0;
  eastl::unique_ptr<AtomicType[]> store;
  void tidy()
  {
    size_t pos = 0;
    while (pos < fillSize)
      pos += destroyAt(pos);
    fillSize = 0;
  }
  void copyFrom(const VariantVector &other)
  {
    grow(other.fillSize);
    size_t pos = 0;
    while (pos < other.fillSize)
      pos += other.copyConstructInto(pos, &store[pos]);
    fillSize = pos;
  }
  void moveFrom(VariantVector &other)
  {
    totalSize = other.totalSize;
    other.totalSize = 0;
    fillSize = other.fillSize;
    other.fillSize = 0;
    store.reset(other.store.release());
  }

  void doGrow(size_t size)
  {
    size_t newSize = max(totalSize + size, totalSize * 2);
    eastl::unique_ptr<AtomicType[]> newStore;
    newStore.reset(new AtomicType[newSize]);

    size_t pos = 0;
    while (pos < fillSize)
      pos += moveConstructInto(pos, &newStore[pos]);
    tidy();
    store.reset(newStore.release());
    fillSize = pos;
    totalSize = newSize;
  }

  inline void grow(size_t size)
  {
    if (fillSize + size < totalSize)
      return;
    doGrow(size);
  }

  typedef size_t (ThisType::*DestructorType)(size_t);
  typedef size_t (ThisType::*CopyType)(void *, size_t);
  typedef size_t (ThisType::*MoveType)(void *, size_t);

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerDestructor
  {
    static size_t destructOp(void *ptr)
    {
      // vs thinks ptr is not used here
      G_UNUSED(ptr);
      reinterpret_cast<T *>(ptr)->~T();
      return sizeof(T);
    }
  };

  template <typename T>
  struct TypeHandlerDestructor<T, false>
  {
    static size_t destructOp(void *) { return sizeof(T); }
  };

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerCopyConstructor
  {
    static size_t copyOp(void *dst, const void *src)
    {
      ::new (dst) T(*reinterpret_cast<const T *>(src));
      return sizeof(T);
    }
  };

  template <typename T>
  struct TypeHandlerCopyConstructor<T, true>
  {
    static size_t copyOp(void *dst, const void *src)
    {
      memcpy(dst, src, sizeof(T));
      return sizeof(T);
    }
  };

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerMoveConstructor
  {
    static size_t moveOp(void *dst, void *src)
    {
      ::new (dst) T(eastl::move(*reinterpret_cast<T *>(src)));
      return sizeof(T);
    }
  };

  template <typename T>
  struct TypeHandlerMoveConstructor<T, true>
  {
    static size_t moveOp(void *dst, void *src)
    {
      memcpy(dst, src, sizeof(T));
      return sizeof(T);
    }
  };

  // Builds an optimized handler that tries to simplifies stuff if they are trivial
  // Second param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool>
  struct TypeHandler : TypeHandlerDestructor<T, eastl::is_trivially_destructible<T>::value>,
                       TypeHandlerCopyConstructor<T, eastl::is_trivially_copy_constructible<T>::value>,
                       TypeHandlerMoveConstructor<T, eastl::is_trivially_move_constructible<T>::value>
  {
    template <typename U>
    static size_t callOp(void *self, U &u)
    {
      u(*reinterpret_cast<T *>(self));
      return sizeof(T);
    }

    static constexpr size_t size = sizeof(T);
  };
  // need to have b to prevent full specialization, as its not allowed in class scope
  template <bool b>
  struct TypeHandler<void, b>
  {
    static size_t destructOp(void *) { return 0; }

    static size_t copyOp(void *, const void *) { return 0; }

    static size_t moveOp(void *, void *) { return 0; }

    template <typename U>
    static size_t callOp(void *, U &)
    {
      return 0;
    }

    static constexpr size_t size = 0;
  };

  template <typename T>
  size_t destructOp(size_t pos)
  {
    return TypeHandler<T, true>::destructOp(&store[pos]);
  }

  template <typename T>
  size_t copyOp(void *dst, size_t pos)
  {
    return TypeHandler<T, true>::copyOp(dst, &store[pos]);
  }

  template <typename T>
  size_t moveOp(void *dst, size_t pos)
  {
    return TypeHandler<T, true>::moveOp(dst, &store[pos]);
  }

  template <typename U, typename T>
  size_t callOp(size_t pos, U &u)
  {
    return TypeHandler<T, true>::callOp(&store[pos], u);
  }

  static size_t calculateTotalSpaceForTypeSize(size_t sz) { return 1 + ((sz + sizeof(AtomicType) - 1) / sizeof(AtomicType)); }

  size_t destroyAt(size_t pos)
  {
    static const DestructorType destructList[] = {&ThisType::destructOp<Types>...};
    return calculateTotalSpaceForTypeSize((this->*(destructList[store[pos]]))(pos + 1));
  }
  size_t copyConstructInto(size_t pos, AtomicType *data)
  {
    static const CopyType copyList[] = {&ThisType::copyOp<Types>...};
    data[0] = store[pos];
    return calculateTotalSpaceForTypeSize((this->*(copyList[store[pos]]))(&data[1], pos + 1));
  }
  size_t moveConstructInto(size_t pos, AtomicType *data)
  {
    static const MoveType moveList[] = {&ThisType::moveOp<Types>...};
    data[0] = store[pos];
    return calculateTotalSpaceForTypeSize((this->*(moveList[store[pos]]))(&data[1], pos + 1));
  }
  size_t sizeAt(size_t pos)
  {
    static const size_t sizeList[] = {sizeof(Types)...};
    return calculateTotalSpaceForTypeSize(sizeList[store[pos]]);
  }
  template <typename U>
  size_t callAt(size_t pos, U &u)
  {
    typedef size_t (ThisType::*CallType)(size_t, U &);
    static const CallType callList[] = {&ThisType::callOp<U, Types>...};
    return calculateTotalSpaceForTypeSize((this->*(callList[store[pos]]))(pos + 1, u));
  }

public:
  // just in case we need to debug stuff
  class Iterator
  {
    VariantVector *parent = nullptr;
    size_t index = 0;

  public:
    Iterator(VariantVector &p, size_t i) : parent{&p}, index{i} {}
    Iterator() = default;
    ~Iterator() = default;

    Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;

    Iterator(Iterator &&) = default;
    Iterator &operator=(Iterator &&) = default;

    Iterator &operator++()
    {
      index += parent->sizeAt(index);
      return *this;
    }
    Iterator operator++(int)
    {
      Iterator copy;
      copy.parent = parent;
      copy.index = index + parent->sizeAt(index);
      return copy;
    }

    template <typename U>
    void visit(U &&u)
    {
      parent->callAt(index, eastl::forward<U>(u));
    }

    template <typename U>
    bool isType() const
    {
      typedef typename eastl::decay<U>::type DecayedU;
      return parent->store[index] == TypeIndexOf<DecayedU, ThisTypesPack>::value;
    }

    template <typename U>
    const U *get() const
    {
      if (isType<U>())
      {
        return reinterpret_cast<const U *>(&parent->store[index + 1]);
      }
      return nullptr;
    }

    friend bool operator==(const Iterator &l, const Iterator &r) { return l.parent == r.parent && l.index == r.index; }
    friend bool operator!=(const Iterator &l, const Iterator &r) { return !(l == r); }
    friend bool operator<=(const Iterator &l, const Iterator &r) { return (l < r) || (l == r); }
    friend bool operator>=(const Iterator &l, const Iterator &r) { return (l > r) || (l == r); }
    friend bool operator<(const Iterator &l, const Iterator &r) { return l.parent == r.parent && l.index < r.index; }
    friend bool operator>(const Iterator &l, const Iterator &r) { return r < l; }
  };
  VariantVector() = default;
  ~VariantVector() = default;

  VariantVector(const VariantVector &other) { copyFrom(other); }
  VariantVector &operator=(const VariantVector &other)
  {
    tidy();
    copyFrom(other);
    return *this;
  }
  VariantVector(VariantVector &&other) { moveFrom(other); }
  VariantVector &operator=(VariantVector &&other)
  {
    tidy();
    moveFrom(other);
    return *this;
  }

  template <typename T>
  Iterator push_back(T &&v)
  {
    typedef typename eastl::decay<T>::type DecayedT;
    typedef TypeHandler<DecayedT, true> DecayedTHandler;
    // prevent push of empty types
    G_STATIC_ASSERT(DecayedTHandler::size != 0);
    auto sizeNeeded = calculateTotalSpaceForTypeSize(sizeof(DecayedT));
    grow(sizeNeeded);
    auto objectPos = fillSize;
    fillSize += sizeNeeded;
    store[objectPos] = TypeIndexOf<DecayedT, ThisTypesPack>::value;
    DecayedTHandler::moveOp(&store[objectPos + 1], &v);
    return {*this, objectPos};
  }

  template <typename T>
  Iterator push_back(const T &v)
  {
    typedef typename eastl::decay<T>::type DecayedT;
    typedef TypeHandler<DecayedT, true> DecayedTHandler;
    // prevent push of empty types
    G_STATIC_ASSERT(DecayedTHandler::size != 0);
    auto sizeNeeded = calculateTotalSpaceForTypeSize(sizeof(DecayedT));
    grow(sizeNeeded);
    auto objectPos = fillSize;
    fillSize += sizeNeeded;
    store[objectPos] = TypeIndexOf<DecayedT, ThisTypesPack>::value;
    DecayedTHandler::copyOp(&store[objectPos + 1], &v);
    return {*this, objectPos};
  }

  Iterator begin() { return {*this, 0}; }
  Iterator end() { return {*this, fillSize}; }

  template <typename U>
  void visitAll(U &&u)
  {
    size_t pos = 0;
    while (pos < fillSize)
      pos += callAt(pos, u);
  }

  void clear() { tidy(); }

  size_t size() const { return fillSize * sizeof(AtomicType); }

  size_t capacity() const { return totalSize * sizeof(AtomicType); }
};