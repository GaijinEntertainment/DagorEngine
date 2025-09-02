//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/type_traits.h>

#if _MSC_VER && !__clang__
#include <supp/dag_msvc_atomics.h>
#endif

/// @file A more sane replacement for std::atomic
/// Why?
/// * std::atomic can silently fall back to using a mutex
/// * std::atomic silently does seq_cst operations through overloaded operators
///   and some people (a lot, it turns out) get confused and think that these operators actually
///   do non-atomic ops (which they don't, they do seq_cst ops). Moreover, this implicit behavior makes
///   the code less explicit: the expensive synchronization is hidden in operator overloads and you
///   cannot distinguish it from plain old register arithmetic by simply reading the code.
/// * std::atomic passes the atomic operation semantic (aka memory_order) as an regular argument.
///   First, different memory_orders literally are different hardware operations, so unless the
///   call is inlined, branching can occur. Second, this allows for invalid combinations, which
///   is UNDEFINED BEHAVIOR per the standard instead of being a simple compilation error. Third,
///   memory_order_consume exists but is useless. Instead, we do an overloading-based trick
///   to maintain the same API as std but have separate functions for each memory order and
///   ban invalid combinations at compile time.
/// * Due to everything being stuffed into a single template, (INCLUDING atomic_shared_ptr !!!),
///   implementations of std::atomic are impossible to read for mere mortals, and hard for
///   compilers to compile because of all the partial specializations.
///   It also makes little sense to join a bunch of types with totally different APIs into
///   a single template. We have separate templates for each use-case.
/// * Interlocked ops we have in dag_atomic.h are error prone. In most use-cases of atomic operations
///   you always access a certain memory cell using atomics only, and this is easy to accidentally violate
///   with interlocked ops.
///

// TODO: some of these methods are not actually implemented on MSVC. Thankfully, templates are instantiated lazily
// and so you should implement them as you need them.

// TO CONSIDER: should we allow mixing non-atomic operations with atomic ones?
// Technically, it is supported by the standard and some valid use-cases exist,
// but in existing code it seems that such mixing occurs by mistake most of the time.

namespace dag
{

// NOTE: Strong types are used here instead of a enum to avoid having runtime checks in
// respective functions and allow for compile-time checks of invalid combinations.

/// If you are in doubt about what semantics you need, use seq_cst everywhere, always.
namespace memory_order
{
// clang-format off
inline constexpr struct _Relaxed {} relaxed = {};
inline constexpr struct _Acquire {} acquire = {};
inline constexpr struct _Release {} release = {};
inline constexpr struct _AcqRel  {} acq_rel = {};
inline constexpr struct _SeqCst  {} seq_cst = {};
// clang-format on
} // namespace memory_order

namespace mo = memory_order;

inline constexpr auto memory_order_relaxed = memory_order::relaxed;
inline constexpr auto memory_order_acquire = memory_order::acquire;
inline constexpr auto memory_order_release = memory_order::release;
inline constexpr auto memory_order_acq_rel = memory_order::acq_rel;
inline constexpr auto memory_order_seq_cst = memory_order::seq_cst;

// TODO: switch to C++20
// 16-byte atomics not supported for now.
template <class T>
constexpr bool HardwareAtomicsFeasible = sizeof(T) <= 8;

template <class T, class = eastl::enable_if_t<eastl::is_integral_v<T> && HardwareAtomicsFeasible<T>>>
class AtomicInteger
{
public:
  using ValueType = T;

  AtomicInteger() = default;
  AtomicInteger(const AtomicInteger &) = delete;
  AtomicInteger &operator=(const AtomicInteger &) = delete;
  AtomicInteger(AtomicInteger &&) = delete;
  AtomicInteger &operator=(AtomicInteger &&) = delete;

  // STD-compatible API

  // Initialization is not atomic and needs synchronization.
  AtomicInteger(T v) : value(v) {}
  // Finalization is not atomic and needs synchronization.
  ~AtomicInteger() = default;

