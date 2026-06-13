// Copyright  © 2023 Advanced Micro Devices, Inc.
// Copyright  © 2024-2025 Arm Limited.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "./fsr2/ffxm_fsr2_resources.h"

#if defined(FFXM_GPU)
#include "./ffxm_core.h"
#endif // #if defined(FFXM_GPU)

#if defined(FFXM_GPU)
#ifndef FFXM_PREFER_WAVE64
#define FFXM_PREFER_WAVE64
#endif // FFXM_PREFER_WAVE64

#define SET_0_CB_START 2

// Workaround
#if FFXM_SHADER_PLATFORM_GLES_3_2
#define FFXM_UAV_RG_QUALIFIER rg32f
#else
#define FFXM_UAV_RG_QUALIFIER rgba32f
#endif

#if defined(FSR2_BIND_CB_FSR2)
    layout (set = 0, binding = FSR2_BIND_CB_FSR2 + SET_0_CB_START, std140) uniform cbFSR2_t
    {
		FfxInt32x2    iRenderSize;
		FfxInt32x2    iMaxRenderSize;
		FfxInt32x2    iDisplaySize;
		FfxInt32x2    iInputColorResourceDimensions;
		FfxInt32x2    iLumaMipDimensions;
		FfxInt32      iLumaMipLevelToUse;
		FfxInt32      iFrameIndex;

		FfxFloat32x4  fDeviceToViewDepth;
		FfxFloat32x2  fJitter;
		FfxFloat32x2  fMotionVectorScale;
		FfxFloat32x2  fDownscaleFactor;
		FfxFloat32x2  fMotionVectorJitterCancellation;
		FfxFloat32    fPreExposure;
		FfxFloat32    fPreviousFramePreExposure;
		FfxFloat32    fTanHalfFOV;
		FfxFloat32    fJitterSequenceLength;
		FfxFloat32    fDeltaTime;
		FfxFloat32    fDynamicResChangeFactor;
		FfxFloat32    fViewSpaceToMetersFactor;
	} cbFSR2;


FfxInt32x2 RenderSize()
{
    return cbFSR2.iRenderSize;
}

FfxInt32x2 MaxRenderSize()
{
    return cbFSR2.iMaxRenderSize;
}

FfxInt32x2 DisplaySize()
{
    return cbFSR2.iDisplaySize;
}

FfxInt32x2 InputColorResourceDimensions()
{
    return cbFSR2.iInputColorResourceDimensions;
}

FfxInt32x2 LumaMipDimensions()
{
    return cbFSR2.iLumaMipDimensions;
}

FfxInt32  LumaMipLevelToUse()
{
    return cbFSR2.iLumaMipLevelToUse;
}

FfxInt32 FrameIndex()
{
    return cbFSR2.iFrameIndex;
}

FfxFloat32x4 DeviceToViewSpaceTransformFactors()
{
    return cbFSR2.fDeviceToViewDepth;
}

FfxFloat32x2 Jitter()
{
    return cbFSR2.fJitter;
}

FfxFloat32x2 MotionVectorScale()
{
    return cbFSR2.fMotionVectorScale;
}

FfxFloat32x2 DownscaleFactor()
{
    return cbFSR2.fDownscaleFactor;
}

FfxFloat32x2 MotionVectorJitterCancellation()
{
    return cbFSR2.fMotionVectorJitterCancellation;
}

FfxFloat32 PreExposure()
{
    return cbFSR2.fPreExposure;
}

FfxFloat32 PreviousFramePreExposure()
{
    return cbFSR2.fPreviousFramePreExposure;
}

FfxFloat32 TanHalfFoV()
{
    return cbFSR2.fTanHalfFOV;
}

FfxFloat32 JitterSequenceLength()
{
    return cbFSR2.fJitterSequenceLength;
}

FfxFloat32 DeltaTime()
{
    return cbFSR2.fDeltaTime;
}

FfxFloat32 DynamicResChangeFactor()
{
    return cbFSR2.fDynamicResChangeFactor;
}

FfxFloat32 ViewSpaceToMetersFactor()
{
    return cbFSR2.fViewSpaceToMetersFactor;
}

#endif // #if defined(FSR2_BIND_CB_FSR2)

#if defined(FSR2_BIND_CB_RCAS)
layout(set = 0, binding = FSR2_BIND_CB_RCAS + SET_0_CB_START, std140) uniform cbRCAS_t
{
    FfxUInt32x4 rcasConfig;
} cbRCAS;

FfxUInt32x4 RCASConfig()
{
    return cbRCAS.rcasConfig;
}
#endif // #if defined(FSR2_BIND_CB_RCAS)


#if defined(FSR2_BIND_CB_REACTIVE)
layout(set = 0, binding = FSR2_BIND_CB_REACTIVE + SET_0_CB_START, std140) uniform cbGenerateReactive_t
{
    FfxFloat32   scale;
    FfxFloat32   threshold;
    FfxFloat32   binaryValue;
    FfxUInt32    flags;
} cbGenerateReactive;

FfxFloat32 GenReactiveScale()
{
    return cbGenerateReactive.scale;
}

