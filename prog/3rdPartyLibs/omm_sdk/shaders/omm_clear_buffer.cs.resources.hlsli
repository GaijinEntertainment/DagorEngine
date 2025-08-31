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
    OMM_OUTPUT_RESOURCE( RWByteAddressBuffer, u_targetBuffer, u, 0 )

#define OMM_DECLARE_LOCAL_CONSTANT_BUFFER						\
OMM_PUSH_CONSTANTS_START(LocalConstants)						\
																\
	OMM_PUSH_CONSTANT(uint, ClearValue)							\
	OMM_PUSH_CONSTANT(uint, NumElements)						\
	OMM_PUSH_CONSTANT(uint, BufferOffset)						\
	OMM_PUSH_CONSTANT(uint, gPad0)								\
																\
OMM_PUSH_CONSTANTS_END(LocalConstants, 1)