  void store(T v, mo::_Relaxed) { __atomic_store_n(&this->value, v, __ATOMIC_RELAXED); }
  void store(T v, mo::_Release) { __atomic_store_n(&this->value, v, __ATOMIC_RELEASE); }
  void store(T v, mo::_SeqCst = {}) { __atomic_store_n(&this->value, v, __ATOMIC_SEQ_CST); }

  T load(mo::_Relaxed) const { return __atomic_load_n(&this->value, __ATOMIC_RELAXED); }
  T load(mo::_Acquire) const { return __atomic_load_n(&this->value, __ATOMIC_ACQUIRE); }
  T load(mo::_SeqCst = {}) const { return __atomic_load_n(&this->value, __ATOMIC_SEQ_CST); }

  T exchange(T v, mo::_Relaxed) { return __atomic_exchange_n(&this->value, v, __ATOMIC_RELAXED); }
  T exchange(T v, mo::_Release) { return __atomic_exchange_n(&this->value, v, __ATOMIC_RELEASE); }
  T exchange(T v, mo::_Acquire) { return __atomic_exchange_n(&this->value, v, __ATOMIC_ACQUIRE); }
  T exchange(T v, mo::_AcqRel) { return __atomic_exchange_n(&this->value, v, __ATOMIC_ACQ_REL); }
  T exchange(T v, mo::_SeqCst = {}) { return __atomic_exchange_n(&this->value, v, __ATOMIC_SEQ_CST); }