FfxFloat32 GenReactiveThreshold()
{
    return cbGenerateReactive.threshold;
}

FfxFloat32 GenReactiveBinaryValue()
{
    return cbGenerateReactive.binaryValue;
}

FfxUInt32 GenReactiveFlags()
{
    return cbGenerateReactive.flags;
}
#endif // #if defined(FSR2_BIND_CB_REACTIVE)


#if defined(FSR2_BIND_CB_SPD)
layout(set = 0, binding = FSR2_BIND_CB_SPD + SET_0_CB_START, std140) uniform cbSPD_t
{
    FfxUInt32   mips;
    FfxUInt32   numWorkGroups;
    FfxUInt32x2 workGroupOffset;
    FfxUInt32x2 renderSize;
} cbSPD;

FfxUInt32 MipCount()
{
    return cbSPD.mips;
}

FfxUInt32 NumWorkGroups()
{
    return cbSPD.numWorkGroups;
}

FfxUInt32x2 WorkGroupOffset()
{
    return cbSPD.workGroupOffset;
}

FfxUInt32x2 SPD_RenderSize()
{
    return cbSPD.renderSize;
}
#endif // #if defined(FSR2_BIND_CB_SPD)

layout (set = 0, binding = 0) uniform sampler s_PointClamp;
layout (set = 0, binding = 1) uniform sampler s_LinearClamp;

// SRVs
#if defined(FSR2_BIND_SRV_INPUT_OPAQUE_ONLY)
	layout (set = 1, binding = FSR2_BIND_SRV_INPUT_OPAQUE_ONLY)                       uniform texture2D  r_input_opaque_only;
#endif
#if defined(FSR2_BIND_SRV_INPUT_COLOR)
	layout (set = 1, binding = FSR2_BIND_SRV_INPUT_COLOR)                             uniform texture2D  r_input_color_jittered;
#endif
#if defined(FSR2_BIND_SRV_INPUT_MOTION_VECTORS)
	layout (set = 1, binding = FSR2_BIND_SRV_INPUT_MOTION_VECTORS)                    uniform texture2D  r_input_motion_vectors;
#endif
#if defined(FSR2_BIND_SRV_INPUT_DEPTH)
	layout (set = 1, binding = FSR2_BIND_SRV_INPUT_DEPTH)                             uniform texture2D  r_input_depth;
#endif
#if defined(FSR2_BIND_SRV_INPUT_EXPOSURE)
	layout (set = 1, binding = FSR2_BIND_SRV_INPUT_EXPOSURE)                          uniform texture2D  r_input_exposure;
#endif
#if defined(FSR2_BIND_SRV_AUTO_EXPOSURE)
	layout(set = 1, binding = FSR2_BIND_SRV_AUTO_EXPOSURE)                            uniform texture2D  r_auto_exposure;
#endif
#if defined(FSR2_BIND_SRV_REACTIVE_MASK)
	layout (set = 1, binding = FSR2_BIND_SRV_REACTIVE_MASK)                           uniform texture2D  r_reactive_mask;
#endif
#if defined(FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK)
	layout (set = 1, binding = FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK)       uniform texture2D  r_transparency_and_composition_mask;
#endif
#if defined(FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
	layout (set = 1, binding = FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH)        uniform utexture2D r_reconstructed_previous_nearest_depth;
#endif
#if defined(FSR2_BIND_SRV_DILATED_MOTION_VECTORS)
	layout (set = 1, binding = FSR2_BIND_SRV_DILATED_MOTION_VECTORS)                  uniform texture2D  r_dilated_motion_vectors;
#endif
#if defined (FSR2_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS)
	layout(set = 1, binding = FSR2_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS)          uniform texture2D  r_previous_dilated_motion_vectors;
#endif
#if defined(FSR2_BIND_SRV_DILATED_DEPTH)
	layout (set = 1, binding = FSR2_BIND_SRV_DILATED_DEPTH)                           uniform texture2D  r_dilatedDepth;
#endif
#if defined(FSR2_BIND_SRV_INTERNAL_UPSCALED)
	layout (set = 1, binding = FSR2_BIND_SRV_INTERNAL_UPSCALED)                       uniform texture2D  r_internal_upscaled_color;
#endif
#if defined(FSR2_BIND_SRV_LOCK_STATUS)
	layout (set = 1, binding = FSR2_BIND_SRV_LOCK_STATUS)                             uniform texture2D  r_lock_status;
#endif
#if defined(FSR2_BIND_SRV_LOCK_INPUT_LUMA)
	layout (set = 1, binding = FSR2_BIND_SRV_LOCK_INPUT_LUMA)                         uniform texture2D  r_lock_input_luma;
#endif
#if defined(FSR2_BIND_SRV_NEW_LOCKS)
	layout(set = 1, binding = FSR2_BIND_SRV_NEW_LOCKS)                                uniform texture2D  r_new_locks;
#endif
#if defined(FSR2_BIND_SRV_PREPARED_INPUT_COLOR)
	layout (set = 1, binding = FSR2_BIND_SRV_PREPARED_INPUT_COLOR)                    uniform texture2D  r_prepared_input_color;
