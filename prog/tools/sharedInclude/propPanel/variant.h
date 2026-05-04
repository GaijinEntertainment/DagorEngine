//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <EASTL/functional.h>
#include <EASTL/variant.h>
#include <EASTL/type_traits.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

namespace PropPanel
{
class Variant final
{
  struct GenericMem
  {
    eastl::unique_ptr<std::byte> ptr;
    eastl::function<void(std::byte *)> destructor;
  };

  template <typename... T>
  struct TypeList
  {
    using ExtendedVariant = eastl::variant<eastl::monostate, T..., GenericMem>;

    template <typename U>
    inline static constexpr bool Contains = eastl::disjunction_v<std::is_same<T, U>...>;
  };

  using BaseTypes = TypeList<bool, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::int8_t, std::int16_t, std::int32_t,
    std::int64_t, float, double, char, E3DCOLOR, Point2, Point3, Point4>;

public:
  Variant() noexcept : data(eastl::monostate{}) {}
  ~Variant() noexcept
  {
    if (eastl::holds_alternative<GenericMem>(data))
    {
      GenericMem &mem = eastl::get<GenericMem>(data);
      if (mem.ptr)
      {
        mem.destructor(mem.ptr.get());
      }
    }
  }

  template <typename T>
  Variant(T &&val)
  {
    using RawT = eastl::remove_cvref_t<T>;
    static_assert(!eastl::is_pointer_v<RawT>, "pointers not supported");

    if constexpr (BaseTypes::Contains<RawT>)
    {
      data = std::move(val);
    }
    else
    {
      GenericMem mem;
      mem.ptr.reset(new std::byte[sizeof(RawT)]);
      new (mem.ptr.get()) RawT(std::forward<T>(val));
      mem.destructor = [](std::byte *ptr) { reinterpret_cast<RawT *>(ptr)->~RawT(); };
      data = std::move(mem);
    }
  }

  Variant(const Variant &) = delete;
  Variant &operator=(const Variant &) = delete;

  Variant(Variant &&) noexcept = default;
  Variant &operator=(Variant &&) noexcept = default;

  bool isValid() const { return !eastl::holds_alternative<eastl::monostate>(data); }

  template <typename T>
  eastl::optional<T> convert() const
  {
    using RawT = std::remove_cvref_t<T>;

    if (!isValid())
    {
      return {};
    }

    if (eastl::holds_alternative<GenericMem>(data))
    {
      const GenericMem &mem = eastl::get<GenericMem>(data);
      return reinterpret_cast<T &>(*mem.ptr);
    }
    else
    {
      eastl::optional<T> extracted;
      eastl::visit(
        [&extracted](auto &value) {
          using TVar = eastl::remove_cvref_t<decltype(value)>;
          if constexpr (eastl::is_same_v<RawT, TVar>)
          {
            extracted = value;
          }
          else if constexpr (eastl::is_convertible_v<TVar, T> && eastl::is_assignable_v<T &, TVar>)
          {
            extracted.emplace(value);
          }
        },
        data);

      return extracted;
    }
  }

private:
  BaseTypes::ExtendedVariant data;
};
} // namespace PropPanel
