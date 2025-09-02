/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef OMM_COMMON_HLSLI
#define OMM_COMMON_HLSLI

#include "omm_platform.hlsli"
#include "omm.hlsli"
#include "omm_global_cb.hlsli"
#include "omm_global_samplers.hlsli"

/// Unrolled version of murmur hash that takes N integers as input (up to 8)
uint murmur_32_scramble(uint k)
{
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
};

uint murmur_32_process(uint k, uint h)
{
	h ^= murmur_32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xe6546b64;
	return h;
};

uint murmurUint7(
	uint key0,
	uint key1,
	uint key2,
	uint key3,
	uint key4,
	uint key5,
	uint key6,
	uint SEED)
{
	uint h = SEED;
	h = murmur_32_process(key0, h);
	h = murmur_32_process(key1, h);
	h = murmur_32_process(key2, h);
	h = murmur_32_process(key3, h);
	h = murmur_32_process(key4, h);
	h = murmur_32_process(key5, h);
	h = murmur_32_process(key6, h);

	// A swap is *not* necessary here because the preceding loop already
	// places the low bytes in the low places according to whatever endianness
	// we use. Swaps only apply when the memory is copied in a chunk.
	h ^= murmur_32_scramble(0);
	/* Finalize. */
	h ^= 24; // len = 6 * sizeof(float)
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
};

uint GetOMMFormatBitCount(OMMFormat vmFormat)
{
	return (uint)vmFormat;
}

struct TexCoords
{
	float2 p0;
	float2 p1;
	float2 p2;

	void Init(float2 _p0, float2 _p1, float2 _p2) {
		p0 = _p0;
		p1 = _p1;
		p2 = _p2;
	}
};

uint GetHash(TexCoords tex, uint subdivisionLevel)
{
	const uint seed = 1337;

	return murmurUint7(
		asuint(tex.p0.x),
		asuint(tex.p0.y), 
		asuint(tex.p1.x), 
		asuint(tex.p1.y),
		asuint(tex.p2.x),
		asuint(tex.p2.y),
		asuint(subdivisionLevel),
		seed);
}

float2 UnpackUNORM16Pair(uint packed)
{
    float2 data;
	data.x = float(packed & 0x0000FFFF) / float(0xFFFF);
    data.y = float((packed >> 16u) & 0x0000FFFF) / float(0xFFFF);
    return data;
}

float2 UnpackFP16Pair(uint packed)
{
    uint2 data;
	data.x = packed & 0x0000FFFF;
    data.y = (packed >> 16u) & 0x0000FFFF;
    return f16tof32(data);
}

TexCoords FetchTexCoords(Buffer<uint> indexBuffer, ByteAddressBuffer texCoordBuffer, uint primitiveIndex)
{
	uint3 indices;
	indices.x		= indexBuffer[primitiveIndex * 3 + 0];
	indices.y		= indexBuffer[primitiveIndex * 3 + 1];
	indices.z		= indexBuffer[primitiveIndex * 3 + 2];

	float2 vertexUVs[3];
	
    if ((TexCoordFormat)g_GlobalConstants.TexCoordFormat == TexCoordFormat::UV16_UNORM)
    {
        vertexUVs[0] = UnpackUNORM16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.x * g_GlobalConstants.TexCoordStride));
        vertexUVs[1] = UnpackUNORM16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.y * g_GlobalConstants.TexCoordStride));
        vertexUVs[2] = UnpackUNORM16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.z * g_GlobalConstants.TexCoordStride));
    }
    else if ((TexCoordFormat)g_GlobalConstants.TexCoordFormat == TexCoordFormat::UV16_FLOAT)
    {
        vertexUVs[0] = UnpackFP16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.x * g_GlobalConstants.TexCoordStride));
        vertexUVs[1] = UnpackFP16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.y * g_GlobalConstants.TexCoordStride));
        vertexUVs[2] = UnpackFP16Pair(texCoordBuffer.Load(g_GlobalConstants.TexCoordOffset + indices.z * g_GlobalConstants.TexCoordStride));
    }
    else // if (g_GlobalConstants.TexCoordFormat == TexCoordFormat::UV32_FLOAT)
    {
        vertexUVs[0] = asfloat(texCoordBuffer.Load2(g_GlobalConstants.TexCoordOffset + indices.x * g_GlobalConstants.TexCoordStride));
        vertexUVs[1] = asfloat(texCoordBuffer.Load2(g_GlobalConstants.TexCoordOffset + indices.y * g_GlobalConstants.TexCoordStride));
        vertexUVs[2] = asfloat(texCoordBuffer.Load2(g_GlobalConstants.TexCoordOffset + indices.z * g_GlobalConstants.TexCoordStride));
    }

	TexCoords tex;
	tex.Init(vertexUVs[0], vertexUVs[1], vertexUVs[2]);
	return tex;
}

