// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// @defgroup FRAMEINTERPOLATION

#pragma once

// Include the interface for the backend of the Frameinterpolation API.
#include "../../../api/internal/ffx_interface.h"

/// FidelityFX Frameinterpolation major version.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
#define FFX_FRAMEINTERPOLATION_VERSION_MAJOR      (3)

/// FidelityFX Frameinterpolation minor version.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
#define FFX_FRAMEINTERPOLATION_VERSION_MINOR      (1)

/// FidelityFX Frameinterpolation patch version.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
#define FFX_FRAMEINTERPOLATION_VERSION_PATCH      (5)

/// FidelityFX Frame Interpolation context count
///
/// Defines the number of internal effect contexts required by Frame Interpolation
///
/// @ingroup ffxFrameInterpolation
#define FFX_FRAMEINTERPOLATION_CONTEXT_COUNT      (1)

/// The size of the context specified in 32bit values.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
#define FFX_FRAMEINTERPOLATION_CONTEXT_SIZE (FFX_SDK_DEFAULT_CONTEXT_SIZE)

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

/// An enumeration of all the passes which constitute the FSR3 algorithm.
///
/// FSR3 is implemented as a composite of several compute passes each
/// computing a key part of the final result. Each call to the
/// <c><i>FfxFsr3ScheduleGpuJobFunc</i></c> callback function will
/// correspond to a single pass included in <c><i>FfxFsr3Pass</i></c>. For a
/// more comprehensive description of each pass, please refer to the FSR3
/// reference documentation.
///
/// Please note in some cases e.g.: <c><i>FFX_FSR3_PASS_ACCUMULATE</i></c>
/// and <c><i>FFX_FSR3_PASS_ACCUMULATE_SHARPEN</i></c> either one pass or the
/// other will be used (they are mutually exclusive). The choice of which will
/// depend on the way the <c><i>FfxFsr3Context</i></c> is created and the
/// precise contents of <c><i>FfxFsr3DispatchParamters</i></c> each time a call
/// is made to <c><i>ffxFsr3ContextDispatch</i></c>.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
typedef enum FfxFrameInterpolationPass
{
    FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_AND_DILATE,
    FFX_FRAMEINTERPOLATION_PASS_SETUP,
    FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_PREV_DEPTH,
    FFX_FRAMEINTERPOLATION_PASS_GAME_MOTION_VECTOR_FIELD,
    FFX_FRAMEINTERPOLATION_PASS_OPTICAL_FLOW_VECTOR_FIELD,
    FFX_FRAMEINTERPOLATION_PASS_DISOCCLUSION_MASK,
    FFX_FRAMEINTERPOLATION_PASS_INTERPOLATION,
    FFX_FRAMEINTERPOLATION_PASS_INPAINTING_PYRAMID,
    FFX_FRAMEINTERPOLATION_PASS_INPAINTING,
    FFX_FRAMEINTERPOLATION_PASS_GAME_VECTOR_FIELD_INPAINTING_PYRAMID,
    FFX_FRAMEINTERPOLATION_PASS_DEBUG_VIEW,
    FFX_FRAMEINTERPOLATION_PASS_COUNT  ///< The number of passes performed by FrameInterpolation.
} FfxFrameInterpolationPass;

// forward declarations
struct FfxFrameInterpolationContext;

/// An enumeration of bit flags used when creating a
/// <c><i>FfxFrameInterpolationContext</i></c>. See <c><i>FfxFrameInterpolationContextDescription</i></c>.
///
/// @ingroup FRAMEINTERPOLATIONFRAMEINTERPOLATION
typedef enum FfxFrameInterpolationInitializationFlagBits {

    FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED                    = (1<<0), ///< A bit indicating that the input depth buffer data provided is inverted [1..0].
    FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE                    = (1<<1), ///< A bit indicating that the input depth buffer data provided is using an infinite far plane.
    FFX_FRAMEINTERPOLATION_ENABLE_TEXTURE1D_USAGE                   = (1<<2), ///< A bit indicating that the backend should use 1D textures.
    FFX_FRAMEINTERPOLATION_ENABLE_HDR_COLOR_INPUT                   = (1<<3), ///< A bit indicating that HDR values are present in the imaging pipeline.
    FFX_FRAMEINTERPOLATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS = (1<<4), ///< A bit indicating if the motion vectors are rendered at display resolution.
    FFX_FRAMEINTERPOLATION_ENABLE_JITTER_MOTION_VECTORS             = (1<<5),
    FFX_FRAMEINTERPOLATION_ENABLE_ASYNC_SUPPORT                     = (1<<6),
    FFX_FRAMEINTERPOLATION_ENABLE_DEBUG_CHECKING                    = (1<<7), ///< A bit indicating that the runtime should check some API values and report issues.
} FfxFrameInterpolationInitializationFlagBits;