#endif
#if defined(FSR2_BIND_SRV_LUMA_HISTORY)
	layout (set = 1, binding = FSR2_BIND_SRV_LUMA_HISTORY)                            uniform texture2D  r_luma_history;
#endif
#if defined(FSR2_BIND_SRV_RCAS_INPUT)
	layout (set = 1, binding = FSR2_BIND_SRV_RCAS_INPUT)                              uniform texture2D  r_rcas_input;
#endif
#if defined(FSR2_BIND_SRV_LANCZOS_LUT)
	layout (set = 1, binding = FSR2_BIND_SRV_LANCZOS_LUT)                             uniform texture2D  r_lanczos_lut;
#endif
#if defined(FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS)
	layout (set = 1, binding = FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS)                    uniform texture2D  r_imgMips;
#endif
#if defined(FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT)
	layout (set = 1, binding = FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT)                uniform texture2D  r_upsample_maximum_bias_lut;
#endif
#if defined(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS)
	layout (set = 1, binding = FSR2_BIND_SRV_DILATED_REACTIVE_MASKS)                  uniform texture2D  r_dilated_reactive_masks;
#endif
#if defined FSR2_BIND_SRV_TEMPORAL_REACTIVE
	layout(set = 1, binding = FSR2_BIND_SRV_TEMPORAL_REACTIVE) 			              uniform texture2D  r_internal_temporal_reactive;
#endif

// UAV
#if defined FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH
	layout (set = 1, binding = FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH, r32ui) uniform uimage2D   rw_reconstructed_previous_nearest_depth;
#endif
#if defined FSR2_BIND_UAV_DILATED_MOTION_VECTORS
	layout (set = 1, binding = FSR2_BIND_UAV_DILATED_MOTION_VECTORS, rg16f)           writeonly uniform image2D  rw_dilated_motion_vectors;
#endif
#if defined FSR2_BIND_UAV_DILATED_DEPTH
	layout (set = 1, binding = FSR2_BIND_UAV_DILATED_DEPTH, r16f)                     writeonly uniform image2D  rw_dilatedDepth;
#endif
#if defined FSR2_BIND_UAV_INTERNAL_UPSCALED
	layout (set = 1, binding = FSR2_BIND_UAV_INTERNAL_UPSCALED, rgba16f)              writeonly uniform image2D  rw_internal_upscaled_color;
#endif
#if defined FSR2_BIND_UAV_LOCK_STATUS
	layout (set = 1, binding = FSR2_BIND_UAV_LOCK_STATUS, rg16f)                      uniform image2D    rw_lock_status;
#endif
#if defined(FSR2_BIND_UAV_LOCK_INPUT_LUMA)
	layout(set = 1, binding = FSR2_BIND_UAV_LOCK_INPUT_LUMA, r16f)                    writeonly uniform image2D    rw_lock_input_luma;
#endif
#if defined FSR2_BIND_UAV_NEW_LOCKS
	layout(set = 1, binding = FSR2_BIND_UAV_NEW_LOCKS, r8)				 		      uniform image2D    rw_new_locks;
#endif
#if defined FSR2_BIND_UAV_PREPARED_INPUT_COLOR
	layout (set = 1, binding = FSR2_BIND_UAV_PREPARED_INPUT_COLOR, rgba16)            writeonly uniform image2D  rw_prepared_input_color;
#endif
#if defined FSR2_BIND_UAV_LUMA_HISTORY
	layout (set = 1, binding = FSR2_BIND_UAV_LUMA_HISTORY, rgba8)                     uniform image2D  rw_luma_history;
#endif
#if defined FSR2_BIND_UAV_UPSCALED_OUTPUT
	layout (set = 1, binding = FSR2_BIND_UAV_UPSCALED_OUTPUT /* app controlled format */) writeonly uniform image2D  rw_upscaled_output;
#endif
#if defined FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE
	layout (set = 1, binding = FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE, r16f)              coherent uniform image2D  rw_img_mip_shading_change;
#endif
#if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
	layout (set = 1, binding = FSR2_BIND_UAV_EXPOSURE_MIP_5, r16f)                        coherent uniform image2D  rw_img_mip_5;
#endif
#if defined FSR2_BIND_UAV_DILATED_REACTIVE_MASKS
	layout (set = 1, binding = FSR2_BIND_UAV_DILATED_REACTIVE_MASKS, rg8)                 writeonly uniform image2D	 rw_dilated_reactive_masks;
#endif
#if defined FSR2_BIND_UAV_EXPOSURE
	layout (set = 1, binding = FSR2_BIND_UAV_EXPOSURE, FFXM_UAV_RG_QUALIFIER)                         uniform image2D    rw_exposure;
#endif
#if defined FSR2_BIND_UAV_AUTO_EXPOSURE
	layout(set = 1, binding = FSR2_BIND_UAV_AUTO_EXPOSURE, FFXM_UAV_RG_QUALIFIER)                         uniform image2D    rw_auto_exposure;
#endif
#if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
	layout (set = 1, binding = FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC, r32ui)       coherent uniform uimage2D   rw_spd_global_atomic;
