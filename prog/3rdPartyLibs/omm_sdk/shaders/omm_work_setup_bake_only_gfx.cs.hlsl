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
#include "omm_work_setup_bake_only_gfx.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_common.hlsli"

bool TryScheduledForBake(uint ommDescIndex)
{
	const uint kNotScheduled = 0;
	const uint kScheduled = 1;
	uint existing = 0;
	OMM_SUBRESOURCE_CAS(TempOmmBakeScheduleTrackerBuffer, 4 * ommDescIndex, kNotScheduled, kScheduled, existing);
	return existing == kNotScheduled;
}

uint TryScheduledForBake(uint ommDescIndex, uint primitiveIndex)
{
	const uint kNotScheduled = 0;
	uint existing = 0;
	OMM_SUBRESOURCE_CAS(TempOmmBakeScheduleTrackerBuffer, 4 * ommDescIndex, kNotScheduled, primitiveIndex + 1, existing);

	uint alreadyScheduledPrimitiveIndex = existing - 1;

	return existing == kNotScheduled ? primitiveIndex : alreadyScheduledPrimitiveIndex;
}

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= g_GlobalConstants.PrimitiveCount)
		return;

	const uint primitiveIndex = tid.x;
	int ommDescOffset = GetOmmDescOffset(t_ommIndexBuffer, tid.x);
	const uint kOMMFormatNum = 2;

	const bool IsSpecialIndex = ommDescOffset < 0;

	if (IsSpecialIndex)
	{
		// No baking to do.
		OMM_SUBRESOURCE_STORE(TempOmmIndexBuffer, 4 * primitiveIndex, ommDescOffset);
		return;
	}

	const uint scheduledPrimitiveIndex = TryScheduledForBake(ommDescOffset, primitiveIndex);

	if (scheduledPrimitiveIndex == primitiveIndex)
	{
		// Fetch the desc info.
		const uint ommArrayOffset = t_ommDescArrayBuffer.Load(ommDescOffset * 8);
		const uint ommDescData = t_ommDescArrayBuffer.Load(ommDescOffset * 8 + 4);

		const uint ommFormat = (ommDescData >> 16u) & 0x0000FFFF;
		const uint subdivisionLevel = ommDescData & 0x0000FFFF;
		const uint numMicroTriangles = GetNumMicroTriangles(subdivisionLevel);

		{
			const uint vmDataBitSize = GetOMMFormatBitCount((OMMFormat)ommFormat) * numMicroTriangles;

			// spec allows 1 byte alignment but we require 4 byte to make sure UAV writes
			// are DW aligned.
			const uint vmDataByteSize = max(vmDataBitSize >> 3u, 4u);

			uint _dummy;
			OMM_SUBRESOURCE_INTERLOCKEDADD(OmmArrayAllocatorCounterBuffer, 0, vmDataByteSize, _dummy);
		}

		{
			uint _dummy;
			OMM_SUBRESOURCE_INTERLOCKEDADD(OmmDescAllocatorCounterBuffer, 0, 1, _dummy);
		}

		{
			/// ---- Setup baking parameters ----- 

			// Allocate a slot in the raster items array.
			uint bakeResultGlobalOffset = 0;
			uint bakeResultBatchIndex = 0;
			{
				const uint offset = 4 * subdivisionLevel;

				OMM_SUBRESOURCE_INTERLOCKEDADD(BakeResultBufferCounterBuffer, offset, 1, bakeResultGlobalOffset);

				const uint maxItemsPerBatch = GetMaxItemsPerBatch(subdivisionLevel);
				bakeResultBatchIndex = bakeResultGlobalOffset / maxItemsPerBatch;
			}

			// Store the VM-that will be procesed by the rasterizer.
			{
				const uint ommFormatAndPrimitiveIndex = (primitiveIndex) | ((uint)ommFormat << 30);

				const uint offset = 8 * (bakeResultGlobalOffset + subdivisionLevel * g_GlobalConstants.PrimitiveCount);

				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset, ommArrayOffset);
				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset + 4, ommFormatAndPrimitiveIndex);
			}

			// Increment the drawcall count for the current batch & subdivisiolevel.
			{
				const uint strideInBytes = g_GlobalConstants.IndirectDispatchEntryStride;	// arg count of DrawIndexedInstanced 
				const uint InstanceCountOffsetInBytes = 4;	// offset of InstanceCount in DrawIndexedInstanced
				const uint offset = InstanceCountOffsetInBytes + strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				OMM_SUBRESOURCE_INTERLOCKEDADD(IEBakeBuffer, offset, 1, bakeResultGlobalOffset);
			}

			// Increment the thread count for the current batch & subdivisiolevel.
			uint threadGroupCountX = 0;
			{
				// This is the most number of micro-triangles that will be processed per thread
				// 32 allows non-atomic writes to the vmArrayBuffer for 2 and 4-state vm formats.
				const uint kMaxNumMicroTrianglePerThread = 32;
				const uint numMicroTrianglePerThread = min(kMaxNumMicroTrianglePerThread, numMicroTriangles);
				const uint numThreadsNeeded = max(numMicroTriangles / numMicroTrianglePerThread, 1u);

				const uint strideInBytes = 4; // sizeof(uint32_t)
				const uint offset = strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				uint oldGlobalThreadCountX;
				OMM_SUBRESOURCE_INTERLOCKEDADD(DispatchIndirectThreadCountBuffer, offset, numThreadsNeeded, oldGlobalThreadCountX);
				uint newGlobalThreadCountX = numThreadsNeeded + oldGlobalThreadCountX;

				threadGroupCountX = (newGlobalThreadCountX + 127) / 128;
			}

			// Increment the thread GROUP count for the current batch & subdivisiolevel.
			{
				const uint strideInBytes = g_GlobalConstants.IndirectDispatchEntryStride; // arg count of Dispatch
				const uint ThreadCountXOffsetInBytes = 0;	 // offset of ThreadCountX in Dispatch
				const uint offset = ThreadCountXOffsetInBytes + strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				uint _dummy;
				OMM_SUBRESOURCE_INTERLOCKEDMAX(IECompressCsBuffer, offset, threadGroupCountX, _dummy);
			}
		}
	}
	else
	{
		ommDescOffset = (uint)(-scheduledPrimitiveIndex - 5);
	}

	OMM_SUBRESOURCE_STORE(TempOmmIndexBuffer, 4 * primitiveIndex, ommDescOffset);
}