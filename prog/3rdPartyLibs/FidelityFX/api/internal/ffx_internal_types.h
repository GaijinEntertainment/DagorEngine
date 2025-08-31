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

#pragma once

#include "../include/ffx_api_types.h"

#include <stdint.h>

///
/// @defgroup ffxSDK SDK
/// The SDK module provides detailed descriptions of the various class, structs, and function which comprise the FidelityFX SDK. It is divided into several sub-modules.
///

/// @defgroup ffxHost Host
/// The FidelityFX SDK host (CPU-side) references
///
/// @ingroup ffxSDK

/// @defgroup Defines Defines
/// Top level defines used by the FidelityFX SDK
///
/// @ingroup ffxHost

// When defining custom mutex you have to also define:
// FFX_MUTEX_LOCK - for exclusive locking of mutex
// FFX_MUTEX_LOCK_SHARED - for shared locking of mutex
// FFX_MUTEX_UNLOCK - for exclusive unlocking of mutex
// FFX_MUTEX_UNLOCK_SHARED - for shared unlocking of mutex
//
// If your mutex type doesn't support shared locking mechanism you can rely
// on exclusive locks only (define _SHARED variants to the same exclusive operation).
#ifndef FFX_MUTEX
#if __cplusplus >= 201703L
#include <shared_mutex>
/// FidelityFX mutex wrapper.
///
/// @ingroup SDKTypes
#define FFX_MUTEX std::shared_mutex
#define FFX_MUTEX_IMPL_SHARED
#else
#include <mutex>
/// FidelityFX mutex wrapper.
///
/// @ingroup SDKTypes
#define FFX_MUTEX std::mutex
#define FFX_MUTEX_IMPL_STANDARD
#endif // #if __cplusplus >= 201703L
#endif // #ifndef FFX_MUTEX

#if defined(FFX_GCC) || !defined(FFX_BUILD_AS_DLL)
/// FidelityFX exported functions
///
/// @ingroup Defines
#define FFX_API
#else
/// FidelityFX exported functions
///
/// @ingroup Defines
#define FFX_API __declspec(dllexport)
#endif // #if defined (FFX_GCC)

#define FFX_SDK_DEFAULT_CONTEXT_SIZE (1024 * 128)

/// Maximum supported number of simultaneously bound SRVs.
///
/// @ingroup Defines
#define FFX_MAX_NUM_SRVS            64

/// Maximum supported number of simultaneously bound UAVs.
///
/// @ingroup Defines
#define FFX_MAX_NUM_UAVS            64

/// Maximum number of constant buffers bound.
///
/// @ingroup Defines
#define FFX_MAX_NUM_CONST_BUFFERS   3

/// Maximum number of characters in a resource name
///
/// @ingroup Defines
#define FFX_RESOURCE_NAME_SIZE      64

/// Maximum number of queued frames in the backend
///
/// @ingroup Defines
#define FFX_MAX_QUEUED_FRAMES          (4)

/// Maximum number of resources per effect context
///
/// @ingroup Defines
#define FFX_MAX_RESOURCE_COUNT         (512)

/// Maximum number of passes per effect component
///
/// @ingroup Defines
#define FFX_MAX_PASS_COUNT             (50)

/// Total number of descriptors in ring buffer needed for a single effect context
///
/// @ingroup Defines
#define FFX_RING_BUFFER_DESCRIPTOR_COUNT    (FFX_MAX_QUEUED_FRAMES * FFX_MAX_PASS_COUNT * FFX_MAX_RESOURCE_COUNT)

/// Size of constant buffer entry in the ring buffer table
///
/// @ingroup Defines
#define FFX_BUFFER_SIZE                (4096)

/// Total constant buffer ring buffer size for a single effect context
///
/// @ingroup Defines
#define FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE (FFX_MAX_QUEUED_FRAMES * FFX_MAX_PASS_COUNT * FFX_BUFFER_SIZE)

/// Maximum number of barriers per flush
///
/// @ingroup Defines
#define FFX_MAX_BARRIERS               (128)

/// Maximum number of GPU jobs per submission
///
/// @ingroup Defines
#define FFX_MAX_GPU_JOBS               (256)

/// Maximum number of samplers supported
///
/// @ingroup Defines
#define FFX_MAX_SAMPLERS               (16)

/// Maximum number of simultaneous upload jobs
///
/// @ingroup Defines
#define UPLOAD_JOB_COUNT               (16)

// Off by default warnings
#pragma warning(disable : 4365 4710 4820 5039)

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

/// @defgroup CPUTypes CPU Types
/// CPU side type defines for all commonly used variables
///
/// @ingroup ffxHost

/// A typedef for version numbers returned from functions in the FidelityFX SDK.
///
/// @ingroup CPUTypes
    typedef uint32_t FfxVersionNumber;

/// A typedef for a boolean value.
///
/// @ingroup CPUTypes
typedef bool FfxBoolean;

/// A typedef for a unsigned 8bit integer.
///
/// @ingroup CPUTypes
typedef uint8_t FfxUInt8;

/// A typedef for a unsigned 16bit integer.
///
/// @ingroup CPUTypes
typedef uint16_t FfxUInt16;

/// A typedef for a unsigned 32bit integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxUInt32;

/// A typedef for a unsigned 64bit integer.
///
/// @ingroup CPUTypes
typedef uint64_t FfxUInt64;

/// A typedef for a signed 8bit integer.
///
/// @ingroup CPUTypes
typedef int8_t FfxInt8;

/// A typedef for a signed 16bit integer.
///
/// @ingroup CPUTypes
typedef int16_t FfxInt16;

/// A typedef for a signed 32bit integer.
///
/// @ingroup CPUTypes
typedef int32_t FfxInt32;

/// A typedef for a signed 64bit integer.
///
/// @ingroup CPUTypes
typedef int64_t FfxInt64;

