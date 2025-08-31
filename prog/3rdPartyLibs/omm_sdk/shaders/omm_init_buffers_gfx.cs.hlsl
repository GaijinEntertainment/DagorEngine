/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "omm_platform.hlsli"
#include "omm_global_cb.hlsli"
#include "omm_global_samplers.hlsli"
#include "omm_init_buffers_gfx.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_INPUT_RESOURCES

OMM_DECLARE_SUBRESOURCES

uint GetNumMicroTri(uint subdivisionLevel) {
	return (1u << (subdivisionLevel << 1u));
}

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x > (g_GlobalConstants.MaxSubdivisionLevel) * g_GlobalConstants.MaxBatchCount)
		return;

	const uint subdivisionLevel = tid.x / g_GlobalConstants.MaxBatchCount;
	const uint batchIndex		= tid.x % g_GlobalConstants.MaxBatchCount;

	{
		const uint strideInBytes = g_GlobalConstants.IndirectDispatchEntryStride;

		OMM_SUBRESOURCE_STORE(IEBakeBuffer, strideInBytes * tid.x + 0, 3 * GetNumMicroTri(subdivisionLevel)); /*IndexCountPerInstance*/
		OMM_SUBRESOURCE_STORE(IEBakeBuffer, strideInBytes * tid.x + 4, 0);/*InstanceCount*/
		OMM_SUBRESOURCE_STORE(IEBakeBuffer, strideInBytes * tid.x + 8, 0);/*StartIndexLocation*/
		OMM_SUBRESOURCE_STORE(IEBakeBuffer, strideInBytes * tid.x + 12, 0);/*BaseVertexLocation*/
		OMM_SUBRESOURCE_STORE(IEBakeBuffer, strideInBytes * tid.x + 16, 0);/*StartInstanceLocation*/
	}

	{
		const uint strideInBytes = g_GlobalConstants.IndirectDispatchEntryStride;

		OMM_SUBRESOURCE_STORE(IECompressCsBuffer, strideInBytes * tid.x + 0, 0);
		OMM_SUBRESOURCE_STORE(IECompressCsBuffer, strideInBytes * tid.x + 4, 1);
		OMM_SUBRESOURCE_STORE(IECompressCsBuffer, strideInBytes * tid.x + 8, 1);
	}

	if (batchIndex == 0)
	{
		const uint kOMMFormatNum = 2;

		// [OC1, OC2]x12
		[unroll]
		for (uint vmFormatIt			= 0; vmFormatIt < 2; ++vmFormatIt) {
			const uint index			= vmFormatIt + kOMMFormatNum * subdivisionLevel;
			const uint formatAndLevel	= ((vmFormatIt + 1) << 16u) | subdivisionLevel;

			// sizeof(VisibilityMapUsageDesc), [count32, format16, level16]
			if (g_GlobalConstants.DoSetup)
			{
				u_ommDescArrayHistogramBuffer.Store(8 * index, 0);
				u_ommDescArrayHistogramBuffer.Store(8 * index + 4, formatAndLevel);
			}

			u_ommIndexHistogramBuffer.Store(8 * index, 0);
			u_ommIndexHistogramBuffer.Store(8 * index + 4, formatAndLevel);
		}
	}
}