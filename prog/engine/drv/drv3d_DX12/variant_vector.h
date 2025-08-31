// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"

#include <atomic>
#include <EASTL/numeric.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/optional.h>
#include <EASTL/span.h>
#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <perfMon/dag_statDrv.h>


template <typename...>
struct TypePack;

template <typename, typename>
struct TypeIndexOf
{};

template <typename T, typename... A>
struct TypeIndexOf<T, TypePack<T, A...>>
{
  static constexpr size_t value = 0;
};

template <typename T, typename U, typename... A>
struct TypeIndexOf<T, TypePack<U, A...>>
{
  static constexpr size_t value = 1 + TypeIndexOf<T, TypePack<A...>>::value;
};

template <typename>
class SelectIndexType;

template <typename... Types>
class SelectIndexType<TypePack<Types...>>
{
private:
  using Type32Bits =
    typename eastl::conditional<(sizeof...(Types) <= eastl::numeric_limits<uint32_t>::max()), uint32_t, uint64_t>::type;
  using Type16Bits =
    typename eastl::conditional<(sizeof...(Types) <= eastl::numeric_limits<uint16_t>::max()), uint16_t, Type32Bits>::type;
  using Type8Bits =
    typename eastl::conditional<(sizeof...(Types) <= eastl::numeric_limits<uint8_t>::max()), uint8_t, Type16Bits>::type;

public:
  using Type = Type8Bits;
};

template <typename T, typename P0>
struct ExtendedVariant
{
  T &cmd;
  eastl::span<P0> p0;
};

template <typename T, typename P0, typename P1>
struct ExtendedVariant2
{
  T &cmd;
  eastl::span<P0> p0;
  eastl::span<P1> p1;
};

template <typename T>
struct TypeMemoryRequirement
{
  static constexpr size_t size_of = sizeof(T);
  static constexpr size_t aling_of = alignof(T);
  // extra space for align the pointer
  static constexpr size_t value = size_of + aling_of - 1;
  static constexpr size_t array_of(size_t count)
  {
    // only the first element gets aligned all follow up are tightly packed
    return 0 == count ? 0 : (value + ((count - 1) * size_of));
  }
  static void *align_pointer(void *ptr)
  {
    auto asInt = reinterpret_cast<uintptr_t>(ptr);
    asInt = (asInt + aling_of - 1) & ~(aling_of - 1);
    return reinterpret_cast<void *>(asInt);
  }
  static const void *align_pointer(const void *ptr)
  {
    auto asInt = reinterpret_cast<uintptr_t>(ptr);
    asInt = (asInt + aling_of - 1) & ~(aling_of - 1);
    return reinterpret_cast<const void *>(asInt);
  }
};

struct MemoryWriter
{
  uint8_t *at = nullptr;

  MemoryWriter(void *ptr) : at{reinterpret_cast<uint8_t *>(ptr)} {}

  template <typename T>
  T &writeMove(T &&data)
  {
    auto obj = ::new (TypeMemoryRequirement<T>::align_pointer(at)) T{eastl::forward<T>(data)};
    at = reinterpret_cast<uint8_t *>(obj + 1);
    return *obj;
  }

  template <typename T>
  T &write(const T &data)
  {
    auto obj = ::new (TypeMemoryRequirement<T>::align_pointer(at)) T{data};
    at = reinterpret_cast<uint8_t *>(obj + 1);
    return *obj;
  }

  template <typename T>
  T &rawWrite(const void *data)
  {
    auto obj = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    memcpy(obj, data, sizeof(T));
    at = reinterpret_cast<uint8_t *>(obj + 1);
    return *obj;
  }

  template <typename T>
  T *rawWriteArray(const void *data, size_t count)
  {
    auto obj = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    memcpy(obj, data, sizeof(T) * count);
    at = reinterpret_cast<uint8_t *>(obj + count);
    return obj;
  }

  template <typename T>
  T *rawReserveArray(size_t count)
  {
    auto obj = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    at = reinterpret_cast<uint8_t *>(obj + count);
    return obj;
  }

  template <typename T>
  T *writeArray(T *data, size_t count)
  {
    auto p = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    for (size_t i = 0; i < count; ++i)
    {
      ::new (&p[i]) T{eastl::move(data[i])};
    }
    at = reinterpret_cast<uint8_t *>(&p[count]);
    return p;
  }

  template <typename T>
  T *writeArray(const T *data, size_t count)
  {
    auto p = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    for (size_t i = 0; i < count; ++i)
    {
      ::new (&p[i]) T{data[i]};
    }
    at = reinterpret_cast<uint8_t *>(&p[count]);
    return p;
  }

  template <typename T>
  T *writeArray(eastl::span<T> data)
  {
    auto p = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    auto s = p;
    for (auto &&s : data)
    {
      ::new (p++) T{eastl::move(data)};
    }
    at = reinterpret_cast<uint8_t *>(p);
    return s;
  }

  template <typename T>
  T *writeArray(eastl::span<const T> data)
  {
    auto p = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    auto s = p;
    for (auto &&s : data)
    {
      ::new (p++) T{data};
    }
    at = reinterpret_cast<uint8_t *>(p);
    return s;
  }
};

struct MemoryInterpreter
{
  uint8_t *at = nullptr;

  MemoryInterpreter(void *ptr) : at{reinterpret_cast<uint8_t *>(ptr)} {}

  template <typename T>
  T &as()
  {
    auto obj = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    at = reinterpret_cast<uint8_t *>(obj + 1);
    return *obj;
  }

  template <typename T>
  eastl::span<T> asArray(size_t sz)
  {
    auto s = reinterpret_cast<T *>(TypeMemoryRequirement<T>::align_pointer(at));
    at = reinterpret_cast<uint8_t *>(s + sz);
    return {s, s + sz};
  }
};

struct MemoryInterpreterConst
{
  const uint8_t *at = nullptr;

  MemoryInterpreterConst(const void *ptr) : at{reinterpret_cast<const uint8_t *>(ptr)} {}

  template <typename T>
  const T &as()
  {
    auto obj = reinterpret_cast<const T *>(TypeMemoryRequirement<T>::align_pointer(at));
    at = reinterpret_cast<const uint8_t *>(obj + 1);
    return *obj;
  }

  template <typename T>
  eastl::span<const T> asArray(size_t sz)
  {
    auto obj = reinterpret_cast<const T *>(TypeMemoryRequirement<T>::align_pointer(at));
    at = reinterpret_cast<const uint8_t *>(obj + sz);
    return {obj, obj + sz};
  }
};


// Has no public interface, it is expected that the use of the public interface is done with an overload on
// VariantContainerNullMetricsCollector and the other collector types and do nothing on this one.
template <typename>
class VariantContainerNullMetricsCollector
{
protected:
  void recordVisitOf(size_t) {}

public:
  static constexpr bool metrics_available = false;
};

template <typename>
class VariantContainerVisitMetricsCollector;

template <typename... Types>
class VariantContainerVisitMetricsCollector<TypePack<Types...>>
{
  size_t visitCount[sizeof...(Types)]{};

protected:
  void recordVisitOf(size_t index) { ++visitCount[index]; }

public:
  void resetMetrics() { eastl::fill(eastl::begin(visitCount), eastl::end(visitCount), 0); }

  template <typename T>
  void resetMetricsFor()
  {
    using DT = typename eastl::decay<T>::type;
    using ThisTypesPack = TypePack<Types...>;
    visitCount[TypeIndexOf<DT, ThisTypesPack>::value] = 0;
  }

