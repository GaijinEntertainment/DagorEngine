//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !_MSC_VER || __clang__ || defined(__ATOMIC_SEQ_CST)
#error This file is for MSVC only! Not even clang-cl!
#endif

#include <util/dag_stdint.h>
#include <intrin.h>


// TODO: we should turn off the /volatile:ms flag, because it pessimizes the codegen
// on ARM for relaxed operations. This is not a problem though, because we don't use MSVC for much of anything.

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

// __atomic_load_n

// NOTE: we use /volatile:ms, which means that volatile stores/loads are automatically acq/rel
__forceinline bool     _non_gcc_atomic_load_n__ATOMIC_RELAXED(bool     volatile const *ptr) { return *ptr; }
__forceinline int8_t   _non_gcc_atomic_load_n__ATOMIC_RELAXED(int8_t   volatile const *ptr) { return *ptr; }
__forceinline uint8_t  _non_gcc_atomic_load_n__ATOMIC_RELAXED(uint8_t  volatile const *ptr) { return *ptr; }
__forceinline int16_t  _non_gcc_atomic_load_n__ATOMIC_RELAXED(int16_t  volatile const *ptr) { return *ptr; }
__forceinline uint16_t _non_gcc_atomic_load_n__ATOMIC_RELAXED(uint16_t volatile const *ptr) { return *ptr; }
__forceinline int32_t  _non_gcc_atomic_load_n__ATOMIC_RELAXED(int32_t  volatile const *ptr) { return *ptr; }
__forceinline uint32_t _non_gcc_atomic_load_n__ATOMIC_RELAXED(uint32_t volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
__forceinline int64_t  _non_gcc_atomic_load_n__ATOMIC_RELAXED(int64_t  volatile const *ptr) { return *ptr; }
__forceinline uint64_t _non_gcc_atomic_load_n__ATOMIC_RELAXED(uint64_t volatile const *ptr) { return *ptr; }
#else
// TODO: Suboptimal but we don't care much for Win32 perf and a better way requires doing raw ASM
__forceinline int64_t  _non_gcc_atomic_load_n__ATOMIC_RELAXED(int64_t  volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
__forceinline uint64_t _non_gcc_atomic_load_n__ATOMIC_RELAXED(uint64_t volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
template <class T>
__forceinline T *      _non_gcc_atomic_load_n__ATOMIC_RELAXED(T *      volatile const *ptr) { return *ptr; }

__forceinline bool     _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(bool     volatile const *ptr) { return *ptr; }
__forceinline int8_t   _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(int8_t   volatile const *ptr) { return *ptr; }
__forceinline uint8_t  _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(uint8_t  volatile const *ptr) { return *ptr; }
__forceinline int16_t  _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(int16_t  volatile const *ptr) { return *ptr; }
__forceinline uint16_t _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(uint16_t volatile const *ptr) { return *ptr; }
__forceinline int32_t  _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(int32_t  volatile const *ptr) { return *ptr; }
__forceinline uint32_t _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(uint32_t volatile const *ptr) { return *ptr; }
#if _TARGET_64BIT
__forceinline int64_t  _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(int64_t  volatile const *ptr) { return *ptr; }
__forceinline uint64_t _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(uint64_t volatile const *ptr) { return *ptr; }
#else
// TODO: Suboptimal but we don't care much for Win32 perf and a better way requires doing raw ASM
__forceinline int64_t  _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(int64_t  volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
__forceinline uint64_t _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(uint64_t volatile const *ptr) { return _InterlockedCompareExchange64((long long volatile *) ptr, 0, 0); }
#endif
template <class T>
__forceinline T *      _non_gcc_atomic_load_n__ATOMIC_ACQUIRE(T *      volatile const *ptr) { return *ptr; }


// __atomic_store_n

__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(bool     volatile *ptr, bool     val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { *ptr = val; }
#if _TARGET_64BIT
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { *ptr = val; }
#else
// TODO: Suboptimal too, but better solution requires raw ASM
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { _InterlockedExchange64((long long volatile *) ptr, val); }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { _InterlockedExchange64((long long volatile *) ptr, val); }
#endif
template <class T>
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELEASE(T *      volatile *ptr, T *      val) { *ptr = val; }

__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(bool     volatile *ptr, bool     val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { *ptr = val; }
#if _TARGET_64BIT
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { *ptr = val; }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { *ptr = val; }
#else
// TODO: Suboptimal too, but better solution requires raw ASM
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { _InterlockedExchange64((long long volatile *) ptr, val); }
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { _InterlockedExchange64((long long volatile *) ptr, val); }
#endif
template <class T>
__forceinline void _non_gcc_atomic_store_n__ATOMIC_RELAXED(T *volatile *ptr, T *val) { *ptr = val; }


// __atomic_exchange_n

#define XCHG_IMPL(type, msvc_exchange, msvc_type) return (type) msvc_exchange((msvc_type volatile *) ptr, (msvc_type) val);

__forceinline bool     _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8      , char     ) }
__forceinline int8_t   _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8      , char     ) }
__forceinline uint8_t  _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8      , char     ) }
__forceinline int16_t  _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16     , short    ) }
__forceinline uint16_t _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16     , short    ) }
__forceinline int32_t  _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange       , long     ) }
__forceinline uint32_t _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange       , long     ) }
__forceinline int64_t  _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64     , long long) }
__forceinline uint64_t _non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64     , long long) }
template <class T>
__forceinline T *_non_gcc_atomic_exchange_n__ATOMIC_SEQ_CST(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer, void *) }

