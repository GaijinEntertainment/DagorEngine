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
#include "omm_rasterize.gs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_LOCAL_CONSTANT_BUFFER
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_rasterize_common.hlsli"

struct VSOutput
{
	uint	o_primitiveId		: SV_InstanceID;
	float2  o_texCoord			: TEXCOORD0;
	uint	o_vmStateAtVertex	: TEXCOORD1;
	float4	o_posClip			: SV_Position;
};

struct GSOutput
{
	uint	o_primitiveId	: SV_InstanceID;
	float4	o_posClip		: SV_Position;
	uint	o_microTriIndex	: SV_PrimitiveID;

	linear float2			o_texCoord			: TEXCOORD0;
	nointerpolation float2  o_texCoord0			: TEXCOORD1;
	nointerpolation float2  o_texCoord1			: TEXCOORD2;
	nointerpolation float2  o_texCoord2			: TEXCOORD3;
};

#define DEBUG_ENABLE_AABB_TEST (0)

void DoAABBTest(uint i_microTriIndex, triangle VSOutput Input[3])
{
	const float2 uv0 = Input[0].o_texCoord.xy;
	const float2 uv1 = Input[1].o_texCoord.xy;
	const float2 uv2 = Input[2].o_texCoord.xy;

	TextureFilterMode mode = (TextureFilterMode)g_GlobalConstants.FilterType;

	const uint i_primitiveId = Input[0].o_primitiveId;
	const uint microTriOffset = 8 * (i_primitiveId * g_LocalConstants.VmResultBufferStride + i_microTriIndex);

	const float2 p0 = uv0.xy * g_GlobalConstants.TexSize;
	const float2 p1 = uv1.xy * g_GlobalConstants.TexSize;
	const float2 p2 = uv2.xy * g_GlobalConstants.TexSize;

	{
		int from_x = min(min(p0.x, p1.x), p2.x);
		int to_x = (int)max(max(p0.x, p1.x), p2.x) + 1;

		int from_y = min(min(p0.y, p1.y), p2.y);
		int to_y = (int)max(max(p0.y, p1.y), p2.y) + 1;

		for (int y = from_y; y < to_y; ++y)
		{
			for (int x = from_x; x < to_x; ++x)
			{
				float2 uv = float2(x, y) * g_GlobalConstants.InvTexSize;

				const float2 halfTexel = 0;// 0.5 * g_GlobalConstants.InvTexSize;
				const float4 alpha = t_alphaTexture.GatherAlpha(s_samplers[g_GlobalConstants.SamplerIndex], uv.xy + halfTexel, 0);

				const float min_a = min(alpha.x, min(alpha.y, min(alpha.z, alpha.w)));
				const float max_a = max(alpha.x, max(alpha.y, max(alpha.z, alpha.w)));

				bool IsOpaque = g_GlobalConstants.AlphaCutoff < max_a;
				bool IsTransparent = g_GlobalConstants.AlphaCutoff >= min_a;

				if (IsOpaque)
					OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset, 1);
				if (IsTransparent)
					OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset + 4, 1);
			}
		}
	}
}

[maxvertexcount(3)]
void main(
	uint i_microTriIndex	: SV_PrimitiveID,
	triangle VSOutput Input[3],
	inout TriangleStream<GSOutput> Output)
{
#if DEBUG_ENABLE_AABB_TEST
	// This is a mode only useful to compare the raster based approach with.
	DoAABBTest(i_microTriIndex, Input);

	const bool AlwaysFalse = Input[0].o_vmStateAtVertex == 100;
	if (AlwaysFalse)
	{
		for (uint i = 0; i < 3; ++i)
		{
			GSOutput output;
			output.o_primitiveId = Input[i].o_primitiveId;
			output.o_posClip = TexCoord_to_VS_SV_Position(Input[i].o_texCoord.xy);
			output.o_microTriIndex = i_microTriIndex;
			output.o_texCoord0 = Input[0].o_texCoord;
			output.o_texCoord1 = Input[1].o_texCoord;
			output.o_texCoord2 = Input[2].o_texCoord;
			output.o_texCoord = Input[i].o_texCoord;
			Output.Append(output);
		}
	}
#else

	const uint i_primitiveId = Input[0].o_primitiveId;
	const uint microTriOffset = 8 * (i_primitiveId * g_LocalConstants.VmResultBufferStride + i_microTriIndex);

	// Here we check the 3 vertex states:
	//: If they all are the same we continue the search in the raster state.
	//: If they _not_ the same we can set it's unknown state already, and skip passing it to rasterizer
	const bool AllOMMStatesEqual =
		(Input[0].o_vmStateAtVertex == Input[1].o_vmStateAtVertex) &&
		(Input[0].o_vmStateAtVertex == Input[2].o_vmStateAtVertex);

	// Write the base state for this micro triangle.
	// IF there are different states then we write unknown state.
	{
		// Store one of the states, if they're all equal it doesn't matter which one.
		const OpacityState vmState = (OpacityState)Input[0].o_vmStateAtVertex;

		const bool isOpaque = vmState == OpacityState::Opaque || vmState == OpacityState::UnknownOpaque;
		const bool isTransparent = vmState == OpacityState::Transparent || vmState == OpacityState::UnknownTransparent;

		if (isOpaque || !AllOMMStatesEqual)
			OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset, 1);
		if (isTransparent || !AllOMMStatesEqual)
			OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset + 4, 1);
	}

	// If all states are equal we need to pass
	if (AllOMMStatesEqual)
	{
		for (uint i = 0; i < 3; ++i)
		{
			GSOutput output;
			output.o_primitiveId = Input[i].o_primitiveId;
			output.o_posClip = raster::TexCoord_to_VS_SV_Position(Input[i].o_texCoord.xy);
			output.o_microTriIndex = i_microTriIndex;
			output.o_texCoord0 = Input[0].o_texCoord;
			output.o_texCoord1 = Input[1].o_texCoord;
			output.o_texCoord2 = Input[2].o_texCoord;
			output.o_texCoord = Input[i].o_texCoord;
			Output.Append(output);
		}
	}
#endif
	
}