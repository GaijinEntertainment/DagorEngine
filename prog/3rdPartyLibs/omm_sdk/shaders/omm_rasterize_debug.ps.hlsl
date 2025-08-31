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
#include "omm_rasterize.vs.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_LOCAL_CONSTANT_BUFFER
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_rasterize_common.hlsli"

void main(
	nointerpolation in uint	i_primitiveIndex	: SV_InstanceID
	, in float4				i_svPosition		: SV_Position
	, in uint				i_microTriIndex		: SV_PrimitiveID
	, out float4			o_color				: SV_Target0
)
{
	// [0| 1 + PrimitiveId]
	// [macro state|micro-tri states]
	const uint microTriOffset = 8 * (i_primitiveIndex * g_LocalConstants.VmResultBufferStride + i_microTriIndex);

	const uint numOpaque		= OMM_SUBRESOURCE_LOAD(BakeResultBuffer, microTriOffset);
	const uint numTrans			= OMM_SUBRESOURCE_LOAD(BakeResultBuffer, microTriOffset + 4);
    const OpacityState state	= GetOpacityState(numOpaque, numTrans, g_GlobalConstants.AlphaCutoffGreater, g_GlobalConstants.AlphaCutoffLessEqual, OMMFormat::OC1_4);
	const float3 debugColor		= GetDebugColorForState(state);

	const float2 texCoord		= raster::PS_SV_Position_to_TexCoord(i_svPosition);

	const float4 color			= t_alphaTexture.SampleLevel(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
	const float alpha			= color[g_GlobalConstants.AlphaTextureChannel];

	//o_color = float4(alpha.rgb, 1);
	o_color = float4(debugColor * alpha.xxx, 1);
	//float NumLevels = (float)g_LocalConstants.SubdivisionLevel;
	//o_color = float4(i_microTriIndex / NumLevels, i_microTriIndex / NumLevels, i_microTriIndex / NumLevels, 1);
}