/// A structure encapsulating the parameters required to initialize
/// FidelityFX Frameinterpolation.
///
/// @ingroup FRAMEINTERPOLATION
typedef struct FfxFrameInterpolationContextDescription {
    uint32_t                        flags;                             ///< A collection of <c><i>FfxFrameInterpolationInitializationFlagBits</i></c>.
    FfxApiDimensions2D              maxRenderSize;                     ///< The maximum size that rendering will be performed at.
    FfxApiDimensions2D              displaySize;                       ///< The size of the presentation resolution
    FfxApiSurfaceFormat             backBufferFormat;                  ///< the format of the backbuffer
    FfxApiSurfaceFormat             previousInterpolationSourceFormat; ///< the format of the texture that will store the interpolation source for the next frame. Can be different than the backbuffer one, especially when using hudless
    FfxInterface                    backendInterface;                  ///< A set of pointers to the backend implementation for FidelityFX SDK
} FfxFrameInterpolationContextDescription;

/// A structure encapsulating the resource descriptions for internal resources for this effect.
///
/// @ingroup FRAMEINTERPOLATION
typedef struct FfxFrameInterpolationInternalResourceDescriptions
{
    FfxCreateResourceDescription    reconstructedDepthInterpolatedFrame;   ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>reconstructedDepthInterpolatedFrame</i></c> internal resource.
    FfxCreateResourceDescription    gameMotionVectorFieldX;                ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>gameMotionVectorFieldX</i></c> internal resource.
    FfxCreateResourceDescription    gameMotionVectorFieldY;                ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>gameMotionVectorFieldY</i></c> internal resource.
    FfxCreateResourceDescription    inpaintingPyramid;                     ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>inpaintingPyramid</i></c> internal resource.
    FfxCreateResourceDescription    counters;                              ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>counters</i></c> internal resource.
    FfxCreateResourceDescription    opticalFlowMotionVectorFieldX;         ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>opticalFlowMotionVectorFieldX</i></c> internal resource.
    FfxCreateResourceDescription    opticalFlowMotionVectorFieldY;         ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>opticalFlowMotionVectorFieldY</i></c> internal resource.
    FfxCreateResourceDescription    previousInterpolationSource;           ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>previousInterpolationSource</i></c> internal resource.
    FfxCreateResourceDescription    inpaintingMask;                        ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>inpaintingMask</i></c> internal resource.
    FfxCreateResourceDescription    disocclusionMask;                      ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>disocclusionMask</i></c> internal resource.
    FfxCreateResourceDescription    defaultDistortionField;                ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>defaultDistortionField</i></c> internal resource.

} FfxFrameInterpolationInternalResourceDescriptions;

/// A structure encapsulating the resource descriptions for shared resources for this effect.
///
/// @ingroup FRAMEINTERPOLATION
typedef struct FfxFrameInterpolationSharedResourceDescriptions
{
    FfxCreateResourceDescription    reconstructedPrevNearestDepth;  ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>reconstructedPrevNearestDepth</i></c> shared resource.
    FfxCreateResourceDescription    dilatedDepth;  ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>dilatedDepth</i></c> shared resource.
    FfxCreateResourceDescription    dilatedMotionVectors;  ///< The <c><i>FfxCreateResourceDescription</i></c> for allocating the <c><i>dilatedMotionVectors</i></c> shared resource.
} FfxFrameInterpolationSharedResourceDescriptions;

