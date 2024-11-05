//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// atomic/interlocked wrappers
// why not std::atomic?
//  there is legacy reason (std::atomic appears only in c++11)
//  but there are still many reasons to use ours over std::atomic
//  1) compilation time. std::atomic is very heavy on most platforms, lot's of templated code
//  2) std::atomic is a _type_, while interlocked instructions are _instructions_ (on all platforms we have).
//    The only real HW requirement from 'type' model - is alignment (it has to be aligned on it's size in memory)
//    so, if sometimes you can live with non-interlocked instructions - std::atomic doesn't really allow that
//  3) std::atomic are/were not allowing to make simple initializers till c++20 (
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0883r2.pdf ) that's unfortunate, and although it was fixed in a later
//  revision, it is still like that in a version we use if you need _strong_ type, to guarantee on a API level that everything is
//  interlocked/atomic - use std::atomic otherwise prefer usage of ours

#if _MSC_VER && !__clang__
#include <supp/dag_msvc_atomics.h>
#endif

#include <util/dag_stdint.h>

// int32_t (int)

//! atomic ++, returns resulted incremented value for v
inline int interlocked_increment(volatile int &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }

//! atomic --, returns resulted decremented value for v
inline int interlocked_decrement(volatile int &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline int interlocked_add(volatile int &v, int inc) { return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST); }
//! atomic |=, returns original value for v
inline int interlocked_or(volatile int &v, int or_bits) { return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST); }
//! atomic &=, returns original value for v
inline int interlocked_and(volatile int &v, int and_bits) { return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST); }
//! atomic exchange, returns original value for v
inline int interlocked_exchange(volatile int &dest, int xchg) { return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST); }

//! atomic exchange (with acquire), returns initial value of dest
inline int interlocked_exchange_acquire(volatile int &dest, int xchg) { return __atomic_exchange_n(&dest, xchg, __ATOMIC_ACQUIRE); }

//! atomic compare-and-exchange, returns initial value of dest
inline int interlocked_compare_exchange(volatile int &dest, int xchg, int cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}

//! atomic compare-and-exchange (with weak acquire), returns initial value of dest
inline int interlocked_compare_exchange_weak_acquire(volatile int &dest, int xchg, int cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  return cmpr;
}

inline int interlocked_acquire_load(volatile const int &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline int interlocked_relaxed_load(volatile const int &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile int &i, int val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile int &i, int val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }


// uint32_t (unsigned)

//! atomic ++, returns resulted incremented value for v
inline uint32_t interlocked_increment(volatile uint32_t &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }

//! atomic --, returns resulted decremented value for v
inline uint32_t interlocked_decrement(volatile uint32_t &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline uint32_t interlocked_add(volatile uint32_t &v, uint32_t inc) { return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST); }
//! atomic |=, returns original value for v
inline uint32_t interlocked_or(volatile uint32_t &v, uint32_t or_bits) { return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST); }
//! atomic &=, returns original value for v
inline uint32_t interlocked_and(volatile uint32_t &v, uint32_t and_bits) { return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST); }
//! atomic exchange, returns original value for v
inline uint32_t interlocked_exchange(volatile uint32_t &dest, uint32_t xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}

//! atomic exchange (with acquire), returns initial value of dest
inline uint32_t interlocked_exchange_acquire(volatile uint32_t &dest, uint32_t xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_ACQUIRE);
}

//! atomic compare-and-exchange, returns initial value of dest
inline uint32_t interlocked_compare_exchange(volatile uint32_t &dest, uint32_t xchg, uint32_t cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}

//! atomic compare-and-exchange (with weak acquire), returns initial value of dest
inline uint32_t interlocked_compare_exchange_weak_acquire(volatile uint32_t &dest, uint32_t xchg, uint32_t cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  return cmpr;
}


inline uint32_t interlocked_acquire_load(volatile const uint32_t &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline uint32_t interlocked_relaxed_load(volatile const uint32_t &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile uint32_t &i, uint32_t val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }

inline void interlocked_relaxed_store(volatile uint32_t &i, uint32_t val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }

//! atomic ++, returns resulted incremented value for v
inline uint64_t interlocked_increment(volatile uint64_t &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic --, returns resulted decremented value for v
inline uint64_t interlocked_decrement(volatile uint64_t &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline uint64_t interlocked_add(volatile uint64_t &v, uint64_t inc) { return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST); }
//! atomic |=, returns original value for v
inline uint64_t interlocked_or(volatile uint64_t &v, uint64_t or_bits) { return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST); }
//! atomic &=, returns original value for v
inline uint64_t interlocked_and(volatile uint64_t &v, uint64_t and_bits) { return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST); }
//! atomic exchange, returns original value for v
inline uint64_t interlocked_exchange(volatile uint64_t &dest, uint64_t xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}
//! returns initial value of dest
inline uint64_t interlocked_compare_exchange(volatile uint64_t &dest, uint64_t xchg, uint64_t cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}

inline uint64_t interlocked_acquire_load(volatile const uint64_t &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline uint64_t interlocked_relaxed_load(volatile const uint64_t &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile uint64_t &i, uint64_t val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile uint64_t &i, uint64_t val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }


//! atomic ++, returns resulted incremented value for v
inline int64_t interlocked_increment(volatile int64_t &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic --, returns resulted decremented value for v
inline int64_t interlocked_decrement(volatile int64_t &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline int64_t interlocked_add(volatile int64_t &v, int64_t inc) { return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST); }
//! atomic |=, returns original value for v
inline int64_t interlocked_or(volatile int64_t &v, int64_t or_bits) { return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST); }
//! atomic &=, returns original value for v
inline int64_t interlocked_and(volatile int64_t &v, int64_t and_bits) { return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST); }
//! atomic exchange, returns original value for v
inline int64_t interlocked_exchange(volatile int64_t &dest, int64_t xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}
//! returns initial value of dest
inline int64_t interlocked_compare_exchange(volatile int64_t &dest, int64_t xchg, int64_t cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}

