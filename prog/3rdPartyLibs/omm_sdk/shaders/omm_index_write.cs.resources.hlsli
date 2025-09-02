/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define OMM_DECLARE_INPUT_RESOURCES \
    OMM_INPUT_RESOURCE( ByteAddressBuffer, t_heap0, t, 0 )

#define OMM_DECLARE_OUTPUT_RESOURCES \
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_ommIndexBuffer, u, 0 )

#define OMM_DECLARE_SUBRESOURCES \
    OMM_SUBRESOURCE(ByteAddressBuffer, TempOmmIndexBuffer, t_heap0)

#define OMM_DECLARE_LOCAL_CONSTANT_BUFFER						\
OMM_PUSH_CONSTANTS_START(LocalConstants)								\
																\
	OMM_PUSH_CONSTANT(uint, ThreadCount)							\
	OMM_PUSH_CONSTANT(uint, pad0)								\
	OMM_PUSH_CONSTANT(uint, pad1)						\
	OMM_PUSH_CONSTANT(uint, pad2)						\
																\
OMM_PUSH_CONSTANTS_END(LocalConstants, 1)