/// A structure encapsulating the FidelityFX Super Resolution 2 context.
///
/// This sets up an object which contains all persistent internal data and
/// resources that are required by FSR3.
///
/// The <c><i>FfxFsr3Context</i></c> object should have a lifetime matching
/// your use of FSR3. Before destroying the FSR3 context care should be taken
/// to ensure the GPU is not accessing the resources created or used by FSR3.
/// It is therefore recommended that the GPU is idle before destroying the
/// FSR3 context.
///
/// @ingroup FRAMEINTERPOLATION
typedef struct FfxFrameInterpolationContext
{
    uint32_t data[FFX_FRAMEINTERPOLATION_CONTEXT_SIZE];  ///< An opaque set of <c>uint32_t</c> which contain the data for the context.
} FfxFrameInterpolationContext;


/// Create a FidelityFX Super Resolution 2 context from the parameters
/// programmed to the <c><i>FfxFsr3CreateParams</i></c> structure.
///
/// The context structure is the main object used to interact with the FSR3
/// API, and is responsible for the management of the internal resources used
/// by the FSR3 algorithm. When this API is called, multiple calls will be
/// made via the pointers contained in the <c><i>callbacks</i></c> structure.
/// These callbacks will attempt to retreive the device capabilities, and
/// create the internal resources, and pipelines required by FSR3's
/// frame-to-frame function. Depending on the precise configuration used when
/// creating the <c><i>FfxFsr3Context</i></c> a different set of resources and
/// pipelines might be requested via the callback functions.
///
/// The flags included in the <c><i>flags</i></c> field of
/// <c><i>FfxFsr3Context</i></c> how match the configuration of your
/// application as well as the intended use of FSR3. It is important that these
/// flags are set correctly (as well as a correct programmed
/// <c><i>FfxFsr3DispatchDescription</i></c>) to ensure correct operation. It is
/// recommended to consult the overview documentation for further details on
/// how FSR3 should be integerated into an application.
///
/// When the <c><i>FfxFsr3Context</i></c> is created, you should use the
/// <c><i>ffxFsr3ContextDispatch</i></c> function each frame where FSR3
/// upscaling should be applied. See the documentation of
/// <c><i>ffxFsr3ContextDispatch</i></c> for more details.
///
/// The <c><i>FfxFsr3Context</i></c> should be destroyed when use of it is
/// completed, typically when an application is unloaded or FSR3 upscaling is
/// disabled by a user. To destroy the FSR3 context you should call
/// <c><i>ffxFsr3ContextDestroy</i></c>.
///
/// @param [out] context                A pointer to a <c><i>FfxFsr3Context</i></c> structure to populate.
/// @param [in]  contextDescription     A pointer to a <c><i>FfxFsr3ContextDescription</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>contextDescription</i></c> was <c><i>NULL</i></c>.
/// @retval
/// FFX_ERROR_INCOMPLETE_INTERFACE      The operation failed because the <c><i>FfxFsr3ContextDescription.callbacks</i></c>  was not fully specified.
/// @retval
/// FFX_ERROR_BACKEND_API_ERROR         The operation failed because of an error returned from the backend.
///
/// @ingroup FRAMEINTERPOLATION
FFX_API FfxErrorCode ffxFrameInterpolationContextCreate(FfxFrameInterpolationContext* context, FfxFrameInterpolationContextDescription* contextDescription);

/// Get GPU memory usage of the FidelityFX Frame Interpolation context.
///
/// @param [in]  pContext                A pointer to a <c><i>FfxInterface</i></c> structure.
/// @param [out] pVramUsage              A pointer to a <c><i>FfxApiEffectMemoryUsage</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>vramUsage</i></c> were <c><i>NULL</i></c>.
///
/// @ingroup ffxFrameInterpolation
FFX_API FfxErrorCode ffxFrameInterpolationContextGetGpuMemoryUsage(FfxFrameInterpolationContext* pContext, FfxApiEffectMemoryUsage* vramUsage);

/// Provides the descriptions for shared resources that must be allocated for this effect.
///
/// @param [in] context                    A pointer to a <c><i>FfxFrameInterpolationContext</i></c> structure.
/// @param [out] sharedResources           A pointer to a <c><i>FfxFrameInterpolationSharedResourceDescriptions</i></c> to populate.
///
/// @returns
/// FFX_OK                                The operation completed successfully.
/// @returns
/// Anything else                         The operation failed.
///
/// @ingroup ffxFrameInterpolation
FFX_API FfxErrorCode ffxFrameInterpolationGetSharedResourceDescriptions(FfxFrameInterpolationContext* pContext, FfxFrameInterpolationSharedResourceDescriptions* sharedResources);