  // clang-format off
  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_weak(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_weak(T &expected, T desired, MO1, MO2) = delete;

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_strong(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_strong(T &expected, T desired, MO1, MO2) = delete;
  // clang-format on

  T fetch_add(T v, mo::_Relaxed) { return __atomic_fetch_add(&this->value, v, __ATOMIC_RELAXED); }
  T fetch_add(T v, mo::_Release) { return __atomic_fetch_add(&this->value, v, __ATOMIC_RELEASE); }
  T fetch_add(T v, mo::_Acquire) { return __atomic_fetch_add(&this->value, v, __ATOMIC_ACQUIRE); }
  T fetch_add(T v, mo::_AcqRel) { return __atomic_fetch_add(&this->value, v, __ATOMIC_ACQ_REL); }
  T fetch_add(T v, mo::_SeqCst = {}) { return __atomic_fetch_add(&this->value, v, __ATOMIC_SEQ_CST); }

  T fetch_sub(T v, mo::_Relaxed) { return __atomic_fetch_sub(&this->value, v, __ATOMIC_RELAXED); }
  T fetch_sub(T v, mo::_Release) { return __atomic_fetch_sub(&this->value, v, __ATOMIC_RELEASE); }
  T fetch_sub(T v, mo::_Acquire) { return __atomic_fetch_sub(&this->value, v, __ATOMIC_ACQUIRE); }
  T fetch_sub(T v, mo::_AcqRel) { return __atomic_fetch_sub(&this->value, v, __ATOMIC_ACQ_REL); }
  T fetch_sub(T v, mo::_SeqCst = {}) { return __atomic_fetch_sub(&this->value, v, __ATOMIC_SEQ_CST); }

  T fetch_and(T v, mo::_Relaxed) { return __atomic_fetch_and(&this->value, v, __ATOMIC_RELAXED); }
  T fetch_and(T v, mo::_Release) { return __atomic_fetch_and(&this->value, v, __ATOMIC_RELEASE); }
  T fetch_and(T v, mo::_Acquire) { return __atomic_fetch_and(&this->value, v, __ATOMIC_ACQUIRE); }
  T fetch_and(T v, mo::_AcqRel) { return __atomic_fetch_and(&this->value, v, __ATOMIC_ACQ_REL); }
  T fetch_and(T v, mo::_SeqCst = {}) { return __atomic_fetch_and(&this->value, v, __ATOMIC_SEQ_CST); }

  T fetch_or(T v, mo::_Relaxed) { return __atomic_fetch_or(&this->value, v, __ATOMIC_RELAXED); }
  T fetch_or(T v, mo::_Release) { return __atomic_fetch_or(&this->value, v, __ATOMIC_RELEASE); }
  T fetch_or(T v, mo::_Acquire) { return __atomic_fetch_or(&this->value, v, __ATOMIC_ACQUIRE); }
  T fetch_or(T v, mo::_AcqRel) { return __atomic_fetch_or(&this->value, v, __ATOMIC_ACQ_REL); }
  T fetch_or(T v, mo::_SeqCst = {}) { return __atomic_fetch_or(&this->value, v, __ATOMIC_SEQ_CST); }

  // Extended API

  T add_fetch(T v, mo::_Relaxed) { return __atomic_add_fetch(&this->value, v, __ATOMIC_RELAXED); }
  T add_fetch(T v, mo::_Release) { return __atomic_add_fetch(&this->value, v, __ATOMIC_RELEASE); }
  T add_fetch(T v, mo::_Acquire) { return __atomic_add_fetch(&this->value, v, __ATOMIC_ACQUIRE); }
  T add_fetch(T v, mo::_AcqRel) { return __atomic_add_fetch(&this->value, v, __ATOMIC_ACQ_REL); }
  T add_fetch(T v, mo::_SeqCst = {}) { return __atomic_add_fetch(&this->value, v, __ATOMIC_SEQ_CST); }

  T sub_fetch(T v, mo::_Relaxed) { return __atomic_sub_fetch(&this->value, v, __ATOMIC_RELAXED); }
  T sub_fetch(T v, mo::_Release) { return __atomic_sub_fetch(&this->value, v, __ATOMIC_RELEASE); }
  T sub_fetch(T v, mo::_Acquire) { return __atomic_sub_fetch(&this->value, v, __ATOMIC_ACQUIRE); }
  T sub_fetch(T v, mo::_AcqRel) { return __atomic_sub_fetch(&this->value, v, __ATOMIC_ACQ_REL); }
  T sub_fetch(T v, mo::_SeqCst = {}) { return __atomic_sub_fetch(&this->value, v, __ATOMIC_SEQ_CST); }

private:
  T value;
};

class AtomicFlag
{
  using T = bool;

public:
  using ValueType = T;

  AtomicFlag(const AtomicFlag &) = delete;
  AtomicFlag &operator=(const AtomicFlag &) = delete;
  AtomicFlag(AtomicFlag &&) = delete;
  AtomicFlag &operator=(AtomicFlag &&) = delete;

  // STD-compatible API

  // Initialization is not atomic and needs synchronization.
  AtomicFlag() : value(false) {}
  // Finalization is not atomic and needs synchronization.
  ~AtomicFlag() = default;

  void clear(mo::_Relaxed) { __atomic_clear(&this->value, __ATOMIC_RELAXED); }
  void clear(mo::_Release) { __atomic_clear(&this->value, __ATOMIC_RELEASE); }
  void clear(mo::_SeqCst = {}) { __atomic_clear(&this->value, __ATOMIC_SEQ_CST); }

  T test(mo::_Relaxed) const { return __atomic_load_n(&this->value, __ATOMIC_RELAXED); }
  T test(mo::_Acquire) const { return __atomic_load_n(&this->value, __ATOMIC_ACQUIRE); }
  T test(mo::_SeqCst = {}) const { return __atomic_load_n(&this->value, __ATOMIC_SEQ_CST); }

  T test_and_set(mo::_Relaxed) { return __atomic_test_and_set(&this->value, __ATOMIC_RELAXED); }
  T test_and_set(mo::_Release) { return __atomic_test_and_set(&this->value, __ATOMIC_RELEASE); }
  T test_and_set(mo::_Acquire) { return __atomic_test_and_set(&this->value, __ATOMIC_ACQUIRE); }
  T test_and_set(mo::_AcqRel) { return __atomic_test_and_set(&this->value, __ATOMIC_ACQ_REL); }
  T test_and_set(mo::_SeqCst = {}) { return __atomic_test_and_set(&this->value, __ATOMIC_SEQ_CST); }

private:
  T value;
};

template <class U>
class AtomicPointer
{
  using T = U *;

public:
  using ValueType = T;

  AtomicPointer() = default;
  AtomicPointer(const AtomicPointer &) = delete;
  AtomicPointer &operator=(const AtomicPointer &) = delete;
  AtomicPointer(AtomicPointer &&) = delete;
  AtomicPointer &operator=(AtomicPointer &&) = delete;

  // STD-compatible API

  // Initialization is not atomic and needs synchronization.
  AtomicPointer(T v) : value(v) {}
  // Finalization is not atomic and needs synchronization.
  ~AtomicPointer() = default;

  void store(T v, mo::_Relaxed) { __atomic_store_n(&this->value, v, __ATOMIC_RELAXED); }
  void store(T v, mo::_Release) { __atomic_store_n(&this->value, v, __ATOMIC_RELEASE); }
  void store(T v, mo::_SeqCst = {}) { __atomic_store_n(&this->value, v, __ATOMIC_SEQ_CST); }

  T load(mo::_Relaxed) const { return __atomic_load_n(&this->value, __ATOMIC_RELAXED); }
  T load(mo::_Acquire) const { return __atomic_load_n(&this->value, __ATOMIC_ACQUIRE); }
  T load(mo::_SeqCst = {}) const { return __atomic_load_n(&this->value, __ATOMIC_SEQ_CST); }

  T exchange(T v, mo::_Relaxed) { return __atomic_exchange_n(&this->value, v, __ATOMIC_RELAXED); }
  T exchange(T v, mo::_Release) { return __atomic_exchange_n(&this->value, v, __ATOMIC_RELEASE); }
  T exchange(T v, mo::_Acquire) { return __atomic_exchange_n(&this->value, v, __ATOMIC_ACQUIRE); }
  T exchange(T v, mo::_AcqRel) { return __atomic_exchange_n(&this->value, v, __ATOMIC_ACQ_REL); }
  T exchange(T v, mo::_SeqCst = {}) { return __atomic_exchange_n(&this->value, v, __ATOMIC_SEQ_CST); }

  // clang-format off
  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_weak(T &expected, T desired, mo::_SeqCst = {}, mo::_SeqCst = {}) { return __atomic_compare_exchange_n(&this->value, &expected, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_weak(T &expected, T desired, MO1, MO2) = delete;

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_strong(T &expected, T desired, mo::_SeqCst = {}, mo::_SeqCst = {}) { return __atomic_compare_exchange_n(&this->value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_strong(T &expected, T desired, MO1, MO2) = delete;
  // clang-format on

private:
  T value;
};

template <class T, class = eastl::enable_if_t<eastl::is_floating_point_v<T> && HardwareAtomicsFeasible<T>>>
class AtomicFloat
{
public:
  using ValueType = T;

  AtomicFloat() = default;
  AtomicFloat(const AtomicFloat &) = delete;
  AtomicFloat &operator=(const AtomicFloat &) = delete;
  AtomicFloat(AtomicFloat &&) = delete;
  AtomicFloat &operator=(AtomicFloat &&) = delete;

  // STD-compatible API

  // Initialization is not atomic and needs synchronization.
  AtomicFloat(T v) : value(v) {}
  // Finalization is not atomic and needs synchronization.
  ~AtomicFloat() = default;

  void store(T v, mo::_Relaxed) { __atomic_store(&this->value, &v, __ATOMIC_RELAXED); }
  void store(T v, mo::_Release) { __atomic_store(&this->value, &v, __ATOMIC_RELEASE); }
  void store(T v, mo::_SeqCst = {}) { __atomic_store(&this->value, &v, __ATOMIC_SEQ_CST); }

  // clang-format off
  T load(mo::_Relaxed) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_RELAXED); return ret; }
  T load(mo::_Acquire) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_ACQUIRE); return ret; }
  T load(mo::_SeqCst = {}) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_SEQ_CST); return ret; }
  // clang-format on

