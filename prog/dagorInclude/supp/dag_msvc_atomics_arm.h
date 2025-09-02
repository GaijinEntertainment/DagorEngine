//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !_MSC_VER || __clang__ || defined(__ATOMIC_SEQ_CST) || !defined(_M_ARM64)
#error This file is for MSVC arm64 only!
#endif

#include <util/dag_stdint.h>

#include <intrin.h>

// NOTE: this is only for aarch64, we don't support MSVC + 32-bit arm

// TODO: the implementation is suboptimal.
// https://learn.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics?view=msvc-170
// Reference:
// https://www.cl.cam.ac.uk/~pes20/cpp/cpp0xmappings.html#:~:text=AArch64%20implementation

// MSVC's compiler barrier which presumably needs to be done after various intrinsics except for __iso_volatile_load*
// This is all quite dubious, but this is the way MSVC STL does it.
#define cb() _Pragma("warning(push)") _Pragma("warning(disable : 4996)") _ReadWriteBarrier() _Pragma("warning(pop)")

// clang-format off

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// NUMERIC STORAGE OPERATIONS ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// __atomic_load_n

#define ACQ_LOAD_IMPL(type, msvc_load, msvc_type) type res = (type)msvc_load((unsigned msvc_type volatile *) ptr); cb(); return res;

__forceinline bool     _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(bool     volatile const *ptr) { ACQ_LOAD_IMPL(bool    , __load_acquire8,  __int8 ) }
__forceinline int8_t   _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(int8_t   volatile const *ptr) { ACQ_LOAD_IMPL(int8_t  , __load_acquire8,  __int8 ) }
__forceinline uint8_t  _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(uint8_t  volatile const *ptr) { ACQ_LOAD_IMPL(uint8_t , __load_acquire8,  __int8 ) }
__forceinline int16_t  _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(int16_t  volatile const *ptr) { ACQ_LOAD_IMPL(int16_t , __load_acquire16, __int16) }
__forceinline uint16_t _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(uint16_t volatile const *ptr) { ACQ_LOAD_IMPL(uint16_t, __load_acquire16, __int16) }
__forceinline int32_t  _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(int32_t  volatile const *ptr) { ACQ_LOAD_IMPL(int32_t , __load_acquire32, __int32) }
__forceinline uint32_t _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(uint32_t volatile const *ptr) { ACQ_LOAD_IMPL(uint32_t, __load_acquire32, __int32) }
__forceinline int64_t  _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(int64_t  volatile const *ptr) { ACQ_LOAD_IMPL(int64_t , __load_acquire64, __int64) }
__forceinline uint64_t _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(uint64_t volatile const *ptr) { ACQ_LOAD_IMPL(uint64_t, __load_acquire64, __int64) }
template <class T>
__forceinline T *      _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(T *      volatile const *ptr) { ACQ_LOAD_IMPL(T*,       __load_acquire64, __int64) }
template <class T> requires (sizeof(T) == 16)
__forceinline T        _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(T        volatile const *ptr)
{
  __int64 result[2];
  _InterlockedCompareExchange128((__int64 volatile*)ptr, 0, 0, result);
  return *(const T *)result;
}

__forceinline bool     _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(bool     volatile const *ptr) { ACQ_LOAD_IMPL(bool    , __load_acquire8,  __int8 ) }
__forceinline int8_t   _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(int8_t   volatile const *ptr) { ACQ_LOAD_IMPL(int8_t  , __load_acquire8,  __int8 ) }
__forceinline uint8_t  _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(uint8_t  volatile const *ptr) { ACQ_LOAD_IMPL(uint8_t , __load_acquire8,  __int8 ) }
__forceinline int16_t  _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(int16_t  volatile const *ptr) { ACQ_LOAD_IMPL(int16_t , __load_acquire16, __int16) }
__forceinline uint16_t _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(uint16_t volatile const *ptr) { ACQ_LOAD_IMPL(uint16_t, __load_acquire16, __int16) }
__forceinline int32_t  _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(int32_t  volatile const *ptr) { ACQ_LOAD_IMPL(int32_t , __load_acquire32, __int32) }
__forceinline uint32_t _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(uint32_t volatile const *ptr) { ACQ_LOAD_IMPL(uint32_t, __load_acquire32, __int32) }
__forceinline int64_t  _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(int64_t  volatile const *ptr) { ACQ_LOAD_IMPL(int64_t , __load_acquire64, __int64) }
__forceinline uint64_t _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(uint64_t volatile const *ptr) { ACQ_LOAD_IMPL(uint64_t, __load_acquire64, __int64) }
template <class T>
__forceinline T *      _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(T *      volatile const *ptr) { ACQ_LOAD_IMPL(T*,       __load_acquire64, __int64) }
template <class T> requires (sizeof(T) == 16)
__forceinline T        _msvc_arm_atomic_load_n__ATOMIC_ACQUIRE(T        volatile const *ptr) { return _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(ptr); }