#endif

#if defined FSR2_BIND_UAV_AUTOREACTIVE
	layout(set = 1, binding = FSR2_BIND_UAV_AUTOREACTIVE, r32f)                       uniform image2D   	    rw_output_autoreactive;
#endif

#if defined(FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS)
FfxFloat32 LoadMipLuma(FfxInt32x2 iPxPos, FfxInt32 mipLevel)
{
	return texelFetch(r_imgMips, iPxPos, FfxInt32(mipLevel)).r;
}
#endif

#if defined(FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS)
FfxFloat32 SampleMipLuma(FfxFloat32x2 fUV, FfxInt32 mipLevel)
{
	return textureLod(sampler2D(r_imgMips, s_LinearClamp), fUV, FfxFloat32(mipLevel)).r;
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_DEPTH)
FfxFloat32 LoadInputDepth(FfxInt32x2 iPxPos)
{
	return texelFetch(r_input_depth, iPxPos, 0).r;
}
#endif

#if defined(FSR2_BIND_SRV_REACTIVE_MASK)
FfxFloat32 LoadReactiveMask(FfxInt32x2 iPxPos)
{
	return texelFetch(r_reactive_mask, FfxInt32x2(iPxPos), 0).r;
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherReactiveRQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_reactive_mask, s_PointClamp), fUV, 0));
    col01 = FFXM_MIN16_F(rrrr.w);
    col11 = FFXM_MIN16_F(rrrr.z);
    col10 = FFXM_MIN16_F(rrrr.y);
    col00 = FFXM_MIN16_F(rrrr.x);
}
#endif

#if defined(FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK)
FfxFloat32 LoadTransparencyAndCompositionMask(FfxUInt32x2 iPxPos)
{
	return texelFetch(r_transparency_and_composition_mask, FfxInt32x2(iPxPos), 0).r;
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherTransparencyAndCompositionMaskRQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_transparency_and_composition_mask, s_PointClamp), fUV, 0));
    col01 = FFXM_MIN16_F(rrrr.w);
    col11 = FFXM_MIN16_F(rrrr.z);
    col10 = FFXM_MIN16_F(rrrr.y);
    col00 = FFXM_MIN16_F(rrrr.x);
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_COLOR)
FFXM_MIN16_F3 LoadInputColor(FfxInt32x2 iPxPos)
{
	return FFXM_MIN16_F3(texelFetch(r_input_color_jittered, iPxPos, 0).rgb);
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherInputColorRGBQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_input_color_jittered, s_PointClamp), fUV, 0));
    FFXM_MIN16_F4 gggg = FFXM_MIN16_F4(textureGather(sampler2D(r_input_color_jittered, s_PointClamp), fUV, 1));
    FFXM_MIN16_F4 bbbb = FFXM_MIN16_F4(textureGather(sampler2D(r_input_color_jittered, s_PointClamp), fUV, 2));
    col01 = FFXM_MIN16_F3(rrrr.w, gggg.w, bbbb.w);
    col11 = FFXM_MIN16_F3(rrrr.z, gggg.z, bbbb.z);
    col10 = FFXM_MIN16_F3(rrrr.y, gggg.y, bbbb.y);
    col00 = FFXM_MIN16_F3(rrrr.x, gggg.x, bbbb.x);
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_COLOR)
FFXM_MIN16_F3 SampleInputColor(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F3(textureLod(sampler2D(r_input_color_jittered, s_LinearClamp), fUV, 0.0f).rgb);
}
#endif

#if defined(FSR2_BIND_SRV_PREPARED_INPUT_COLOR)
FFXM_MIN16_F3 LoadPreparedInputColor(FfxInt32x2 iPxPos)
{
	return FFXM_MIN16_F3(texelFetch(r_prepared_input_color, iPxPos, 0).xyz);
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherPreparedInputColorRGBQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F3 col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_prepared_input_color, s_PointClamp), fUV, 0));
    FFXM_MIN16_F4 gggg = FFXM_MIN16_F4(textureGather(sampler2D(r_prepared_input_color, s_PointClamp), fUV, 1));
    FFXM_MIN16_F4 bbbb = FFXM_MIN16_F4(textureGather(sampler2D(r_prepared_input_color, s_PointClamp), fUV, 2));
    col01 = FFXM_MIN16_F3(rrrr.w, gggg.w, bbbb.w);
    col11 = FFXM_MIN16_F3(rrrr.z, gggg.z, bbbb.z);
    col10 = FFXM_MIN16_F3(rrrr.y, gggg.y, bbbb.y);
    col00 = FFXM_MIN16_F3(rrrr.x, gggg.x, bbbb.x);
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_MOTION_VECTORS)
FFXM_MIN16_F2 LoadInputMotionVector(FfxInt32x2 iPxDilatedMotionVectorPos)
{
	FFXM_MIN16_F2 fSrcMotionVector = FFXM_MIN16_F2(texelFetch(r_input_motion_vectors, iPxDilatedMotionVectorPos, 0).xy);

	FFXM_MIN16_F2 fUvMotionVector = fSrcMotionVector * FFXM_MIN16_F2(MotionVectorScale());

#if FFXM_FSR2_OPTION_JITTERED_MOTION_VECTORS
	fUvMotionVector -= FFXM_MIN16_F2(MotionVectorJitterCancellation());
#endif

	return fUvMotionVector;
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherInputMotionVectorRGQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F2 col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F2 col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F2 col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F2 col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_input_motion_vectors, s_PointClamp), fUV, 0));
    FFXM_MIN16_F4 gggg = FFXM_MIN16_F4(textureGather(sampler2D(r_input_motion_vectors, s_PointClamp), fUV, 1));
    col01 = FFXM_MIN16_F2(rrrr.w, gggg.w) * FFXM_MIN16_F2(MotionVectorScale());
    col11 = FFXM_MIN16_F2(rrrr.z, gggg.z) * FFXM_MIN16_F2(MotionVectorScale());
    col10 = FFXM_MIN16_F2(rrrr.y, gggg.y) * FFXM_MIN16_F2(MotionVectorScale());
    col00 = FFXM_MIN16_F2(rrrr.x, gggg.x) * FFXM_MIN16_F2(MotionVectorScale());
