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
#include "omm_rasterize_common.vs.hlsli"

void main(
	in uint		i_pos				: POSITION,
	uint		i_primitiveIndex	: SV_InstanceID,
	out uint	o_primitiveIndex	: SV_InstanceID,
	out float2  o_texCoord			: TEXCOORD0,
	out uint	o_vmStateAtVertex	: TEXCOORD1,
	out float4	o_posClip			: SV_Position
)
{
	// This gives us the texture coordinate at the current vertex for this primitive.
	const float2 texCoord		= GetTexCoordForVertex(i_pos, i_primitiveIndex);

	// If we run linear sampling (with the precise method) a base state must always be present.
	// We select the current 
	const float4 color			= t_alphaTexture.SampleLevel(s_samplers[g_GlobalConstants.SamplerIndex], texCoord.xy, 0);
	const float alpha			= color[g_GlobalConstants.AlphaTextureChannel];

    const OpacityState vertexState = (OpacityState) (g_GlobalConstants.AlphaCutoff < alpha ? (uint) OpacityState::Opaque : (uint) OpacityState::Transparent);

	o_primitiveIndex		= i_primitiveIndex;
	o_texCoord				= texCoord;
	o_vmStateAtVertex		= (uint)vertexState;
	o_posClip				= raster::TexCoord_to_VS_SV_Position(texCoord);
}