/// A typedef for a floating point value.
///
/// @ingroup CPUTypes
typedef float FfxFloat32;

/// A typedef for a 2-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxFloat32x2[2];

/// A typedef for a 3-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxFloat32x3[3];

/// A typedef for a 4-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxFloat32x4[4];

/// A typedef for a 4x4 floating point matrix.
///
/// @ingroup CPUTypes
typedef float FfxFloat32x4x4[16];

/// A typedef for a 2-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxUInt32x2[2];

/// A typedef for a 3-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxUInt32x3[3];

/// A typedef for a 4-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxUInt32x4[4];

/// A typedef for a 2-dimensional 32bit signed integer.
///
/// @ingroup CPUTypes
typedef int32_t FfxInt32x2[2];

/// A typedef for a 3-dimensional 32bit signed integer.
///
/// @ingroup CPUTypes
typedef int32_t FfxInt32x3[3];

/// A typedef for a 4-dimensional 32bit signed integer.
///
/// @ingroup CPUTypes
typedef int32_t FfxInt32x4[4];

/// @defgroup SDKTypes SDK Types
/// Structure and enumeration definitions used by the FidelityFX SDK
///
/// @ingroup ffxHost

typedef enum FfxIndexFormat
{
    FFX_INDEX_TYPE_UINT32,
    FFX_INDEX_TYPE_UINT16
} FfxIndexFormat;

/// An enumeration of resource view dimensions.
///
/// @ingroup SDKTypes
typedef enum FfxResourceViewDimension
{
    FFX_RESOURCE_VIEW_DIMENSION_BUFFER,             ///< A resource view on a buffer.
    FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_1D,         ///< A resource view on a single dimension.
    FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_1D_ARRAY,   ///< A resource view on a single dimensional array.
    FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_2D,         ///< A resource view on two dimensions.
    FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_2D_ARRAY,   ///< A resource view on two dimensional array.
    FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_3D,         ///< A resource view on three dimensions.
} FfxResourceViewDimension;

/// An enumeration of all resource view types.
///
/// @ingroup SDKTypes
typedef enum FfxResourceViewType {

    FFX_RESOURCE_VIEW_UNORDERED_ACCESS,             ///< The resource view is an unordered access view (UAV).
    FFX_RESOURCE_VIEW_SHADER_READ,                  ///< The resource view is a shader resource view (SRV).
} FfxResourceViewType;

/// The type of filtering to perform when reading a texture.
///
/// @ingroup SDKTypes
typedef enum FfxFilterType {

    FFX_FILTER_TYPE_MINMAGMIP_POINT,        ///< Point sampling.
    FFX_FILTER_TYPE_MINMAGMIP_LINEAR,       ///< Sampling with interpolation.
    FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT,  ///< Use linear interpolation for minification and magnification; use point sampling for mip-level sampling.
} FfxFilterType;

/// The address mode used when reading a texture.
///
/// @ingroup SDKTypes
typedef enum FfxAddressMode {

    FFX_ADDRESS_MODE_WRAP,                  ///< Wrap when reading texture.
    FFX_ADDRESS_MODE_MIRROR,                ///< Mirror when reading texture.
    FFX_ADDRESS_MODE_CLAMP,                 ///< Clamp when reading texture.
    FFX_ADDRESS_MODE_BORDER,                ///< Border color when reading texture.
    FFX_ADDRESS_MODE_MIRROR_ONCE,           ///< Mirror once when reading texture.
} FfxAddressMode;

/// An enumeration of all supported shader models.
///
/// @ingroup SDKTypes
typedef enum FfxShaderModel {

    FFX_SHADER_MODEL_5_1,                           ///< Shader model 5.1.
    FFX_SHADER_MODEL_6_0,                           ///< Shader model 6.0.
    FFX_SHADER_MODEL_6_1,                           ///< Shader model 6.1.
    FFX_SHADER_MODEL_6_2,                           ///< Shader model 6.2.
    FFX_SHADER_MODEL_6_3,                           ///< Shader model 6.3.
    FFX_SHADER_MODEL_6_4,                           ///< Shader model 6.4.
    FFX_SHADER_MODEL_6_5,                           ///< Shader model 6.5.
    FFX_SHADER_MODEL_6_6,                           ///< Shader model 6.6.
    FFX_SHADER_MODEL_6_7,                           ///< Shader model 6.7.
} FfxShaderModel;

/// An enumeration for different heap types
///
/// @ingroup SDKTypes
typedef enum FfxHeapType {

    FFX_HEAP_TYPE_DEFAULT = 0,                      ///< Local memory.
    FFX_HEAP_TYPE_UPLOAD,                           ///< Heap used for uploading resources.
    FFX_HEAP_TYPE_READBACK                          ///< Heap used for reading back resources.
} FfxHeapType;

/// An enumeration for different render job types
///
/// @ingroup SDKTypes
typedef enum FfxGpuJobType {

    FFX_GPU_JOB_CLEAR_FLOAT = 0,                    ///< The GPU job is performing a floating-point clear.
    FFX_GPU_JOB_COPY = 1,                           ///< The GPU job is performing a copy.
    FFX_GPU_JOB_COMPUTE = 2,                        ///< The GPU job is performing a compute dispatch.
    FFX_GPU_JOB_BARRIER = 3,                        ///< The GPU job is performing a barrier.

    FFX_GPU_JOB_DISCARD = 4,                        ///< The GPU job is performing a floating-point clear.

} FfxGpuJobType;

/// An enumeration for flags for render job
///
/// @ingroup SDKTypes
typedef enum FfxGpuJobFlags
{
    FFX_GPU_JOB_FLAGS_NONE = 0,         ///< No flags.
    FFX_GPU_JOB_FLAGS_SKIP_BARRIERS = (1 << 0),  ///< A bit indicating a job shouldn't emit barriers
} FfxGpuJobFlags;

