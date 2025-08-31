//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/internal/function_detail.h>
#include <debug/dag_assert.h>


namespace dag
{

/**
 * \brief A class template for a non-owning reference to a function object.
 * This preserves const-correctness w.r.t. the function object and is faster
 * than storing a reference to a dag::FixedMoveOnlyFunction.
 * \details Use FunctionRef<void() const> for a const callable function
 * object and FixedMoveOnlyFunction<void()> for a non-const callable one.
 *
 * \tparam Signature The type-erased signature of the function.
 */
template <typename Signature>
class FunctionRef;

template <size_t size, typename Signature>
class FixedMoveOnlyFunction;

namespace detail
{

template <typename CallSignature>
struct FunctionRefBase
{
  FunctionRefBase() : call{nullptr} {}; //-V730
  explicit FunctionRefBase(void *data_) : call{nullptr}, data{data_} {}

  FunctionRefBase(const FunctionRefBase &) = default;
  FunctionRefBase &operator=(const FunctionRefBase &) = default;
  FunctionRefBase(FunctionRefBase &&) = default;
  FunctionRefBase &operator=(FunctionRefBase &&) = default;
  ~FunctionRefBase() = default;

  void reset() { call = nullptr; }

  explicit operator bool() const { return call != nullptr; }

  // call should be hot
  CallSignature *call = nullptr;
  void *data;
};

template <typename>
struct IsFunctionRef : public eastl::false_type
{};

template <typename Signature>
struct IsFunctionRef<FunctionRef<Signature>> : public eastl::true_type
{};

template <typename T>
constexpr bool is_function_ref_v = IsFunctionRef<T>::value;

} // namespace detail

template <typename Ret, typename... Args>
class FunctionRef<Ret(Args...)> : private detail::FunctionRefBase<Ret(void *, Args...)>
{
  using Base = detail::FunctionRefBase<Ret(void *, Args...)>;

  template <typename F>
  static Ret callImpl(void *func_object, Args... args)
  {
    return eastl::invoke(*static_cast<F *>(func_object), ((Args &&)args)...);
  }

  using Base::call;
  using Base::data;

public:
  FunctionRef() = default;

  template <size_t size, typename Ret2, typename... Args2>
  FunctionRef(FixedMoveOnlyFunction<size, Ret2(Args2...)> &fmof) : Base(fmof.storage)
  {
    call = fmof.call;
  }

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, FunctionRef),
    typename = eastl::disable_if_t<detail::is_function_ref_v<eastl::decay_t<F>>>>
  FunctionRef(F &&func_object) : Base(&func_object)
  {
    call = &callImpl<eastl::decay_t<F>>;
  }

  FunctionRef &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }

  using Base::operator bool;

  Ret operator()(Args... args)
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty FunctionRef!");
    return call(data, ((Args &&)args)...);
  }
};

template <typename Ret, typename... Args>
class FunctionRef<Ret(Args...) const> : private detail::FunctionRefBase<Ret(void const *, Args...)>
{
  using Base = detail::FunctionRefBase<Ret(void const *, Args...)>;

  template <typename F>
  static Ret callImpl(void const *func_object, Args... args)
  {
    // The "const" word on the next line is central to preserving const-correctness.
    return eastl::invoke(*static_cast<F const *>(func_object), args...);
  }

  using Base::call;
  using Base::data;

public:
  FunctionRef() : Base() {}

  template <size_t size, typename Ret2, typename... Args2>
  FunctionRef(const FixedMoveOnlyFunction<size, Ret2(Args2...) const> &fmof) :
    Base(const_cast<void *>(static_cast<const void *>(fmof.storage)))
  {
    call = fmof.call;
  }

  template <typename F, typename = EASTL_INTERNAL_FUNCTION_VALID_FUNCTION_ARGS(F, Ret, Args..., Base, FunctionRef),
    typename = eastl::disable_if_t<detail::is_function_ref_v<eastl::decay_t<F>>>>
  FunctionRef(F &&func_object) : Base(&func_object)
  {
    call = &callImpl<eastl::decay_t<F>>;
  }

  FunctionRef &operator=(std::nullptr_t)
  {
    this->reset();
    return *this;
  }

  using Base::operator bool;

  Ret operator()(Args... args) const
  {
    G_ASSERTF(call != nullptr, "Attempt to call an empty FunctionRef!");
    return call(data, ((Args &&)args)...);
  }
};

} // namespace dag