#undef ACQ_LOAD_IMPL

__forceinline bool     _msvc_arm_atomic_load_n__ATOMIC_RELAXED(bool     volatile const *ptr) { return __iso_volatile_load8 ((char      volatile *) ptr); }
__forceinline int8_t   _msvc_arm_atomic_load_n__ATOMIC_RELAXED(int8_t   volatile const *ptr) { return __iso_volatile_load8 ((char      volatile *) ptr); }
__forceinline uint8_t  _msvc_arm_atomic_load_n__ATOMIC_RELAXED(uint8_t  volatile const *ptr) { return __iso_volatile_load8 ((char      volatile *) ptr); }
__forceinline int16_t  _msvc_arm_atomic_load_n__ATOMIC_RELAXED(int16_t  volatile const *ptr) { return __iso_volatile_load16((short     volatile *) ptr); }
__forceinline uint16_t _msvc_arm_atomic_load_n__ATOMIC_RELAXED(uint16_t volatile const *ptr) { return __iso_volatile_load16((short     volatile *) ptr); }
__forceinline int32_t  _msvc_arm_atomic_load_n__ATOMIC_RELAXED(int32_t  volatile const *ptr) { return __iso_volatile_load32((int       volatile *) ptr); }
__forceinline uint32_t _msvc_arm_atomic_load_n__ATOMIC_RELAXED(uint32_t volatile const *ptr) { return __iso_volatile_load32((int       volatile *) ptr); }
__forceinline int64_t  _msvc_arm_atomic_load_n__ATOMIC_RELAXED(int64_t  volatile const *ptr) { return __iso_volatile_load64((long long volatile *) ptr); }
__forceinline uint64_t _msvc_arm_atomic_load_n__ATOMIC_RELAXED(uint64_t volatile const *ptr) { return __iso_volatile_load64((long long volatile *) ptr); }
template <class T>
__forceinline T *      _msvc_arm_atomic_load_n__ATOMIC_RELAXED(T *      volatile const *ptr) { return (T*)__iso_volatile_load64((long long*) ptr); }
template <class T> requires (sizeof(T) == 16)
__forceinline T        _msvc_arm_atomic_load_n__ATOMIC_RELAXED(T        volatile const *ptr) {  return _msvc_arm_atomic_load_n__ATOMIC_SEQ_CST(ptr); }

// __atomic_store_n