/// An enumeration for various descriptor types
///
/// @ingroup SDKTypes
typedef enum FfxDescriptorType {

    //FFX_DESCRIPTOR_CBV = 0,   // All CBVs currently mapped to root signature
    //FFX_DESCRIPTOR_SAMPLER,   // All samplers currently static
    FFX_DESCRIPTOR_TEXTURE_SRV = 0,
    FFX_DESCRIPTOR_BUFFER_SRV,
    FFX_DESCRIPTOR_TEXTURE_UAV,
    FFX_DESCRIPTOR_BUFFER_UAV,
} FfxDescriptiorType;

/// An enumeration for view binding stages
///
/// @ingroup SDKTypes
typedef enum FfxBindStage {

    FFX_BIND_PIXEL_SHADER_STAGE = 1 << 0,
    FFX_BIND_VERTEX_SHADER_STAGE = 1 << 1,
    FFX_BIND_COMPUTE_SHADER_STAGE = 1 << 2,

} FfxBindStage;

/// An enumeration for barrier types
///
/// @ingroup SDKTypes
typedef enum FfxBarrierType
{
    FFX_BARRIER_TYPE_TRANSITION = 0,
    FFX_BARRIER_TYPE_UAV,
} FfxBarrierType;

typedef void (*ffxMessageCallback)(uint32_t type, const wchar_t* message);

/// An enumeration for message types that can be passed
///
/// @ingroup SDKTypes
typedef enum FfxMsgType {
    FFX_MESSAGE_TYPE_ERROR      = 0,
    FFX_MESSAGE_TYPE_WARNING    = 1,
    FFX_MESSAGE_TYPE_COUNT
} FfxMsgType;

/// An enumeration of all the effects which constitute the FidelityFX SDK.
///
/// Dictates what effect shader blobs to fetch for pipeline creation
///
/// @ingroup SDKTypes
typedef enum FfxEffect
{

    FFX_EFFECT_FSR2 = 0,               ///< FidelityFX Super Resolution v2
    FFX_EFFECT_FSR1,                   ///< FidelityFX Super Resolution
    FFX_EFFECT_SPD,                    ///< FidelityFX Single Pass Downsampler
    FFX_EFFECT_BLUR,                   ///< FidelityFX Blur
    FFX_EFFECT_BREADCRUMBS,            ///< FidelityFX Breadcrumbs
    FFX_EFFECT_BRIXELIZER,             ///< FidelityFX Brixelizer
    FFX_EFFECT_BRIXELIZER_GI,          ///< FidelityFX Brixelizer GI
    FFX_EFFECT_CACAO,                  ///< FidelityFX Combined Adaptive Compute Ambient Occlusion
    FFX_EFFECT_CAS,                    ///< FidelityFX Contrast Adaptive Sharpening
    FFX_EFFECT_DENOISER,               ///< FidelityFX Denoiser
    FFX_EFFECT_LENS,                   ///< FidelityFX Lens
    FFX_EFFECT_PARALLEL_SORT,          ///< FidelityFX Parallel Sort
    FFX_EFFECT_SSSR,                   ///< FidelityFX Stochastic Screen Space Reflections
    FFX_EFFECT_VARIABLE_SHADING,       ///< FidelityFX Variable Shading
    FFX_EFFECT_LPM,                    ///< FidelityFX Luma Preserving Mapper
    FFX_EFFECT_DOF,                    ///< FidelityFX Depth of Field
    FFX_EFFECT_CLASSIFIER,             ///< FidelityFX Classifier
    FFX_EFFECT_FSR3UPSCALER,           ///< FidelityFX Super Resolution v3
    FFX_EFFECT_FRAMEINTERPOLATION,     ///< FidelityFX Frame Interpolation, part of FidelityFX Super Resolution v3
    FFX_EFFECT_OPTICALFLOW,            ///< FidelityFX Optical Flow, part of FidelityFX Super Resolution v3
    FFX_EFFECT_FSR4UPSCALER,           ///< FidelityFX Super Resolution v4 (MLSR)

    FFX_EFFECT_SHAREDRESOURCES = 127,  ///< FidelityFX Shared resources effect ID
    FFX_EFFECT_SHAREDAPIBACKEND = 128  ///< FidelityFX Shared backend context used with DLL API
} FfxEffect;

/// A typedef representing the graphics device.
///
/// @ingroup SDKTypes
typedef void* FfxDevice;

typedef void* FfxCommandQueue;

typedef void* FfxSwapchain;

/// A typedef representing a command list or command buffer.
///
/// @ingroup SDKTypes
typedef void* FfxCommandList;

/// A typedef for a root signature.
///
/// @ingroup SDKTypes
typedef void* FfxRootSignature;

/// A typedef for a command signature, used for indirect workloads
///
/// @ingroup SDKTypes
typedef void* FfxCommandSignature;

/// A typedef for a pipeline state object.
///
/// @ingroup SDKTypes
typedef void* FfxPipeline;

/// Allocate block of memory.
///
/// The callback function for requesting memory of provided size.
/// <c><i>size</i></c> cannot be 0.
///
/// @param [in]  size               Size in bytes of memory to allocate.
///
/// @retval
/// NULL                            The operation failed.
/// @retval
/// Anything else                   The operation completed successfully.
///
/// @ingroup SDKTypes
typedef void* (*FfxAllocFunc)(
    size_t size);

/// Reallocate block of memory.
///
/// The callback function for reallocating provided block of memory to new location
/// with specified size. When provided with <c><i>NULL</i></c> as <c><i>ptr</i></c>
/// then it should behave as <c><i>FfxBreadcrumbsAllocFunc</i></c>.
/// If the operation failed then contents of <c><i>ptr</i></c>
/// cannot be changed. <c><i>size</i></c> cannot be 0.
///
/// @param [in]  ptr                A pointer to previous block of memory.
/// @param [in]  size               Size in bytes of memory to allocate.
///
/// @retval
/// NULL                            The operation failed.
/// @retval
/// Anything else                   The operation completed successfully.
///
/// @ingroup SDKTypes
typedef void* (*FfxReallocFunc)(
    void* ptr,
    size_t size);