inline int64_t interlocked_acquire_load(volatile const int64_t &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline int64_t interlocked_relaxed_load(volatile const int64_t &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile int64_t &i, int64_t val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile int64_t &i, int64_t val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }

template <typename T>
inline T *interlocked_acquire_load_ptr(T *volatile &ptr)
{
  return __atomic_load_n(&ptr, __ATOMIC_ACQUIRE);
}

//! atomic exchange pointer, returns original value of dest
template <typename T>
inline T *interlocked_exchange_ptr(T *volatile &dest, T *xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}
//! atomic compare-and-exchange pointer, returns initial value of dest
template <typename T>
inline T *interlocked_compare_exchange_ptr(T *volatile &dest, T *xchg, T *cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}

template <typename T>
inline const T *interlocked_acquire_load_ptr(const T *const volatile &ptr)
{
  return __atomic_load_n(&ptr, __ATOMIC_ACQUIRE);
}

template <typename T>
inline T *interlocked_relaxed_load_ptr(T *volatile &ptr)
{
  return __atomic_load_n(&ptr, __ATOMIC_RELAXED);
}

template <typename T>
inline const T *interlocked_relaxed_load_ptr(const T *const volatile &ptr)
{
  return __atomic_load_n(&ptr, __ATOMIC_RELAXED);
}

template <typename T>
inline void interlocked_release_store_ptr(T *volatile &ptr, T *val)
{
  __atomic_store_n(&ptr, val, __ATOMIC_RELEASE);
}

template <typename T>
inline void interlocked_relaxed_store_ptr(T *volatile &ptr, T *val)
{
  __atomic_store_n(&ptr, val, __ATOMIC_RELAXED);
}


//! atomic exchange, returns original value for v
inline bool interlocked_exchange(volatile bool &dest, bool xchg) { return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST); }
//! atomic compare-and-exchange, returns initial value of dest
inline bool interlocked_compare_exchange(volatile bool &dest, bool xchg, bool cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}
inline bool interlocked_acquire_load(volatile const bool &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline bool interlocked_relaxed_load(volatile const bool &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile bool &i, bool val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile bool &i, bool val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }


// 8 bit atomics (unsigned char)

//! atomic ++, returns resulted incremented value for v
inline unsigned char interlocked_increment(volatile unsigned char &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }

//! atomic --, returns resulted decremented value for v
inline unsigned char interlocked_decrement(volatile unsigned char &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline unsigned char interlocked_add(volatile unsigned char &v, unsigned char inc)
{
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
}
//! atomic |=, returns original value for v
inline unsigned char interlocked_or(volatile unsigned char &v, unsigned char or_bits)
{
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
}
//! atomic &=, returns original value for v
inline unsigned char interlocked_and(volatile unsigned char &v, unsigned char and_bits)
{
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
}
//! atomic exchange, returns original value for v
inline unsigned char interlocked_exchange(volatile unsigned char &dest, unsigned char xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}
//! atomic compare-and-exchange, returns initial value of dest
inline unsigned char interlocked_compare_exchange(volatile unsigned char &dest, unsigned char xchg, unsigned char cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}
inline unsigned char interlocked_acquire_load(volatile const unsigned char &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline unsigned char interlocked_relaxed_load(volatile const unsigned char &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile unsigned char &i, unsigned char val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile unsigned char &i, unsigned char val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }


// 16 bit atomics (unsigned short)

//! atomic ++, returns resulted incremented value for v
inline unsigned short interlocked_increment(volatile unsigned short &v) { return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST); }

//! atomic --, returns resulted decremented value for v
inline unsigned short interlocked_decrement(volatile unsigned short &v) { return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST); }
//! atomic +=, returns resulted incremented value for v
inline unsigned short interlocked_add(volatile unsigned short &v, unsigned short inc)
{
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
}
//! atomic |=, returns original value for v
inline unsigned short interlocked_or(volatile unsigned short &v, unsigned short or_bits)
{
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
}
//! atomic &=, returns original value for v
inline unsigned short interlocked_and(volatile unsigned short &v, unsigned short and_bits)
{
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
}
//! atomic exchange, returns original value for v
inline unsigned short interlocked_exchange(volatile unsigned short &dest, unsigned short xchg)
{
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
}
//! atomic compare-and-exchange, returns initial value of dest
inline unsigned short interlocked_compare_exchange(volatile unsigned short &dest, unsigned short xchg, unsigned short cmpr)
{
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
}
inline unsigned short interlocked_acquire_load(volatile const unsigned short &i) { return __atomic_load_n(&i, __ATOMIC_ACQUIRE); }
inline unsigned short interlocked_relaxed_load(volatile const unsigned short &i) { return __atomic_load_n(&i, __ATOMIC_RELAXED); }
inline void interlocked_release_store(volatile unsigned short &i, unsigned short val) { __atomic_store_n(&i, val, __ATOMIC_RELEASE); }
inline void interlocked_relaxed_store(volatile unsigned short &i, unsigned short val) { __atomic_store_n(&i, val, __ATOMIC_RELAXED); }
