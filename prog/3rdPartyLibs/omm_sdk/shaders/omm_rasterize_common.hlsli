/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

namespace raster
{
	float4 TexCoord_to_VS_SV_Position(float2 texCoord)
	{
		TextureFilterMode mode = (TextureFilterMode)g_GlobalConstants.FilterType;

		if (mode == TextureFilterMode::Linear)
		{
			const float2 halfTexel = 0.5 * g_GlobalConstants.InvTexSize;
			texCoord -= halfTexel;
		}

		const float2 uv = (texCoord * g_GlobalConstants.TexSize + g_GlobalConstants.ViewportOffset) * g_GlobalConstants.InvViewportSize;
		return float4(float(uv.x) * 2 - 1, 1 - float(uv.y) * 2, 0, 1);
	}

	float2 PS_SV_Position_to_TexCoord(float4 svPosition)
	{
		TextureFilterMode mode = (TextureFilterMode)g_GlobalConstants.FilterType;

		float2 uv = (svPosition.xy - g_GlobalConstants.ViewportOffset) * g_GlobalConstants.InvTexSize;
		const float2 halfTexel = 0.5 * g_GlobalConstants.InvTexSize;
		float2 texCoord = mode == TextureFilterMode::Linear ? uv - halfTexel : uv;
		return texCoord;
	}
}