// TODO: silent promotion to SEQ_CST, this is a pessimization!!!
__forceinline bool     _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8      , char     ) }
__forceinline int8_t   _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8      , char     ) }
__forceinline uint8_t  _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8      , char     ) }
__forceinline int16_t  _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16     , short    ) }
__forceinline uint16_t _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16     , short    ) }
__forceinline int32_t  _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange       , long     ) }
__forceinline uint32_t _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange       , long     ) }
__forceinline int64_t  _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64     , long long) }
__forceinline uint64_t _non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64     , long long) }
template <class T>
__forceinline T *_non_gcc_atomic_exchange_n__ATOMIC_ACQUIRE(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer, void *) }

#undef XCHG_IMPL


// __atomic_compare_exchange_n

#define CAS_IMPL(type, msvc_cas, msvc_type) type const observed = (type) msvc_cas((msvc_type volatile *)ptr, (msvc_type) desired, (msvc_type) *expected); const bool success = *expected == observed; *expected = observed; return success;

__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void *   ) }

// TODO: silent promotion to SEQ_CST, this is a pessimization!!!
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _non_gcc_atomic_compare_exchange_n_false__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void *   ) }

#undef CAS_IMPL


// __atomic_add_fetch

#define AF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, (msvc_type) val) + val;

__forceinline int8_t   _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _non_gcc_atomic_add_fetch__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef AF_IMPL

// __atomic_sub_fetch

#define SF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, -(msvc_type) val) - val;

__forceinline int8_t   _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _non_gcc_atomic_sub_fetch__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef SF_IMPL


// __atomic_fetch_and

__forceinline int8_t   _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline uint16_t _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline int32_t  _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64((long long volatile *) ptr, val); }
__forceinline uint64_t _non_gcc_atomic_fetch_and__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64((long long volatile *) ptr, val); }


// __atomic_fetch_or

__forceinline int8_t   _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline uint16_t _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline int32_t  _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline uint32_t _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline int64_t  _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64((long long volatile *) ptr, val); }
__forceinline uint64_t _non_gcc_atomic_fetch_or__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64((long long volatile *) ptr, val); }

// clang-format on


#define __atomic_load_n(ptr, order)          _non_gcc_atomic_load_n##order((ptr))
#define __atomic_store_n(ptr, val, order)    _non_gcc_atomic_store_n##order((ptr), (val))
#define __atomic_exchange_n(ptr, val, order) _non_gcc_atomic_exchange_n##order((ptr), (val))
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, success, failure) \
  _non_gcc_atomic_compare_exchange_n_##weak##success##failure((ptr), (expected), (desired))
#define __atomic_add_fetch(ptr, val, order) _non_gcc_atomic_add_fetch##order((ptr), (val))
#define __atomic_sub_fetch(ptr, val, order) _non_gcc_atomic_sub_fetch##order((ptr), (val))
#define __atomic_and_fetch(ptr, val, order) _non_gcc_atomic_and_fetch##order((ptr), (val))
#define __atomic_or_fetch(ptr, val, order)  _non_gcc_atomic_or_fetch##order((ptr), (val))
#define __atomic_fetch_add(ptr, val, order) _non_gcc_atomic_fetch_add##order((ptr), (val))
#define __atomic_fetch_sub(ptr, val, order) _non_gcc_atomic_fetch_sub##order((ptr), (val))
#define __atomic_fetch_and(ptr, val, order) _non_gcc_atomic_fetch_and##order((ptr), (val))
#define __atomic_fetch_or(ptr, val, order)  _non_gcc_atomic_fetch_or##order((ptr), (val))