/// Free block of memory.
///
/// The callback function for freeing provided block of memory.
/// <c><i>ptr</i></c> cannot be <c><i>NULL</i></c>.
///
/// @param [in]  ptr                A pointer to block of memory.
///
/// @ingroup SDKTypes
typedef void (*FfxFreeFunc)(
    void* ptr);

/// A structure encapsulating a set of allocation callbacks.
///
/// @ingroup SDKTypes
typedef struct FfxAllocationCallbacks {

    FfxAllocFunc                    fpAlloc;                                ///< Callback for allocating memory in the library.
    FfxReallocFunc                  fpRealloc;                              ///< Callback for reallocating memory in the library.
    FfxFreeFunc                     fpFree;                                 ///< Callback for freeing allocated memory in the library.
} FfxAllocationCallbacks;

/// A structure encapsulating the bindless descriptor configuration of an effect.
///
/// @ingroup SDKTypes
typedef struct FfxEffectBindlessConfig {
    uint32_t                        maxTextureSrvs;                         ///< Maximum number of texture SRVs needed in the bindless table.
    uint32_t                        maxBufferSrvs;                          ///< Maximum number of buffer SRVs needed in the bindless table.
    uint32_t                        maxTextureUavs;                         ///< Maximum number of texture UAVs needed in the bindless table.
    uint32_t                        maxBufferUavs;                          ///< Maximum number of buffer UAVs needed in the bindless table.
} FfxEffectBindlessConfig;

/// A structure encapsulating a collection of device capabilities.
///
/// @ingroup SDKTypes
typedef struct FfxDeviceCapabilities {

    FfxShaderModel                  maximumSupportedShaderModel;                ///< The maximum shader model supported by the device.
    uint32_t                        waveLaneCountMin;                           ///< The minimum supported wavefront width.
    uint32_t                        waveLaneCountMax;                           ///< The maximum supported wavefront width.
    bool                            fp16Supported;                              ///< The device supports FP16 in hardware.
    bool                            raytracingSupported;                        ///< The device supports ray tracing.
    bool                            deviceCoherentMemorySupported;              ///< The device supports AMD coherent memory.
    bool                            dedicatedAllocationSupported;               ///< The device supports dedicated allocations for resources.
    bool                            bufferMarkerSupported;                      ///< The device supports AMD buffer markers.
    bool                            extendedSynchronizationSupported;           ///< The device supports extended synchronization mechanism.
    bool                            shaderStorageBufferArrayNonUniformIndexing; ///< The device supports shader storage buffer array non uniform indexing.
} FfxDeviceCapabilities;

/// A structure encapsulating a 2-dimensional point.
///
/// @ingroup SDKTypes
typedef struct FfxIntCoords2D {

    int32_t                         x;                                      ///< The x coordinate of a 2-dimensional point.
    int32_t                         y;                                      ///< The y coordinate of a 2-dimensional point.
} FfxIntCoords2D;

/// A structure describing a constant buffer allocation.
///
/// @ingroup SDKTypes
typedef struct FfxConstantAllocation
{
    FfxApiResource  resource;        ///< The resource representing the constant buffer resource.
    FfxUInt64       handle;          ///< The binding handle for the constant buffer

} FfxRootConstantAllocation;

/// A function definition for a constant buffer allocation callback
///
/// Used to provide a constant buffer allocator to the calling backend
///
/// @param [in] data                       The constant buffer data.
/// @param [in] dataSize                   The size of the constant buffer data.
///
///
/// @ingroup SDKTypes
typedef FfxConstantAllocation(*FfxConstantBufferAllocator)(
    void* data,
    const FfxUInt64 dataSize);

/// A structure describing a static resource.
///
/// @ingroup SDKTypes
typedef struct FfxStaticResourceDescription
{
    const FfxApiResource*   resource;        ///< The resource to register.
    FfxDescriptorType       descriptorType;  ///< The type of descriptor to create.
    uint32_t                descriptorIndex; ///< The destination index of the descriptor within the static table.

    union
    {
        uint32_t bufferOffset;  ///< The buffer offset in bytes.
        uint32_t textureUavMip; ///< The mip of the texture resource to create a UAV for.
    };

    uint32_t bufferSize;    ///< The buffer size in bytes.
    uint32_t bufferStride;  ///< The buffer stride in bytes.
} FfxStaticResourceDescription;

/// An internal structure containing a handle to a resource and resource views
///
/// @ingroup SDKTypes
typedef struct FfxResourceInternal {
    int32_t                         internalIndex;                          ///< The index of the resource.
} FfxResourceInternal;

/// An enumeration for resource init data types that can be passed
///
/// @ingroup SDKTypes
typedef enum FfxResourceInitDataType {
    FFX_RESOURCE_INIT_DATA_TYPE_INVALID = 0,
    FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED,
    FFX_RESOURCE_INIT_DATA_TYPE_BUFFER,
    FFX_RESOURCE_INIT_DATA_TYPE_VALUE,
} FfxResourceInitDataType;

/// An structure housing all that is needed for resource initialization
///
/// @ingroup SDKTypes
typedef struct FfxResourceInitData
{
    FfxResourceInitDataType type; ///< Indicates that the resource will be initialized from a buffer or a value, or stay uninitialized.
    size_t                  size; ///< The size, in bytes, of the resource that needed be initialized.
    union
    {
        void*         buffer;  ///< The buffer used to initialize the resource.
        unsigned char value;   ///< Indicates that the resource will be filled up with this value.
    };

    static FfxResourceInitData FfxResourceInitValue(size_t dataSize, uint8_t initVal)
    {
        FfxResourceInitData initData = { FFX_RESOURCE_INIT_DATA_TYPE_VALUE };
        initData.size = dataSize;
        initData.value = initVal;
        return initData;
    }

    static FfxResourceInitData FfxResourceInitBuffer(size_t dataSize, void* pInitData)
    {
        FfxResourceInitData initData = { FFX_RESOURCE_INIT_DATA_TYPE_BUFFER };
        initData.size = dataSize;
        initData.buffer = pInitData;
        return initData;
    }

} FfxResourceInitData;

