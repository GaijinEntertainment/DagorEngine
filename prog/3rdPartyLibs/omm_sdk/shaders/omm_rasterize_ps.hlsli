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
#include "omm_rasterize.ps.resources.hlsli"

OMM_DECLARE_GLOBAL_CONSTANT_BUFFER
OMM_DECLARE_GLOBAL_SAMPLERS
OMM_DECLARE_LOCAL_CONSTANT_BUFFER
OMM_DECLARE_INPUT_RESOURCES
OMM_DECLARE_OUTPUT_RESOURCES
OMM_DECLARE_SUBRESOURCES

#include "omm_rasterize_common.hlsli"
#include "omm_resample_common.hlsli"

void main(
	nointerpolation in uint	     i_primitiveId	    : SV_InstanceID
	, in float4				     i_svPosition	    : SV_Position
	, in uint				     i_microTriIndex    : SV_PrimitiveID
	//, out float4			     o_color		    : SV_Target0
    , linear in float2           i_texCoord         : TEXCOORD0
    , nointerpolation in float2  i_texCoord0        : TEXCOORD1
    , nointerpolation in float2  i_texCoord1        : TEXCOORD2
    , nointerpolation in float2  i_texCoord2        : TEXCOORD3
)
{
	// [0| 1 + PrimitiveId]
	// [macro state|micro-tri states]
	const uint microTriOffset	= 8 * (i_primitiveId * g_LocalConstants.VmResultBufferStride + i_microTriIndex);

	bool isOpaque = 0;
	bool isTransparent = 0;

    const float2 texCoord = i_texCoord;// - 0.5 * g_GlobalConstants.InvTexSize;// PS_SV_Position_to_TexCoord(i_svPosition);
    const float2 texel = (i_texCoord - 0.5 * g_GlobalConstants.InvTexSize) * g_GlobalConstants.TexSize;// PS_SV_Position_to_Texel(i_svPosition);

    raster::GetStateAtPixelCenter(texCoord, texel, i_texCoord0, i_texCoord1, i_texCoord2, i_primitiveId, isOpaque, isTransparent);
#if ENABLE_EXACT_CLASSIFICATION
    uint, _dummy;
    if (isOpaque)
        OMM_SUBRESOURCE_INTERLOCKEDADD(BakeResultBuffer, microTriOffset, 1, _dummy);
    if (isTransparent)
        OMM_SUBRESOURCE_INTERLOCKEDADD(BakeResultBuffer, microTriOffset + 4, 1, _dummy);
#else
	if (isOpaque)
        OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset, 1);
	if (isTransparent)
        OMM_SUBRESOURCE_STORE(BakeResultBuffer, microTriOffset + 4, 1);
#endif
}