//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !_MSC_VER || __clang__ || defined(__ATOMIC_SEQ_CST) || !(defined(_M_IX86) || defined(_M_X64))
#error This file is for MSVC x86/x64 only!
#endif

#include <util/dag_stdint.h>
#include <intrin.h>

#if _MSVC_LANG < 202002L
#include <EASTL/type_traits.h>
#endif


// TODO: refactor to work with /volatile:iso by using explicit compiler barriers with __iso_volatile_load* instead
// Reference:
// https://www.cl.cam.ac.uk/~pes20/cpp/cpp0xmappings.html

#if !_TARGET_64BIT

// NOTE: fallback implementations of 64-bit atomic ops for x86 are copied from winnt.h but with less loads from memory.

__forceinline long long _InterlockedExchange64(long long volatile *ptr, long long val)
{
  long long old = *ptr;
  while (true)
  {
    long long tmp = _InterlockedCompareExchange64(ptr, val, old);
    if (tmp == old)
      break;
    old = tmp;
  }
  return old;
}
__forceinline long long _InterlockedExchangeAdd64(long long volatile *ptr, long long val)
{
  long long old = *ptr;
  while (true)
  {
    long long tmp = _InterlockedCompareExchange64(ptr, old + val, old);
    if (tmp == old)
      break;
    old = tmp;
  }
  return old;
}
__forceinline long long _InterlockedAnd64(long long volatile *ptr, long long val)
{
  long long old = *ptr;
  while (true)
  {
    long long tmp = _InterlockedCompareExchange64(ptr, old & val, old);
    if (tmp == old)
      break;
    old = tmp;
  }
  return old;
}
__forceinline long long _InterlockedOr64(long long volatile *ptr, long long val)
{
  long long old = *ptr;
  while (true)
  {
    long long tmp = _InterlockedCompareExchange64(ptr, old | val, old);
    if (tmp == old)
      break;
    old = tmp;
  }
  return old;
}

#endif

// clang-format off

// The ONLY reason we have to do it this way is because one of our components still cannot compile with C++20
// TODO: get rid of this when we can
#if _MSVC_LANG < 202002L
#define TEMPLATE_FOR_SIZE(T, size, R) template <class T> __forceinline typename eastl::enable_if<sizeof(T) == size, R>::type
#else
#define TEMPLATE_FOR_SIZE(T, size, R) template <class T> requires (sizeof(T) == size) __forceinline R
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// NUMERIC STORAGE OPERATIONS ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// __atomic_load_n

