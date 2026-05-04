//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/internal/function_detail.h>
#include <EASTL/unique_ptr.h>
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
 * \brief A class template for a heap-allocated move-only function object.
 * It works a tiny bit faster than eastl::function, does not
 * violate const-correctness and does not require the function object
 * to be copyable.
 * \details Use MoveOnlyFunction<void() const> for a const
 * callable function object and MoveOnlyFunction<void()> for
 * a non-const callable one.
 *
 * \tparam Signature The type-erased signature of the function.
 */
template <typename Signature>
class MoveOnlyFunction;

template <typename Signature>
class FunctionRef;

namespace detail
{

template <typename F>
static void GCC_USED relocateImpl(eastl::unique_ptr<char[]> &from, eastl::unique_ptr<char[]> *to)
{
  if (to != nullptr)
  {
    to->reset(new char[sizeof(F)]);
    new (to->get(), _NEW_INPLACE) F{eastl::move(*reinterpret_cast<F *>(from.get()))};
  }

  reinterpret_cast<F *>(from.get())->~F();
  from.reset();
}

template <typename CallSignature>
struct MoveOnlyFunctionBase
{
  using ManagerSignature = void(eastl::unique_ptr<char[]> &, eastl::unique_ptr<char[]> *);

  // Note that we intentionally leave fields uninitialized.
  MoveOnlyFunctionBase() : call{nullptr} {}; //-V730

  template <typename F>
  MoveOnlyFunctionBase(F &&func_object) : call{nullptr}
  {
    using UnqualF = eastl::decay_t<F>;

    relocate = &relocateImpl<UnqualF>;
    storage.reset(new char[sizeof(UnqualF)]);
    new (storage.get(), _NEW_INPLACE) UnqualF{eastl::forward<F>(func_object)};
  }

  MoveOnlyFunctionBase(const MoveOnlyFunctionBase &) = delete;
  MoveOnlyFunctionBase &operator=(const MoveOnlyFunctionBase &) = delete;

  MoveOnlyFunctionBase(MoveOnlyFunctionBase &&other) : call{other.call}, relocate{other.relocate} //-V1077
  {
    if (other.call == nullptr)
      return;

    relocate(other.storage, &storage);
    other.call = nullptr;
  }

  MoveOnlyFunctionBase &operator=(MoveOnlyFunctionBase &&other)
  {
    if (this == &other)
      return *this;

    if (call != nullptr)
      relocate(storage, nullptr);

    call = other.call;

    if (other.call == nullptr)
      return *this;

    relocate = other.relocate;
    relocate(other.storage, &storage);

    other.call = nullptr;

    return *this;
  }

  ~MoveOnlyFunctionBase()
  {
    if (call != nullptr)
      relocate(storage, nullptr);
  }

  void reset()
  {
    if (call != nullptr)
      relocate(storage, nullptr);

    call = nullptr;
  }

  explicit operator bool() const { return call != nullptr; }

  // call should be hot
  CallSignature *call;
  ManagerSignature *relocate;
  eastl::unique_ptr<char[]> storage;
};

template <typename>
struct IsMoveOnlyFunction : public eastl::false_type
{};

template <typename Signature>
struct IsMoveOnlyFunction<MoveOnlyFunction<Signature>> : public eastl::true_type
{};

template <typename T>
constexpr bool is_move_only_function_v = IsMoveOnlyFunction<T>::value;

} // namespace detail

template <typename Ret, typename... Args>
class MoveOnlyFunction<Ret(Args...)> : private detail::MoveOnlyFunctionBase<Ret(void *, Args...)>
{
  using Base = detail::MoveOnlyFunctionBase<Ret(void *, Args...)>;

  friend class FunctionRef<Ret(Args...)>;

  template <typename F>
  static Ret callImpl(void *func_object, Args... args)
  {
    return eastl::invoke(*static_cast<eastl::decay_t<F> *>(func_object), ((Args &&)args)...);
  }

  using Base::call;
  using Base::storage;

public:
  MoveOnlyFunction() = default;

  MoveOnlyFunction(std::nullptr_t) : Base() {}

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, MoveOnlyFunction),
    typename = eastl::disable_if_t<detail::is_move_only_function_v<eastl::decay_t<F>>>>
  MoveOnlyFunction(F &&func_object) : Base((F &&)func_object)
  {
    call = &callImpl<F>;
  }

  MoveOnlyFunction &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }
  using Base::operator bool;

  Ret operator()(Args... args)
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty FixedMoveOnlyFunction!");
    return call(storage.get(), ((Args &&)args)...);
  }
};

template <typename Ret, typename... Args>
class MoveOnlyFunction<Ret(Args...) const> : private detail::MoveOnlyFunctionBase<Ret(void const *, Args...)>
{
  using Base = detail::MoveOnlyFunctionBase<Ret(void const *, Args...)>;

  friend class FunctionRef<Ret(Args...) const>;

  template <typename F>
  static Ret callImpl(void const *func_object, Args... args)
  {
    // The "const" word on the next line is central to preserving const-correctness.
    return eastl::invoke(*static_cast<eastl::decay_t<F> const *>(func_object), ((Args &&)args)...);
  }

  using Base::call;
  using Base::storage;

public:
  MoveOnlyFunction() : Base() {}

  MoveOnlyFunction(std::nullptr_t) : Base() {}

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, MoveOnlyFunction),
    typename = eastl::disable_if_t<detail::is_move_only_function_v<eastl::decay_t<F>>>>
  MoveOnlyFunction(F &&func_object) : Base((F &&)func_object)
  {
    call = &callImpl<F>;
  }

  MoveOnlyFunction &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }
  using Base::operator bool;

  Ret operator()(Args... args) const
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty MoveOnlyFunction!");
    return call(storage.get(), ((Args &&)args)...);
  }
};

} // namespace dag

#undef GCC_USED