/// An internal structure housing all that is needed for backend resource descriptions
///
/// @ingroup SDKTypes
typedef struct FfxInternalResourceDescription {

    uint32_t                    id;         ///< Resource identifier
    const wchar_t*              name;       ///< Name to set to the resource for easier debugging
    FfxApiResourceType          type;       ///< The type of resource (see <c><i>FfxApiResourceType</i></c>)
    FfxApiResourceUsage         usage;      ///< Resource usage flags (see <c><i>FfxApiResourceUsage</i></c>)
    FfxApiSurfaceFormat         format;     ///< The resource format to use
    uint32_t                    width;      ///< The width (textures) or size (buffers) of the resource
    uint32_t                    height;     ///< The height (textures) or stride (buffers) of the resource
    uint32_t                    mipCount;   ///< Mip count (textures) of the resource
    FfxApiResourceFlags         flags;      ///< Resource flags (see <c><i>FfxResourceFlags</i></c>)
    FfxResourceInitData         initData;   ///< Resource initialization definition (see <c><i>FfxResourceInitData</i></c>)
} FfxInternalResourceDescription;

/// A structure defining the view to create
///
/// @ingroup SDKTypes
typedef struct FfxViewDescription
{
    bool                        uavView;                    ///< Indicates that the view is a UAV.
    FfxResourceViewDimension    viewDimension;              ///< The view dimension to map
    union {
        int32_t mipLevel;                                   ///< The mip level of the view, (-1) for default
        int32_t firstElement;                               ///< The first element of a buffer view, (-1) for default
    };

    union {
        int32_t arraySize;                                  ///< The array size of the view, (-1) for full depth/array size
        int32_t elementCount;                               ///< The number of elements in a buffer view, (-1) for full depth/array size
    };

    int32_t                     firstSlice;                 ///< The first slice to map to, (-1) for default first slice
    wchar_t                     name[FFX_RESOURCE_NAME_SIZE];
} FfxViewDescription;

static FfxViewDescription s_FfxViewDescInit = { false, FFX_RESOURCE_VIEW_DIMENSION_TEXTURE_2D, -1, -1, -1, L"" };

/// A structure defining a resource bind point
///
/// @ingroup SDKTypes
typedef struct FfxResourceBinding
{
    uint32_t    slotIndex;                      ///< The slot into which to bind the resource
    uint32_t    arrayIndex;                     ///< The resource offset for mip/array access
    uint32_t    resourceIdentifier;             ///< A unique resource identifier representing an internal resource index
    wchar_t     name[FFX_RESOURCE_NAME_SIZE];   ///< A debug name to help track the resource binding
}FfxResourceBinding;

/// A structure encapsulating a single pass of an algorithm.
///
/// @ingroup SDKTypes
typedef struct FfxPipelineState {

    FfxRootSignature                rootSignature;                                      ///< The pipelines rootSignature
    FfxCommandSignature             cmdSignature;                                       ///< The command signature used for indirect workloads
    FfxPipeline                     pipeline;                                           ///< The pipeline object
    uint32_t                        uavTextureCount;                                    ///< Count of Texture UAVs used in this pipeline
    uint32_t                        srvTextureCount;                                    ///< Count of Texture SRVs used in this pipeline
    uint32_t                        srvBufferCount;                                     ///< Count of Buffer SRV used in this pipeline
    uint32_t                        uavBufferCount;                                     ///< Count of Buffer UAVs used in this pipeline
    uint32_t                        staticTextureSrvCount;                              ///< Count of static Texture SRVs used in this pipeline
    uint32_t                        staticBufferSrvCount;                               ///< Count of static Buffer SRVs used in this pipeline
    uint32_t                        staticTextureUavCount;                              ///< Count of static Texture UAVs used in this pipeline
    uint32_t                        staticBufferUavCount;                               ///< Count of static Buffer UAVs used in this pipeline
    uint32_t                        constCount;                                         ///< Count of constant buffers used in this pipeline

    FfxResourceBinding              uavTextureBindings[FFX_MAX_NUM_UAVS];               ///< Array of ResourceIdentifiers bound as texture UAVs
    FfxResourceBinding              srvTextureBindings[FFX_MAX_NUM_SRVS];               ///< Array of ResourceIdentifiers bound as texture SRVs
    FfxResourceBinding              srvBufferBindings[FFX_MAX_NUM_SRVS];                ///< Array of ResourceIdentifiers bound as buffer SRVs
    FfxResourceBinding              uavBufferBindings[FFX_MAX_NUM_UAVS];                ///< Array of ResourceIdentifiers bound as buffer UAVs
    FfxResourceBinding              constantBufferBindings[FFX_MAX_NUM_CONST_BUFFERS];  ///< Array of ResourceIdentifiers bound as CBs

    wchar_t                         name[FFX_RESOURCE_NAME_SIZE];                       ///< Pipeline name for debugging/profiling purposes
} FfxPipelineState;

/// A structure containing the data required to create a resource.
///
/// @ingroup SDKTypes
typedef struct FfxCreateResourceDescription {

    FfxHeapType                     heapType;                               ///< The heap type to hold the resource, typically <c><i>FFX_HEAP_TYPE_DEFAULT</i></c>.
    FfxApiResourceDescription       resourceDescription;                    ///< A resource description.
    FfxApiResourceState             initialState;                            ///< The initial resource state.
    const wchar_t*                  name;                                   ///< Name of the resource.
    uint32_t                        id;                                     ///< Internal resource ID.
    FfxResourceInitData             initData;                               ///< A struct used to initialize the resource.
} FfxCreateResourceDescription;

