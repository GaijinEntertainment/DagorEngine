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

#pragma once

#include "ffxm_assert.h"
#include "ffxm_types.h"
#include "ffxm_error.h"

namespace arm
{

#if defined(__cplusplus)
#define FFXM_CPU
extern "C" {
#endif // #if defined(__cplusplus)

/// @defgroup Backends Backends
/// Core interface declarations and natively supported backends
///
/// @ingroup ffxmSDK

/// @defgroup FfxmInterface FfxmInterface
/// FidelityFX SDK function signatures and core defines requiring
/// overrides for backend implementation.
///
/// @ingroup Backends
FFXM_FORWARD_DECLARE(FfxmInterface);

/// FidelityFX SDK major version.
///
/// @ingroup FfxmInterface
#define FFXM_SDK_VERSION_MAJOR (1)

/// FidelityFX SDK minor version.
///
/// @ingroup FfxmInterface
#define FFXM_SDK_VERSION_MINOR (0)

/// FidelityFX SDK patch version.
///
/// @ingroup FfxmInterface
#define FFXM_SDK_VERSION_PATCH (0)

/// Macro to pack a FidelityFX SDK version id together.
///
/// @ingroup FfxmInterface
#define FFXM_SDK_MAKE_VERSION( major, minor, patch ) ( ( major << 22 ) | ( minor << 12 ) | patch )

/// An enumeration of all the effects which constitute the FidelityFX SDK.
///
/// Dictates what effect shader blobs to fetch for pipeline creation
///
/// @ingroup FfxmInterface
typedef enum FfxmEffect
{
    FFXM_EFFECT_FSR2 = 0,               ///< FidelityFX Super Resolution v2

} FfxmEffect;

/// Stand in type for FfxmPass
///
/// These will be defined for each effect individually (i.e. FfxmFsr2Pass).
/// They are used to fetch the proper blob index to build effect shaders
///
/// @ingroup FfxmInterface
typedef uint32_t FfxmPass;

/// Stand in type for FfxmShaderQuality
///
/// These will be defined for each effect individually (i.e. FfxmFsr2QualityMode).
/// They are used to fetch the proper blob index to build effect shaders
///
/// @ingroup FfxmInterface
typedef uint32_t FfxmShaderQuality;

/// Get the SDK version of the backend context.
///
/// Newer effects may require support that legacy versions of the SDK will not be
/// able to provide. A version query is thus required to ensure an effect component
/// will always be paired with a backend which will support all needed functionality.
///
/// @param [in]  backendInterface                    A pointer to the backend interface.
///
/// @returns
/// The SDK version a backend was built with.
///
/// @ingroup FfxmInterface
typedef FfxmUInt32(*FfxmGetSDKVersionFunc)(
    FfxmInterface* backendInterface);

/// Create and initialize the backend context.
///
/// The callback function sets up the backend context for rendering.
/// It will create or reference the device and create required internal data structures.
///
/// @param [in]  backendInterface                    A pointer to the backend interface.
/// @param [out] effectContextId                     The context space to be used for the effect in question.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmCreateBackendContextFunc)(
    FfxmInterface* backendInterface,
    FfxmUInt32* effectContextId);

/// Get a list of capabilities of the device.
///
/// When creating an <c><i>FfxmEffectContext</i></c> it is desirable for the FFX
/// core implementation to be aware of certain characteristics of the platform
/// that is being targetted. This is because some optimizations which FFX SDK
/// attempts to perform are more effective on certain classes of hardware than
/// others, or are not supported by older hardware. In order to avoid cases
/// where optimizations actually have the effect of decreasing performance, or
/// reduce the breadth of support provided by FFX SDK, the FFX interface queries the
/// capabilities of the device to make such decisions.
///
/// For target platforms with fixed hardware support you need not implement
/// this callback function by querying the device, but instead may hardcore
/// what features are available on the platform.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [out] outDeviceCapabilities              The device capabilities structure to fill out.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode(*FfxmGetDeviceCapabilitiesFunc)(
    FfxmInterface* backendInterface,
    FfxmDeviceCapabilities* outDeviceCapabilities);