__forceinline bool     _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(bool     volatile const *ptr) { return *ptr; }
__forceinline int8_t   _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(int8_t   volatile const *ptr) { return *ptr; }
__forceinline uint8_t  _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(uint8_t  volatile const *ptr) { return *ptr; }
__forceinline int16_t  _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(int16_t  volatile const *ptr) { return *ptr; }
__forceinline uint16_t _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(uint16_t volatile const *ptr) { return *ptr; }
__forceinline int32_t  _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(int32_t  volatile const *ptr) { return *ptr; }
__forceinline uint32_t _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(uint32_t volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(int64_t  volatile const *ptr) { return *ptr; }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(uint64_t volatile const *ptr) { return *ptr; }
#else
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(int64_t  volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(uint64_t volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
template <class T>
__forceinline T *      _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(T *      volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, T)
                       _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(T        volatile const *ptr)
{
  __int64 result[2];
  _InterlockedCompareExchange128((__int64 volatile*)ptr, 0, 0, result);
  return *(const T *)result;
}
#endif

// NOTE: we use /volatile:ms, which means that volatile stores/loads are automatically acq/rel
__forceinline bool     _msvc_x86_atomic_load_n__ATOMIC_RELAXED(bool     volatile const *ptr) { return (bool    ) __iso_volatile_load8 ((char      volatile const *)ptr); }
__forceinline int8_t   _msvc_x86_atomic_load_n__ATOMIC_RELAXED(int8_t   volatile const *ptr) { return (int8_t  ) __iso_volatile_load8 ((char      volatile const *)ptr); }
__forceinline uint8_t  _msvc_x86_atomic_load_n__ATOMIC_RELAXED(uint8_t  volatile const *ptr) { return (uint8_t ) __iso_volatile_load8 ((char      volatile const *)ptr); }
__forceinline int16_t  _msvc_x86_atomic_load_n__ATOMIC_RELAXED(int16_t  volatile const *ptr) { return (int16_t ) __iso_volatile_load16((short     volatile const *)ptr); }
__forceinline uint16_t _msvc_x86_atomic_load_n__ATOMIC_RELAXED(uint16_t volatile const *ptr) { return (uint16_t) __iso_volatile_load16((short     volatile const *)ptr); }
__forceinline int32_t  _msvc_x86_atomic_load_n__ATOMIC_RELAXED(int32_t  volatile const *ptr) { return (int32_t ) __iso_volatile_load32((int       volatile const *)ptr); }
__forceinline uint32_t _msvc_x86_atomic_load_n__ATOMIC_RELAXED(uint32_t volatile const *ptr) { return (uint32_t) __iso_volatile_load32((int       volatile const *)ptr); }
#if _TARGET_64BIT
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_RELAXED(int64_t  volatile const *ptr) { return (int64_t ) __iso_volatile_load64((long long volatile const *)ptr); }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_RELAXED(uint64_t volatile const *ptr) { return (uint64_t) __iso_volatile_load64((long long volatile const *)ptr); }
template <class T>
__forceinline T *      _msvc_x86_atomic_load_n__ATOMIC_RELAXED(T *      volatile const *ptr) { return (T*      ) __iso_volatile_load64((long long volatile const *)ptr); }
#else
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_RELAXED(int64_t  volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_RELAXED(uint64_t volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
template <class T>
__forceinline T *      _msvc_x86_atomic_load_n__ATOMIC_RELAXED(T *      volatile const *ptr) { return (T*      ) __iso_volatile_load32((int       volatile const *)ptr); }
#endif
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, T)
                       _msvc_x86_atomic_load_n__ATOMIC_RELAXED(T        volatile const *ptr) { return _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(ptr); }
#endif

__forceinline bool     _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(bool     volatile const *ptr) { return *ptr; }
__forceinline int8_t   _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(int8_t   volatile const *ptr) { return *ptr; }
__forceinline uint8_t  _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(uint8_t  volatile const *ptr) { return *ptr; }
__forceinline int16_t  _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(int16_t  volatile const *ptr) { return *ptr; }
__forceinline uint16_t _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(uint16_t volatile const *ptr) { return *ptr; }
__forceinline int32_t  _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(int32_t  volatile const *ptr) { return *ptr; }
__forceinline uint32_t _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(uint32_t volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(int64_t  volatile const *ptr) { return *ptr; }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(uint64_t volatile const *ptr) { return *ptr; }
#else
__forceinline int64_t  _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(int64_t  volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
__forceinline uint64_t _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(uint64_t volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
template <class T>
__forceinline T *      _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(T *      volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, T)
                       _msvc_x86_atomic_load_n__ATOMIC_ACQUIRE(T        volatile const *ptr) { return _msvc_x86_atomic_load_n__ATOMIC_SEQ_CST(ptr); }
#endif

// __atomic_store_n

#define SEQ_CST_STORE_IMPL(msvc_exchange, msvc_type) msvc_exchange((msvc_type volatile *) ptr, (msvc_type) val);

__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     val) { SEQ_CST_STORE_IMPL(_InterlockedExchange8      , char     ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { SEQ_CST_STORE_IMPL(_InterlockedExchange8      , char     ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { SEQ_CST_STORE_IMPL(_InterlockedExchange8      , char     ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { SEQ_CST_STORE_IMPL(_InterlockedExchange16     , short    ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { SEQ_CST_STORE_IMPL(_InterlockedExchange16     , short    ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { SEQ_CST_STORE_IMPL(_InterlockedExchange       , long     ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { SEQ_CST_STORE_IMPL(_InterlockedExchange       , long     ) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { SEQ_CST_STORE_IMPL(_InterlockedExchange64     , long long) }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { SEQ_CST_STORE_IMPL(_InterlockedExchange64     , long long) }
template <class T>
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_SEQ_CST(T *      volatile *ptr, T *      val) { SEQ_CST_STORE_IMPL(_InterlockedExchangePointer, void *) }

#undef SEQ_CST_STORE_IMPL

__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(bool     volatile *ptr, bool     val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { *ptr = val; }
#if _TARGET_64BIT
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { *ptr = val; }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { *ptr = val; }
#else
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { _InterlockedExchange64((long long volatile *) ptr, val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { _InterlockedExchange64((long long volatile *) ptr, val); }
#endif
template <class T>
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELEASE(T *      volatile *ptr, T *      val) { *ptr = val; }

__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(bool     volatile *ptr, bool     val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { __iso_volatile_store16((short     volatile *) ptr, (short    ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { __iso_volatile_store16((short     volatile *) ptr, (short    ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { __iso_volatile_store32((int       volatile *) ptr, (int      ) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { __iso_volatile_store32((int       volatile *) ptr, (int      ) val); }
#if _TARGET_64BIT
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { __iso_volatile_store64((long long volatile *) ptr, (long long) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { __iso_volatile_store64((long long volatile *) ptr, (long long) val); }
template <class T>
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(T *      volatile *ptr, T *      val) { __iso_volatile_store64((long long volatile *) ptr, (long long) val); }
#else
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { _InterlockedExchange64((long long volatile *) ptr, (long long) val); }
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { _InterlockedExchange64((long long volatile *) ptr, (long long) val); }
template <class T>
__forceinline void _msvc_x86_atomic_store_n__ATOMIC_RELAXED(T *      volatile *ptr, T *      val) { __iso_volatile_store32((int       volatile *) ptr, (int      ) val); }
#endif


// __atomic_exchange_n

#define XCHG_IMPL(type, msvc_exchange, msvc_type) return (type) msvc_exchange((msvc_type volatile *) ptr, (msvc_type) val);

__forceinline bool     _msvc_x86_atomic_exchange_n(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8      , char     ) }
__forceinline int8_t   _msvc_x86_atomic_exchange_n(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8      , char     ) }
__forceinline uint8_t  _msvc_x86_atomic_exchange_n(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8      , char     ) }
__forceinline int16_t  _msvc_x86_atomic_exchange_n(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16     , short    ) }
__forceinline uint16_t _msvc_x86_atomic_exchange_n(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16     , short    ) }
__forceinline int32_t  _msvc_x86_atomic_exchange_n(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange       , long     ) }
__forceinline uint32_t _msvc_x86_atomic_exchange_n(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange       , long     ) }
__forceinline int64_t  _msvc_x86_atomic_exchange_n(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64     , long long) }
__forceinline uint64_t _msvc_x86_atomic_exchange_n(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64     , long long) }
template <class T>
__forceinline T *_msvc_x86_atomic_exchange_n(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer, void *) }

#undef XCHG_IMPL


// __atomic_compare_exchange_n

#define CAS_IMPL(type, msvc_cas, msvc_type) type const observed = (type) msvc_cas((msvc_type volatile *)ptr, (msvc_type) desired, (msvc_type) *expected); const bool success = *expected == observed; *expected = observed; return success;

__forceinline bool _msvc_x86_atomic_compare_exchange_n(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _msvc_x86_atomic_compare_exchange_n(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _msvc_x86_atomic_compare_exchange_n(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void *   ) }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, bool)
                   _msvc_x86_atomic_compare_exchange_n(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  unsigned char result = _InterlockedCompareExchange128((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
  return !!result;
}
#endif

#undef CAS_IMPL

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// GENERIC STORAGE OPERATIONS ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// __atomic_load

TEMPLATE_FOR_SIZE(T,  1, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)  { *ret = *ptr; }
TEMPLATE_FOR_SIZE(T,  2, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)  { *ret = *ptr; }
TEMPLATE_FOR_SIZE(T,  4, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)  { *ret = *ptr; }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)  { *ret = *ptr; }
#else
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)  { *(long long*)ret = _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, void) _msvc_x86_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret)
{
  _InterlockedCompareExchange128((__int64 volatile*)ptr, 0, 0, (__int64*)&ret);
}
#endif

TEMPLATE_FOR_SIZE(T,  1, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret)  { *(char      *)ret = __iso_volatile_load8 ((char      volatile *) ptr); }
TEMPLATE_FOR_SIZE(T,  2, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret)  { *(short     *)ret = __iso_volatile_load16((short     volatile *) ptr); }
TEMPLATE_FOR_SIZE(T,  4, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret)  { *(int       *)ret = __iso_volatile_load32((int       volatile *) ptr); }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret)  { *(long long *)ret = __iso_volatile_load64((long long volatile *) ptr); }
#else
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret)  { *(long long *)ret = _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, void) _msvc_x86_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) { _msvc_x86_atomic_load__ATOMIC_SEQ_CST(ptr, ret); }
#endif

TEMPLATE_FOR_SIZE(T,  1, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { *ret = *ptr; }
TEMPLATE_FOR_SIZE(T,  2, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { *ret = *ptr; }
TEMPLATE_FOR_SIZE(T,  4, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { *ret = *ptr; }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { *ret = *ptr; }
#else
TEMPLATE_FOR_SIZE(T,  8, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { *(long long*)ret = _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, void) _msvc_x86_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) { _msvc_x86_atomic_load__ATOMIC_SEQ_CST(ptr, ret); }
#endif

// __atomic_store

#define SEQ_CST_STORE_IMPL(msvc_exchange, msvc_type) msvc_exchange((msvc_type volatile *) ptr, *(msvc_type*) val);

TEMPLATE_FOR_SIZE(T, 1, void) _msvc_x86_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) { SEQ_CST_STORE_IMPL(_InterlockedExchange8      , char     ) }
TEMPLATE_FOR_SIZE(T, 2, void) _msvc_x86_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) { SEQ_CST_STORE_IMPL(_InterlockedExchange16     , short    ) }
TEMPLATE_FOR_SIZE(T, 4, void) _msvc_x86_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) { SEQ_CST_STORE_IMPL(_InterlockedExchange       , long     ) }
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) { SEQ_CST_STORE_IMPL(_InterlockedExchange64     , long long) }

#undef SEQ_CST_STORE_IMPL

TEMPLATE_FOR_SIZE(T, 1, void) _msvc_x86_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) { *ptr = *val; }
TEMPLATE_FOR_SIZE(T, 2, void) _msvc_x86_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) { *ptr = *val; }
TEMPLATE_FOR_SIZE(T, 4, void) _msvc_x86_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) { *ptr = *val; }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) { *ptr = *val; }
#else
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) { _InterlockedExchange64((long long volatile *) ptr, *(long long*)val); }
#endif

TEMPLATE_FOR_SIZE(T, 1, void) _msvc_x86_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) { __iso_volatile_store8 ((char      volatile *) ptr, *(char     *)val); }
TEMPLATE_FOR_SIZE(T, 2, void) _msvc_x86_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) { __iso_volatile_store16((short     volatile *) ptr, *(short    *)val); }
TEMPLATE_FOR_SIZE(T, 4, void) _msvc_x86_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) { __iso_volatile_store32((int       volatile *) ptr, *(int      *)val); }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) { __iso_volatile_store64((long long volatile *) ptr, *(long long*)val); }
#else
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) { _InterlockedExchange64((long long volatile *) ptr, *(long long*)val); }
#endif


// __atomic_exchange

#define XCHG_IMPL(type, msvc_exchange, msvc_type) *(msvc_type*)ret = (type) msvc_exchange((msvc_type volatile *) ptr, *(msvc_type*) val);

TEMPLATE_FOR_SIZE(T, 1, void) _msvc_x86_atomic_exchange(T volatile *ptr, const T *val, T* ret) { XCHG_IMPL(T, _InterlockedExchange8      , char     ) }
TEMPLATE_FOR_SIZE(T, 2, void) _msvc_x86_atomic_exchange(T volatile *ptr, const T *val, T* ret) { XCHG_IMPL(T, _InterlockedExchange16     , short    ) }
TEMPLATE_FOR_SIZE(T, 4, void) _msvc_x86_atomic_exchange(T volatile *ptr, const T *val, T* ret) { XCHG_IMPL(T, _InterlockedExchange       , long     ) }
TEMPLATE_FOR_SIZE(T, 8, void) _msvc_x86_atomic_exchange(T volatile *ptr, const T *val, T* ret) { XCHG_IMPL(T, _InterlockedExchange64     , long long) }

#undef XCHG_IMPL


// __atomic_compare_exchange

#define CAS_IMPL(type, msvc_cas, msvc_type) type const observed = (type) msvc_cas((msvc_type volatile *)ptr, *(msvc_type*) desired, *(msvc_type*) expected); const bool success = *(msvc_type*)expected == *(msvc_type*)&observed; *(msvc_type*)expected = *(msvc_type*)&observed; return success;

TEMPLATE_FOR_SIZE(T,  1, bool) _msvc_x86_atomic_compare_exchange(T volatile *ptr, T *expected, const T *desired) { CAS_IMPL(T, _InterlockedCompareExchange8      , char     ) }
TEMPLATE_FOR_SIZE(T,  2, bool) _msvc_x86_atomic_compare_exchange(T volatile *ptr, T *expected, const T *desired) { CAS_IMPL(T, _InterlockedCompareExchange16     , short    ) }
TEMPLATE_FOR_SIZE(T,  4, bool) _msvc_x86_atomic_compare_exchange(T volatile *ptr, T *expected, const T *desired) { CAS_IMPL(T, _InterlockedCompareExchange       , long     ) }
TEMPLATE_FOR_SIZE(T,  8, bool) _msvc_x86_atomic_compare_exchange(T volatile *ptr, T *expected, const T *desired) { CAS_IMPL(T, _InterlockedCompareExchange64     , long long) }
#if _TARGET_64BIT
TEMPLATE_FOR_SIZE(T, 16, bool) _msvc_x86_atomic_compare_exchange(T volatile *ptr, T *expected, const T *desired)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  unsigned char result = _InterlockedCompareExchange128((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
  return !!result;
}
#endif

#undef CAS_IMPL

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// ARITHMETIC OPERATIONS //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// __atomic_fetch_add

__forceinline int8_t   _msvc_x86_atomic_fetch_add(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_x86_atomic_fetch_add(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_x86_atomic_fetch_add(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_x86_atomic_fetch_add(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_x86_atomic_fetch_add(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_x86_atomic_fetch_add(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_x86_atomic_fetch_add(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_x86_atomic_fetch_add(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }

// __atomic_fetch_sub

__forceinline int8_t   _msvc_x86_atomic_fetch_sub(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_x86_atomic_fetch_sub(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_x86_atomic_fetch_sub(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_x86_atomic_fetch_sub(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_x86_atomic_fetch_sub(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_x86_atomic_fetch_sub(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_x86_atomic_fetch_sub(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_x86_atomic_fetch_sub(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }

// __atomic_add_fetch

#define AF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, (msvc_type) val) + val;

__forceinline int8_t   _msvc_x86_atomic_add_fetch(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_x86_atomic_add_fetch(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_x86_atomic_add_fetch(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_x86_atomic_add_fetch(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_x86_atomic_add_fetch(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_x86_atomic_add_fetch(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_x86_atomic_add_fetch(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_x86_atomic_add_fetch(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef AF_IMPL

// __atomic_sub_fetch

#define SF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, -(msvc_type) val) - val;

__forceinline int8_t   _msvc_x86_atomic_sub_fetch(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_x86_atomic_sub_fetch(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_x86_atomic_sub_fetch(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_x86_atomic_sub_fetch(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_x86_atomic_sub_fetch(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_x86_atomic_sub_fetch(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_x86_atomic_sub_fetch(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_x86_atomic_sub_fetch(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef SF_IMPL


// __atomic_fetch_and

__forceinline int8_t   _msvc_x86_atomic_fetch_and(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_x86_atomic_fetch_and(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_x86_atomic_fetch_and(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_x86_atomic_fetch_and(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_x86_atomic_fetch_and(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_x86_atomic_fetch_and(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_x86_atomic_fetch_and(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_x86_atomic_fetch_and(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64((long long volatile *) ptr, val); }


// __atomic_fetch_or

__forceinline int8_t   _msvc_x86_atomic_fetch_or(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_x86_atomic_fetch_or(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_x86_atomic_fetch_or(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_x86_atomic_fetch_or(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_x86_atomic_fetch_or(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_x86_atomic_fetch_or(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_x86_atomic_fetch_or(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_x86_atomic_fetch_or(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64((long long volatile *) ptr, val); }

// __atomic_test_and_set

__forceinline bool _msvc_x86_atomic_test_and_set(bool volatile *ptr) { return _InterlockedExchange8((char volatile *) ptr, 1); }

// __atomic_clear

__forceinline void _msvc_x86_atomic_clear__ATOMIC_RELAXED(bool volatile *ptr) { __iso_volatile_store8((char volatile *) ptr, 0); }
__forceinline void _msvc_x86_atomic_clear__ATOMIC_RELEASE(bool volatile *ptr) { *ptr = false; }
__forceinline void _msvc_x86_atomic_clear__ATOMIC_SEQ_CST(bool volatile *ptr) { _InterlockedExchange8((char volatile *) ptr, 0); }

// clang-format on

#define __atomic_load_n(ptr, order)          _msvc_x86_atomic_load_n##order((ptr))
#define __atomic_store_n(ptr, val, order)    _msvc_x86_atomic_store_n##order((ptr), (val))
#define __atomic_exchange_n(ptr, val, order) _msvc_x86_atomic_exchange_n((ptr), (val))
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, success, failure) \
  _msvc_x86_atomic_compare_exchange_n((ptr), (expected), (desired))

#define __atomic_load(ptr, ret, order)          _msvc_x86_atomic_load##order((ptr), (ret))
#define __atomic_store(ptr, val, order)         _msvc_x86_atomic_store##order((ptr), (val))
#define __atomic_exchange(ptr, val, ret, order) _msvc_x86_atomic_exchange((ptr), (val), (ret))
#define __atomic_compare_exchange(ptr, expected, desired, weak, success, failure) \
  _msvc_x86_atomic_compare_exchange((ptr), (expected), (desired))

#define __atomic_add_fetch(ptr, val, order) _msvc_x86_atomic_add_fetch((ptr), (val))
#define __atomic_sub_fetch(ptr, val, order) _msvc_x86_atomic_sub_fetch((ptr), (val))
#define __atomic_and_fetch(ptr, val, order) _msvc_x86_atomic_and_fetch((ptr), (val))
#define __atomic_or_fetch(ptr, val, order)  _msvc_x86_atomic_or_fetch((ptr), (val))
#define __atomic_fetch_add(ptr, val, order) _msvc_x86_atomic_fetch_add((ptr), (val))
#define __atomic_fetch_sub(ptr, val, order) _msvc_x86_atomic_fetch_sub((ptr), (val))
#define __atomic_fetch_and(ptr, val, order) _msvc_x86_atomic_fetch_and((ptr), (val))
#define __atomic_fetch_or(ptr, val, order)  _msvc_x86_atomic_fetch_or((ptr), (val))

#define __atomic_test_and_set(ptr, order) _msvc_x86_atomic_test_and_set((ptr))
#define __atomic_clear(ptr, order)        _msvc_x86_atomic_clear##order((ptr))

#undef TEMPLATE_FOR_SIZE