#if FFXM_FSR2_OPTION_JITTERED_MOTION_VECTORS
    col01 -= FFXM_MIN16_F2(MotionVectorJitterCancellation());
    col11 -= FFXM_MIN16_F2(MotionVectorJitterCancellation());
    col10 -= FFXM_MIN16_F2(MotionVectorJitterCancellation());
    col00 -= FFXM_MIN16_F2(MotionVectorJitterCancellation());
#endif
}
#endif

#if defined(FSR2_BIND_SRV_INTERNAL_UPSCALED)
FFXM_MIN16_F4 LoadHistory(FfxInt32x2 iPxHistory)
{
	return FFXM_MIN16_F4(texelFetch(r_internal_upscaled_color, iPxHistory, 0));
}

FFXM_MIN16_F4 SampleUpscaledHistory(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F4(textureLod(sampler2D(r_internal_upscaled_color, s_LinearClamp), fUV, 0));
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherHistoryColorRGBQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F4 col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F4 col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F4 col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F4 col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_internal_upscaled_color, s_PointClamp), fUV, 0));
    FFXM_MIN16_F4 gggg = FFXM_MIN16_F4(textureGather(sampler2D(r_internal_upscaled_color, s_PointClamp), fUV, 1));
    FFXM_MIN16_F4 bbbb = FFXM_MIN16_F4(textureGather(sampler2D(r_internal_upscaled_color, s_PointClamp), fUV, 2));
    col01 = FFXM_MIN16_F4(rrrr.w, gggg.w, bbbb.w, 0.0f);
    col11 = FFXM_MIN16_F4(rrrr.z, gggg.z, bbbb.z, 0.0f);
    col10 = FFXM_MIN16_F4(rrrr.y, gggg.y, bbbb.y, 0.0f);
    col00 = FFXM_MIN16_F4(rrrr.x, gggg.x, bbbb.x, 0.0f);
}
#endif

#if defined(FSR2_BIND_UAV_LUMA_HISTORY)
void StoreLumaHistory(FfxInt32x2 iPxPos, FfxFloat32x4 fLumaHistory)
{
	imageStore(rw_luma_history, FfxInt32x2(iPxPos), fLumaHistory);
}
#endif

#if defined(FSR2_BIND_SRV_LUMA_HISTORY)
FFXM_MIN16_F4 SampleLumaHistory(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F4(textureLod(sampler2D(r_luma_history, s_LinearClamp), fUV, 0.0f));
}
#endif

FFXM_MIN16_F4 LoadRCAS_Input(FfxInt32x2 iPxPos)
{
#if defined(FSR2_BIND_SRV_RCAS_INPUT)
    return FFXM_MIN16_F4(texelFetch(r_rcas_input, iPxPos, 0));
#else
    return FFXM_MIN16_F4(0.0);
#endif
}

#if defined(FSR2_BIND_UAV_INTERNAL_UPSCALED)
void StoreReprojectedHistory(FfxInt32x2 iPxHistory, FfxFloat32x4 fHistory)
{
	imageStore(rw_internal_upscaled_color, iPxHistory, fHistory);
}
#endif

#if defined(FSR2_BIND_UAV_INTERNAL_UPSCALED)
void StoreInternalColorAndWeight(FfxInt32x2 iPxPos, FfxFloat32x4 fColorAndWeight)
{
	imageStore(rw_internal_upscaled_color, FfxInt32x2(iPxPos), fColorAndWeight);
}
#endif

#if defined(FSR2_BIND_UAV_UPSCALED_OUTPUT)
void StoreUpscaledOutput(FfxInt32x2 iPxPos, FfxFloat32x3 fColor)
{
    imageStore(rw_upscaled_output, FfxInt32x2(iPxPos), FfxFloat32x4(fColor, 1.f));
}
#endif

#if defined(FSR2_BIND_SRV_LOCK_STATUS)
FfxFloat32x2 LoadLockStatus(FfxInt32x2 iPxPos)
{
	FfxFloat32x2 fLockStatus = texelFetch(r_lock_status, iPxPos, 0).rg;

    return fLockStatus;
}
#endif