  template <typename T>
  size_t getVisitCountFor() const
  {
    using DT = typename eastl::decay<T>::type;
    using ThisTypesPack = TypePack<Types...>;
    return visitCount[TypeIndexOf<DT, ThisTypesPack>::value];
  }

  size_t getTotalVisitCount() const { return eastl::accumulate(eastl::begin(visitCount), eastl::end(visitCount), size_t{0}); }

  template <typename T>
  size_t getAllVisitCounts(T clb) const
  {
    size_t total = 0;
    for (size_t i = 0; i < sizeof...(Types); ++i)
    {
      clb(i, visitCount[i]);
      total += visitCount[i];
    }
    return total;
  }

  static constexpr size_t command_count = sizeof...(Types);
  static constexpr bool metrics_available = true;
};

// #define DX12_ENABLE_VARIANT_VECTOR_METRICS 1
#if DX12_ENABLE_VARIANT_VECTOR_METRICS
template <typename A>
using VariantContainerMetricsCollector = VariantContainerVisitMetricsCollector<A>;
#else
template <typename A>
using VariantContainerMetricsCollector = VariantContainerNullMetricsCollector<A>;
#endif

template <typename, typename>
class VariantContainerBase;

// Base class for all variant container types, it provides types and other facilities for
// storage independent handling of those type packs.
template <typename... Types, typename IType>
class VariantContainerBase<TypePack<Types...>, IType> : public VariantContainerMetricsCollector<TypePack<Types...>>
{
protected:
  using MetricsCollectorType = VariantContainerMetricsCollector<TypePack<Types...>>;
  using ThisTypesPack = TypePack<Types...>;

  static constexpr size_t num_types = sizeof...(Types);

  using IndexType = IType;
  using DataStoreType = uint8_t;

