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
#include "omm_compress.cs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_LOCAL_CONSTANT_BUFFER
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

uint GetMaxThreadCountX()
{
	const uint strideInBytes	= 4; // sizeof(uint32_t)
	const uint offset			= strideInBytes * (g_LocalConstants.SubdivisionLevel * g_GlobalConstants.MaxBatchCount + g_LocalConstants.BatchIndex);

	const uint threadCount		= OMM_SUBRESOURCE_LOAD(DispatchIndirectThreadCountBuffer, offset);

	return threadCount;
}

void StoreMacroTriangleState(OpacityState opacityState, uint primitiveIndex)
{
	const uint bakeResultMacroOffset = 12 * (primitiveIndex);

	if (g_GlobalConstants.EnablePostDispatchInfoStats)
	{
		uint dummy;
		if (opacityState == OpacityState::Opaque)
			OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset, 1, dummy);
		else if (opacityState == OpacityState::Transparent)
			OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset + 4, 1, dummy);
		else // if (opacityState == OpacityState::UnknownTransparent || opacityState == OpacityState::UnknownOpaque)
			OMM_SUBRESOURCE_INTERLOCKEDADD(SpecialIndicesStateBuffer, bakeResultMacroOffset + 8, 1, dummy);
	}
	else if (g_GlobalConstants.EnableSpecialIndices)
	{
		if (opacityState == OpacityState::Opaque)
			OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset, 1);
		else if (opacityState == OpacityState::Transparent)
			OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset + 4, 1);
		else // if (opacityState == OpacityState::UnknownTransparent || opacityState == OpacityState::UnknownOpaque)
			OMM_SUBRESOURCE_STORE(SpecialIndicesStateBuffer, bakeResultMacroOffset + 8, 1);
	}
}

[numthreads(128, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= GetMaxThreadCountX())
		return;

	const uint numMicroTrianglesPerPrimitive	= 1u << (g_LocalConstants.SubdivisionLevel << 1u);

	// This is the most number of micro-triangles that will be processed per thread
	// 32 allows non-atomic writes to the vmArrayBuffer for 2 and 4-state vm formats.
	const uint kMaxNumMicroTrianglePerThread	= 32; // For 2-state this means 1DW per thread, for 4-state it's 2DW

	const uint numMicroTrianglePerThread		= min(kMaxNumMicroTrianglePerThread, numMicroTrianglesPerPrimitive);

	// Since we process subdivision levels in batches, we can easilly compute the primitive offset based on thread id..
	const uint primitiveIndex					= (tid.x * numMicroTrianglePerThread) / numMicroTrianglesPerPrimitive;
	const uint rasterItemLocalIdx				= ((tid.x * numMicroTrianglePerThread) % numMicroTrianglesPerPrimitive) / numMicroTrianglePerThread;

	const uint rasterItemIdx					= (g_LocalConstants.PrimitiveIdOffset + primitiveIndex) + g_LocalConstants.SubdivisionLevel * g_GlobalConstants.PrimitiveCount;

	const uint vmArrayOffset					= OMM_SUBRESOURCE_LOAD(RasterItemsBuffer, 8 * rasterItemIdx);
	const uint vmFormatAndPrimitiveIndex		= OMM_SUBRESOURCE_LOAD(RasterItemsBuffer, 8 * rasterItemIdx + 4);
	const uint vmPrimitiveIndex					= (vmFormatAndPrimitiveIndex & 0x3FFFFFFF);
	const OMMFormat vmFormat					= (OMMFormat)(vmFormatAndPrimitiveIndex >> 30);

	[branch]
	if (vmFormat == OMMFormat::OC1_4)
	{
		const uint NumStatesPerDW = 16;
		const uint DwProcessCount = max(1, numMicroTrianglePerThread / NumStatesPerDW);
		[loop]
		for (uint j = 0; j < DwProcessCount; ++j)
		{
			uint stateDW = 0;

			uint offset = primitiveIndex * numMicroTrianglesPerPrimitive + rasterItemLocalIdx * numMicroTrianglePerThread;

			//[unroll(4)]
			for (uint i = 0; i < NumStatesPerDW; ++i)
			{
				const uint microTriangleIndex = i + j * NumStatesPerDW;
				const uint vmBakeResultOffset = 8 * (offset + microTriangleIndex);
				const uint2 counts = OMM_SUBRESOURCE_LOAD2(BakeResultBuffer, vmBakeResultOffset);
				OMM_SUBRESOURCE_STORE2(BakeResultBuffer, vmBakeResultOffset, 0); // clear for next iteration

				OpacityState opacityState = GetOpacityState(/*numOpaque*/ counts.x, /*numTransparent*/ counts.y, g_GlobalConstants.AlphaCutoffGreater, g_GlobalConstants.AlphaCutoffLessEqual, vmFormat);

				if (microTriangleIndex < numMicroTrianglesPerPrimitive)
					StoreMacroTriangleState(opacityState, vmPrimitiveIndex);

				const uint state = 0x3 & (uint)opacityState;
				stateDW |= (state << (i << 1u));
			}

			u_vmArrayBuffer.Store(vmArrayOffset + 4 * (DwProcessCount * rasterItemLocalIdx + j), stateDW);
		}
	}
	else // if (vmFormat == OMMFormat::OC1_2)
	{
		const uint NumStatesPerDW = 32;
		const uint DwProcessCount = max(1, numMicroTrianglePerThread / NumStatesPerDW);
		[loop]
		for (uint j = 0; j < DwProcessCount; ++j)
		{
			uint stateDW = 0;

			uint offset = primitiveIndex * numMicroTrianglesPerPrimitive + rasterItemLocalIdx * numMicroTrianglePerThread;
			
			//[unroll(4)]
			for (uint i = 0; i < NumStatesPerDW; ++i)
			{
				const uint microTriangleIndex = i + j * NumStatesPerDW;
				const uint vmBakeResultOffset = 8 * (offset + microTriangleIndex);
				const uint2 counts = OMM_SUBRESOURCE_LOAD2(BakeResultBuffer, vmBakeResultOffset);
				OMM_SUBRESOURCE_STORE2(BakeResultBuffer, vmBakeResultOffset, 0); // clear for next iteration

				OpacityState opacityState = GetOpacityState(/*numOpaque*/ counts.x, /*numTransparent*/ counts.y, g_GlobalConstants.AlphaCutoffGreater, g_GlobalConstants.AlphaCutoffLessEqual, vmFormat);

				if (microTriangleIndex < numMicroTrianglesPerPrimitive)
					StoreMacroTriangleState(opacityState, vmPrimitiveIndex);

				const uint state = 0x1 & (uint)opacityState;
				stateDW |= (state << (i));
			}

			u_vmArrayBuffer.Store(vmArrayOffset + 4 * (DwProcessCount * rasterItemLocalIdx + j), stateDW);
		}
	}
}