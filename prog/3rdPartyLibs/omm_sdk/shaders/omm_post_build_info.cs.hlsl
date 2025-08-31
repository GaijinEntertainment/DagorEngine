/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "omm_platform.hlsli"
#include "omm.hlsli"

#include "omm_global_cb.hlsli"
#include "omm_global_samplers.hlsli"
#include "omm_post_build_info.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x > 0)
		return;

	const uint ommArrayByteSize		= OMM_SUBRESOURCE_LOAD(OmmArrayAllocatorCounterBuffer, 0);
	const uint ommDescCount			= OMM_SUBRESOURCE_LOAD(OmmDescAllocatorCounterBuffer, 0);
	const uint ommDescByteSize		= ommDescCount * 8;

	u_postBuildInfo.Store(0, min(ommArrayByteSize, g_GlobalConstants.MaxOutOmmArraySize));
	u_postBuildInfo.Store(4, ommDescByteSize);
	u_postBuildInfo.Store(8, 0);
	u_postBuildInfo.Store(12, 0);
	u_postBuildInfo.Store(16, 0);
	u_postBuildInfo.Store(20, 0);
	u_postBuildInfo.Store(24, 0);
	u_postBuildInfo.Store(28, 0);
}