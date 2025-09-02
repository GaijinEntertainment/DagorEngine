//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/internal/function_detail.h>
#include <debug/dag_assert.h>

#ifndef GCC_USED
#if defined(__GNUC__) && !defined(__clang__)
#define GCC_USED __attribute__((used))
#else
#define GCC_USED
#endif
#endif

namespace dag
{

/**
 * \brief A class template for a fixed-size move-only function object.
 * It works a tiny bit faster than eastl::fixed_function, does not
 * violate const-correctness and does not require the function object
 * to be copyable.
 * \details Use FixedMoveOnlyFunction<16, void() const> for a const
 * callable function object and FixedMoveOnlyFunction<void(), 16> for
 * a non-const callable one.
 *
 * \tparam size The size of the in-place storage in bytes.
 * \tparam Signature The type-erased signature of the function.
 */
template <size_t size, typename Signature>
class FixedMoveOnlyFunction;

template <typename Signature>
class FunctionRef;

namespace detail
{

template <typename F>
static void GCC_USED relocateImpl(void *from, void *to)
{
  if (to != nullptr)
    new (to, _NEW_INPLACE) F{(F &&)*static_cast<F *>(from)};

  static_cast<F *>(from)->~F();
}

template <size_t size, typename CallSignature>
struct FixedMoveOnlyFunctionBase
{
  using ManagerSignature = void(void *, void *);

  static constexpr size_t align = 16;

  // Note that we intentionally leave fields uninitialized.
  FixedMoveOnlyFunctionBase() : call{nullptr} {}; //-V730

  template <typename F>
  FixedMoveOnlyFunctionBase(F &&func_object) : call{nullptr}
  {
    using UnqualF = eastl::remove_cvref_t<F>;

    static_assert(sizeof(UnqualF) <= size, "Function object is too big!");
    static_assert(alignof(UnqualF) <= align, "Function object over-aligned!");

    relocate = &relocateImpl<UnqualF>;
    new (storage) UnqualF{(F &&)func_object};
  }

  FixedMoveOnlyFunctionBase(const FixedMoveOnlyFunctionBase &) = delete;
  FixedMoveOnlyFunctionBase &operator=(const FixedMoveOnlyFunctionBase &) = delete;

  FixedMoveOnlyFunctionBase(FixedMoveOnlyFunctionBase &&other) : call{other.call}, relocate{other.relocate} //-V1077
  {
    if (other.call == nullptr)
      return;

    relocate(&other.storage, storage);
    other.call = nullptr;
  }

  FixedMoveOnlyFunctionBase &operator=(FixedMoveOnlyFunctionBase &&other)
  {
    if (this == &other)
      return *this;

    if (call != nullptr)
      relocate(&storage, nullptr);

    call = other.call;

    if (other.call == nullptr)
      return *this;

    relocate = other.relocate;
    relocate(&other.storage, storage);

    other.call = nullptr;

    return *this;
  }

  ~FixedMoveOnlyFunctionBase()
  {
    if (call != nullptr)
      relocate(&storage, nullptr);
  }

  void reset()
  {
    if (call != nullptr)
      relocate(&storage, nullptr);

    call = nullptr;
  }

  explicit operator bool() const { return call != nullptr; }

  // call should be hot
  CallSignature *call;
  ManagerSignature *relocate;
  // On win32, we still want this to be aligned to 16 bytes
  alignas(align) char storage[size];
};

template <typename>
struct IsFixedMoveOnlyFunction : public eastl::false_type
{};

template <size_t size, typename Signature>
struct IsFixedMoveOnlyFunction<FixedMoveOnlyFunction<size, Signature>> : public eastl::true_type
{};

template <typename T>
constexpr bool is_fixed_move_only_function_v = IsFixedMoveOnlyFunction<T>::value;

} // namespace detail

template <size_t size, typename Ret, typename... Args>
class FixedMoveOnlyFunction<size, Ret(Args...)> : private detail::FixedMoveOnlyFunctionBase<size, Ret(void *, Args...)>
{
  using Base = detail::FixedMoveOnlyFunctionBase<size, Ret(void *, Args...)>;

  friend class FunctionRef<Ret(Args...)>;

  template <typename F>
  static Ret callImpl(void *func_object, Args... args)
  {
    return eastl::invoke(*static_cast<F *>(func_object), ((Args &&)args)...);
  }

  using Base::call;
  using Base::storage;

public:
  FixedMoveOnlyFunction() = default;

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, FixedMoveOnlyFunction),
    typename = eastl::disable_if_t<detail::is_fixed_move_only_function_v<eastl::decay_t<F>>>>
  FixedMoveOnlyFunction(F &&func_object) : Base((F &&)func_object)
  {
    call = &callImpl<eastl::decay_t<F>>;
  }

  FixedMoveOnlyFunction &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }
  using Base::operator bool;

  Ret operator()(Args... args)
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty FixedMoveOnlyFunction!");
    return call(storage, ((Args &&)args)...);
  }
};

template <size_t size, typename Ret, typename... Args>
class FixedMoveOnlyFunction<size, Ret(Args...) const> : private detail::FixedMoveOnlyFunctionBase<size, Ret(void const *, Args...)>
{
  using Base = detail::FixedMoveOnlyFunctionBase<size, Ret(void const *, Args...)>;

  friend class FunctionRef<Ret(Args...) const>;

  template <typename F>
  static Ret callImpl(void const *func_object, Args... args)
  {
    // The "const" word on the next line is central to preserving const-correctness.
    return eastl::invoke(*static_cast<F const *>(func_object), ((Args &&)args)...);
  }

  using Base::call;
  using Base::storage;

public:
  FixedMoveOnlyFunction() : Base() {}

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, FixedMoveOnlyFunction),
    typename = eastl::disable_if_t<detail::is_fixed_move_only_function_v<eastl::decay_t<F>>>>
  FixedMoveOnlyFunction(F &&func_object) : Base((F &&)func_object)
  {
    call = &callImpl<eastl::decay_t<F>>;
  }

  FixedMoveOnlyFunction &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }
  using Base::operator bool;

  Ret operator()(Args... args) const
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty FixedMoveOnlyFunction!");
    return call(storage, ((Args &&)args)...);
  }
};

} // namespace dag

#undef GCC_USED