/// Destroy the backend context and dereference the device.
///
/// This function is called when the <c><i>FfxmEffectContext</i></c> is destroyed.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode(*FfxmDestroyBackendContextFunc)(
    FfxmInterface* backendInterface,
    FfxmUInt32 effectContextId);

/// Create a resource.
///
/// This callback is intended for the backend to create internal resources.
///
/// Please note: It is also possible that the creation of resources might
/// itself cause additional resources to be created by simply calling the
/// <c><i>FfxmCreateResourceFunc</i></c> function pointer again. This is
/// useful when handling the initial creation of resources which must be
/// initialized. The flow in such a case would be an initial call to create the
/// CPU-side resource, another to create the GPU-side resource, and then a call
/// to schedule a copy render job to move the data between the two. Typically
/// this type of function call flow is only seen during the creation of an
/// <c><i>FfxmEffectContext</i></c>.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] createResourceDescription           A pointer to a <c><i>FfxmCreateResourceDescription</i></c>.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
/// @param [out] outResource                        A pointer to a <c><i>FfxmResource</i></c> object.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmCreateResourceFunc)(
    FfxmInterface* backendInterface,
    const FfxmCreateResourceDescription* createResourceDescription,
    FfxmUInt32 effectContextId,
    FfxmResourceInternal* outResource);

/// Register a resource in the backend for the current frame.
///
/// Since the FfxmInterface and the backends are not aware how many different
/// resources will get passed in over time, it's not safe
/// to register all resources simultaneously in the backend.
/// Also passed resources may not be valid after the dispatch call.
/// As a result it's safest to register them as FfxmResourceInternal
/// and clear them at the end of the dispatch call.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] inResource                          A pointer to a <c><i>FfxmResource</i></c>.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
/// @param [out] outResource                        A pointer to a <c><i>FfxmResourceInternal</i></c> object.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode(*FfxmRegisterResourceFunc)(
    FfxmInterface* backendInterface,
    const FfxmResource* inResource,
    FfxmUInt32 effectContextId,
    FfxmResourceInternal* outResource);


/// Get an FfxmResource from an FfxmResourceInternal resource.
///
/// At times it is necessary to create an FfxmResource representation
/// of an internally created resource in order to register it with a
/// child effect context. This function sets up the FfxmResource needed
/// to register.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] resource                            The <c><i>FfxmResourceInternal</i></c> for which to setup an FfxmResource.
///
/// @returns
/// An FfxmResource built from the internal resource
///
/// @ingroup FfxmInterface
typedef FfxmResource(*FfxmGetResourceFunc)(
    FfxmInterface* backendInterface,
    FfxmResourceInternal resource);

/// Unregister all temporary FfxmResourceInternal from the backend.
///
/// Unregister FfxmResourceInternal referencing resources passed to
/// a function as a parameter.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] commandList                         A pointer to a <c><i>FfxmCommandList</i></c> structure.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode(*FfxmUnregisterResourcesFunc)(
    FfxmInterface* backendInterface,
    FfxmCommandList commandList,
    FfxmUInt32 effectContextId);

/// Retrieve a <c><i>FfxmResourceDescription</i></c> matching a
/// <c><i>FfxmResource</i></c> structure.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] resource                            A pointer to a <c><i>FfxmResource</i></c> object.
///
/// @returns
/// A description of the resource.
///
/// @ingroup FfxmInterface
typedef FfxmResourceDescription (*FfxmGetResourceDescriptionFunc)(
    FfxmInterface* backendInterface,
    FfxmResourceInternal resource);

