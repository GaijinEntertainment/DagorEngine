//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <drv/3d/dag_info.h>

/**
 * @brief Extended draw call arguments structure.
 *
 * This structure is used for platforms that pass draw call id/per draw parameters using per draw root constants.
 * Currently it is used only for DX12.
 */
struct ExtendedDrawIndexedIndirectArgs
{
  uint32_t drawcallId;
  DrawIndexedIndirectArgs args;
};

struct MultiDrawInfo
{
/**
 * @brief Checks if extended draw call arguments structure is used.
 */
#if _TARGET_PC_WIN
#define CONSTEXPR_EXT_MULTIDRAW
  static inline uint8_t bytesCountPerDrawcall = 0;
  static bool usesExtendedMultiDrawStruct()
  {
    if (DAGOR_UNLIKELY(!bytesCountPerDrawcall))
      bytesCountPerDrawcall =
        d3d::get_driver_code().is(d3d::dx12) ? sizeof(ExtendedDrawIndexedIndirectArgs) : sizeof(DrawIndexedIndirectArgs);
    static_assert(sizeof(ExtendedDrawIndexedIndirectArgs) != sizeof(DrawIndexedIndirectArgs));
    return bytesCountPerDrawcall == sizeof(ExtendedDrawIndexedIndirectArgs);
  }
#elif _TARGET_XBOX
#define CONSTEXPR_EXT_MULTIDRAW constexpr
  static constexpr uint8_t bytesCountPerDrawcall = sizeof(ExtendedDrawIndexedIndirectArgs);
  static constexpr bool usesExtendedMultiDrawStruct() { return true; } // DX12
#else
#define CONSTEXPR_EXT_MULTIDRAW constexpr
  static constexpr uint8_t bytesCountPerDrawcall = sizeof(DrawIndexedIndirectArgs);
  static constexpr bool usesExtendedMultiDrawStruct() { return false; }
#endif
};
