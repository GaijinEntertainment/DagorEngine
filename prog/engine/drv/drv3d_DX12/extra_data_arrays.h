// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "extra_data_array.h"
#include "image_view_state.h"
#include "info_types.h"
#include "viewport_state.h"

#include <drv/3d/rayTrace/dag_drvRayTrace.h>

namespace drv3d_dx12
{
class Image;
class Query;

using StringIndexRef = ExtraDataArray<const char>;
using WStringIndexRef = ExtraDataArray<const wchar_t>;

#if D3D_HAS_RAY_TRACING
using RaytraceGeometryDescriptionBufferResourceReferenceSetListRef =
  ExtraDataArray<const RaytraceGeometryDescriptionBufferResourceReferenceSet>;
using D3D12_RAYTRACING_GEOMETRY_DESC_ListRef = ExtraDataArray<const D3D12_RAYTRACING_GEOMETRY_DESC>;
#endif
using ImageViewStateListRef = ExtraDataArray<const ImageViewState>;
using ImagePointerListRef = ExtraDataArray<Image *const>;
using QueryPointerListRef = ExtraDataArray<Query *const>;
using D3D12_CPU_DESCRIPTOR_HANDLE_ListRef = ExtraDataArray<const D3D12_CPU_DESCRIPTOR_HANDLE>;
using BufferImageCopyListRef = ExtraDataArray<const BufferImageCopy>;
using BufferResourceReferenceAndShaderResourceViewListRef = ExtraDataArray<const BufferResourceReferenceAndShaderResourceView>;
using ViewportListRef = ExtraDataArray<const ViewportState>;
using ScissorRectListRef = ExtraDataArray<const D3D12_RECT>;
using UInt32ListRef = ExtraDataArray<const uint32_t>;
} // namespace drv3d_dx12