  using DestructorType = size_t (*)(MemoryInterpreter);
  using CopyType = size_t (*)(MemoryWriter, MemoryInterpreterConst);
  using MoveType = size_t (*)(MemoryWriter, MemoryInterpreter);
  using QuerySizeType = size_t (*)(MemoryInterpreterConst);

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerDestructor
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      m.as<T>().~T();
      return TypeMemoryRequirement<T>::value;
    }
  };

  template <typename T>
  struct TypeHandlerDestructor<T, true>
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      m.as<T>();
      return TypeMemoryRequirement<T>::value;
    }
  };

  template <typename T, typename P0>
  struct TypeHandlerDestructor<ExtendedVariant<T, P0>, true>
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      auto p0Count = m.as<size_t>();
      m.as<T>().~T();
      m.asArray<T>(p0Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0>
  struct TypeHandlerDestructor<ExtendedVariant<T, P0>, false>
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      auto p0Count = m.as<size_t>();
      m.as<T>().~T();
      for (auto &&p : m.asArray<P0>(p0Count))
      {
        // vc fails to see that calling the destructor on a variable is a use
        G_UNUSED(p);
        p.~P0();
      }
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerDestructor<ExtendedVariant2<T, P0, P1>, true>
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      auto p0Count = m.as<size_t>();
      auto p1Count = m.as<size_t>();
      m.as<T>();
      m.asArray<P0>(p0Count);
      m.asArray<P1>(p1Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerDestructor<ExtendedVariant2<T, P0, P1>, false>
  {
    static size_t destructOp(MemoryInterpreter m)
    {
      auto p0Count = m.as<size_t>();
      auto p1Count = m.as<size_t>();
      m.as<T>().~T();
      for (auto p0 : m.asArray<P0>(p0Count))
      {
        p0.~P0();
      }
      for (auto p1 : m.asArray<P1>(p1Count))
      {
        p1.~P1();
      }
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }
  };

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerCopyConstructor
  {
    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      d.write<T>(s.as<T>());
      return TypeMemoryRequirement<T>::value;
    }

    static void copyConstruct(MemoryWriter d, const T &value) { d.write<T>(value); }
  };

  template <typename T>
  struct TypeHandlerCopyConstructor<T, true>
  {
    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      d.rawWrite(s.as<T>());
      return TypeMemoryRequirement<T>::value;
    }

    static void copyConstruct(MemoryWriter d, const T &value) { d.rawWrite<T>(&value); }
  };

  template <typename T, typename P0>
  struct TypeHandlerCopyConstructor<ExtendedVariant<T, P0>, true>
  {
    static void copyConstruct(MemoryWriter d, const T &value, const P0 *p0, size_t p0_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<T>(&value);
      d.rawWriteArray<P0>(p0, p0_count);
    }

    template <typename G0>
    static void copyConstructDataGenerator(MemoryWriter d, const T &value, size_t p0_count, G0 g0)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<T>(&value);
      auto p0 = d.rawReserveArray<P0>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        auto data = g0(i);
        memcpy(&p0[i], &data, sizeof(P0));
      }
    }

    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.rawWrite<T>(&s.as<T>());
      auto r = s.asArray<P0>(p0Count);
      d.rawWriteArray<P0>(r.data(), p0Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0>
  struct TypeHandlerCopyConstructor<ExtendedVariant<T, P0>, false>
  {
    static void copyConstruct(MemoryWriter d, const T &value, const P0 *p0, size_t p0_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.write<T>(value);
      d.writeArray<P0>(p0, p0_count);
    }

    template <typename G0>
    static void copyConstructDataGenerator(MemoryWriter d, const T &value, size_t p0_count, G0 g0)
    {
      d.rawWrite<size_t>(&p0_count);
      d.write<T>(value);
      auto p0 = d.rawReserveArray<P0>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        ::new (reinterpret_cast<void *>(&p0[i])) T{g0(i)};
      }
    }

    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.write<T>(s.as<T>());
      d.writeArray<P0>(s.asArray<P0>(p0Count));
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerCopyConstructor<ExtendedVariant2<T, P0, P1>, false>
  {
    static void copyConstruct(MemoryWriter d, const T &value, const P0 *p0, size_t p0_count, const P0 *p1, size_t p1_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p1_count);
      d.write<T>(value);
      d.writeArray<P0>(p0, p0_count);
      d.writeArray<P1>(p1, p1_count);
    }

    template <typename G>
    static void copyConstructDataGenerator(MemoryWriter d, const void *value, size_t p0_count, G g)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p0_count);
      d.write<T>(*reinterpret_cast<const T *>(value));
      auto p0 = d.rawReserveArray<P0>(p0_count);
      auto p1 = d.rawReserveArray<P1>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        g(i, [t = p0 + i](auto v) { ::new (t) P0{eastl::move(v)}; }, [t = p1 + i](auto v) { ::new (t) P1{eastl::move(v)}; });
      }
    }
    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      auto p1Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.write<T>(s.as<T>());
      d.writeArray<P0>(s.asArray<P0>(p0Count));
      d.writeArray<P1>(s.asArray<P1>(p1Count));
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p0Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerCopyConstructor<ExtendedVariant2<T, P0, P1>, true>
  {
    static void copyConstruct(MemoryWriter d, const T &value, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p1_count);
      d.rawWrite<T>(&value);
      d.rawWriteArray<P0>(p0, p0_count);
      d.rawWriteArray<P1>(p1, p1_count);
    }

    template <typename G>
    static void copyConstructDataGenerator(MemoryWriter d, const void *value, size_t p0_count, G g)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p0_count);
      d.write<T>(*reinterpret_cast<const T *>(value));
      auto p0 = d.rawReserveArray<P0>(p0_count);
      auto p1 = d.rawReserveArray<P1>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        g(i, [t = p0 + i](auto v) { ::new (t) P0{eastl::move(v)}; }, [t = p1 + i](auto v) { ::new (t) P1{eastl::move(v)}; });
      }
    }

    static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      auto p1Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.write<T>(s.as<T>());
      d.writeArray<P0>(s.asArray<P0>(p0Count));
      d.writeArray<P1>(s.asArray<P1>(p1Count));
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p0Count);
    }
  };

  // third param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool IsTrivial = false, bool = false>
  struct TypeHandlerMoveConstructor
  {
    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      d.writeMove<T>(eastl::move(s.as<T>()));
      return TypeMemoryRequirement<T>::value;
    }

    static void moveContruct(MemoryWriter d, T &&value) { d.writeMove<T>(eastl::forward<T>(value)); }
  };

  template <typename T>
  struct TypeHandlerMoveConstructor<T, true>
  {
    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      d.rawWrite<T>(&s.as<T>());
      return TypeMemoryRequirement<T>::value;
    }

    static void moveContruct(MemoryWriter d, T &&value) { d.rawWrite<T>(&value); }
  };

  template <typename T, typename P0>
  struct TypeHandlerMoveConstructor<ExtendedVariant<T, P0>, true>
  {
    template <typename V0>
    static void moveContruct(MemoryWriter d, T &&value, V0 *p0, size_t p0_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<T>(&value);
      d.rawWriteArray<P0>(p0, p0_count);
    }

    template <typename G0>
    static void moveContructDataGenerator(MemoryWriter d, T &&value, size_t p0_count, G0 g0)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<T>(&value);
      auto p0 = d.rawReserveArray<P0>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        auto pv0 = g0(i);
        memcpy(&p0[i], &pv0, sizeof(P0));
      }
    }

    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.rawWrite<T>(&s.as<T>());
      d.rawWriteArray<P0>(s.asArray<P0>(p0Count).data(), p0Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0>
  struct TypeHandlerMoveConstructor<ExtendedVariant<T, P0>, false>
  {
    template <typename V0>
    static void moveContruct(MemoryWriter d, T &&value, V0 *p0, size_t p0_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.writeMove<T>(eastl::forward<T>(value));
      auto dp0 = d.rawReserveArray<P0>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        ::new (reinterpret_cast<void *>(&dp0[i])) P0{eastl::forward<V0>(p0[i])};
      }
    }

    template <typename G0>
    static void moveContructDataGenerator(MemoryWriter dst, T &&value, size_t p0_count, G0 g0)
    {
      MemoryWriter d{dst};
      d.rawWrite<size_t>(&p0_count);
      d.writeMove<T>(eastl::forward<T>(value));
      auto dp0 = d.rawReserveArray<P0>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        ::new (reinterpret_cast<void *>(&dp0[i])) T{eastl::move(g0(i))};
      }
    }

    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.writeMove<T>(eastl::move(s.as<T>()));
      auto sP0 = d.rawReserveArray<P0>(p0Count);
      for (auto &&p0 : s.asArray<P0>(p0Count))
      {
        ::new (sP0++) P0{eastl::move(p0)};
      }
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerMoveConstructor<ExtendedVariant2<T, P0, P1>, true>
  {
    static void moveContruct(MemoryWriter d, T &&value, P0 *p0, size_t p0_count, P1 *p1, size_t p1_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p1_count);
      d.rawWrite<T>(&value);
      d.rawWriteArray<P0>(p0, p0_count);
      d.rawWriteArray<P1>(p1, p1_count);
    }

    template <typename G>
    static void moveContructDataGenerator(MemoryWriter d, T &&value, size_t p0_count, G g)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<T>(&value);
      auto p0 = d.rawReserveArray<P0>(p0_count);
      auto p1 = d.rawReserveArray<P1>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        g(i, [t = p0 + i](auto v) { ::new (t) P0{eastl::move(v)}; }, [t = p1 + i](auto v) { ::new (t) P1{eastl::move(v)}; });
      }
    }

    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      auto p1Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.rawWrite<T>(&s.as<T>());
      d.rawWriteArray<P0>(s.asArray<P0>(p0Count).data(), p0Count);
      d.rawWriteArray<P1>(s.asArray<P1>(p1Count).data(), p1Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }
  };

  template <typename T, typename P0, typename P1>
  struct TypeHandlerMoveConstructor<ExtendedVariant2<T, P0, P1>, false>
  {
    static void moveContruct(MemoryWriter d, T &&value, P0 *p0, size_t p0_count, P1 *p1, size_t p1_count)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p1_count);
      d.writeMove<T>(eastl::forward<T>(value));
      d.writeArray<P0>(p0, p0_count);
      d.writeArray<P1>(p1, p1_count);
    }

    template <typename G>
    static void moveContructDataGenerator(MemoryWriter d, T &&value, size_t p0_count, G g)
    {
      d.rawWrite<size_t>(&p0_count);
      d.rawWrite<size_t>(&p0_count);
      d.writeMove<T>(eastl::forward<T>(value));
      auto p0 = d.rawReserveArray<P0>(p0_count);
      auto p1 = d.rawReserveArray<P1>(p0_count);
      for (size_t i = 0; i < p0_count; ++i)
      {
        g(i, [t = p0 + i](auto v) { ::new (t) P0{eastl::move(v)}; }, [t = p1 + i](auto v) { ::new (t) P1{eastl::move(v)}; });
      }
    }

    static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
    {
      auto p0Count = d.rawWrite<size_t>(&s.as<size_t>());
      auto p1Count = d.rawWrite<size_t>(&s.as<size_t>());
      d.writeMove<T>(eastl::move(s.as<T>()));
      d.rawWriteArray<P0>(s.asArray<P0>(p0Count).data(), p0Count);
      d.rawWriteArray<P1>(s.asArray<P1>(p1Count).data(), p1Count);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }
  };

  // Builds an optimized handler that tries to simplifies stuff if they are trivial
  // Second param is to prevent full specialization, as its not allowed in class scope
  template <typename T, bool>
  struct TypeHandler : TypeHandlerDestructor<T, eastl::is_trivially_destructible<T>::value>,
                       TypeHandlerCopyConstructor<T, eastl::is_trivially_copy_constructible<T>::value>,
                       TypeHandlerMoveConstructor<T, eastl::is_trivially_move_constructible<T>::value>
  {
    static size_t sizeOp(MemoryInterpreterConst) { return TypeMemoryRequirement<T>::value; }

    template <typename U>
    static size_t callOp(MemoryInterpreter m, U &&u)
    {
      u(m.as<T>());
      return TypeMemoryRequirement<T>::value;
    }

    template <typename U>
    static size_t callOpAndIndex(MemoryInterpreter m, size_t index, U &&u)
    {
      u(m.as<T>(), index);
      return TypeMemoryRequirement<T>::value;
    }

    static constexpr bool is_valid = true;
  };

  template <typename T, typename P0, bool B>
  struct TypeHandler<ExtendedVariant<T, P0>, B>
    : TypeHandlerDestructor<ExtendedVariant<T, P0>,
        eastl::is_trivially_destructible<T>::value && eastl::is_trivially_destructible<P0>::value>,
      TypeHandlerCopyConstructor<ExtendedVariant<T, P0>,
        eastl::is_trivially_copy_constructible<T>::value && eastl::is_trivially_copy_constructible<P0>::value>,
      TypeHandlerMoveConstructor<ExtendedVariant<T, P0>,
        eastl::is_trivially_move_constructible<T>::value && eastl::is_trivially_move_constructible<P0>::value>
  {
    static size_t sizeOp(MemoryInterpreterConst m)
    {
      auto p0Count = m.as<size_t>();
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }

    template <typename U>
    static size_t callOp(MemoryInterpreter m, U &&u)
    {
      auto p0Count = m.as<size_t>();
      auto &value = m.as<T>();
      auto p0 = m.asArray<P0>(p0Count);
      ExtendedVariant<T, P0> arg{value, p0};
      u(arg);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }

    template <typename U>
    static size_t callOpAndIndex(MemoryInterpreter m, size_t index, U &&u)
    {
      auto p0Count = m.as<size_t>();
      auto &value = m.as<T>();
      auto p0 = m.asArray<P0>(p0Count);
      ExtendedVariant<T, P0> arg{value, p0};
      u(arg, index);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value + TypeMemoryRequirement<P0>::array_of(p0Count);
    }

    static constexpr bool is_valid = true;
  };

  template <typename T, typename P0, typename P1, bool B>
  struct TypeHandler<ExtendedVariant2<T, P0, P1>, B>
    : TypeHandlerDestructor<ExtendedVariant2<T, P0, P1>, eastl::is_trivially_destructible<T>::value &&
                                                           eastl::is_trivially_destructible<P0>::value &&
                                                           eastl::is_trivially_destructible<P1>::value>,
      TypeHandlerCopyConstructor<ExtendedVariant2<T, P0, P1>, eastl::is_trivially_copy_constructible<T>::value &&
                                                                eastl::is_trivially_copy_constructible<P0>::value &&
                                                                eastl::is_trivially_copy_constructible<P1>::value>,
      TypeHandlerMoveConstructor<ExtendedVariant2<T, P0, P1>, eastl::is_trivially_move_constructible<T>::value &&
                                                                eastl::is_trivially_move_constructible<P0>::value &&
                                                                eastl::is_trivially_move_constructible<P1>::value>
  {
    static size_t sizeOp(MemoryInterpreterConst m)
    {
      auto p0Count = m.as<size_t>();
      auto p1Count = m.as<size_t>();
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }

    template <typename U>
    static size_t callOp(MemoryInterpreter m, U &&u)
    {
      auto p0Count = m.as<size_t>();
      auto p1Count = m.as<size_t>();
      auto &value = m.as<T>();
      auto p0 = m.asArray<P0>(p0Count);
      auto p1 = m.asArray<P1>(p1Count);
      ExtendedVariant2<T, P0, P1> arg{value, p0, p1};
      u(arg);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }

    template <typename U>
    static size_t callOpAndIndex(MemoryInterpreter m, size_t index, U &&u)
    {
      auto p0Count = m.as<size_t>();
      auto p1Count = m.as<size_t>();
      auto &value = m.as<T>();
      auto p0 = m.asArray<P0>(p0Count);
      auto p1 = m.asArray<P1>(p1Count);
      ExtendedVariant2<T, P0, P1> arg{value, p0, p1};
      u(arg, index);
      return TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<T>::value +
             TypeMemoryRequirement<P0>::array_of(p0Count) + TypeMemoryRequirement<P1>::array_of(p1Count);
    }

    static constexpr bool is_valid = true;
  };
  // need to have b to prevent full specialization, as its not allowed in class scope
  template <bool b>
  struct TypeHandler<void, b>
  {
    static size_t destructOp(MemoryInterpreter) { return 0; }

    static size_t copyOp(MemoryWriter, MemoryInterpreterConst) { return 0; }

    static size_t moveOp(MemoryWriter, MemoryInterpreter) { return 0; }

    template <typename U>
    static size_t callOp(MemoryInterpreter, U &&)
    {
      return 0;
    }

    template <typename U>
    static size_t callOpAndIndex(MemoryInterpreter, size_t, U &&)
    {
      return 0;
    }

    static size_t sizeOp(MemoryInterpreterConst) { return 0; }

    static constexpr bool is_valid = false;
  };

  template <typename T>
  static size_t destructOp(MemoryInterpreter m)
  {
    return TypeHandler<T, true>::destructOp(m);
  }

  template <typename T>
  static size_t copyOp(MemoryWriter d, MemoryInterpreterConst s)
  {
    return TypeHandler<T, true>::copyOp(d, s);
  }

  template <typename T>
  static size_t moveOp(MemoryWriter d, MemoryInterpreter s)
  {
    return TypeHandler<T, true>::moveOp(d, s);
  }

  template <typename U, typename T>
  static size_t callOp(MemoryInterpreter m, U &&u)
  {
    return TypeHandler<T, true>::callOp(m, eastl::forward<U>(u));
  }

  template <typename U, typename T>
  static size_t callOpAndIndex(MemoryInterpreter m, size_t index, U &&u)
  {
    return TypeHandler<T, true>::callOpAndIndex(m, index, eastl::forward<U>(u));
  }

  template <typename U, typename T>
  static size_t callOpAndDestroy(MemoryInterpreter m, U &&u)
  {
    TypeHandler<T, true>::callOp(m, eastl::forward<U>(u));
    return TypeHandler<T, true>::destructOp(m);
  }

  template <typename U, typename T>
  static size_t callOpIndexAndDestroy(MemoryInterpreter m, size_t index, U &&u)
  {
    TypeHandler<T, true>::callOpAndIndex(m, index, eastl::forward<U>(u));
    return TypeHandler<T, true>::destructOp(m);
  }

  template <typename T>
  static size_t sizeOp(MemoryInterpreterConst m)
  {
    return TypeHandler<T, true>::sizeOp(m);
  }

  static size_t destroyAt(DataStoreType *pos)
  {
    MemoryInterpreter m{pos};
    auto typeIndex = m.as<IndexType>();
    static const DestructorType destructList[] = {&destructOp<Types>...};
    return TypeMemoryRequirement<IndexType>::value + destructList[typeIndex](m);
  }
  static size_t copyConstructInto(DataStoreType *pos, DataStoreType *data)
  {
    MemoryInterpreterConst s{data};
    MemoryWriter d{pos};
    static const CopyType copyList[] = {&copyOp<Types>...};
    auto typeIndex = d.write<IndexType>(s.as<IndexType>());
    return TypeMemoryRequirement<IndexType>::value + copyList[typeIndex](d, s);
  }
  static size_t moveConstructInto(DataStoreType *pos, DataStoreType *data)
  {
    MemoryInterpreter s{pos};
    MemoryWriter d{data};
    static const MoveType moveList[] = {&moveOp<Types>...};
    auto typeIndex = d.write<IndexType>(s.as<IndexType>());
    return TypeMemoryRequirement<IndexType>::value + moveList[typeIndex](d, s);
  }
  static size_t sizeAt(DataStoreType *pos)
  {
    MemoryInterpreterConst m{pos};
    auto typeIndex = m.as<IndexType>();
    static const QuerySizeType sizeList[] = {&sizeOp<Types>...};
    return TypeMemoryRequirement<IndexType>::value + sizeList[typeIndex](m);
  }
  template <typename U>
  size_t callAt(DataStoreType *pos, U &&u)
  {
    MemoryInterpreter m{pos};
    auto typeIndex = m.as<IndexType>();
    MetricsCollectorType::recordVisitOf(typeIndex);
    using CallType = size_t (*)(MemoryInterpreter, U &&);
    static const CallType callList[] = {&callOp<U, Types>...};
    return TypeMemoryRequirement<IndexType>::value + callList[typeIndex](m, eastl::forward<U>(u));
  }

  template <typename U>
  size_t callAtAndDestroy(DataStoreType *pos, U &&u)
  {
    MemoryInterpreter m{pos};
    auto typeIndex = m.as<IndexType>();
    MetricsCollectorType::recordVisitOf(typeIndex);
    using CallType = size_t (*)(MemoryInterpreter, U &&);
    static const CallType callList[] = {&callOpAndDestroy<U, Types>...};
    return TypeMemoryRequirement<IndexType>::value + callList[typeIndex](m, eastl::forward<U>(u));
  }

  template <typename T>
  static void copyConstructAt(DataStoreType *pos, const T &value)
  {
    using DT = typename eastl::decay<T>::type;
    using UsedTypeHandler = TypeHandler<DT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<DT, ThisTypesPack>::value);
    UsedTypeHandler::copyConstruct(m, value);
  }

  template <typename T>
  static void moveConstructAt(DataStoreType *pos, T &&value)
  {
    using DT = typename eastl::decay<T>::type;
    using UsedTypeHandler = TypeHandler<DT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<DT, ThisTypesPack>::value);
    UsedTypeHandler::moveContruct(m, eastl::forward<DT>(value));
  }

  template <typename T, typename P0>
  static void copyConstructAt(DataStoreType *pos, const T &value, const P0 *p0, size_t p0_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using UsedTypeHandler = TypeHandler<ExtendedVariant<DT, DP0>, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<ExtendedVariant<DT, DP0>, ThisTypesPack>::value);
    UsedTypeHandler::copyConstruct(m, value, p0, p0_count);
  }

  template <typename T, typename P0>
  static void moveConstructAt(DataStoreType *pos, T &&value, const P0 *p0, size_t p0_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using UsedTypeHandler = TypeHandler<ExtendedVariant<DT, DP0>, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<ExtendedVariant<DT, DP0>, ThisTypesPack>::value);
    UsedTypeHandler::moveContruct(m, eastl::forward<DT>(value), p0, p0_count);
  }

  template <typename T, typename P0, typename P1>
  static void copyConstructAt(DataStoreType *pos, const T &value, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using DP1 = typename eastl::decay<P1>::type;
    using EXT = ExtendedVariant2<DT, DP0, DP1>;
    using UsedTypeHandler = TypeHandler<EXT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<EXT, ThisTypesPack>::value);
    UsedTypeHandler::copyConstruct(m, value, p0, p0_count, p1, p1_count);
  }

  template <typename T, typename P0, typename P1>
  static void moveConstructAt(DataStoreType *pos, T &&value, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using DP1 = typename eastl::decay<P1>::type;
    using EXT = ExtendedVariant2<DT, DP0, DP1>;
    using UsedTypeHandler = TypeHandler<EXT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<EXT, ThisTypesPack>::value);
    UsedTypeHandler::moveContruct(m, eastl::forward<DT>(value), p0, p0_count, p1, p1_count);
  }

  template <typename T, typename P0, typename G0>
  static void copyConstructDataGeneratorAt(DataStoreType *pos, const T &value, size_t p0_count, G0 g0)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using UsedTypeHandler = TypeHandler<ExtendedVariant<DT, DP0>, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<ExtendedVariant<DT, DP0>, ThisTypesPack>::value);
    UsedTypeHandler::copyConstructDataGenerator(m, value, p0_count, g0);
  }

  template <typename T, typename P0, typename G0>
  static void moveConstructDataGeneratorAt(DataStoreType *pos, T &&value, size_t p0_count, G0 g0)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using UsedTypeHandler = TypeHandler<ExtendedVariant<DT, DP0>, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<ExtendedVariant<DT, DP0>, ThisTypesPack>::value);
    UsedTypeHandler::moveContructDataGenerator(m, eastl::forward<DT>(value), p0_count, g0);
  }

  template <typename T>
  static size_t calculateTotalSpaceForTypeSize()
  {
    using DT = typename eastl::decay<T>::type;
    return TypeMemoryRequirement<IndexType>::value + TypeMemoryRequirement<T>::value;
  }

  template <typename T, typename P0>
  static size_t calculateTotalSpaceForExtenedVariant(size_t p0_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    return TypeMemoryRequirement<IndexType>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<DT>::value +
           TypeMemoryRequirement<DP0>::array_of(p0_count);
  }

  template <typename T, typename P0, typename P1>
  static size_t calculateTotalSpaceForExtenedVariant2(size_t p0_count, size_t p1_count)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using DP1 = typename eastl::decay<P1>::type;
    return TypeMemoryRequirement<IndexType>::value + TypeMemoryRequirement<size_t>::value + TypeMemoryRequirement<size_t>::value +
           TypeMemoryRequirement<DT>::value + TypeMemoryRequirement<DP0>::array_of(p0_count) +
           TypeMemoryRequirement<DP1>::array_of(p1_count);
  }

  template <typename T, typename P0, typename P1, typename G>
  static void moveConstructExtended2DataGenerator(DataStoreType *pos, T &&value, size_t p0_count, G g)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using DP1 = typename eastl::decay<P1>::type;
    using EXT = ExtendedVariant2<DT, DP0, DP1>;

    using UsedTypeHandler = TypeHandler<EXT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<EXT, ThisTypesPack>::value);
    UsedTypeHandler::moveContructDataGenerator(m, &value, p0_count, g);
  }

  template <typename T, typename P0, typename P1, typename G>
  static void copyConstructExtended2DataGenerator(DataStoreType *pos, const T &value, size_t p0_count, G g)
  {
    using DT = typename eastl::decay<T>::type;
    using DP0 = typename eastl::decay<P0>::type;
    using DP1 = typename eastl::decay<P1>::type;
    using EXT = ExtendedVariant2<DT, DP0, DP1>;

    using UsedTypeHandler = TypeHandler<EXT, true>;
    // Prevents creation of meta types
    G_STATIC_ASSERT(UsedTypeHandler::is_valid);
    MemoryWriter m{pos};
    m.write<IndexType>(TypeIndexOf<EXT, ThisTypesPack>::value);
    UsedTypeHandler::copyConstructDataGenerator(m, &value, p0_count, g);
  }
};