/// Get GPU memory usage of the FidelityFX Frame Interpolation's Shared context.
///
/// @param [in]  pContext                A pointer to a <c><i>FfxInterface</i></c> structure.
/// @param [out] pVramUsage              A pointer to a <c><i>FfxApiEffectMemoryUsage</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>vramUsage</i></c> were <c><i>NULL</i></c>.
///
/// @ingroup ffxFrameInterpolation
FFX_API FfxErrorCode ffxSharedContextGetGpuMemoryUsage(FfxInterface* backendInterfaceShared, FfxApiEffectMemoryUsage* vramUsage);

/// Get GPU memory usage of the FidelityFX Frame Interpolation.
///
/// @param [in]  device                  A <c><i>FfxDevice</i></c>.
/// @param [in]  maxRenderSize           A pointer to a <c><i>FfxApiDimensions2D</i></c> structure.
/// @param [in]  displaySize             A pointer to a <c><i>FfxApiDimensions2D</i></c> structure.
/// @param [in]  previousInterpolationSourceFormat  A pointer to a <c><i>FfxApiSurfaceFormat</i></c> structure.
/// @param [out] pVramUsage              A pointer to a <c><i>FfxApiEffectMemoryUsage</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>vramUsage</i></c> were <c><i>NULL</i></c>.
///
/// @ingroup ffxFrameInterpolation
FFX_API FfxErrorCode ffxFrameInterpolationGetGpuMemoryUsage(FfxDevice device, FfxApiDimensions2D* maxRenderSize, FfxApiDimensions2D* displaySize, FfxApiSurfaceFormat previousInterpolationSourceFormat, FfxApiEffectMemoryUsage* pVramUsage);

typedef struct FfxFrameInterpolationPrepareDescription
{
    uint32_t            flags;                      ///< combination of FfxFrameInterpolationDispatchFlags
    FfxCommandList      commandList;                ///< The <c><i>FfxCommandList</i></c> to record frame interpolation commands into.
    FfxApiDimensions2D     renderSize;                 ///< The dimensions used to render game content, dilatedDepth, dilatedMotionVectors are expected to be of ths size.
    FfxApiFloatCoords2D    jitterOffset;               ///< The subpixel jitter offset applied to the camera.     jitter;
    FfxApiFloatCoords2D    motionVectorScale;          ///< The scale factor to apply to motion vectors.     motionVectorScale;

    float               frameTimeDelta;
    float               cameraNear;
    float               cameraFar;
    float               viewSpaceToMetersFactor;
    float               cameraFovAngleVertical;

    FfxApiResource         depth;                      ///< The depth buffer data
    FfxApiResource         motionVectors;              ///< The motion vector data
    uint64_t            frameID;

    FfxApiResource         dilatedDepth;                       ///< The dilated depth buffer data
    FfxApiResource         dilatedMotionVectors;               ///< The dilated motion vector data
    FfxApiResource         reconstructedPrevDepth;             ///< The reconstructed depth buffer data

    FfxFloat32x3        cameraPosition;             ///< The camera position in world space
    FfxFloat32x3        cameraUp;                   ///< The camera up normalized vector in world space.
    FfxFloat32x3        cameraRight;                ///< The camera right normalized vector in world space.
    FfxFloat32x3        cameraForward;              ///< The camera forward normalized vector in world space.
} FfxFrameInterpolationPrepareDescription;

FFX_API FfxErrorCode ffxFrameInterpolationPrepare(FfxFrameInterpolationContext* context, const FfxFrameInterpolationPrepareDescription* params);

typedef enum FfxFrameInterpolationDispatchFlags
{
    FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_TEAR_LINES       = (1 << 0),  ///< A bit indicating that the debug tear lines will be drawn to the interpolated output.
    FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_RESET_INDICATORS = (1 << 1),  ///< A bit indicating that the debug reset indicators will be drawn to the generated output.
    FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_VIEW             = (1 << 2),  ///< A bit indicating that the interpolated output resource will contain debug views with relevant information.
    FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_PACING_LINES     = (1 << 3),  ///< A bit indicating that the debug pacing lines will be drawn to the generated output.
    FFX_FRAMEINTERPOLATION_DISPATCH_RESERVED_1 = (1 << 4),
    FFX_FRAMEINTERPOLATION_DISPATCH_RESERVED_2 = (1 << 5), 
} FfxFrameInterpolationDispatchFlags;

