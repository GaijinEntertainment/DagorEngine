/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define OMM_DECLARE_INPUT_RESOURCES \
    OMM_INPUT_RESOURCE( ByteAddressBuffer, u_heap0, t, 0 )

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_postBuildInfo, u, 0 )

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(ByteAddressBuffer, OmmArrayAllocatorCounterBuffer, u_heap0)     \
    OMM_SUBRESOURCE(ByteAddressBuffer, OmmDescAllocatorCounterBuffer, u_heap0)