namespace drv3d_dx12
{
#if _TARGET_PC_WIN
extern eastl::add_pointer_t<decltype(::WaitOnAddress)> WaitOnAddress;
extern eastl::add_pointer_t<decltype(::WakeByAddressAll)> WakeByAddressAll;
extern eastl::add_pointer_t<decltype(::WakeByAddressSingle)> WakeByAddressSingle;
#endif
} // namespace drv3d_dx12

// With c++20 atomics have wait, notify_one and notify_all to wait on value changes
// this does this by polling with gradually giving up more cpu cycles to the system.
// Makes use of WaitOnAddress (win8+), WakeByAddressSingle and WakeByAddressAll when available.
template <typename T>
inline void wait(std::atomic<T> &adr, T value, std::memory_order order = std::memory_order_seq_cst)
{
#if _TARGET_PC_WIN
  using drv3d_dx12::WaitOnAddress;
  if (WaitOnAddress) //-V516
#endif
  {
    G_STATIC_ASSERT(sizeof(std::atomic<T>) == sizeof(T));
    WaitOnAddress(&adr, &value, sizeof(T), INFINITE);
  }
#if _TARGET_PC_WIN
  else
  {
    for (uint32_t i = 0;; ++i)
    {
      if (adr.load(order) != value)
      {
        break;
      }
      if (i < 16)
      {
        continue;
      }
      if (i < 64)
      {
        SwitchToThread();
        continue;
      }
      Sleep(2);
    }
  }
#else
  G_UNUSED(order);
#endif
}