/// A structure containing the data required to create sampler mappings
///
/// @ingroup SDKTypes
typedef struct FfxSamplerDescription {

    FfxFilterType   filter;
    FfxAddressMode  addressModeU;
    FfxAddressMode  addressModeV;
    FfxAddressMode  addressModeW;
    FfxBindStage    stage;
} FfxSamplerDescription;

/// A structure containing the data required to create root constant buffer mappings
///
/// @ingroup SDKTypes
typedef struct FfxRootConstantDescription
{
    uint32_t      size;
    FfxBindStage  stage;
} FfxRootConstantDescription;

/// A structure containing the description used to create a
/// <c><i>FfxPipeline</i></c> structure.
///
/// A pipeline is the name given to a shader and the collection of state that
/// is required to dispatch it. In the context of the FidelityFX SDK and its architecture
/// this means that a <c><i>FfxPipelineDescription</i></c> will map to either a
/// monolithic object in an explicit API (such as a
/// <c><i>PipelineStateObject</i></c> in DirectX 12). Or a shader and some
/// ancillary API objects (in something like DirectX 11).
///
/// The <c><i>contextFlags</i></c> field contains a copy of the flags passed
/// to <c><i>ffxContextCreate</i></c> via the <c><i>flags</i></c> field of
/// the <c><i>Ffx<Effect>InitializationParams</i></c> structure. These flags are
/// used to determine which permutation of a pipeline for a specific
/// <c><i>Ffx<Effect>Pass</i></c> should be used to implement the features required
/// by each application, as well as to achieve the best performance on specific
/// target hardware configurations.
///
/// When using one of the provided backends for FidelityFX SDK (such as DirectX 12 or
/// Vulkan) the data required to create a pipeline is compiled off line and
/// included into the backend library that you are using. For cases where the
/// backend interface is overridden by providing custom callback function
/// implementations care should be taken to respect the contents of the
/// <c><i>contextFlags</i></c> field in order to correctly support the options
/// provided by the FidelityFX SDK, and achieve best performance.
/// ///
/// @ingroup SDKTypes
typedef struct FfxPipelineDescription {

    uint32_t                            contextFlags;                   ///< A collection of <c><i>FfxInitializationFlagBits</i></c> which were passed to the context.
    const FfxSamplerDescription*        samplers;                       ///< A collection of samplers to use when building the root signature for the pipeline
    size_t                              samplerCount;                   ///< Number of samplers to create for the pipeline
    const FfxRootConstantDescription*   rootConstants;                  ///< A collection of root constant descriptions to use when building the root signature for the pipeline
    uint32_t                            rootConstantBufferCount;        ///< Number of root constant buffers to create for the pipeline
    wchar_t                             name[64];                       ///< Pipeline name with which to name the pipeline object
    FfxBindStage                        stage;                          ///< The stage(s) for which this pipeline is being built
    uint32_t                            indirectWorkload;               ///< Whether this pipeline has an indirect workload
    FfxApiSurfaceFormat                 backbufferFormat;               ///< For raster pipelines this contains the backbuffer format
} FfxPipelineDescription;

/// A structure containing the data required to create a barrier
///
/// @ingroup SDKTypes
typedef struct FfxBarrierDescription
{
    FfxResourceInternal resource;           ///< The resource representation
    FfxBarrierType      barrierType;          ///< The type of barrier to execute
    FfxApiResourceState currentState;         ///< The initial state of the resource
    FfxApiResourceState newState;             ///< The new state of the resource after barrier
    uint32_t            subResourceID;        ///< The subresource id to apply barrier operation to
} FfxBarrierDescription;


/// A structure containing a constant buffer.
///
/// @ingroup SDKTypes
typedef struct FfxConstantBuffer {

    uint32_t                        num32BitEntries;    ///< The size (expressed in 32-bit chunks) stored in data.
    uint32_t*                       data;               ///< Pointer to constant buffer data
}FfxConstantBuffer;

/// A structure containing a shader resource view.
typedef struct FfxTextureSRV
{
    FfxResourceInternal resource;               ///< Resource corresponding to the shader resource view.
#ifdef FFX_DEBUG
    wchar_t             name[FFX_RESOURCE_NAME_SIZE];
#endif
} FfxTextureSRV;

/// A structure containing a shader resource view.
typedef struct FfxBufferSRV
{
    uint32_t            offset;                 ///< Offset of resource to bind in bytes.
    uint32_t            size;                   ///< Size of resource to bind in bytes.
    uint32_t            stride;                 ///< Size of resource to bind in bytes.
    FfxResourceInternal resource;               ///< Resource corresponding to the shader resource view.
#ifdef FFX_DEBUG
    wchar_t             name[FFX_RESOURCE_NAME_SIZE];
#endif
} FfxBufferSRV;

/// A structure containing a unordered access view.
typedef struct FfxTextureUAV
{
    uint32_t            mip;                    ///< Mip level of resource to bind.
    FfxResourceInternal resource;               ///< Resource corresponding to the unordered access view.
#ifdef FFX_DEBUG
    wchar_t             name[FFX_RESOURCE_NAME_SIZE];
#endif
} FfxTextureUAV;

/// A structure containing a unordered access view.
typedef struct FfxBufferUAV
{
    uint32_t            offset;                 ///< Offset of resource to bind in bytes.
    uint32_t            size;                   ///< Size of resource to bind in bytes.
    uint32_t            stride;                 ///< Size of resource to bind in bytes.
    FfxResourceInternal resource;               ///< Resource corresponding to the unordered access view.
#ifdef FFX_DEBUG
    wchar_t             name[FFX_RESOURCE_NAME_SIZE];
#endif
} FfxBufferUAV;