#if defined(FSR2_BIND_UAV_LOCK_STATUS)
void StoreLockStatus(FfxInt32x2 iPxPos, FfxFloat32x2 fLockstatus)
{
	imageStore(rw_lock_status, iPxPos, vec4(fLockstatus, 0.0f, 0.0f));
}
#endif

#if defined(FSR2_BIND_SRV_LOCK_INPUT_LUMA)
FFXM_MIN16_F LoadLockInputLuma(FfxInt32x2 iPxPos)
{
	return FFXM_MIN16_F(texelFetch(r_lock_input_luma, iPxPos, 0).r);
}

/*
   col00 (-1,1) *------* col10 (0,-1)
                |      |
                |      |
   col01 (-1,0) *------* col11 (0,0)
*/
void GatherLockInputLumaRQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col00,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col10,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col01,
    FFXM_PARAMETER_INOUT FFXM_MIN16_F col11)
{
    FFXM_MIN16_F4 rrrr = FFXM_MIN16_F4(textureGather(sampler2D(r_lock_input_luma, s_PointClamp), fUV, 0));
    col01 = FFXM_MIN16_F(rrrr.w);
    col11 = FFXM_MIN16_F(rrrr.z);
    col10 = FFXM_MIN16_F(rrrr.y);
    col00 = FFXM_MIN16_F(rrrr.x);
}
#endif

#if defined(FSR2_BIND_UAV_LOCK_INPUT_LUMA)
void StoreLockInputLuma(FfxInt32x2 iPxPos, FfxFloat32 fLuma)
{
	imageStore(rw_lock_input_luma, iPxPos, vec4(fLuma, 0, 0, 0));
}
#endif

#if defined(FSR2_BIND_SRV_NEW_LOCKS)
FfxFloat32 LoadNewLocks(FfxInt32x2 iPxPos)
{
	return texelFetch(r_new_locks, iPxPos, 0).r;
}
#endif

#if defined(FSR2_BIND_UAV_NEW_LOCKS)
FFXM_MIN16_F LoadRwNewLocks(FfxInt32x2 iPxPos)
{
	return FFXM_MIN16_F(imageLoad(rw_new_locks, iPxPos).r);
}
#endif

#if defined(FSR2_BIND_UAV_NEW_LOCKS)
void StoreNewLocks(FfxInt32x2 iPxPos, FfxFloat32 newLock)
{
	imageStore(rw_new_locks, iPxPos, vec4(newLock, 0, 0, 0));
}
#endif

#if defined(FSR2_BIND_UAV_PREPARED_INPUT_COLOR)
void StorePreparedInputColor(FFXM_PARAMETER_IN FfxInt32x2 iPxPos, FFXM_PARAMETER_IN FfxFloat32x4 fTonemapped)
{
	imageStore(rw_prepared_input_color, iPxPos, fTonemapped);
}
#endif

#if defined(FSR2_BIND_SRV_PREPARED_INPUT_COLOR)
FfxFloat32 SampleDepthClip(FfxFloat32x2 fUV)
{
	return textureLod(sampler2D(r_prepared_input_color, s_LinearClamp), fUV, 0.0f).w;
}
#endif

#if defined(FSR2_BIND_SRV_LOCK_STATUS)
FFXM_MIN16_F2 SampleLockStatus(FfxFloat32x2 fUV)
{
	FFXM_MIN16_F2 fLockStatus = FFXM_MIN16_F2(textureLod(sampler2D(r_lock_status, s_LinearClamp), fUV, 0.0f).rg);
	return fLockStatus;
}
#endif

#if defined(FSR2_BIND_SRV_DEPTH)
FfxFloat32 LoadSceneDepth(FfxInt32x2 iPxInput)
{
	return texelFetch(r_input_depth, iPxInput, 0).r;
}
#endif

#if defined(FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
FfxFloat32 LoadReconstructedPrevDepth(FfxInt32x2 iPxPos)
{
	return uintBitsToFloat(texelFetch(r_reconstructed_previous_nearest_depth, iPxPos, 0).r);
}

/*
   d00 (-1,1) *------* d10 (0,-1)
              |      |
              |      |
   d01 (-1,0) *------* d11 (0,0)
*/
void GatherReconstructedPreviousDepthRQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FfxFloat32 d00,
    FFXM_PARAMETER_INOUT FfxFloat32 d10,
    FFXM_PARAMETER_INOUT FfxFloat32 d01,
    FFXM_PARAMETER_INOUT FfxFloat32 d11)
{
    FfxUInt32x4 rrrr = textureGather(usampler2D(r_reconstructed_previous_nearest_depth, s_PointClamp), fUV, 0);
    d01 = FfxFloat32(uintBitsToFloat(rrrr.w));
    d11 = FfxFloat32(uintBitsToFloat(rrrr.z));
    d10 = FfxFloat32(uintBitsToFloat(rrrr.y));
    d00 = FfxFloat32(uintBitsToFloat(rrrr.x));
}
#endif

