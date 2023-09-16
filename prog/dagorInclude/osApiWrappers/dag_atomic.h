//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <util/dag_stdint.h>

// int32_t (int)

//! atomic ++, returns resulted incremented value for v
inline int interlocked_increment(volatile int &v)
{
#ifdef _MSC_VER
  return _InterlockedIncrement((volatile long *)&v);
#else
  return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}

//! atomic --, returns resulted decremented value for v
inline int interlocked_decrement(volatile int &v)
{
#ifdef _MSC_VER
  return _InterlockedDecrement((volatile long *)&v);
#else
  return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}
//! atomic +=, returns resulted incremented value for v
inline int interlocked_add(volatile int &v, int inc)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd((volatile long *)&v, inc) + inc;
#else
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
#endif
}
//! atomic |=, returns original value for v
inline int interlocked_or(volatile int &v, int or_bits)
{
#ifdef _MSC_VER
  return _InterlockedOr((volatile long *)&v, or_bits);
#else
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic &=, returns original value for v
inline int interlocked_and(volatile int &v, int and_bits)
{
#ifdef _MSC_VER
  return _InterlockedAnd((volatile long *)&v, and_bits);
#else
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic exchange, returns original value for v
inline int interlocked_exchange(volatile int &dest, int xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange((volatile long *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}

//! atomic exchange (with acquire), returns initial value of dest
inline int interlocked_exchange_acquire(volatile int &dest, int xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange((volatile long *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_ACQUIRE);
#endif
}

//! atomic compare-and-exchange, returns initial value of dest
inline int interlocked_compare_exchange(volatile int &dest, int xchg, int cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange((volatile long *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}

//! atomic compare-and-exchange (with weak acquire), returns initial value of dest
inline int interlocked_compare_exchange_weak_acquire(volatile int &dest, int xchg, int cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange((volatile long *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  return cmpr;
#endif
}

inline int interlocked_acquire_load(volatile const int &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline int interlocked_relaxed_load(volatile const int &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile int &i, int val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}
inline void interlocked_relaxed_store(volatile int &i, int val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}


// uint32_t (unsigned)

//! atomic ++, returns resulted incremented value for v
inline uint32_t interlocked_increment(volatile uint32_t &v)
{
#ifdef _MSC_VER
  return _InterlockedIncrement((volatile long *)&v);
#else
  return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}

//! atomic --, returns resulted decremented value for v
inline uint32_t interlocked_decrement(volatile uint32_t &v)
{
#ifdef _MSC_VER
  return _InterlockedDecrement((volatile long *)&v);
#else
  return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}
//! atomic +=, returns resulted incremented value for v
inline uint32_t interlocked_add(volatile uint32_t &v, uint32_t inc)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd((volatile long *)&v, inc) + inc;
#else
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
#endif
}
//! atomic |=, returns original value for v
inline uint32_t interlocked_or(volatile uint32_t &v, uint32_t or_bits)
{
#ifdef _MSC_VER
  return _InterlockedOr((volatile long *)&v, or_bits);
#else
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic &=, returns original value for v
inline uint32_t interlocked_and(volatile uint32_t &v, uint32_t and_bits)
{
#ifdef _MSC_VER
  return _InterlockedAnd((volatile long *)&v, and_bits);
#else
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic exchange, returns original value for v
inline uint32_t interlocked_exchange(volatile uint32_t &dest, uint32_t xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange((volatile long *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}

//! atomic exchange (with acquire), returns initial value of dest
inline uint32_t interlocked_exchange_acquire(volatile uint32_t &dest, uint32_t xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange((volatile long *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_ACQUIRE);
#endif
}

//! atomic compare-and-exchange, returns initial value of dest
inline uint32_t interlocked_compare_exchange(volatile uint32_t &dest, uint32_t xchg, uint32_t cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange((volatile long *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}

//! atomic compare-and-exchange (with weak acquire), returns initial value of dest
inline uint32_t interlocked_compare_exchange_weak_acquire(volatile uint32_t &dest, uint32_t xchg, uint32_t cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange((volatile long *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  return cmpr;
#endif
}


inline uint32_t interlocked_acquire_load(volatile const uint32_t &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline uint32_t interlocked_relaxed_load(volatile const uint32_t &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile uint32_t &i, uint32_t val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}

inline void interlocked_relaxed_store(volatile uint32_t &i, uint32_t val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}

#if _TARGET_64BIT
// uint64_t is available only in 64bit

//! atomic ++, returns resulted incremented value for v
inline uint64_t interlocked_increment(volatile uint64_t &v)
{
#ifdef _MSC_VER
  return _InterlockedIncrement64((volatile __int64 *)&v);
#else
  return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}

//! atomic --, returns resulted decremented value for v
inline uint64_t interlocked_decrement(volatile uint64_t &v)
{
#ifdef _MSC_VER
  return _InterlockedDecrement64((volatile __int64 *)&v);
#else
  return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}
//! atomic +=, returns resulted incremented value for v
inline uint64_t interlocked_add(volatile uint64_t &v, uint64_t inc)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd64((volatile __int64 *)&v, inc) + inc;
#else
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
#endif
}
//! atomic |=, returns original value for v
inline uint64_t interlocked_or(volatile uint64_t &v, uint64_t or_bits)
{
#ifdef _MSC_VER
  return _InterlockedOr64((volatile __int64 *)&v, or_bits);
#else
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic &=, returns original value for v
inline uint64_t interlocked_and(volatile uint64_t &v, uint64_t and_bits)
{
#ifdef _MSC_VER
  return _InterlockedAnd64((volatile __int64 *)&v, and_bits);
#else
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic exchange, returns original value for v
inline uint64_t interlocked_exchange(volatile uint64_t &dest, uint64_t xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange64((volatile __int64 *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}
//! returns initial value of dest
inline uint64_t interlocked_compare_exchange(volatile uint64_t &dest, uint64_t xchg, uint64_t cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange64((volatile __int64 *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}

inline uint64_t interlocked_acquire_load(volatile const uint64_t &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline uint64_t interlocked_relaxed_load(volatile const uint64_t &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile uint64_t &i, uint64_t val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}
inline void interlocked_relaxed_store(volatile uint64_t &i, uint64_t val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}
#endif

template <typename T>
inline T *interlocked_acquire_load_ptr(T *volatile &ptr)
{
#ifdef _MSC_VER
  return ptr;
#else
  return __atomic_load_n(&ptr, __ATOMIC_ACQUIRE);
#endif
}

//! atomic exchange pointer, returns original value of dest
template <typename T>
inline T *interlocked_exchange_ptr(T *volatile &dest, T *xchg)
{
#ifdef _MSC_VER
  return (T *)_InterlockedExchangePointer((void *volatile *)&dest, (void *)xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}
//! atomic compare-and-exchange pointer, returns initial value of dest
template <typename T>
inline T *interlocked_compare_exchange_ptr(T *volatile &dest, T *xchg, T *cmpr)
{
#ifdef _MSC_VER
  return (T *)_InterlockedCompareExchangePointer((void *volatile *)&dest, (void *)xchg, (void *)cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}

template <typename T>
inline const T *interlocked_acquire_load_ptr(const T *const volatile &ptr)
{
#ifdef _MSC_VER
  return ptr;
#else
  return __atomic_load_n(&ptr, __ATOMIC_ACQUIRE);
#endif
}

template <typename T>
inline T *interlocked_relaxed_load_ptr(T *volatile &ptr)
{
#ifdef _MSC_VER
  return ptr;
#else
  return __atomic_load_n(&ptr, __ATOMIC_RELAXED);
#endif
}

template <typename T>
inline const T *interlocked_relaxed_load_ptr(const T *const volatile &ptr)
{
#ifdef _MSC_VER
  return ptr;
#else
  return __atomic_load_n(&ptr, __ATOMIC_RELAXED);
#endif
}

template <typename T>
inline void interlocked_release_store_ptr(T *volatile &ptr, T *val)
{
#ifdef _MSC_VER
  ptr = val;
#else
  __atomic_store_n(&ptr, val, __ATOMIC_RELEASE);
#endif
}

template <typename T>
inline void interlocked_relaxed_store_ptr(T *volatile &ptr, T *val)
{
#ifdef _MSC_VER
  ptr = val;
#else
  __atomic_store_n(&ptr, val, __ATOMIC_RELAXED);
#endif
}


//! atomic exchange, returns original value for v
inline bool interlocked_exchange(volatile bool &dest, bool xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange8((volatile char *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}
//! atomic compare-and-exchange, returns initial value of dest
inline bool interlocked_compare_exchange(volatile bool &dest, bool xchg, bool cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange8((volatile char *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}
inline bool interlocked_acquire_load(volatile const bool &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline bool interlocked_relaxed_load(volatile const bool &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile bool &i, bool val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}
inline void interlocked_relaxed_store(volatile bool &i, bool val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}


// 8 bit atomics (unsigned char)

//! atomic ++, returns resulted incremented value for v
inline unsigned char interlocked_increment(volatile unsigned char &v)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd8((volatile char *)&v, 1) + 1;
#else
  return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}

//! atomic --, returns resulted decremented value for v
inline unsigned char interlocked_decrement(volatile unsigned char &v)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd8((volatile char *)&v, -1) - 1;
#else
  return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}
//! atomic +=, returns resulted incremented value for v
inline unsigned char interlocked_add(volatile unsigned char &v, unsigned char inc)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd8((volatile char *)&v, inc) + inc;
#else
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
#endif
}
//! atomic |=, returns original value for v
inline unsigned char interlocked_or(volatile unsigned char &v, unsigned char or_bits)
{
#ifdef _MSC_VER
  return _InterlockedOr8((volatile char *)&v, or_bits);
#else
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic &=, returns original value for v
inline unsigned char interlocked_and(volatile unsigned char &v, unsigned char and_bits)
{
#ifdef _MSC_VER
  return _InterlockedAnd8((volatile char *)&v, and_bits);
#else
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic exchange, returns original value for v
inline unsigned char interlocked_exchange(volatile unsigned char &dest, unsigned char xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange8((volatile char *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}
//! atomic compare-and-exchange, returns initial value of dest
inline unsigned char interlocked_compare_exchange(volatile unsigned char &dest, unsigned char xchg, unsigned char cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange8((volatile char *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}
inline unsigned char interlocked_acquire_load(volatile const unsigned char &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline unsigned char interlocked_relaxed_load(volatile const unsigned char &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile unsigned char &i, unsigned char val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}
inline void interlocked_relaxed_store(volatile unsigned char &i, unsigned char val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}


// 16 bit atomics (unsigned short)

//! atomic ++, returns resulted incremented value for v
inline unsigned short interlocked_increment(volatile unsigned short &v)
{
#ifdef _MSC_VER
  return _InterlockedIncrement16((volatile short *)&v);
#else
  return __atomic_add_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}

//! atomic --, returns resulted decremented value for v
inline unsigned short interlocked_decrement(volatile unsigned short &v)
{
#ifdef _MSC_VER
  return _InterlockedDecrement16((volatile short *)&v);
#else
  return __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST);
#endif
}
//! atomic +=, returns resulted incremented value for v
inline unsigned short interlocked_add(volatile unsigned short &v, unsigned short inc)
{
#ifdef _MSC_VER
  return _InterlockedExchangeAdd16((volatile short *)&v, inc) + inc;
#else
  return __atomic_add_fetch(&v, inc, __ATOMIC_SEQ_CST);
#endif
}
//! atomic |=, returns original value for v
inline unsigned short interlocked_or(volatile unsigned short &v, unsigned short or_bits)
{
#ifdef _MSC_VER
  return _InterlockedOr16((volatile short *)&v, or_bits);
#else
  return __atomic_fetch_or(&v, or_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic &=, returns original value for v
inline unsigned short interlocked_and(volatile unsigned short &v, unsigned short and_bits)
{
#ifdef _MSC_VER
  return _InterlockedAnd16((volatile short *)&v, and_bits);
#else
  return __atomic_fetch_and(&v, and_bits, __ATOMIC_SEQ_CST);
#endif
}
//! atomic exchange, returns original value for v
inline unsigned short interlocked_exchange(volatile unsigned short &dest, unsigned short xchg)
{
#ifdef _MSC_VER
  return _InterlockedExchange16((volatile short *)&dest, xchg);
#else
  return __atomic_exchange_n(&dest, xchg, __ATOMIC_SEQ_CST);
#endif
}
//! atomic compare-and-exchange, returns initial value of dest
inline unsigned short interlocked_compare_exchange(volatile unsigned short &dest, unsigned short xchg, unsigned short cmpr)
{
#ifdef _MSC_VER
  return _InterlockedCompareExchange16((volatile short *)&dest, xchg, cmpr);
#else
  __atomic_compare_exchange_n(&dest, &cmpr, xchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return cmpr;
#endif
}
inline unsigned short interlocked_acquire_load(volatile const unsigned short &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_ACQUIRE);
#endif
}
inline unsigned short interlocked_relaxed_load(volatile const unsigned short &i)
{
#ifdef _MSC_VER
  return i;
#else
  return __atomic_load_n(&i, __ATOMIC_RELAXED);
#endif
}
inline void interlocked_release_store(volatile unsigned short &i, unsigned short val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELEASE);
#endif
}
inline void interlocked_relaxed_store(volatile unsigned short &i, unsigned short val)
{
#ifdef _MSC_VER
  i = val;
#else
  __atomic_store_n(&i, val, __ATOMIC_RELAXED);
#endif
}