/// A structure describing a clear render job.
///
/// @ingroup SDKTypes
typedef struct FfxClearFloatJobDescription {

    float                           color[4];                               ///< The clear color of the resource.
    FfxResourceInternal             target;                                 ///< The resource to be cleared.
} FfxClearFloatJobDescription;

/// A structure describing a compute render job.
///
/// @ingroup SDKTypes
typedef struct FfxComputeJobDescription {

    FfxPipelineState                pipeline;                               ///< Compute pipeline for the render job.
    uint32_t                        dimensions[3];                          ///< Dispatch dimensions.
    FfxResourceInternal             cmdArgument;                            ///< Dispatch indirect cmd argument buffer
    uint32_t                        cmdArgumentOffset;                      ///< Dispatch indirect offset within the cmd argument buffer
    FfxTextureSRV                   srvTextures[FFX_MAX_NUM_SRVS];          ///< SRV texture resources to be bound in the compute job.
    FfxBufferSRV                    srvBuffers[FFX_MAX_NUM_SRVS];           ///< SRV buffer resources to be bound in the compute job.
    FfxTextureUAV                   uavTextures[FFX_MAX_NUM_UAVS];          ///< UAV texture resources to be bound in the compute job.
    FfxBufferUAV                    uavBuffers[FFX_MAX_NUM_UAVS];           ///< UAV buffer resources to be bound in the compute job.

    FfxConstantBuffer               cbs[FFX_MAX_NUM_CONST_BUFFERS];         ///< Constant buffers to be bound in the compute job.
    uint32_t                        flags;                                  ///< Dispatch flags
#ifdef FFX_DEBUG
    wchar_t                         cbNames[FFX_MAX_NUM_CONST_BUFFERS][FFX_RESOURCE_NAME_SIZE];
#endif
} FfxComputeJobDescription;

typedef struct FfxRasterJobDescription
{
    FfxPipelineState                pipeline;                               ///< Raster pipeline for the render job.
    uint32_t                        numVertices;
    FfxResourceInternal             renderTarget;
    FfxTextureSRV                   srvTextures[FFX_MAX_NUM_SRVS];  ///< SRV texture resources to be bound in the compute job.
    FfxTextureUAV                   uavTextures[FFX_MAX_NUM_UAVS];  ///< UAV texture resources to be bound in the compute job.

    FfxConstantBuffer               cbs[FFX_MAX_NUM_CONST_BUFFERS];         ///< Constant buffers to be bound in the compute job.
#ifdef FFX_DEBUG
    wchar_t                         cbNames[FFX_MAX_NUM_CONST_BUFFERS][FFX_RESOURCE_NAME_SIZE];
#endif
} FfxRasterJobDescription;

/// A structure describing a copy render job.
///
/// @ingroup SDKTypes
typedef struct FfxCopyJobDescription
{
    FfxResourceInternal                     src;                                    ///< Source resource for the copy.
    uint32_t                                srcOffset;                              ///< Offset into the source buffer in bytes.
    FfxResourceInternal                     dst;                                    ///< Destination resource for the copy.
    uint32_t                                dstOffset;                              ///< Offset into the destination buffer in bytes.
    uint32_t                                size;                                   ///< Number of bytes to copy (Set to 0 to copy entire buffer).
} FfxCopyJobDescription;

typedef struct FfxDiscardJobDescription {

    FfxResourceInternal                     target;                                 ///< The resource to be discarded.
} FfxDiscardJobDescription;

/// A structure describing a single render job.
///
/// @ingroup SDKTypes
typedef struct FfxGpuJobDescription{

    FfxGpuJobType       jobType;                                    ///< Type of the job.
    wchar_t             jobLabel[FFX_RESOURCE_NAME_SIZE];           ///< Job label for markers

    union {
        FfxClearFloatJobDescription clearJobDescriptor;                     ///< Clear job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFX_RENDER_JOB_CLEAR_FLOAT</i></c>.
        FfxCopyJobDescription       copyJobDescriptor;                      ///< Copy job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFX_RENDER_JOB_COPY</i></c>.
        FfxComputeJobDescription    computeJobDescriptor;                   ///< Compute job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFX_RENDER_JOB_COMPUTE</i></c>.
        FfxRasterJobDescription     rasterJobDescriptor;
        FfxBarrierDescription       barrierDescriptor;
        FfxDiscardJobDescription    discardJobDescriptor;
    };
} FfxGpuJobDescription;

#if defined(POPULATE_SHADER_BLOB_FFX)
#undef POPULATE_SHADER_BLOB_FFX
#endif // #if defined(POPULATE_SHADER_BLOB_FFX)

/// Macro definition to copy header shader blob information into its SDK structural representation
///
/// @ingroup SDKTypes
#define POPULATE_SHADER_BLOB_FFX(info, index)        \
    {                                                \
        info[index].blobData,                        \
        info[index].blobSize,                        \
        info[index].numConstantBuffers,              \
        info[index].numSRVTextures,                  \
        info[index].numUAVTextures,                  \
        info[index].numSRVBuffers,                   \
        info[index].numUAVBuffers,                   \
        info[index].numSamplers,                     \
        info[index].numRTAccelerationStructures,     \
        info[index].constantBufferNames,             \
        info[index].constantBufferBindings,          \
        info[index].constantBufferCounts,            \
        info[index].constantBufferSpaces,            \
        info[index].srvTextureNames,                 \
        info[index].srvTextureBindings,              \
        info[index].srvTextureCounts,                \
        info[index].srvTextureSpaces,                \
        info[index].uavTextureNames,                 \
        info[index].uavTextureBindings,              \
        info[index].uavTextureCounts,                \
        info[index].uavTextureSpaces,                \
        info[index].srvBufferNames,                  \
        info[index].srvBufferBindings,               \
        info[index].srvBufferCounts,                 \
        info[index].srvBufferSpaces,                 \
        info[index].uavBufferNames,                  \
        info[index].uavBufferBindings,               \
        info[index].uavBufferCounts,                 \
        info[index].uavBufferSpaces,                 \
        info[index].samplerNames,                    \
        info[index].samplerBindings,                 \
        info[index].samplerCounts,                   \
        info[index].samplerSpaces,                   \
        info[index].rtAccelerationStructureNames,    \
        info[index].rtAccelerationStructureBindings, \
        info[index].rtAccelerationStructureCounts,   \
        info[index].rtAccelerationStructureSpaces    \
    }

