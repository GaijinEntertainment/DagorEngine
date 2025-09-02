/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define OMM_DECLARE_INPUT_RESOURCES \
	OMM_INPUT_RESOURCE(Texture2D<float4>,	t_alphaTexture,		t, 0) \
	OMM_INPUT_RESOURCE(Buffer<uint>,		t_indexBuffer,		t, 1) \
	OMM_INPUT_RESOURCE(ByteAddressBuffer,	t_texCoordBuffer,	t, 2) \
	OMM_INPUT_RESOURCE(ByteAddressBuffer,	t_heap0,			t, 3)

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_heap1, u, 0 )

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(ByteAddressBuffer, RasterItemsBuffer, t_heap0) \
    OMM_SUBRESOURCE(RWByteAddressBuffer, BakeResultBuffer, u_heap1)

#define OMM_DECLARE_LOCAL_CONSTANT_BUFFER						\
OMM_PUSH_CONSTANTS_START(LocalConstants)						\
																\
	OMM_PUSH_CONSTANT(uint, SubdivisionLevel)					\
	OMM_PUSH_CONSTANT(uint, VmResultBufferStride)				\
	OMM_PUSH_CONSTANT(uint, BatchIndex)							\
	OMM_PUSH_CONSTANT(uint, PrimitiveIdOffset)					\
																\
	OMM_PUSH_CONSTANT(uint, gPad0)								\
	OMM_PUSH_CONSTANT(uint, gPad1)								\
	OMM_PUSH_CONSTANT(uint, gPad2)								\
	OMM_PUSH_CONSTANT(uint, gPad4)								\
																\
OMM_PUSH_CONSTANTS_END(LocalConstants, 1)