  // clang-format off
  T exchange(T v, mo::_Relaxed) { T ret; __atomic_exchange(&this->value, v, &ret, __ATOMIC_RELAXED); return ret; }
  T exchange(T v, mo::_Release) { T ret; __atomic_exchange(&this->value, v, &ret, __ATOMIC_RELEASE); return ret; }
  T exchange(T v, mo::_Acquire) { T ret; __atomic_exchange(&this->value, v, &ret, __ATOMIC_ACQUIRE); return ret; }
  T exchange(T v, mo::_AcqRel) { T ret; __atomic_exchange(&this->value, v, &ret, __ATOMIC_ACQ_REL); return ret; }
  T exchange(T v, mo::_SeqCst = {}) { T ret; __atomic_exchange(&this->value, v, &ret, __ATOMIC_SEQ_CST); return ret; }
  // clang-format on

  // clang-format off
  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_weak(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_weak(T &expected, T desired, MO1, MO2) = delete;

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_strong(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_strong(T &expected, T desired, MO1, MO2) = delete;
  // clang-format on

  // TODO: implement fetch_add, fetch_sub, etc, but only if at least one HW supports it.

private:
  T value;
};

/// For small POD structures. Type must be comparable bitwise.
template <class T, class = eastl::enable_if_t<eastl::is_trivially_copyable_v<T> && eastl::is_trivially_copy_assignable_v<T> &&
                                              HardwareAtomicsFeasible<T>>>
class AtomicPod
{
public:
  using ValueType = T;

  AtomicPod() = default;
  AtomicPod(const AtomicPod &) = delete;
  AtomicPod &operator=(const AtomicPod &) = delete;
  AtomicPod(AtomicPod &&) = delete;
  AtomicPod &operator=(AtomicPod &&) = delete;

  // STD-compatible API

  // Initialization is not atomic and needs synchronization.
  AtomicPod(T v) : value(v) {}
  // Finalization is not atomic and needs synchronization.
  ~AtomicPod() = default;

  void store(T v, mo::_Relaxed) { __atomic_store(&this->value, &v, __ATOMIC_RELAXED); }
  void store(T v, mo::_Release) { __atomic_store(&this->value, &v, __ATOMIC_RELEASE); }
  void store(T v, mo::_SeqCst = {}) { __atomic_store(&this->value, &v, __ATOMIC_SEQ_CST); }

  // clang-format off
  T load(mo::_Relaxed) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_RELAXED); return ret; }
  T load(mo::_Acquire) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_ACQUIRE); return ret; }
  T load(mo::_SeqCst = {}) const { T ret; __atomic_load(&this->value, &ret, __ATOMIC_SEQ_CST); return ret; }

  T exchange(T v, mo::_Relaxed) { T ret; __atomic_exchange(&this->value, &v, &ret, __ATOMIC_RELAXED); return ret; }
  T exchange(T v, mo::_Release) { T ret; __atomic_exchange(&this->value, &v, &ret, __ATOMIC_RELEASE); return ret; }
  T exchange(T v, mo::_Acquire) { T ret; __atomic_exchange(&this->value, &v, &ret, __ATOMIC_ACQUIRE); return ret; }
  T exchange(T v, mo::_AcqRel) { T ret; __atomic_exchange(&this->value, &v, &ret, __ATOMIC_ACQ_REL); return ret; }
  T exchange(T v, mo::_SeqCst = {}) { T ret; __atomic_exchange(&this->value, &v, &ret, __ATOMIC_SEQ_CST); return ret; }
  // clang-format on

  // clang-format off
  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_weak(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_weak(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_weak(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange(&this->value, &expected, &desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_weak(T &expected, T desired, MO1, MO2) = delete;

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Relaxed) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED); }

  bool compare_exchange_strong(T &expected, T desired, mo::_Relaxed, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Acquire, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_Release, mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE); }
  bool compare_exchange_strong(T &expected, T desired, mo::_AcqRel,  mo::_Acquire) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); }

  bool compare_exchange_strong(T &expected, T desired, mo::_SeqCst = {},  mo::_SeqCst = {}) { return __atomic_compare_exchange(&this->value, &expected, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
  /// If you encounter this, you are probably doing something unhinged. Stop it. Get some help.
  template <class MO1, class MO2> bool compare_exchange_strong(T &expected, T desired, MO1, MO2) = delete;
  // clang-format on

private:
  T value;
};

} // namespace dag