/// A single shader blob and a description of its resources.
///
/// @ingroup SDKTypes
typedef struct FfxShaderBlob {

    const uint8_t* data;                                ///< A pointer to the blob
    uint32_t  size;                                     ///< Size in bytes.

    uint32_t  cbvCount;                                 ///< Number of CBs.
    uint32_t  srvTextureCount;                          ///< Number of SRV Textures.
    uint32_t  uavTextureCount;                          ///< Number of UAV Textures.
    uint32_t  srvBufferCount;                           ///< Number of SRV Buffers.
    uint32_t  uavBufferCount;                           ///< Number of UAV Buffers.
    uint32_t  samplerCount;                             ///< Number of Samplers.
    uint32_t  rtAccelStructCount;                       ///< Number of RT Acceleration structures.

    // constant buffers
    const char** boundConstantBufferNames;
    const uint32_t* boundConstantBuffers;               ///< Pointer to an array of bound ConstantBuffers.
    const uint32_t* boundConstantBufferCounts;          ///< Pointer to an array of bound ConstantBuffer resource counts
    const uint32_t* boundConstantBufferSpaces;          ///< Pointer to an array of bound ConstantBuffer resource spaces

    // srv textures
    const char** boundSRVTextureNames;
    const uint32_t* boundSRVTextures;                   ///< Pointer to an array of bound SRV resources.
    const uint32_t* boundSRVTextureCounts;              ///< Pointer to an array of bound SRV resource counts
    const uint32_t* boundSRVTextureSpaces;              ///< Pointer to an array of bound SRV resource spaces

    // uav textures
    const char** boundUAVTextureNames;
    const uint32_t* boundUAVTextures;                   ///< Pointer to an array of bound UAV texture resources.
    const uint32_t* boundUAVTextureCounts;              ///< Pointer to an array of bound UAV texture resource counts
    const uint32_t* boundUAVTextureSpaces;              ///< Pointer to an array of bound UAV texture resource spaces

    // srv buffers
    const char** boundSRVBufferNames;
    const uint32_t* boundSRVBuffers;                    ///< Pointer to an array of bound SRV buffer resources.
    const uint32_t* boundSRVBufferCounts;               ///< Pointer to an array of bound SRV buffer resource counts
    const uint32_t* boundSRVBufferSpaces;               ///< Pointer to an array of bound SRV buffer resource spaces

    // uav buffers
    const char** boundUAVBufferNames;
    const uint32_t* boundUAVBuffers;                    ///< Pointer to an array of bound UAV buffer resources.
    const uint32_t* boundUAVBufferCounts;               ///< Pointer to an array of bound UAV buffer resource counts
    const uint32_t* boundUAVBufferSpaces;               ///< Pointer to an array of bound UAV buffer resource spaces

    // samplers
    const char** boundSamplerNames;
    const uint32_t* boundSamplers;                      ///< Pointer to an array of bound sampler resources.
    const uint32_t* boundSamplerCounts;                 ///< Pointer to an array of bound sampler resource counts
    const uint32_t* boundSamplerSpaces;                 ///< Pointer to an array of bound sampler resource spaces

    // rt acceleration structures
    const char** boundRTAccelerationStructureNames;
    const uint32_t* boundRTAccelerationStructures;      ///< Pointer to an array of bound UAV buffer resources.
    const uint32_t* boundRTAccelerationStructureCounts; ///< Pointer to an array of bound UAV buffer resource counts
    const uint32_t* boundRTAccelerationStructureSpaces; ///< Pointer to an array of bound UAV buffer resource spaces

} FfxShaderBlob;

/// A structure describing the parameters passed from the
/// presentation thread to the ui composition callback function.
///
/// @ingroup SDKTypes
typedef struct FfxPresentCallbackDescription
{
    FfxDevice       device;                    ///< The active device
    FfxCommandList  commandList;               ///< The command list on which to register render commands
    FfxApiResource  currentBackBuffer;         ///< The backbuffer resource with scene information
    FfxApiResource  currentUI;                 ///< Optional UI texture (when doing backbuffer + ui blend)
    FfxApiResource  outputSwapChainBuffer;     ///< The swapchain target into which to render ui composition
    bool            isInterpolatedFrame;       ///< Whether this is an interpolated or real frame
    bool            usePremulAlpha;            ///< Toggles whether UI gets premultiplied alpha blending or not
    uint64_t        frameID;
} FfxPresentCallbackDescription;

/// A structure describing the parameters to pass to frame generation passes.
///
/// @ingroup SDKTypes
typedef struct FfxFrameGenerationDispatchDescription {
    FfxCommandList                  commandList;                    ///< The command list on which to register render commands
    FfxApiResource                  presentColor;                   ///< The current presentation color, this will be used as interpolation source data.
    FfxApiResource                  outputs[4];                     ///< Interpolation destination targets (1 for each frame in numInterpolatedFrames)
    uint32_t                        numInterpolatedFrames;          ///< The number of frames to interpolate from the passed in color target
    bool                            reset;                          ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    FfxApiBackbufferTransferFunction   backBufferTransferFunction;     ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                           minMaxLuminance[2];             ///< Min and max luminance values, used when converting HDR colors to linear RGB
    FfxApiRect2D                       interpolationRect;              ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                        frameID;
} FfxFrameGenerationDispatchDescription;

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus
