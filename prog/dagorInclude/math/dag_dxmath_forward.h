//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !defined _XM_NO_INTRINSICS_ && !defined _XM_SSE_INTRINSICS_ && !defined _XM_ARM_NEON_INTRINSICS_
#if (defined(_M_IX86) || defined(_M_X64) || __i386__ || __x86_64__) && !defined(_M_HYBRID_X86_ARM64) && !defined(_M_ARM64EC)
#define _XM_SSE_INTRINSICS_
#elif defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC) || __arm__ || __aarch64__
#define _XM_ARM_NEON_INTRINSICS_
#elif !defined(_XM_NO_INTRINSICS_)
#error DirectX Math does not support this target
#endif
#endif

#if defined(_MSC_VER) && !defined(_M_ARM) && !defined(_M_ARM64) && !defined(_M_HYBRID_X86_ARM64) && !defined(_M_ARM64EC) && \
  (!_MANAGED) && (!_M_CEE) && (!defined(_M_IX86_FP) || (_M_IX86_FP > 1)) && !defined(_XM_NO_INTRINSICS_) && !defined(_XM_VECTORCALL_)
#define _XM_VECTORCALL_ 1
#endif

namespace DirectX
{
struct XMFLOAT2;
struct XMFLOAT3;
struct XMFLOAT4;
struct XMFLOAT3X3;
struct XMFLOAT4X3;
struct XMFLOAT3X4;
struct XMFLOAT4X4;

struct XMFLOAT2A;
struct XMFLOAT3A;
struct XMFLOAT4A;
struct XMFLOAT4X3A;
struct XMFLOAT3X4A;
struct XMFLOAT4X4A;

#if defined(_XM_NO_INTRINSICS_)
struct __vector4;
#endif // _XM_NO_INTRINSICS_

#if defined(_XM_SSE_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
using XMVECTOR = __m128;
#elif defined(_XM_ARM_NEON_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_)
using XMVECTOR = float32x4_t;
#else
using XMVECTOR = __vector4;
#endif

#if (defined(_M_IX86) || defined(_M_ARM) || defined(_M_ARM64) || _XM_VECTORCALL_ || __i386__ || __arm__ || __aarch64__) && \
  !defined(_XM_NO_INTRINSICS_)
typedef const XMVECTOR FXMVECTOR;
#else
typedef const XMVECTOR &FXMVECTOR;
#endif

struct XMMATRIX;

#if (defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC) || _XM_VECTORCALL_ || __aarch64__) && \
  !defined(_XM_NO_INTRINSICS_)
typedef const XMMATRIX FXMMATRIX;
#else
typedef const XMMATRIX &FXMMATRIX;
#endif
} // namespace DirectX

using namespace DirectX;
