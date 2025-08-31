/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define OMM_DECLARE_INPUT_RESOURCES

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_buffer0, u, 0 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_ommDescArrayHistogramBuffer, u, 1 ) \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_ommIndexHistogramBuffer, u, 2 )

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(RWByteAddressBuffer, IEBakeBuffer, u_buffer0) \
    OMM_SUBRESOURCE(RWByteAddressBuffer, IECompressCsBuffer, u_buffer0)