uint GetNextPow2(uint v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

float GetArea2D(float2 p0, float2 p1, float2 p2) {
	const float2 v0 = p2 - p0;
	const float2 v1 = p1 - p0;
	return 0.5f * length(cross(float3(v0, 0), float3(v1, 0)));
}

uint GetLog2(uint v) { // V must be power of 2.
	const unsigned int b[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
									 0xFF00FF00, 0xFFFF0000 };
	unsigned int r = (v & b[0]) != 0;
	for (uint i = 4; i > 0; i--) // unroll for speed...
	{
		r |= ((v & b[i]) != 0) << i;
	}
	return r;
};

uint GetDynamicSubdivisionLevel(TexCoords tex, float scale)
{
	const float pixelUvArea = GetArea2D(tex.p0 * g_GlobalConstants.TexSize, tex.p1 * g_GlobalConstants.TexSize, tex.p2 * g_GlobalConstants.TexSize);

	// Solves the following eqn:
	// targetPixelArea / (4^N) = pixelUvArea 

	const float targetPixelArea = scale * scale;
	const uint ratio			= pixelUvArea / targetPixelArea;
	const uint ratioNextPow2	= GetNextPow2(ratio);
	const uint log2_ratio		= GetLog2(ratioNextPow2);

	const uint SubdivisionLevel = log2_ratio >> 1u; // log2(ratio) / log2(4)

	return min(SubdivisionLevel, g_GlobalConstants.MaxSubdivisionLevel);
}

uint InterlockedAdd(RWByteAddressBuffer buffer, uint offset, uint increment)
{
#if 1
	uint val = 0;
	buffer.InterlockedAdd(offset, increment, val);
	return val;
#else
	const uint totalIncrement		= WaveActiveSum(increment);
	const uint localOffset			= WavePrefixSum(increment);
	
	uint globalOffset = 0;
	if (WaveIsFirstLane())
	{
		buffer.InterlockedAdd(offset, totalIncrement, globalOffset);
	}
	return WaveReadLaneFirst(globalOffset) + localOffset;
#endif
}

uint GetNumMicroTriangles(uint subdivisionLevel)
{
	return 1u << (subdivisionLevel << 1u);
}

uint GetMaxItemsPerBatch(uint subdivisionLevel)
{
	const uint numMicroTri				= GetNumMicroTriangles(subdivisionLevel);
	const uint rasterItemByteSize		= (numMicroTri) * 8; // We need 2 x uint32 for each micro-VM state.
	return g_GlobalConstants.BakeResultBufferSize / rasterItemByteSize;
}

uint GetSubdivisionLevel(TexCoords texCoords)
{
	const bool bEnableDynamicSubdivisionLevel = g_GlobalConstants.DynamicSubdivisionScale > 0.f;

	if (bEnableDynamicSubdivisionLevel)
	{
		return GetDynamicSubdivisionLevel(texCoords, g_GlobalConstants.DynamicSubdivisionScale);
	}
	else
	{
		return g_GlobalConstants.MaxSubdivisionLevel;
	}
}

int GetOmmDescOffset(ByteAddressBuffer ommIndexBuffer, uint primitiveIndex)
{
	if (g_GlobalConstants.IsOmmIndexFormat16bit)
	{
		const uint dwOffset = primitiveIndex.x >> 1u;
		const uint shift	= (primitiveIndex.x & 1u) << 4u; // 0 or 16
		const uint raw		= ommIndexBuffer.Load(4 * dwOffset);
		const uint raw16	= (raw >> shift) & 0xFFFFu;

		if (raw16 > 0xFFFB) // e.g special index
		{
			return (raw16 - 0xFFFF) - 1; // -1, -2, -3 or -4
		}

		return raw16;
	}
	else
	{
		return ommIndexBuffer.Load(4 * primitiveIndex);
	}
}

#endif // OMM_COMMON_HLSLI