template <typename T>
inline void notify_one(std::atomic<T> &adr)
{
#if _TARGET_PC_WIN
  using drv3d_dx12::WakeByAddressSingle;
  if (WakeByAddressSingle) //-V516
#endif
  {
    WakeByAddressSingle(&adr);
  }
}

template <typename T>
inline void notify_all(std::atomic<T> &adr)
{
#if _TARGET_PC_WIN
  using drv3d_dx12::WakeByAddressAll;
  if (WakeByAddressAll) //-V516
#endif
  {
    WakeByAddressAll(&adr);
  }
}

template <typename, typename>
class ConcurrentVariantRingBufferT;

template <typename... Types, typename IType>
class ConcurrentVariantRingBufferT<TypePack<Types...>, IType> : public VariantContainerBase<TypePack<Types...>, IType>
{
public:
  /// Controls how consumers may be notified about changes to this buffer
  enum class ConsumerNotificationPolicy
  {
    /// Never notify, immediate consumption by the consumers is not required
    Never,
    /// Notify when the producer knows that consumers are waiting
    /// NOTE: This may result in no notification even when it might seem it should, as a consumer just entered wait state after the
    /// producer checked if any consumer is waiting
    WhenWaiting,
    /// Always notify, this ensures that a consumer will start consuming right away
    /// NOTE: This may be expensive and should only be used when really needed
    Always,
  };

private:
  using BaseType = VariantContainerBase<TypePack<Types...>, IType>;
  using ThisTypesPack = typename BaseType::ThisTypesPack;
  using ThisType = ConcurrentVariantRingBufferT<ThisTypesPack, IType>;
  using DataStoreType = typename BaseType::DataStoreType;
  using IndexType = typename BaseType::IndexType;
  using DestructorType = typename BaseType::DestructorType;
  using CopyType = typename BaseType::CopyType;
  using MoveType = typename BaseType::MoveType;
  using QuerySizeType = typename BaseType::QuerySizeType;
  template <typename A, bool B>
  using TypeHandler = typename BaseType::template TypeHandler<A, B>;