// NOTE: MSVC STL does a dum-dum here and issues a `dmb ish` after the store, which is not needed even for seq_cst.
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { cb(); __stlr16((unsigned __int16 volatile *) ptr, (unsigned __int16) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { cb(); __stlr16((unsigned __int16 volatile *) ptr, (unsigned __int16) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { cb(); __stlr32((unsigned __int32 volatile *) ptr, (unsigned __int32) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { cb(); __stlr32((unsigned __int32 volatile *) ptr, (unsigned __int32) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { cb(); __stlr64((unsigned __int64 volatile *) ptr, (unsigned __int64) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { cb(); __stlr64((unsigned __int64 volatile *) ptr, (unsigned __int64) val); }
template <class T>
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_SEQ_CST(T *      volatile *ptr, T *      val) { cb(); __stlr64((unsigned __int64 volatile *) ptr, (unsigned __int64) val);; }

__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(bool     volatile *ptr, bool     val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, (unsigned __int8 ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { cb(); __stlr16((unsigned __int16 volatile *) ptr, (unsigned __int16) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { cb(); __stlr16((unsigned __int16 volatile *) ptr, (unsigned __int16) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { cb(); __stlr32((unsigned __int32 volatile *) ptr, (unsigned __int32) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { cb(); __stlr32((unsigned __int32 volatile *) ptr, (unsigned __int32) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { cb(); __stlr64((unsigned __int64 volatile *) ptr, (unsigned __int64) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { cb(); __stlr64((unsigned __int64 volatile *) ptr, (unsigned __int64) val); }
template <class T>
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELEASE(T *      volatile *ptr, T *      val) { *ptr = val; }

__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(bool     volatile *ptr, bool     val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { __iso_volatile_store8 ((char      volatile *) ptr, (char     ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { __iso_volatile_store16((short     volatile *) ptr, (short    ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { __iso_volatile_store16((short     volatile *) ptr, (short    ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { __iso_volatile_store32((int       volatile *) ptr, (int      ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { __iso_volatile_store32((int       volatile *) ptr, (int      ) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { __iso_volatile_store64((long long volatile *) ptr, (long long) val); }
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { __iso_volatile_store64((long long volatile *) ptr, (long long) val); }
template <class T>
__forceinline void _msvc_arm_atomic_store_n__ATOMIC_RELAXED(T *volatile *ptr, T *val) { *ptr = val; }


// __atomic_exchange_n

#define XCHG_IMPL(type, msvc_exchange, msvc_type) return (type) msvc_exchange((msvc_type volatile *) ptr, (msvc_type) val);

__forceinline bool     _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8      , char     ) }
__forceinline int8_t   _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8      , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8      , char     ) }
__forceinline int16_t  _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16     , short    ) }
__forceinline uint16_t _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16     , short    ) }
__forceinline int32_t  _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange       , long     ) }
__forceinline uint32_t _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange       , long     ) }
__forceinline int64_t  _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64     , long long) }
__forceinline uint64_t _msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64     , long long) }
template <class T>
__forceinline T *_msvc_arm_atomic_exchange_n__ATOMIC_SEQ_CST(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer, void *) }

__forceinline bool     _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8_nf      , char     ) }
__forceinline int8_t   _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8_nf      , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8_nf      , char     ) }
__forceinline int16_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16_nf     , short    ) }
__forceinline uint16_t _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16_nf     , short    ) }
__forceinline int32_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange_nf       , long     ) }
__forceinline uint32_t _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange_nf       , long     ) }
__forceinline int64_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64_nf     , long long) }
__forceinline uint64_t _msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64_nf     , long long) }
template <class T>
__forceinline T *_msvc_arm_atomic_exchange_n__ATOMIC_RELAXED(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer_nf, void *) }

__forceinline bool     _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8_rel      , char     ) }
__forceinline int8_t   _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8_rel      , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8_rel      , char     ) }
__forceinline int16_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16_rel     , short    ) }
__forceinline uint16_t _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16_rel     , short    ) }
__forceinline int32_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange_rel       , long     ) }
__forceinline uint32_t _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange_rel       , long     ) }
__forceinline int64_t  _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64_rel     , long long) }
__forceinline uint64_t _msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64_rel     , long long) }
template <class T>
__forceinline T *_msvc_arm_atomic_exchange_n__ATOMIC_RELEASE(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer_rel, void *) }

__forceinline bool     _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8_acq      , char     ) }
__forceinline int8_t   _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8_acq      , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8_acq      , char     ) }
__forceinline int16_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16_acq     , short    ) }
__forceinline uint16_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16_acq     , short    ) }
__forceinline int32_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange_acq       , long     ) }
__forceinline uint32_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange_acq       , long     ) }
__forceinline int64_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64_acq     , long long) }
__forceinline uint64_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64_acq     , long long) }
template <class T>
__forceinline T *_msvc_arm_atomic_exchange_n__ATOMIC_ACQUIRE(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer_acq, void *) }

