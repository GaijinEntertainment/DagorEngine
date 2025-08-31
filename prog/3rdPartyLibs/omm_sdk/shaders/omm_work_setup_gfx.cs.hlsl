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
#include "omm_work_setup_gfx.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_common.hlsli"
#include "omm_hash_table.hlsli"

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= g_GlobalConstants.PrimitiveCount)
		return;

	// TODO: subdivision level should be tuneable per VM.
	const uint kOMMFormatNum		= 2;
	const uint primitiveIndex		= tid.x;

	const TexCoords texCoords		= FetchTexCoords(t_indexBuffer, t_texCoordBuffer, primitiveIndex);
	const uint subdivisionLevel		= GetSubdivisionLevel(texCoords);

	uint hashTableEntryIndex;
	hashTable::Result result		= FindOrInsertOMMEntry(texCoords, subdivisionLevel, hashTableEntryIndex);

	int vmDescOffset = (int)SpecialIndex::FullyUnknownOpaque;

	if (result == hashTable::Result::Null ||
		result == hashTable::Result::Inserted || 
		result == hashTable::Result::ReachedMaxAttemptCount)
	{
		// Store the primitiveIndex in the hash table so it can be referenced later by primitives referencing this one
		if (result == hashTable::Result::Inserted)
		{
			hashTable::Store(hashTableEntryIndex, primitiveIndex);
		}

		const OMMFormat ommFormat		= (OMMFormat)g_GlobalConstants.OMMFormat;
		const uint numMicroTriangles	= GetNumMicroTriangles(subdivisionLevel);

		// Allocate new VM-array offset & vm-index
		uint vmArrayOffset = 0;
		uint vmDataByteSize = 0;
		{
			const uint vmDataBitSize			= GetOMMFormatBitCount(ommFormat) * numMicroTriangles;

			// spec allows 1 byte alignment but we require 4 byte to make sure UAV writes
			// are DW aligned.
			vmDataByteSize						= max(vmDataBitSize >> 3u, 4u);

			OMM_SUBRESOURCE_INTERLOCKEDADD(OmmArrayAllocatorCounterBuffer, 0, vmDataByteSize, vmArrayOffset);
		}

		// Allocate new VM-desc for the vmArrayOffset
		if ((vmArrayOffset + vmDataByteSize) <= g_GlobalConstants.MaxOutOmmArraySize)
		{
			// The rasterItemOffset is the same things as the vmDescOffset,
			OMM_SUBRESOURCE_INTERLOCKEDADD(OmmDescAllocatorCounterBuffer, 0, 1, vmDescOffset);

			// Store vmDesc for current primitive. 
			// vmArrayOffset may be shared at this point.
			{
				const uint vmDescData = ((uint)ommFormat << 16u) | subdivisionLevel;

				u_ommDescArrayBuffer.Store(vmDescOffset * 8, vmArrayOffset);
				u_ommDescArrayBuffer.Store(vmDescOffset * 8 + 4, vmDescData);
			}

			// Increase UsageDesc info struct,
			// resolve uniform vm's later by usage subtraction.
			{
				const uint strideInBytes		= 8;	// sizeof(VisibilityMapUsageDesc), [count32, format16, level16]
				const uint index				= (kOMMFormatNum * subdivisionLevel + ((uint)ommFormat - 1));
				const uint offset				= strideInBytes * index;

				InterlockedAdd(u_ommDescArrayHistogramBuffer, offset, 1);
			}

			/// ---- Setup baking parameters ----- 

			// Allocate a slot in the raster items array.
			uint bakeResultGlobalOffset	= 0;
			uint bakeResultBatchIndex	= 0;
			{
				const uint offset						= 4 * subdivisionLevel;

				OMM_SUBRESOURCE_INTERLOCKEDADD(BakeResultBufferCounterBuffer, offset, 1, bakeResultGlobalOffset);

				const uint maxItemsPerBatch				= GetMaxItemsPerBatch(subdivisionLevel);
				bakeResultBatchIndex					= bakeResultGlobalOffset / maxItemsPerBatch;
			}

			// Store the VM-that will be procesed by the rasterizer.
			{
				const uint ommFormatAndPrimitiveIndex = (primitiveIndex) | ((uint)ommFormat << 30);

				const uint offset					 = 8 * (bakeResultGlobalOffset + subdivisionLevel * g_GlobalConstants.PrimitiveCount);

				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset, vmArrayOffset);
				OMM_SUBRESOURCE_STORE(RasterItemsBuffer, offset + 4, ommFormatAndPrimitiveIndex);
			}

			// Increment the drawcall count for the current batch & subdivisiolevel.
			{
				const uint strideInBytes					= g_GlobalConstants.IndirectDispatchEntryStride;	// arg count of DrawIndexedInstanced 
				const uint InstanceCountOffsetInBytes		= 4;	// offset of InstanceCount in DrawIndexedInstanced
				const uint offset							= InstanceCountOffsetInBytes + strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				OMM_SUBRESOURCE_INTERLOCKEDADD(IEBakeBuffer, offset, 1, bakeResultGlobalOffset);
			}

			// Increment the thread count for the current batch & subdivisiolevel.
			uint threadGroupCountX = 0;
			{
				// This is the most number of micro-triangles that will be processed per thread
				// 32 allows non-atomic writes to the vmArrayBuffer for 2 and 4-state vm formats.
				const uint kMaxNumMicroTrianglePerThread	= 32;
				const uint numMicroTrianglePerThread		= min(kMaxNumMicroTrianglePerThread, numMicroTriangles);
				const uint numThreadsNeeded					= max(numMicroTriangles / numMicroTrianglePerThread, 1u);

				const uint strideInBytes					= 4; // sizeof(uint32_t)
				const uint offset							= strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				uint oldGlobalThreadCountX;
				OMM_SUBRESOURCE_INTERLOCKEDADD(DispatchIndirectThreadCountBuffer, offset, numThreadsNeeded, oldGlobalThreadCountX);
				uint newGlobalThreadCountX = numThreadsNeeded + oldGlobalThreadCountX;

				threadGroupCountX = (newGlobalThreadCountX + 127) / 128;
			}

			// Increment the thread GROUP count for the current batch & subdivisiolevel.
			{
				const uint strideInBytes			 = g_GlobalConstants.IndirectDispatchEntryStride; // arg count of Dispatch
				const uint ThreadCountXOffsetInBytes = 0;	 // offset of ThreadCountX in Dispatch
				const uint offset					 = ThreadCountXOffsetInBytes + strideInBytes * (subdivisionLevel * g_GlobalConstants.MaxBatchCount + bakeResultBatchIndex);

				uint _dummy;
				OMM_SUBRESOURCE_INTERLOCKEDMAX(IECompressCsBuffer, offset, threadGroupCountX, _dummy);
			}
		}
	}
	else // if (status == hashTable::Result::Found
	{
		// Store the hash-table offset and patch up the pointers later.
		vmDescOffset = (uint)(-hashTableEntryIndex - 5);
	}

	OMM_SUBRESOURCE_STORE(TempOmmIndexBuffer, 4 * primitiveIndex, vmDescOffset);
}