  using BaseType::callAt;
  using BaseType::callAtAndDestroy;
  using BaseType::copyConstructAt;
  using BaseType::copyConstructInto;
  using BaseType::destroyAt;
  using BaseType::moveConstructAt;
  using BaseType::moveConstructInto;
  using BaseType::num_types;
  using BaseType::sizeAt;

  // with c++ 17 std lib would have standard defined values
  static constexpr size_t cache_line_size = 64;
  // Keep wrong value for now, 64 is sufficient - according to every thing about this topic, not 2x...
  static constexpr size_t false_sharing_avoidance_alignment = cache_line_size * 2;

  // align to cache line size to avoid false sharing
  alignas(false_sharing_avoidance_alignment) size_t size{0};
  // Memory allocation/reservation counter, memory allocated but not committed is in flight and
  // still be written to by someone
  alignas(false_sharing_avoidance_alignment) std::atomic<size_t> allocationCount{0};
  // Memory committed counter, this is how much memory is ready for consumption
  alignas(false_sharing_avoidance_alignment) std::atomic<size_t> committedCount{0};
  // Free memory counter, consumer adds memory that it consumed to this
  alignas(false_sharing_avoidance_alignment) std::atomic<size_t> freeCount{0};
  // if a consumer enters the wait queue this will be greater than zero, producers will then notify the consumers on next commit
  alignas(false_sharing_avoidance_alignment) std::atomic<size_t> consumerWaiting{0};
  alignas(false_sharing_avoidance_alignment) eastl::unique_ptr<DataStoreType[]> store;

  size_t calculateFreeSize(size_t alloc_pos, size_t free_pos) const
  {
    size_t use = 0;
    if (free_pos <= alloc_pos)
    {
      use = alloc_pos - free_pos;
    }
    else
    {
      // distance from pre wrap to wrap point plus regular (frePos is implicit 0 so just alloc_pos)
      use = (~size_t(0) - free_pos) + alloc_pos;
    }
    return size - use;
  }

  // On ring mode this will wait on freeCount until there is enough free space to provide "sz" bytes
  DAGOR_NOINLINE
  void waitForFree(size_t freePos)
  {
    notify_one(committedCount); // wake up thread just in case we are in batch mode
    // wait until free changes
    TIME_PROFILE_WAIT_DEV("DX12_waitForFree");
    wait(freeCount, freePos);
    // could read allocationCount again, but if we are lucky and it didn't change during wait we save one atomic read
  }

  __forceinline size_t allocateSpace(size_t sz)
  {
    G_FAST_ASSERT(sz < size);

    auto currentAllocationCount = allocationCount.load(std::memory_order_acquire);
    // start with relaxed load. as freePos won't decrease, thus we can only get false-negative
    // which will be fixed with next loop iteration
    // optimizes happy path
    auto freePos = freeCount.load(std::memory_order_relaxed);
    for (;;)
    {
      if (DAGOR_LIKELY(calculateFreeSize(currentAllocationCount, freePos) >= sz))
      {
        auto nextAllocationCount = currentAllocationCount + sz;

        if (allocationCount.compare_exchange_strong(currentAllocationCount, nextAllocationCount))
        {
          break;
        }
      }
      else
      {
        waitForFree(freePos);
      }
      freePos = freeCount.load(std::memory_order_acquire);
    }
    return currentAllocationCount;
  }

  __forceinline void commitSpace(size_t at, size_t sz, ConsumerNotificationPolicy notification_policy)
  {
    auto committedIndex = at;
    auto nextCommited = committedIndex + sz;
    while (!committedCount.compare_exchange_strong(committedIndex, nextCommited))
    {
      committedIndex = at;
    }
    if (ConsumerNotificationPolicy::Never == notification_policy)
    {
      return;
    }
    if (ConsumerNotificationPolicy::WhenWaiting == notification_policy)
    {
      if (0 == consumerWaiting.load(std::memory_order_relaxed))
      {
        return;
      }
    }
    notify_one(committedCount);
  }

  struct VisitableRange
  {
    size_t from;
    size_t to;

    VisitableRange &operator+=(size_t adv)
    {
      from += adv;
      return *this;
    }
    explicit operator bool() const { return from != to; }
    size_t index(size_t sz) const { return from % sz; }
  };

  __forceinline VisitableRange getVisitableRange() const
  {
    return {freeCount.load(std::memory_order_relaxed), committedCount.load(std::memory_order_acquire)};
  }

  __forceinline void updateFree(const VisitableRange &at)
  {
    freeCount.store(at.from, std::memory_order_release);
    notify_all(freeCount);
  }

public:
  ConcurrentVariantRingBufferT() = default;
  ConcurrentVariantRingBufferT(const ConcurrentVariantRingBufferT &) = delete;
  // could be implemented, but no need right now
  ConcurrentVariantRingBufferT(ConcurrentVariantRingBufferT &&) = delete;