#if defined(FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
void StoreReconstructedDepth(FfxInt32x2 iPxSample, FfxFloat32 fDepth)
{
	FfxUInt32 uDepth = floatBitsToUint(fDepth);

	#if FFXM_FSR2_OPTION_INVERTED_DEPTH
		imageAtomicMax(rw_reconstructed_previous_nearest_depth, iPxSample, uDepth);
	#else
		imageAtomicMin(rw_reconstructed_previous_nearest_depth, iPxSample, uDepth); // min for standard, max for inverted depth
	#endif
}
#endif

#if defined(FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
void SetReconstructedDepth(FfxInt32x2 iPxSample, FfxUInt32 uValue)
{
	imageStore(rw_reconstructed_previous_nearest_depth, iPxSample, uvec4(uValue, 0, 0, 0));
}
#endif

#if defined(FSR2_BIND_UAV_DILATED_DEPTH)
void StoreDilatedDepth(FFXM_PARAMETER_IN FfxInt32x2 iPxPos, FFXM_PARAMETER_IN FfxFloat32 fDepth)
{
	//FfxUInt32 uDepth = f32tof16(fDepth);
	imageStore(rw_dilatedDepth, iPxPos, vec4(fDepth, 0.0f, 0.0f, 0.0f));
}
#endif

#if defined(FSR2_BIND_UAV_DILATED_MOTION_VECTORS)
void StoreDilatedMotionVector(FFXM_PARAMETER_IN FfxInt32x2 iPxPos, FFXM_PARAMETER_IN FfxFloat32x2 fMotionVector)
{
	imageStore(rw_dilated_motion_vectors, iPxPos, vec4(fMotionVector, 0.0f, 0.0f));
}
#endif

#if defined(FSR2_BIND_SRV_DILATED_MOTION_VECTORS)
FFXM_MIN16_F2 LoadDilatedMotionVector(FfxInt32x2 iPxInput)
{
	return FFXM_MIN16_F2(texelFetch(r_dilated_motion_vectors, iPxInput, 0).rg);
}
#endif

#if defined(FSR2_BIND_SRV_DILATED_MOTION_VECTORS)
FFXM_MIN16_F2 SampleDilatedMotionVector(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F2(textureLod(sampler2D(r_dilated_motion_vectors, s_LinearClamp), fUV, 0.0f).rg);
}
#endif

#if defined(FSR2_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS)
FFXM_MIN16_F2 LoadPreviousDilatedMotionVector(FfxInt32x2 iPxInput)
{
	return FFXM_MIN16_F2(texelFetch(r_previous_dilated_motion_vectors, iPxInput, 0).rg);
}

FFXM_MIN16_F2 SamplePreviousDilatedMotionVector(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F2(textureLod(sampler2D(r_previous_dilated_motion_vectors, s_LinearClamp), fUV, 0.0f).xy);
}
#endif

#if defined(FSR2_BIND_SRV_DILATED_DEPTH)
FfxFloat32 LoadDilatedDepth(FfxInt32x2 iPxInput)
{
	return texelFetch(r_dilatedDepth, iPxInput, 0).r;
}

/*
   dd00 (-1,1)  *------* dd10 (0,-1)
                |      |
                |      |
   dd01 (-1,0)  *------* dd11 (0,0)
*/
void GatherDilatedDepthRQuad(FfxFloat32x2 fUV,
    FFXM_PARAMETER_INOUT FfxFloat32 dd00,
    FFXM_PARAMETER_INOUT FfxFloat32 dd10,
    FFXM_PARAMETER_INOUT FfxFloat32 dd01,
    FFXM_PARAMETER_INOUT FfxFloat32 dd11)
{
    FfxFloat32x4 rrrr = textureGather(sampler2D(r_dilatedDepth, s_PointClamp), fUV, 0);
    dd01 = FfxFloat32(rrrr.w);
    dd11 = FfxFloat32(rrrr.z);
    dd10 = FfxFloat32(rrrr.y);
    dd00 = FfxFloat32(rrrr.x);
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_EXPOSURE)
FfxFloat32 Exposure()
{
	FfxFloat32 exposure = texelFetch(r_input_exposure, FfxInt32x2(0, 0), 0).x;

	if (exposure == 0.0f) {
		exposure = 1.0f;
	}

	return exposure;
}
#endif

#if defined(FSR2_BIND_SRV_AUTO_EXPOSURE)
FfxFloat32 AutoExposure()
{
	FfxFloat32 exposure = texelFetch(r_auto_exposure, FfxInt32x2(0, 0), 0).x;

	if (exposure == 0.0f) {
		exposure = 1.0f;
	}

	return exposure;
}
#endif

FfxFloat32 SampleLanczos2Weight(FfxFloat32 x)
{
#if defined(FSR2_BIND_SRV_LANCZOS_LUT)
	return textureLod(sampler2D(r_lanczos_lut, s_LinearClamp), FfxFloat32x2(x / 2.0f, 0.5f), 0.0f).x;
#else
    return 0.f;
#endif
}

