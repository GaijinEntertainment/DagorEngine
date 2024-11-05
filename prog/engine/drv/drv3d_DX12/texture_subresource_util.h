// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"


namespace drv3d_dx12
{

inline constexpr UINT calculate_subresource_index(UINT mip_slice, UINT array_slice, UINT plane_slice, UINT mip_size, UINT array_size)
{
  return mip_slice + (array_slice * mip_size) + (plane_slice * mip_size * array_size);
}

inline constexpr UINT calculate_mip_slice_from_index(UINT index, UINT mip_size) { return index % mip_size; }

inline constexpr UINT calculate_array_slice_from_index(UINT index, UINT mip_size, UINT array_size)
{
  return (index / mip_size) % array_size;
}

inline constexpr UINT calculate_plane_slice_from_index(UINT index, UINT mip_size, UINT array_size)
{
  return (index / mip_size) / array_size;
}

} // namespace drv3d_dx12
