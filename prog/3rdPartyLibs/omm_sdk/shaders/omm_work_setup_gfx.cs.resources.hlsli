/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define OMM_DECLARE_INPUT_RESOURCES \
    OMM_INPUT_RESOURCE( Buffer<uint>, t_indexBuffer, t, 0 ) \
    OMM_INPUT_RESOURCE( ByteAddressBuffer, t_texCoordBuffer, t, 1 )

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_ommDescArrayBuffer, u, 0 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_ommDescArrayHistogramBuffer, u, 1 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_heap0, u, 2 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_heap1, u, 3 ) \

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(RWByteAddressBuffer, TempOmmIndexBuffer, u_heap0)                   \
    OMM_SUBRESOURCE(RWByteAddressBuffer, HashTableBuffer, u_heap0)                   \
    OMM_SUBRESOURCE(RWByteAddressBuffer, OmmArrayAllocatorCounterBuffer, u_heap0)     \
    OMM_SUBRESOURCE(RWByteAddressBuffer, OmmDescAllocatorCounterBuffer, u_heap0)      \
    OMM_SUBRESOURCE(RWByteAddressBuffer, DispatchIndirectThreadCountBuffer, u_heap0) \
    OMM_SUBRESOURCE(RWByteAddressBuffer, BakeResultBufferCounterBuffer, u_heap0)     \
    OMM_SUBRESOURCE(RWByteAddressBuffer, RasterItemsBuffer, u_heap0)   \
    OMM_SUBRESOURCE(RWByteAddressBuffer, IEBakeBuffer, u_heap1) \
    OMM_SUBRESOURCE(RWByteAddressBuffer, IECompressCsBuffer, u_heap1)