#if defined(FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT)
FfxFloat32 SampleUpsampleMaximumBias(FfxFloat32x2 uv)
{
    // Stored as a SNORM, so make sure to multiply by 2 to retrieve the actual expected range.
    return FfxFloat32(2.0f) * FfxFloat32(textureLod(sampler2D(r_upsample_maximum_bias_lut, s_LinearClamp), abs(uv) * 2.0f, 0.0f).r);
}
#endif

#if defined(FSR2_BIND_SRV_TEMPORAL_REACTIVE)
FfxFloat32 SampleTemporalReactive(FfxFloat32x2 fUV)
{
#if FFXM_SHADER_QUALITY_OPT_SEPARATE_TEMPORAL_REACTIVE
    return textureLod(sampler2D(r_internal_temporal_reactive, s_LinearClamp), fUV, 0).x;
#else
    return 0.0f;
#endif
}
#endif

#if defined(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS)
FFXM_MIN16_F2 SampleDilatedReactiveMasks(FfxFloat32x2 fUV)
{
	return FFXM_MIN16_F2(textureLod(sampler2D(r_dilated_reactive_masks, s_LinearClamp), fUV, 0.0f).rg);
}
#endif

#if defined(FSR2_BIND_SRV_DILATED_REACTIVE_MASKS)
FFXM_MIN16_F2 LoadDilatedReactiveMasks(FFXM_PARAMETER_IN FfxInt32x2 iPxPos)
{
    return FFXM_MIN16_F2(texelFetch(r_dilated_reactive_masks, iPxPos, 0).rg);
}
#endif

#if defined(FSR2_BIND_UAV_DILATED_REACTIVE_MASKS)
void StoreDilatedReactiveMasks(FFXM_PARAMETER_IN FfxInt32x2 iPxPos, FFXM_PARAMETER_IN FfxFloat32x2 fDilatedReactiveMasks)
{
    imageStore(rw_dilated_reactive_masks, iPxPos, vec4(fDilatedReactiveMasks, 0.0f, 0.0f));
}
#endif

#if defined(FSR2_BIND_SRV_INPUT_OPAQUE_ONLY)
FfxFloat32x3 LoadOpaqueOnly(FFXM_PARAMETER_IN FFXM_MIN16_I2 iPxPos)
{
	return texelFetch(r_input_opaque_only, iPxPos, 0).xyz;
}
#endif

FfxFloat32x2 SPD_LoadExposureBuffer()
{
#if defined FSR2_BIND_UAV_AUTO_EXPOSURE
    return imageLoad(rw_auto_exposure, ivec2(0, 0)).xy;
#else
    return FfxFloat32x2(0.f, 0.f);
#endif // #if defined FSR2_BIND_UAV_AUTO_EXPOSURE
}

void SPD_SetExposureBuffer(FfxFloat32x2 value)
{
#if defined FSR2_BIND_UAV_AUTO_EXPOSURE
    imageStore(rw_auto_exposure, ivec2(0, 0), vec4(value, 0.0f, 0.0f));
#endif // #if defined FSR2_BIND_UAV_AUTO_EXPOSURE
}

FfxFloat32x4 SPD_LoadMipmap5(FfxInt32x2 iPxPos)
{
#if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
    return FfxFloat32x4(imageLoad(rw_img_mip_5, iPxPos).x, 0.0f, 0.0f, 0.0f);
#else
    return FfxFloat32x4(0.f, 0.f, 0.f, 0.f);
#endif // #if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
}

void SPD_SetMipmap(FfxInt32x2 iPxPos, FfxUInt32 slice, FfxFloat32 value)
{
    switch (slice)
    {
    case FFXM_FSR2_SHADING_CHANGE_MIP_LEVEL:
#if defined FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE
        imageStore(rw_img_mip_shading_change, iPxPos, FfxFloat32x4(value, 0.0f, 0.0f, 0.0f));
#endif // #if defined FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE
        break;
    case 5:
#if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
        imageStore(rw_img_mip_5, iPxPos, FfxFloat32x4(value, 0.0f, 0.0f, 0.0f));
#endif // #if defined FSR2_BIND_UAV_EXPOSURE_MIP_5
        break;
    default:

        // avoid flattened side effect
#if defined(FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE)
        imageStore(rw_img_mip_shading_change, iPxPos, FfxFloat32x4(imageLoad(rw_img_mip_shading_change, iPxPos).x, 0.0f, 0.0f, 0.0f));
#elif defined(FSR2_BIND_UAV_EXPOSURE_MIP_5)
        imageStore(rw_img_mip_5, iPxPos, FfxFloat32x4(imageLoad(rw_img_mip_5, iPxPos).x, 0.0f, 0.0f, 0.0f));
#endif // #if defined FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE
        break;
    }
}

void SPD_IncreaseAtomicCounter(inout FfxUInt32 spdCounter)
{
#if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
    spdCounter = imageAtomicAdd(rw_spd_global_atomic, ivec2(0, 0), 1);
#endif // #if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
}

void SPD_ResetAtomicCounter()
{
#if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
    imageStore(rw_spd_global_atomic, ivec2(0, 0), uvec4(0));
#endif // #if defined FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC
}

#endif // #if defined(FFXM_GPU)