  ConcurrentVariantRingBufferT &operator=(const ConcurrentVariantRingBufferT &) = delete;
  ConcurrentVariantRingBufferT &operator=(ConcurrentVariantRingBufferT &&) = delete;

  ~ConcurrentVariantRingBufferT() = default;

  // Works on any mode but is not thread safe, it will destroy all active objects when this function
  // is called. Afterwards it will reset all internal counters to 0, concurrent additions during
  // clear will be lost!
  void clear()
  {
    auto range = getVisitableRange();
    while (range)
    {
      range += destroyAt(&store[range.index(size)]);
    }
    committedCount.store(0, std::memory_order_relaxed);
    allocationCount.store(0, std::memory_order_relaxed);
    freeCount.store(0, std::memory_order_relaxed);
  }

  // If operation mode is vector then its a normal vector growing operation, with object migration
  // to new store If operation mode is ring then this requires the ring, this requires that the ring
  // is empty otherwise the resize will be ignored Both appendix_numerator and appendix_denominator
  // have no effect in vector operation mode.
  void resize(size_t ring_size, size_t appendix_numerator = 1, size_t appendix_denominator = 1)
  {
    G_ASSERTF(appendix_numerator > 0, "A memory appendix has to be allocated to handle wrapping "
                                      "correctly!");
    G_ASSERTF(appendix_denominator > 0, "Denominator has to be greater than zero or division by "
                                        "zero!");
    G_ASSERTF(allocationCount == freeCount && committedCount == freeCount, "You can not resize a "
                                                                           "ring buffer with "
                                                                           "active objects in "
                                                                           "it");
    if (allocationCount != freeCount || committedCount != freeCount)
    {
      return;
    }

    // We add extra appendix memory, which is used when an object needs to be stored that is
    // bigger as the rest of available memory before we need to wrap around. With this we can
    // store the whole object as normal and the wrapping logic still works without problems as we
    // only index into the front of each element.
    store = eastl::make_unique<DataStoreType[]>(ring_size + (ring_size * appendix_numerator) / appendix_denominator);
    size = ring_size;
  }

  template <typename T>
  void pushBack(T &&v, ConsumerNotificationPolicy notification_policy)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForTypeSize<T>();

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructAt<T>(&store[allocResult % size], eastl::forward<T>(v));

    commitSpace(allocResult, sizeNeeded, notification_policy);
  }

  template <typename U>
  void visitAll(U &&visitor)
  {
    auto range = getVisitableRange();
    while (range)
    {
      range += callAt(&store[range.index(size)], visitor);
    }
  }

  // Visits as many entries as possible, return the number of visitations
  template <typename U>
  uint32_t tryVisitAllAndDestroy(U &&visitor)
  {
    auto range = getVisitableRange();
    if (!range)
    {
      return 0;
    }
    uint32_t count = 0;
    for (; range; ++count)
    {
      range += callAtAndDestroy(&store[range.index(size)], visitor);
    }
    updateFree(range);
    return count;
  }

  // Tries to visit next object.
  // If it can't find anything to visit it returns false.
  // It will skip extra data.
  // After visitor finishes the object and all seen extra data is destroyed and the memory is
  // available again for ring use.
  template <typename U>
  bool tryVisitAndDestroy(U &&visitor)
  {
    auto range = getVisitableRange();
    if (range)
    {
      range = callAtAndDestroy(&store[range.index(size)], eastl::forward<U>(visitor));
      updateFree(range);
      return true;
    }
    return false;
  }

  void waitToVisit()
  {
#if LOCK_PROFILER_ENABLED
    using namespace da_profiler;
    static desc_id_t dx12WaitDesc = add_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, IsWait, "DX12_waitToVisit");
    ScopeLockProfiler<da_profiler::NoDesc> lp(dx12WaitDesc);
#endif
    ++consumerWaiting;
    wait(committedCount, freeCount.load(std::memory_order_relaxed));
    --consumerWaiting;
  }

  /// Waits until there are no commands pending for execution.
  void waitUntilEmpty()
  {
    auto currentFreeCount = freeCount.load(std::memory_order_relaxed);
    while (currentFreeCount != committedCount.load(std::memory_order_relaxed))
    {
      // ensure the consumers are not waiting while we are waiting on consumers to making progress, otherwise we deadlock waiting for
      // consumers that are waiting to be notified
      notify_one(committedCount);
      wait(freeCount, currentFreeCount);
      currentFreeCount = freeCount.load(std::memory_order_relaxed);
    }
  }

  template <typename T, typename P0>
  void pushBack(T &&v, const P0 *p0, size_t p0_count, ConsumerNotificationPolicy notification_policy)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructAt<T, P0>(&store[allocResult % size], eastl::forward<T>(v), p0, p0_count);

    commitSpace(allocResult, sizeNeeded, notification_policy);
  }

  template <typename T, typename G0>
  void pushBack(T &&v, size_t p0_count, G0 g0, ConsumerNotificationPolicy notification_policy)
  {
    using P0 = decltype(g0(0));
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructDataGeneratorAt<T, P0, G0>(&store[allocResult % size], eastl::forward<T>(v), p0_count, g0);

    commitSpace(allocResult, sizeNeeded, notification_policy);
  }

  template <typename T, typename P0, typename P1>
  void pushBack(const T &v, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count,
    ConsumerNotificationPolicy notification_policy)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant2<T, P0, P1>(p0_count, p1_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructAt<T, P0, P1>(&store[allocResult % size], v, p0, p0_count, p1, p1_count);

    commitSpace(allocResult, sizeNeeded, notification_policy);
  }

  // special variant, creates two extra data segments with equal size and g generates entry "i" of
  // both ranges at the same time
  template <typename T, typename P0, typename P1, typename G>
  void pushBack(const T &v, size_t p0_count, G g, ConsumerNotificationPolicy notification_policy)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant2<T, P0, P1>(p0_count, p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructExtended2DataGenerator<T, P0, P1>(&store[allocResult % size], v, p0_count, g);

    commitSpace(allocResult, sizeNeeded, notification_policy);
  }
};

template <typename T>
using ConcurrentVariantRingBuffer = ConcurrentVariantRingBufferT<T, typename SelectIndexType<T>::Type>;

template <typename, typename>
class VariantStreamBufferT;

// A buffer optimized to stream into continuously, optimized for quick writing
// Currently this behaves like a vector with exponential growth.
// TODO Use better internal data structure optimized for fast allocation.
template <typename... Types, typename IType>
class VariantStreamBufferT<TypePack<Types...>, IType> : public VariantContainerBase<TypePack<Types...>, IType>
{
private:
  using BaseType = VariantContainerBase<TypePack<Types...>, IType>;
  using ThisTypesPack = typename BaseType::ThisTypesPack;
  using ThisType = VariantStreamBufferT<ThisTypesPack, IType>;
  using DataStoreType = typename BaseType::DataStoreType;
  using IndexType = typename BaseType::IndexType;
  using DestructorType = typename BaseType::DestructorType;
  using CopyType = typename BaseType::CopyType;
  using MoveType = typename BaseType::MoveType;
  using QuerySizeType = typename BaseType::QuerySizeType;
  template <typename A, bool B>
  using TypeHandler = typename BaseType::template TypeHandler<A, B>;