/// Destroy a resource
///
/// This callback is intended for the backend to release an internal resource.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] resource                            A pointer to a <c><i>FfxmResource</i></c> object.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmDestroyResourceFunc)(
    FfxmInterface* backendInterface,
    FfxmResourceInternal resource);

/// Create a render pipeline.
///
/// A rendering pipeline contains the shader as well as resource bindpoints
/// and samplers.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] pass                                The identifier for the pass.
/// @param [in] qualityPreset                       Identified for the shader quality preset used during initialization
/// @param [in] permutationOptions                  Permutation flags to identify shader variants
/// @param [in] pipelineDescription                 A pointer to a <c><i>FfxmPipelineDescription</i></c> describing the pipeline to be created.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
/// @param [out] outPipeline                        A pointer to a <c><i>FfxmPipelineState</i></c> structure which should be populated.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmCreatePipelineFunc)(
    FfxmInterface* backendInterface,
    FfxmEffect effect,
    FfxmPass pass,
	FfxmShaderQuality qualityPreset,
    uint32_t permutationOptions,
    const FfxmPipelineDescription* pipelineDescription,
    FfxmUInt32 effectContextId,
    FfxmPipelineState* outPipeline);

/// Destroy a render pipeline.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
/// @param [out] pipeline                           A pointer to a <c><i>FfxmPipelineState</i></c> structure which should be released.
/// @param [in] effectContextId                     The context space to be used for the effect in question.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmDestroyPipelineFunc)(
    FfxmInterface* backendInterface,
    FfxmPipelineState* pipeline,
    FfxmUInt32 effectContextId);

/// Schedule a render job to be executed on the next call of
/// <c><i>FfxmExecuteGpuJobsFunc</i></c>.
///
/// Render jobs can perform one of three different tasks: clear, copy or
/// compute dispatches.
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] job                                 A pointer to a <c><i>FfxmGpuJobDescription</i></c> structure.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmScheduleGpuJobFunc)(
    FfxmInterface* backendInterface,
    const FfxmGpuJobDescription* job);

/// Execute scheduled render jobs on the <c><i>comandList</i></c> provided.
///
/// The recording of the graphics API commands should take place in this
/// callback function, the render jobs which were previously enqueued (via
/// callbacks made to <c><i>FfxmScheduleGpuJobFunc</i></c>) should be
/// processed in the order they were received. Advanced users might choose to
/// reorder the rendering jobs, but should do so with care to respect the
/// resource dependencies.
///
/// Depending on the precise contents of <c><i>FfxmDispatchDescription</i></c> a
/// different number of render jobs might have previously been enqueued (for
/// example if sharpening is toggled on and off).
///
/// @param [in] backendInterface                    A pointer to the backend interface.
/// @param [in] commandList                         A pointer to a <c><i>FfxmCommandList</i></c> structure.
///
/// @retval
/// FFXM_OK                                          The operation completed successfully.
/// @retval
/// Anything else                                   The operation failed.
///
/// @ingroup FfxmInterface
typedef FfxmErrorCode (*FfxmExecuteGpuJobsFunc)(
    FfxmInterface* backendInterface,
    FfxmCommandList commandList);