__forceinline bool     _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(bool     volatile *ptr, bool     val) { XCHG_IMPL(bool    , _InterlockedExchange8      , char     ) }
__forceinline int8_t   _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { XCHG_IMPL(int8_t  , _InterlockedExchange8      , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { XCHG_IMPL(uint8_t , _InterlockedExchange8      , char     ) }
__forceinline int16_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { XCHG_IMPL(int16_t , _InterlockedExchange16     , short    ) }
__forceinline uint16_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { XCHG_IMPL(uint16_t, _InterlockedExchange16     , short    ) }
__forceinline int32_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { XCHG_IMPL(int32_t , _InterlockedExchange       , long     ) }
__forceinline uint32_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { XCHG_IMPL(uint32_t, _InterlockedExchange       , long     ) }
__forceinline int64_t  _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { XCHG_IMPL(int64_t , _InterlockedExchange64     , long long) }
__forceinline uint64_t _msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { XCHG_IMPL(uint64_t, _InterlockedExchange64     , long long) }
template <class T>
__forceinline T *_msvc_arm_atomic_exchange_n__ATOMIC_ACQ_REL(T *volatile *ptr, T *val) { XCHG_IMPL(T *, _InterlockedExchangePointer, void *) }

#undef XCHG_IMPL


// __atomic_compare_exchange_n
// NOTE: MSVC does not provide enough intrinsics to implement CAS optimally for ARM64.

#define CAS_IMPL(type, msvc_cas, msvc_type) type const observed = (type) msvc_cas((msvc_type volatile *)ptr, (msvc_type) desired, (msvc_type) *expected); const bool success = *expected == observed; *expected = observed; return success;

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void *   ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void *   ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T        volatile *ptr, T        *expected, const T &desired)
{
  return _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64_acq     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64_acq     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer_acq, void *   ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8_rel      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8_rel      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8_rel      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16_rel     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16_rel     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange_rel       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange_rel       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64_rel     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64_rel     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer_rel, void *   ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_RELAXED(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_rel((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8_nf      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8_nf      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8_nf      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16_nf     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16_nf     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange_nf       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange_nf       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64_nf     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64_nf     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer_nf, void *   ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_RELAXED(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_nf((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void*    ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T        volatile *ptr, T        *expected, const T &desired)
{
  return _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64_acq     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64_acq     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer_acq, void*    ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer, void*    ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T        volatile *ptr, T        *expected, const T &desired)
{
  return _msvc_arm_atomic_compare_exchange_n__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(bool     volatile *ptr, bool     *expected, bool     desired) { CAS_IMPL(bool    , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   *expected, int8_t   desired) { CAS_IMPL(int8_t  , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  *expected, uint8_t  desired) { CAS_IMPL(uint8_t , _InterlockedCompareExchange8_acq      , char     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  *expected, int16_t  desired) { CAS_IMPL(int16_t , _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t *expected, uint16_t desired) { CAS_IMPL(uint16_t, _InterlockedCompareExchange16_acq     , short    ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  *expected, int32_t  desired) { CAS_IMPL(int32_t , _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t *expected, uint32_t desired) { CAS_IMPL(uint32_t, _InterlockedCompareExchange_acq       , long     ) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  *expected, int64_t  desired) { CAS_IMPL(int64_t , _InterlockedCompareExchange64_acq     , long long) }
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t *expected, uint64_t desired) { CAS_IMPL(uint64_t, _InterlockedCompareExchange64_acq     , long long) }
template <class T>
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T *      volatile *ptr, T *      *expected, T *      desired) { CAS_IMPL(T *     , _InterlockedCompareExchangePointer_acq, void*    ) }
template <class T> requires (sizeof(T) == 16)
__forceinline bool _msvc_arm_atomic_compare_exchange_n__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T        volatile *ptr, T        *expected, const T &desired)
{
  const __int64 *lo = (const __int64 *)&desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

#undef CAS_IMPL

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// GENERIC STORAGE OPERATIONS ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// __atomic_load

#define ACQ_LOAD_IMPL(msvc_load, msvc_type) *(unsigned msvc_type*)ret = msvc_load((unsigned msvc_type volatile *) ptr); cb();

template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret) requires (sizeof(T) == 1)  { ACQ_LOAD_IMPL(__load_acquire8,  __int8 ) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret) requires (sizeof(T) == 2)  { ACQ_LOAD_IMPL(__load_acquire16, __int16) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret) requires (sizeof(T) == 4)  { ACQ_LOAD_IMPL(__load_acquire32, __int32) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret) requires (sizeof(T) == 8)  { ACQ_LOAD_IMPL(__load_acquire64, __int64) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_SEQ_CST(T volatile const *ptr, T *ret) requires (sizeof(T) == 16) { _InterlockedCompareExchange128((__int64 volatile*)ptr, 0, 0, (__int64*)ret); }

template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) requires (sizeof(T) == 1)  { ACQ_LOAD_IMPL(__load_acquire8,  __int8 ) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) requires (sizeof(T) == 2)  { ACQ_LOAD_IMPL(__load_acquire16, __int16) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) requires (sizeof(T) == 4)  { ACQ_LOAD_IMPL(__load_acquire32, __int32) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) requires (sizeof(T) == 8)  { ACQ_LOAD_IMPL(__load_acquire64, __int64) }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_ACQUIRE(T volatile const *ptr, T *ret) requires (sizeof(T) == 16) { _msvc_arm_atomic_load__ATOMIC_SEQ_CST(ptr, ret); }

#undef ACQ_LOAD_IMPL

template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) requires (sizeof(T) == 1)  {  *(char     *)ret = __iso_volatile_load8 ((char      volatile *) ptr); }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) requires (sizeof(T) == 2)  {  *(short    *)ret = __iso_volatile_load16((short     volatile *) ptr); }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) requires (sizeof(T) == 4)  {  *(int      *)ret = __iso_volatile_load32((int       volatile *) ptr); }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) requires (sizeof(T) == 8)  {  *(long long*)ret = __iso_volatile_load64((long long volatile *) ptr); }
template <class T> __forceinline void _msvc_arm_atomic_load__ATOMIC_RELAXED(T volatile const *ptr, T *ret) requires (sizeof(T) == 16) { _msvc_arm_atomic_load__ATOMIC_SEQ_CST(ptr, ret); }

// __atomic_store

template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) requires (sizeof(T) == 1) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, *(const unsigned __int8  *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) requires (sizeof(T) == 2) { cb(); __stlr16((unsigned __int16 volatile *) ptr, *(const unsigned __int16 *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) requires (sizeof(T) == 4) { cb(); __stlr32((unsigned __int32 volatile *) ptr, *(const unsigned __int32 *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_SEQ_CST(T volatile *ptr, const T *val) requires (sizeof(T) == 8) { cb(); __stlr64((unsigned __int64 volatile *) ptr, *(const unsigned __int64 *) val); }

template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) requires (sizeof(T) == 1) { cb(); __stlr8 ((unsigned __int8  volatile *) ptr, *(const unsigned __int8  *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) requires (sizeof(T) == 2) { cb(); __stlr16((unsigned __int16 volatile *) ptr, *(const unsigned __int16 *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) requires (sizeof(T) == 4) { cb(); __stlr32((unsigned __int32 volatile *) ptr, *(const unsigned __int32 *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELEASE(T volatile *ptr, const T *val) requires (sizeof(T) == 8) { cb(); __stlr64((unsigned __int64 volatile *) ptr, *(const unsigned __int64 *) val); }

template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) requires (sizeof(T) == 1) { __iso_volatile_store8 ((char      volatile *) ptr, *(const char      *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) requires (sizeof(T) == 2) { __iso_volatile_store16((short     volatile *) ptr, *(const short     *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) requires (sizeof(T) == 4) { __iso_volatile_store32((int       volatile *) ptr, *(const int       *) val); }
template <class T> __forceinline void _msvc_arm_atomic_store__ATOMIC_RELAXED(T volatile *ptr, const T *val) requires (sizeof(T) == 8) { __iso_volatile_store64((long long volatile *) ptr, *(const long long *) val); }

// __atomic_exchange

#define XCHG_IMPL(msvc_exchange, msvc_type) *(msvc_type *)ret = msvc_exchange((msvc_type volatile *) ptr, *(msvc_type *) val);

template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_SEQ_CST(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 1) { XCHG_IMPL(_InterlockedExchange8     , char     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_SEQ_CST(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 2) { XCHG_IMPL(_InterlockedExchange16    , short    ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_SEQ_CST(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 4) { XCHG_IMPL(_InterlockedExchange      , long     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_SEQ_CST(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 8) { XCHG_IMPL(_InterlockedExchange64    , long long) }

template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELAXED(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 1) { XCHG_IMPL(_InterlockedExchange8_nf  , char     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELAXED(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 2) { XCHG_IMPL(_InterlockedExchange16_nf , short    ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELAXED(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 4) { XCHG_IMPL(_InterlockedExchange_nf   , long     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELAXED(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 8) { XCHG_IMPL(_InterlockedExchange64_nf , long long) }

template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELEASE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 1) { XCHG_IMPL(_InterlockedExchange8_rel , char     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELEASE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 2) { XCHG_IMPL(_InterlockedExchange16_rel, short    ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELEASE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 4) { XCHG_IMPL(_InterlockedExchange_rel  , long     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_RELEASE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 8) { XCHG_IMPL(_InterlockedExchange64_rel, long long) }

template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQUIRE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 1) { XCHG_IMPL(_InterlockedExchange8_acq , char     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQUIRE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 2) { XCHG_IMPL(_InterlockedExchange16_acq, short    ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQUIRE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 4) { XCHG_IMPL(_InterlockedExchange_acq  , long     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQUIRE(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 8) { XCHG_IMPL(_InterlockedExchange64_acq, long long) }

template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQ_REL(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 1) { XCHG_IMPL(_InterlockedExchange8     , char     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQ_REL(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 2) { XCHG_IMPL(_InterlockedExchange16    , short    ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQ_REL(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 4) { XCHG_IMPL(_InterlockedExchange      , long     ) }
template <class T> __forceinline void _msvc_arm_atomic_exchange__ATOMIC_ACQ_REL(T volatile *ptr, const T *val, T *ret) requires (sizeof(T) == 8) { XCHG_IMPL(_InterlockedExchange64    , long long) }

#undef XCHG_IMPL


// __atomic_compare_exchange
// NOTE: MSVC does not provide enough intrinsics to implement CAS optimally for ARM64.
// If we care about performance, we should use inline assembly instead.

#define CAS_IMPL(msvc_cas, msvc_type)                                                                              \
  msvc_type const observed = msvc_cas((msvc_type volatile *)ptr, *(msvc_type *) desired, *(msvc_type *) expected); \
  const bool success = *(msvc_type *) expected == *(msvc_type *) observed;                                         \
  *(msvc_type *) expected = *(msvc_type *) observed;                                                               \
  return success;

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8     , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16    , short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange      , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64    , long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8     , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16    , short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange      , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64    , long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  return _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8_acq , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16_acq, short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange_acq  , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64_acq, long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8_rel , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16_rel, short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange_rel  , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64_rel, long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_rel((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8_nf  , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16_nf , short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange_nf   , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64_nf , long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_RELAXED(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_nf((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8     , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16    , short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange      , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64    , long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQ_REL__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  return _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8_acq , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16_acq, short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange_acq  , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64_acq, long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_ACQUIRE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8     , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16    , short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange      , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64    , long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELEASE__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  return _msvc_arm_atomic_compare_exchange__ATOMIC_SEQ_CST__ATOMIC_SEQ_CST(ptr, expected, desired);
}

template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 1) { CAS_IMPL(_InterlockedCompareExchange8_acq , char     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 2) { CAS_IMPL(_InterlockedCompareExchange16_acq, short    ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 4) { CAS_IMPL(_InterlockedCompareExchange_acq  , long     ) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 8) { CAS_IMPL(_InterlockedCompareExchange64_acq, long long) }
template <class T> __forceinline bool _msvc_arm_atomic_compare_exchange__ATOMIC_RELAXED__ATOMIC_ACQUIRE(T volatile *ptr, T *expected, const T *desired) requires (sizeof(T) == 16)
{
  const __int64 *lo = (const __int64 *)desired;
  const __int64 *hi = lo + 1;
  return !!_InterlockedCompareExchange128_acq((__int64 volatile*)ptr, *hi, *lo, (__int64 *)expected);
}

#undef CAS_IMPL

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// ARITHMETIC OPERATIONS //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// __atomic_fetch_add

__forceinline int8_t   _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_add__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_nf ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_nf ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_nf((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_nf((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_nf  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_nf  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_nf((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_add__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_nf((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_acq ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_acq ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_acq((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_acq((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_acq  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_acq  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_acq((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_acq((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_rel ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_rel ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_rel((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_rel((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_rel  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_rel  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_rel((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_add__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_rel((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_add__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, val); }

// __atomic_fetch_sub

__forceinline int8_t   _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_sub__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_nf ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_nf ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_nf((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_nf((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_nf  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_nf  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_nf((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_nf((long long volatile *) ptr, -(long long) val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_acq ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_acq ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_acq((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_acq((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_acq  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_acq  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_acq((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_acq((long long volatile *) ptr, -(long long) val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8_rel ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8_rel ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16_rel((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16_rel((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd_rel  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd_rel  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64_rel((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_sub__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64_rel((long long volatile *) ptr, -(long long) val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedExchangeAdd8 ((char      volatile *) ptr, -(char     ) val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { return _InterlockedExchangeAdd16((short     volatile *) ptr, -(short    ) val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { return _InterlockedExchangeAdd  ((long      volatile *) ptr, -(long     ) val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_sub__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { return _InterlockedExchangeAdd64((long long volatile *) ptr, -(long long) val); }

// __atomic_add_fetch

#define AF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, (msvc_type) val) + val;

__forceinline int8_t   _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_arm_atomic_add_fetch__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

__forceinline int8_t   _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8_nf , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8_nf , char     ) }
__forceinline int16_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16_nf, short    ) }
__forceinline uint16_t _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16_nf, short    ) }
__forceinline int32_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd_nf  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd_nf  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64_nf, long long) }
__forceinline uint64_t _msvc_arm_atomic_add_fetch__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64_nf, long long) }

__forceinline int8_t   _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8_acq , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8_acq , char     ) }
__forceinline int16_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16_acq, short    ) }
__forceinline uint16_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16_acq, short    ) }
__forceinline int32_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd_acq  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd_acq  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64_acq, long long) }
__forceinline uint64_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64_acq, long long) }

__forceinline int8_t   _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8_rel , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8_rel , char     ) }
__forceinline int16_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16_rel, short    ) }
__forceinline uint16_t _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16_rel, short    ) }
__forceinline int32_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd_rel  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd_rel  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64_rel, long long) }
__forceinline uint64_t _msvc_arm_atomic_add_fetch__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64_rel, long long) }

__forceinline int8_t   _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { AF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { AF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { AF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { AF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { AF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { AF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { AF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_arm_atomic_add_fetch__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { AF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef AF_IMPL

// __atomic_sub_fetch

#define SF_IMPL(type, msvc_add, msvc_type) return (type) msvc_add((msvc_type volatile *) ptr, -(msvc_type) val) - val;

__forceinline int8_t   _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_arm_atomic_sub_fetch__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

__forceinline int8_t   _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8_nf , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8_nf , char     ) }
__forceinline int16_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16_nf, short    ) }
__forceinline uint16_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16_nf, short    ) }
__forceinline int32_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd_nf  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd_nf  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64_nf, long long) }
__forceinline uint64_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64_nf, long long) }

__forceinline int8_t   _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8_acq , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8_acq , char     ) }
__forceinline int16_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16_acq, short    ) }
__forceinline uint16_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16_acq, short    ) }
__forceinline int32_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd_acq  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd_acq  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64_acq, long long) }
__forceinline uint64_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64_acq, long long) }

__forceinline int8_t   _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8_rel , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8_rel , char     ) }
__forceinline int16_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16_rel, short    ) }
__forceinline uint16_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16_rel, short    ) }
__forceinline int32_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd_rel  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd_rel  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64_rel, long long) }
__forceinline uint64_t _msvc_arm_atomic_sub_fetch__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64_rel, long long) }

__forceinline int8_t   _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { SF_IMPL(int8_t  , _InterlockedExchangeAdd8 , char     ) }
__forceinline uint8_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { SF_IMPL(uint8_t , _InterlockedExchangeAdd8 , char     ) }
__forceinline int16_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { SF_IMPL(int16_t , _InterlockedExchangeAdd16, short    ) }
__forceinline uint16_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { SF_IMPL(uint16_t, _InterlockedExchangeAdd16, short    ) }
__forceinline int32_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { SF_IMPL(int32_t , _InterlockedExchangeAdd  , long     ) }
__forceinline uint32_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { SF_IMPL(uint32_t, _InterlockedExchangeAdd  , long     ) }
__forceinline int64_t  _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { SF_IMPL(int64_t , _InterlockedExchangeAdd64, long long) }
__forceinline uint64_t _msvc_arm_atomic_sub_fetch__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { SF_IMPL(uint64_t, _InterlockedExchangeAdd64, long long) }

#undef SF_IMPL


// __atomic_fetch_and

__forceinline int8_t   _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_and__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8_nf ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8_nf ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16_nf((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16_nf((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd_nf  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd_nf  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64_nf((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_and__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64_nf((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8_acq ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8_acq ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16_acq((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16_acq((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd_acq  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd_acq  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64_acq((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64_acq((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8_rel ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8_rel ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16_rel((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16_rel((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd_rel  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd_rel  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64_rel((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_and__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64_rel((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedAnd8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { return _InterlockedAnd16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { return _InterlockedAnd  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { return _InterlockedAnd64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_and__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { return _InterlockedAnd64((long long volatile *) ptr, val); }

// __atomic_fetch_or

__forceinline int8_t   _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_or__ATOMIC_SEQ_CST(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8_nf ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8_nf ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16_nf((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16_nf((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr_nf  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr_nf  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64_nf((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_or__ATOMIC_RELAXED(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64_nf((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8_acq ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8_acq ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16_acq((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16_acq((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr_acq  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr_acq  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64_acq((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQUIRE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64_acq((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8_rel ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8_rel ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16_rel((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16_rel((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr_rel  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr_rel  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64_rel((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_or__ATOMIC_RELEASE(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64_rel((long long volatile *) ptr, val); }

__forceinline int8_t   _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(int8_t   volatile *ptr, int8_t   val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline uint8_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(uint8_t  volatile *ptr, uint8_t  val) { return _InterlockedOr8 ((char      volatile *) ptr, val); }
__forceinline int16_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(int16_t  volatile *ptr, int16_t  val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline uint16_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(uint16_t volatile *ptr, uint16_t val) { return _InterlockedOr16((short     volatile *) ptr, val); }
__forceinline int32_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(int32_t  volatile *ptr, int32_t  val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline uint32_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(uint32_t volatile *ptr, uint32_t val) { return _InterlockedOr  ((long      volatile *) ptr, val); }
__forceinline int64_t  _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(int64_t  volatile *ptr, int64_t  val) { return _InterlockedOr64((long long volatile *) ptr, val); }
__forceinline uint64_t _msvc_arm_atomic_fetch_or__ATOMIC_ACQ_REL(uint64_t volatile *ptr, uint64_t val) { return _InterlockedOr64((long long volatile *) ptr, val); }

// __atomic_test_and_set

__forceinline bool _msvc_arm_atomic_test_and_set__ATOMIC_SEQ_CST(bool volatile *ptr) { return _InterlockedExchange8((char volatile *) ptr, 1); }
__forceinline bool _msvc_arm_atomic_test_and_set__ATOMIC_RELAXED(bool volatile *ptr) { return _InterlockedExchange8_nf((char volatile *) ptr, 1); }
__forceinline bool _msvc_arm_atomic_test_and_set__ATOMIC_ACQUIRE(bool volatile *ptr) { return _InterlockedExchange8_acq((char volatile *) ptr, 1); }
__forceinline bool _msvc_arm_atomic_test_and_set__ATOMIC_RELEASE(bool volatile *ptr) { return _InterlockedExchange8_rel((char volatile *) ptr, 1); }
__forceinline bool _msvc_arm_atomic_test_and_set__ATOMIC_ACQ_REL(bool volatile *ptr) { return _InterlockedExchange8((char volatile *) ptr, 1); }

// __atomic_clear

__forceinline void _msvc_arm_atomic_clear__ATOMIC_RELAXED(bool volatile *ptr) { __iso_volatile_store8((char volatile *) ptr, 0); }
__forceinline void _msvc_arm_atomic_clear__ATOMIC_RELEASE(bool volatile *ptr) { cb(); __stlr8((unsigned __int8  volatile *) ptr, 0); }
__forceinline void _msvc_arm_atomic_clear__ATOMIC_SEQ_CST(bool volatile *ptr) { cb(); __stlr8((unsigned __int8  volatile *) ptr, 0); }

// clang-format on

#define __atomic_load_n(ptr, order)          _msvc_arm_atomic_load_n##order((ptr))
#define __atomic_store_n(ptr, val, order)    _msvc_arm_atomic_store_n##order((ptr), (val))
#define __atomic_exchange_n(ptr, val, order) _msvc_arm_atomic_exchange_n##order((ptr), (val))
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, success, failure) \
  _msvc_arm_atomic_compare_exchange_n##success##failure((ptr), (expected), (desired))

#define __atomic_load(ptr, ret, order)          _msvc_arm_atomic_load##order((ptr), (ret))
#define __atomic_store(ptr, val, order)         _msvc_arm_atomic_store##order((ptr), (val))
#define __atomic_exchange(ptr, val, ret, order) _msvc_arm_atomic_exchange##order((ptr), (val), (ret))
#define __atomic_compare_exchange(ptr, expected, desired, weak, success, failure) \
  _msvc_arm_atomic_compare_exchange##success##failure((ptr), (expected), (desired))

#define __atomic_add_fetch(ptr, val, order) _msvc_arm_atomic_add_fetch##order((ptr), (val))
#define __atomic_sub_fetch(ptr, val, order) _msvc_arm_atomic_sub_fetch##order((ptr), (val))
#define __atomic_and_fetch(ptr, val, order) _msvc_arm_atomic_and_fetch##order((ptr), (val))
#define __atomic_or_fetch(ptr, val, order)  _msvc_arm_atomic_or_fetch##order((ptr), (val))
#define __atomic_fetch_add(ptr, val, order) _msvc_arm_atomic_fetch_add##order((ptr), (val))
#define __atomic_fetch_sub(ptr, val, order) _msvc_arm_atomic_fetch_sub##order((ptr), (val))
#define __atomic_fetch_and(ptr, val, order) _msvc_arm_atomic_fetch_and##order((ptr), (val))
#define __atomic_fetch_or(ptr, val, order)  _msvc_arm_atomic_fetch_or##order((ptr), (val))
#define __atomic_test_and_set(ptr, order)   _msvc_arm_atomic_test_and_set##order((ptr))
#define __atomic_clear(ptr, order)          _msvc_arm_atomic_clear##order((ptr))

#undef cb
#undef mb
