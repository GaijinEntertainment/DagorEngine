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
#include "omm_work_setup_bake_only_cs.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_common.hlsli"

uint TryScheduledForBake(uint ommDescIndex, uint primitiveIndex)
{
	const uint kNotScheduled = 0;
	uint existing			 = 0;
	OMM_SUBRESOURCE_CAS(TempOmmBakeScheduleTrackerBuffer, 4 * ommDescIndex, kNotScheduled, primitiveIndex + 1, existing);

	uint alreadyScheduledPrimitiveIndex = existing - 1;

	return existing == kNotScheduled ? primitiveIndex : alreadyScheduledPrimitiveIndex;
}

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= g_GlobalConstants.PrimitiveCount)
		return;

	const uint primitiveIndex		= tid.x;
	int ommDescOffset			= GetOmmDescOffset(t_ommIndexBuffer, tid.x);
	const uint kOMMFormatNum		= 2;

	const bool IsSpecialIndex				= ommDescOffset < 0;

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
		const uint ommArrayOffset	= t_ommDescArrayBuffer.Load(ommDescOffset * 8);
		const uint ommDescData		= t_ommDescArrayBuffer.Load(ommDescOffset * 8 + 4);

		const uint ommFormat			= (ommDescData >> 16u) & 0x0000FFFF;
		const uint subdivisionLevel		= ommDescData & 0x0000FFFF;
		const uint numMicroTriangles	= GetNumMicroTriangles(subdivisionLevel);

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

		// Schedule the baking task.
		{
			/// ---- Store the OMM-data common for all microtriangles ----- 

			// Allocate a slot in the raster items array.
			uint bakeResultGlobalOffset = 0;
			{
				const uint offset = 4 * subdivisionLevel;

				OMM_SUBRESOURCE_INTERLOCKEDADD(BakeResultBufferCounterBuffer, offset, 1, bakeResultGlobalOffset);
			}

			{
				const uint ommFormatAndPrimitiveIndex = (primitiveIndex) | ((uint)ommFormat << 30);

				const uint offset = 8 * (bakeResultGlobalOffset + subdivisionLevel * g_GlobalConstants.PrimitiveCount);

				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset, ommArrayOffset);
				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset + 4, ommFormatAndPrimitiveIndex);
			}

			/// ---- Setup baking parameters ----- 

			{
				// Increment the thread count for the current batch & subdivisiolevel.
				uint threadGroupCountX = 0;
				uint oldGlobalThreadCountX = 0;
				{
					const uint numThreadsNeeded = numMicroTriangles;

					const uint strideInBytes = 4; // sizeof(uint32_t)
					const uint offset = strideInBytes * subdivisionLevel;

					OMM_SUBRESOURCE_INTERLOCKEDADD(IEBakeCsThreadCountBuffer, offset, numThreadsNeeded, oldGlobalThreadCountX);
					uint newGlobalThreadCountX = numThreadsNeeded + oldGlobalThreadCountX;

					threadGroupCountX = (newGlobalThreadCountX + 127) / 128;
				}

				// Increment the drawcall count for the current batch & subdivisiolevel.
				{
					const uint strideInBytes = g_GlobalConstants.IndirectDispatchEntryStride; // arg count of Dispatch
					const uint ThreadCountXOffsetInBytes = 0;	 // offset of ThreadCountX in Dispatch
					const uint offset = ThreadCountXOffsetInBytes + strideInBytes * subdivisionLevel;

					uint _dummy;
					OMM_SUBRESOURCE_INTERLOCKEDMAX(IEBakeCsBuffer, offset, threadGroupCountX, _dummy);
				}
			}
		}
	}
	else
	{
		ommDescOffset = (uint)(-scheduledPrimitiveIndex - 5);
	}

	OMM_SUBRESOURCE_STORE(TempOmmIndexBuffer, 4 * primitiveIndex, ommDescOffset);
}