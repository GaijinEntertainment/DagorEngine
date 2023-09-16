//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_Point2.h"
#include "dag_Point3.h"
#include "dag_Point4.h"
#include "dag_Quat.h"
#include "dag_TMatrix.h"
#include "dag_TMatrix4.h"
#include "dag_TMatrix4.h"
#include <DirectXMath/Inc/DirectXMath.h>

/*
 * These extensions allow us to use the old dagor math types with DirectX math. There are some things to keep
 * in mind here about matrices.
 *
 * The DirectX math matrices are using row-major matrices. This is in line with TMatrix4, but not with TMatrix
 * which is column major. Therefore just as with conversion from TMatrix to TMatrix4, loading and storing between
 * XMMATRIX and TMatrix, transpose is happening.
 *
 * When you are refactoring old math code to DirectX math, keep this in mind. For instance, calling rotxTM on a
 * TMatrix and calling XMMatrixRotationX needs to have a different sign.
 *
 * Also in DirectX math, you use pre-multiplication.
 */

namespace DirectX
{
inline XMVECTOR XM_CALLCONV XMLoadFloat2(const ::Point2 *pSource) noexcept
{
  return XMLoadFloat2(reinterpret_cast<const XMFLOAT2 *>(pSource));
}
inline XMVECTOR XM_CALLCONV XMLoadFloat3(const ::Point3 *pSource) noexcept
{
  return XMLoadFloat3(reinterpret_cast<const XMFLOAT3 *>(pSource));
}
inline XMVECTOR XM_CALLCONV XMLoadFloat4(const ::Point4 *pSource) noexcept
{
  return XMLoadFloat4(reinterpret_cast<const XMFLOAT4 *>(pSource));
}
inline XMVECTOR XM_CALLCONV XMLoadFloat4(const ::Quat *pSource) noexcept
{
  return XMLoadFloat4(reinterpret_cast<const XMFLOAT4 *>(pSource));
}
inline XMMATRIX XM_CALLCONV XMLoadFloat4x3(_In_ const ::TMatrix *pSource) noexcept
{
  return XMMatrixSet(pSource->m[0][0], pSource->m[0][1], pSource->m[0][2], 0, pSource->m[1][0], pSource->m[1][1], pSource->m[1][2], 0,
    pSource->m[2][0], pSource->m[2][1], pSource->m[2][2], 0, pSource->m[3][0], pSource->m[3][1], pSource->m[3][2], 1);
}
inline XMMATRIX XM_CALLCONV XMLoadFloat4x4(_In_ const ::TMatrix4 *pSource) noexcept
{
  return XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4 *>(pSource));
}

inline void XM_CALLCONV XMStoreFloat2(_Out_ ::Point2 *pDestination, _In_ FXMVECTOR V) noexcept
{
  XMStoreFloat2(reinterpret_cast<XMFLOAT2 *>(pDestination), V);
}
inline void XM_CALLCONV XMStoreFloat3(_Out_ ::Point3 *pDestination, _In_ FXMVECTOR V) noexcept
{
  XMStoreFloat3(reinterpret_cast<XMFLOAT3 *>(pDestination), V);
}
inline void XM_CALLCONV XMStoreFloat4(_Out_ ::Point4 *pDestination, _In_ FXMVECTOR V) noexcept
{
  XMStoreFloat4(reinterpret_cast<XMFLOAT4 *>(pDestination), V);
}
inline void XM_CALLCONV XMStoreFloat4(_Out_ ::Quat *pDestination, _In_ FXMVECTOR V) noexcept
{
  XMStoreFloat4(reinterpret_cast<XMFLOAT4 *>(pDestination), V);
}
inline void XM_CALLCONV XMStoreFloat4x3(_Out_ ::TMatrix *pDestination, _In_ FXMMATRIX M) noexcept
{
  pDestination->array[0] = XMVectorGetX(M.r[0]);
  pDestination->array[1] = XMVectorGetY(M.r[0]);
  pDestination->array[2] = XMVectorGetZ(M.r[0]);
  pDestination->array[3] = XMVectorGetX(M.r[1]);
  pDestination->array[4] = XMVectorGetY(M.r[1]);
  pDestination->array[5] = XMVectorGetZ(M.r[1]);
  pDestination->array[6] = XMVectorGetX(M.r[2]);
  pDestination->array[7] = XMVectorGetY(M.r[2]);
  pDestination->array[8] = XMVectorGetZ(M.r[2]);
  XMStoreFloat3(&pDestination->col[3], M.r[3]);
}
inline void XM_CALLCONV XMStoreFloat4x4(_Out_ ::TMatrix4 *pDestination, _In_ FXMMATRIX M) noexcept
{
  XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4 *>(pDestination), M);
}
} // namespace DirectX

using namespace DirectX;
