/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "math.h"
#include <omm.hpp>

namespace omm
{
namespace parse
{
	static int32_t GetOmmIndexForTriangleIndex(const ommCpuBakeResultDesc& resDesc, uint32_t i) {
		OMM_ASSERT(resDesc.indexFormat == ommIndexFormat_UINT_16 || resDesc.indexFormat == ommIndexFormat_UINT_32);
		if (resDesc.indexFormat == ommIndexFormat_UINT_16)
			return reinterpret_cast<const int16_t*>(resDesc.indexBuffer)[i];
		else
			return reinterpret_cast<const int32_t*>(resDesc.indexBuffer)[i];
	}

	static int32_t GetOmmIndexForTriangleIndex(const omm::Cpu::BakeResultDesc& resDesc, uint32_t i) {
		return GetOmmIndexForTriangleIndex(reinterpret_cast<const ommCpuBakeResultDesc&>(resDesc), i);
	}

	static int32_t GetOmmBitSize(const ommCpuOpacityMicromapDesc& desc)
	{
		uint32_t bitSize = ((ommFormat)desc.format == ommFormat_OC1_2_State) ? 2 : 4;
		return bitSize * omm::bird::GetNumMicroTriangles(desc.subdivisionLevel);
	}

	static int32_t GetOmmBitSize(const omm::Cpu::OpacityMicromapDesc& desc)
	{
		return GetOmmBitSize(reinterpret_cast<const ommCpuOpacityMicromapDesc&>(desc));
	}

	static int32_t GetTriangleStates(uint32_t triangleIdx, const ommCpuBakeResultDesc& resDesc, ommOpacityState* outStates) {

		const int32_t vmIdx = GetOmmIndexForTriangleIndex(resDesc, triangleIdx);

		if (vmIdx < 0) {
			if (outStates) {
				outStates[0] = (ommOpacityState)~vmIdx;
			}
			return 0;
		}
		else {

			const ommCpuOpacityMicromapDesc& ommDesc = resDesc.descArray[vmIdx];
			const uint8_t* ommArrayData = (const uint8_t*)((const char*)resDesc.arrayData) + ommDesc.offset;
			const uint32_t numMicroTriangles = 1u << (ommDesc.subdivisionLevel << 1u);
			const uint32_t is2State = (ommFormat)ommDesc.format == ommFormat_OC1_2_State ? 1 : 0;
			if (outStates) {
				const uint32_t vmBitCount = omm::bird::GetBitCount((ommFormat)ommDesc.format);
				for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt)
				{
					int byteIndex = uTriIt >> (2 + is2State);
					uint8_t v = ((uint8_t*)ommArrayData)[byteIndex];
					ommOpacityState state;
					if (is2State)   state = ommOpacityState((v >> ((uTriIt << 0) & 7)) & 1); // 2-state
					else			state = ommOpacityState((v >> ((uTriIt << 1) & 7)) & 3); // 4-state
					outStates[uTriIt] = state;
				}
			}

			return ommDesc.subdivisionLevel;
		}
	}

	static int32_t GetTriangleStates(uint32_t triangleIdx, const omm::Cpu::BakeResultDesc& resDesc, omm::OpacityState* outStates) {
		return GetTriangleStates(triangleIdx, reinterpret_cast<const ommCpuBakeResultDesc&>(resDesc), reinterpret_cast<ommOpacityState*>(outStates));
	}
} // namespace parse
} // namespace omm