typedef struct FfxFrameInterpolationDispatchDescription {

    uint32_t                            flags;                              ///< combination of FfxFrameInterpolationDispatchFlags
    FfxCommandList                      commandList;                        ///< The <c><i>FfxCommandList</i></c> to record frame interpolation commands into.
    FfxApiDimensions2D                     displaySize;                        ///< The destination output dimensions
    FfxApiDimensions2D                     renderSize;                         ///< The dimensions used to render game content, dilatedDepth, dilatedMotionVectors are expected to be of ths size.
    FfxApiResource                         currentBackBuffer;                  ///< The current presentation color, if currentBackBuffer_HUDLess is not used, this will be used as interpolation source data.
    FfxApiResource                         currentBackBuffer_HUDLess;          ///< The current presentation color without HUD content, when use it will be used as interpolation source data.
    FfxApiResource                         output;                             ///< The output resource where to store the interpolated result.

    FfxApiRect2D                           interpolationRect;                  ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars

    FfxApiResource                         opticalFlowVector;                  ///< The optical flow motion vectors (see example computation in the FfxOpticalFlow effect)
    FfxApiResource                         opticalFlowSceneChangeDetection;    ///< The optical flow scene change detection data
    FfxApiDimensions2D                     opticalFlowBufferSize;              ///< The optical flow motion vector resource dimensions
    FfxApiFloatCoords2D                    opticalFlowScale;                   ///< The optical flow motion vector scale factor, used to scale resoure values into [0.0,1.0] range.
    int                                 opticalFlowBlockSize;               ///< The optical flow block dimension size

    float                               cameraNear;                         ///< The distance to the near plane of the camera.
    float                               cameraFar;                          ///< The distance to the far plane of the camera. This is used only used in case of non infinite depth.
    float                               cameraFovAngleVertical;             ///< The camera angle field of view in the vertical direction (expressed in radians).
    float                               viewSpaceToMetersFactor;            ///< The unit to scale view space coordinates to meters.

    float                               frameTimeDelta;                     ///< The time elapsed since the last frame (expressed in milliseconds).
    bool                                reset;                              ///< A boolean value which when set to true, indicates the camera has moved discontinuously.

    FfxApiBackbufferTransferFunction       backBufferTransferFunction;         ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                               minMaxLuminance[2];                 ///< Min and max luminance values, used when converting HDR colors to linear RGB
    uint64_t                            frameID;                            ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.

    FfxApiResource                         dilatedDepth;                       ///< The dilated depth buffer data
    FfxApiResource                         dilatedMotionVectors;               ///< The dilated motion vector data
    FfxApiResource                         reconstructedPrevDepth;             ///< The reconstructed depth buffer data

    FfxApiResource                         distortionField;                    ///< A resource containing distortion offset data used when distortion post effects are enabled.
} FfxFrameInterpolationDispatchDescription;

FFX_API FfxErrorCode ffxFrameInterpolationDispatch(FfxFrameInterpolationContext* context, const FfxFrameInterpolationDispatchDescription* params);

/// Destroy the FidelityFX Super Resolution context.
///
/// @param [out] context                A pointer to a <c><i>FfxFsr3Context</i></c> structure to destroy.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> was <c><i>NULL</i></c>.
///
/// @ingroup FRAMEINTERPOLATION
FFX_API FfxErrorCode ffxFrameInterpolationContextDestroy(FfxFrameInterpolationContext* context);

/// Set global debug message settings
///
/// @retval
/// FFX_OK                              The operation completed successfully.
///
/// @ingroup FRAMEINTERPOLATION
FFX_API FfxErrorCode ffxFrameInterpolationSetGlobalDebugMessage(ffxMessageCallback fpMessage, uint32_t debugLevel);

/// Set global debug message settings
///
/// @retval
/// FFX_OK                              The operation completed successfully.
///
/// @ingroup FRAMEINTERPOLATION
FFX_API FfxErrorCode ffxFrameInterpolationSetGlobalDebugMessage(ffxMessageCallback fpMessage, uint32_t debugLevel);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