/// A structure encapsulating the interface between the core implementation of
/// the FfxmInterface and any graphics API that it should ultimately call.
///
/// This set of functions serves as an abstraction layer between FfxmInterfae and the
/// API used to implement it. While the FidelityFX SDK ships with backends for DirectX12 and
/// Vulkan, it is possible to implement your own backend for other platforms
/// which sit on top of your engine's own abstraction layer. For details on the
/// expectations of what each function should do you should refer the
/// description of the following function pointer types:
///
///     <c><i>FfxmCreateDeviceFunc</i></c>
///     <c><i>FfxmGetDeviceCapabilitiesFunc</i></c>
///     <c><i>FfxmDestroyDeviceFunc</i></c>
///     <c><i>FfxmCreateResourceFunc</i></c>
///     <c><i>FfxmRegisterResourceFunc</i></c>
///     <c><i>FfxmGetResourceFunc</i></c>
///     <c><i>FfxmUnregisterResourcesFunc</i></c>
///     <c><i>FfxmGetResourceDescriptionFunc</i></c>
///     <c><i>FfxmDestroyResourceFunc</i></c>
///     <c><i>FfxmCreatePipelineFunc</i></c>
///     <c><i>FfxmDestroyPipelineFunc</i></c>
///     <c><i>FfxmScheduleGpuJobFunc</i></c>
///     <c><i>FfxmExecuteGpuJobsFunc</i></c>
///
/// Depending on the graphics API that is abstracted by the backend, it may be
/// required that the backend is to some extent stateful. To ensure that
/// applications retain full control to manage the memory used by the FidelityFX SDK, the
/// <c><i>scratchBuffer</i></c> and <c><i>scratchBufferSize</i></c> fields are
/// provided. A backend should provide a means of specifying how much scratch
/// memoryfsr2.h is required for its internal implementation (e.g: via a function
/// or constant value). The application is then responsible for allocating that
/// memory and providing it when setting up the SDK backend. Backends provided
/// with the FidelityFX SDK do not perform dynamic memory allocations, and instead
/// sub-allocate all memory from the scratch buffers provided.
///
/// The <c><i>scratchBuffer</i></c> and <c><i>scratchBufferSize</i></c> fields
/// should be populated according to the requirements of each backend. For
/// example, if using the Vulkan backend you should call the
/// <c><i>ffxmGetScratchMemorySizeVK </i></c> function. It is not required
/// that custom backend implementations use a scratch buffer.
///
/// Any functional addition to this interface mandates a version
/// bump to ensure full functionality across effects and backends.
///
/// @ingroup FfxmInterface
typedef struct FfxmInterface {

    FfxmGetSDKVersionFunc            fpGetSDKVersion;           ///< A callback function to query the SDK version.
    FfxmCreateBackendContextFunc     fpCreateBackendContext;    ///< A callback function to create and initialize the backend context.
    FfxmGetDeviceCapabilitiesFunc    fpGetDeviceCapabilities;   ///< A callback function to query device capabilities.
    FfxmDestroyBackendContextFunc    fpDestroyBackendContext;   ///< A callback function to destroy the backend context. This also dereferences the device.
    FfxmCreateResourceFunc           fpCreateResource;          ///< A callback function to create a resource.
    FfxmRegisterResourceFunc         fpRegisterResource;        ///< A callback function to register an external resource.
    FfxmGetResourceFunc              fpGetResource;             ///< A callback function to convert an internal resource to external resource type
    FfxmUnregisterResourcesFunc      fpUnregisterResources;     ///< A callback function to unregister external resource.
    FfxmGetResourceDescriptionFunc   fpGetResourceDescription;  ///< A callback function to retrieve a resource description.
    FfxmDestroyResourceFunc          fpDestroyResource;         ///< A callback function to destroy a resource.
    FfxmCreatePipelineFunc           fpCreateComputePipeline;   ///< A callback function to create a compute pipeline.
    FfxmCreatePipelineFunc           fpCreateGraphicsPipeline;  ///< A callback function to create a render pipeline.
    FfxmDestroyPipelineFunc          fpDestroyPipeline;         ///< A callback function to destroy a render or compute pipeline.
    FfxmScheduleGpuJobFunc           fpScheduleGpuJob;          ///< A callback function to schedule a render job.
    FfxmExecuteGpuJobsFunc           fpExecuteGpuJobs;          ///< A callback function to execute all queued render jobs.

    void*                           scratchBuffer;             ///< A preallocated buffer for memory utilized internally by the backend.
    size_t                          scratchBufferSize;         ///< Size of the buffer pointed to by <c><i>scratchBuffer</i></c>.
    FfxmDevice                       device;                    ///< A backend specific device

} FfxmInterface;

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)

} // namespace arm