  using BaseType::callAt;
  using BaseType::copyConstructAt;
  using BaseType::copyConstructInto;
  using BaseType::destroyAt;
  using BaseType::moveConstructAt;
  using BaseType::moveConstructInto;
  using BaseType::num_types;
  using BaseType::sizeAt;

  size_t size{0};
  size_t allocatedCount{0};
  eastl::unique_ptr<DataStoreType[]> store;

  DataStoreType *allocateSpace(size_t alloc_size)
  {
    size_t remain = size - allocatedCount;
    if (remain < alloc_size)
    {
      resize(max(size + alloc_size, size * 2));
    }
    return &store[allocatedCount];
  }

  void commitSpace(DataStoreType *at, size_t count) { allocatedCount = (at - &store[0]) + count; }

public:
  struct StreamPosition
  {
    DataStoreType *pos;
    operator DataStoreType *() const { return pos; }
    StreamPosition operator+(size_t o) const { return {pos + o}; }
    bool operator==(const StreamPosition &) const = default;
    bool operator!=(const StreamPosition &) const = default;
    bool operator<=(const StreamPosition &) const = default;
    bool operator>=(const StreamPosition &) const = default;
    bool operator<(const StreamPosition &) const = default;
    bool operator>(const StreamPosition &) const = default;
  };

private:
  StreamPosition getStreamStart() { return {allocatedCount ? store.get() : nullptr}; }

  template <typename T>
  StreamPosition visitAndAdvance(StreamPosition at, T clb)
  {
    auto np = at + clb(at);
    return {np.pos < (getStreamStart() + allocatedCount).pos ? np : nullptr};
  }

public:
  VariantStreamBufferT() = default;
  VariantStreamBufferT(const VariantStreamBufferT &) = delete;
  // could be implemented, but no need right now
  VariantStreamBufferT(VariantStreamBufferT &&) = delete;

  VariantStreamBufferT &operator=(const VariantStreamBufferT &) = delete;
  VariantStreamBufferT &operator=(VariantStreamBufferT &&) = delete;

  ~VariantStreamBufferT() = default;

  // Will destroy all active objects, does not free object store memory
  // To free all memory use resize(0).
  void clear()
  {
    for (auto at = getStreamStart(); at; at = visitAndAdvance(at, [this](auto pos) { return this->destroyAt(pos); })) {}
    allocatedCount = 0;
  }

  void resize(size_t new_size)
  {
    if (size == new_size)
    {
      return;
    }

    eastl::unique_ptr<DataStoreType[]> newStore;
    size_t pos = 0;
    if (new_size > 0)
    {
      newStore.reset(new DataStoreType[new_size]);
      if (new_size > size)
      {
        while (pos < allocatedCount)
        {
          pos += moveConstructInto(&store[pos], &newStore[pos]);
        }
      }
      else
      {
        // when we shrink we need to look if the next object would go over the new size
        // if yes, we stop. The later clear will clean up all non moved objects.
        while (pos < allocatedCount)
        {
          auto objSize = sizeAt(&store[pos]);
          if (objSize + pos > new_size)
          {
            break;
          }
          pos += moveConstructInto(&store[pos], &newStore[pos]);
        }
      }
    }
    clear();
    store.reset(newStore.release());
    // allocatedCount is set to 0 by clear, so have to always assign pos
    allocatedCount = pos;
    size = new_size;
  }

  template <typename T>
  void pushBack(const T &v)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForTypeSize<T>();

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructAt<T>(allocResult, v);

    commitSpace(allocResult, sizeNeeded);
  }

  template <typename T>
  void pushBack(T &&v)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForTypeSize<T>();

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructAt<T>(allocResult, eastl::forward<T>(v));

    commitSpace(allocResult, sizeNeeded);
  }

  // Visits all objects, keeps them intact for later visit
  template <typename U>
  void visit(U &&visitor)
  {
    for (auto at = getStreamStart(); at; at = visitAndAdvance(at, [this, &visitor](auto pos) { return this->callAt(pos, visitor); }))
    {}
  }

  StreamPosition begin() { return getStreamStart(); }
  StreamPosition next(StreamPosition at)
  {
    if (at)
    {
      at = visitAndAdvance(at, [this](auto pos) { return this->sizeAt(pos); });
    }
    return at;
  }

  template <typename U>
  StreamPosition skipUntil(StreamPosition at, U &&terminator)
  {
    while (at)
    {
      bool endReached = false;
      auto newPos = visitAndAdvance(at, [this, &terminator, &endReached](auto pos) {
        return this->callAt(pos, [&terminator, &endReached](auto &obj) { endReached = terminator(obj); });
      });
      if (endReached)
      {
        break;
      }
      at = newPos;
    }
    return at;
  }

  template <typename U>
  void visitRange(StreamPosition from, StreamPosition to, U &&visitor)
  {
    for (; from && from != to; from = visitAndAdvance(from, [this, &visitor](auto pos) { return this->callAt(pos, visitor); })) {}
  }


  // Like visit but destroys all objects, after all visits the container has the same state as after its clear method was invoked
  template <typename U>
  void consume(U &&visitor)
  {
    for (auto at = getStreamStart(); at;
         at = visitAndAdvance(at, [this, &visitor](auto pos) { return this->callAtAndDestroy(pos, visitor); }))
    {}
  }

  template <typename T, typename P0>
  void pushBack(const T &v, const P0 *p0, size_t p0_count)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructAt<T, P0>(allocResult, v, p0, p0_count);

    commitSpace(allocResult, sizeNeeded);
  }

  template <typename T, typename P0>
  void pushBack(T &&v, P0 *p0, size_t p0_count)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructAt<T, P0>(allocResult, eastl::forward<T>(v), p0, p0_count);

    commitSpace(allocResult, sizeNeeded);
  }

  template <typename T, typename G0>
  void pushBack(const T &v, size_t p0_count, G0 g0)
  {
    using P0 = decltype(g0(0));
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructDataGeneratorAt<T, P0, G0>(allocResult, eastl::move(v), p0_count, g0);

    commitSpace(allocResult, sizeNeeded);
  }

  template <typename T, typename G0>
  void pushBack(T &&v, size_t p0_count, G0 g0)
  {
    using P0 = decltype(g0(0));
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant<T, P0>(p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template moveConstructDataGeneratorAt<T, P0, G0>(allocResult, eastl::forward<T>(v), p0_count, g0);

    commitSpace(allocResult, sizeNeeded);
  }

  template <typename T, typename P0, typename P1>
  void pushBack(const T &v, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant2<T, P0, P1>(p0_count, p1_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructAt<T, P0, P1>(allocResult, v, p0, p0_count, p1, p1_count);

    commitSpace(allocResult, sizeNeeded);
  }

  // special variant, creates two extra data segments with equal size and g generates entry "i" of
  // both ranges at the same time
  template <typename T, typename P0, typename P1, typename G>
  void pushBack(const T &v, size_t p0_count, G g)
  {
    auto sizeNeeded = this->template calculateTotalSpaceForExtenedVariant2<T, P0, P1>(p0_count, p0_count);

    auto allocResult = allocateSpace(sizeNeeded);

    this->template copyConstructExtended2DataGenerator<T, P0, P1>(allocResult, v, p0_count, g);

    commitSpace(allocResult, sizeNeeded);
  }
};

template <typename T>
using VariantStreamBuffer = VariantStreamBufferT<T, typename SelectIndexType<T>::Type>;
