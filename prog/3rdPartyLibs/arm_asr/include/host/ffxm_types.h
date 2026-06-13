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

#include <stdint.h>
#include <cstddef>

namespace arm
{

///
/// @defgroup ffxmSDK SDK
/// The FidelityFX SDK
///

/// @defgroup ffxmHost Host
/// The FidelityFX SDK host (CPU-side) References
///
/// @ingroup ffxmSDK

/// @defgroup Defines Defines
/// Top level defines used by the FidelityFX SDK
///
/// @ingroup ffxmHost

#if defined(__clang__) || defined(__GNUC__) || defined(__ANDROID__)
/// FidelityFX exported functions
///
/// @ingroup Defines
#define FFXM_API
#else
/// FidelityFX exported functions
///
/// @ingroup Defines
#define FFXM_API __declspec(dllexport)
#endif // #if defined (FFXM_GCC)

/// Maximum supported number of simultaneously bound SRVs.
///
/// @ingroup Defines
#define FFXM_MAX_NUM_SRVS            16

/// Maximum supported number of simultaneously bound UAVs.
///
/// @ingroup Defines
#define FFXM_MAX_NUM_UAVS            16

/// Maximum supported number of simultaneously bound RenderTargets.
///
/// @ingroup Defines
#define FFXM_MAX_NUM_RTS				8

/// Maximum number of constant buffers bound.
///
/// @ingroup Defines
#define FFXM_MAX_NUM_CONST_BUFFERS   3

/// Maximum size of bound constant buffers.
#define FFXM_MAX_CONST_SIZE          256

/// Maximum number of characters in a resource name
///
/// @ingroup Defines
#define FFXM_RESOURCE_NAME_SIZE      64

/// Maximum number of queued frames in the backend
///
/// @ingroup Defines
#define FFXM_MAX_QUEUED_FRAMES          (4)

/// Maximum number of resources per effect context
///
/// @ingroup Defines
#define FFXM_MAX_RESOURCE_COUNT         (64)

/// Maximum number of passes per effect component
///
/// @ingroup Defines
#define FFXM_MAX_PASS_COUNT             (50)

/// Total ring buffer size needed for a single effect context
///
/// @ingroup Defines
#define FFXM_RING_BUFFER_SIZE           (FFXM_MAX_QUEUED_FRAMES * FFXM_MAX_PASS_COUNT * FFXM_MAX_RESOURCE_COUNT)

/// Size of constant buffer entry in the ring buffer table
///
/// @ingroup Defines
#define FFXM_BUFFER_SIZE                (768)

/// Total ring buffer memory size for a single effect context
///
/// @ingroup Defines
#define FFXM_RING_BUFFER_MEM_BLOCK_SIZE (FFXM_RING_BUFFER_SIZE * FFXM_BUFFER_SIZE)

/// Maximum number of barriers per flush
///
/// @ingroup Defines
#define FFXM_MAX_BARRIERS               (16)

/// Maximum number of GPU jobs per submission
///
/// @ingroup Defines
#define FFXM_MAX_GPU_JOBS               (64)

/// Maximum number of samplers supported
///
/// @ingroup Defines
#define FFXM_MAX_SAMPLERS               (16)

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
/// @ingroup ffxmHost

/// A typedef for a boolean value.
///
/// @ingroup CPUTypes
typedef bool FfxmBoolean;

/// A typedef for a unsigned 8bit integer.
///
/// @ingroup CPUTypes
typedef uint8_t FfxmUInt8;

/// A typedef for a unsigned 16bit integer.
///
/// @ingroup CPUTypes
typedef uint16_t FfxmUInt16;

/// A typedef for a unsigned 32bit integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxmUInt32;

/// A typedef for a unsigned 64bit integer.
///
/// @ingroup CPUTypes
typedef uint64_t FfxmUInt64;

/// A typedef for a signed 8bit integer.
///
/// @ingroup CPUTypes
typedef int8_t FfxmInt8;

/// A typedef for a signed 16bit integer.
///
/// @ingroup CPUTypes
typedef int16_t FfxmInt16;

/// A typedef for a signed 32bit integer.
///
/// @ingroup CPUTypes
typedef int32_t FfxmInt32;

/// A typedef for a signed 64bit integer.
///
/// @ingroup CPUTypes
typedef int64_t FfxmInt64;

/// A typedef for a floating point value.
///
/// @ingroup CPUTypes
typedef float FfxmFloat32;

/// A typedef for a 2-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxmFloat32x2[2];

/// A typedef for a 3-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxmFloat32x3[3];

/// A typedef for a 4-dimensional floating point value.
///
/// @ingroup CPUTypes
typedef float FfxmFloat32x4[4];

/// A typedef for a 2-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxmUInt32x2[2];

/// A typedef for a 3-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxmUInt32x3[3];

/// A typedef for a 4-dimensional 32bit unsigned integer.
///
/// @ingroup CPUTypes
typedef uint32_t FfxmUInt32x4[4];

/// @defgroup SDKTypes SDK Types
/// Structure and Enumeration definitions used by the FidelityFX SDK
///
/// @ingroup ffxmHost


/// An enumeration of surface formats.
///
/// @ingroup SDKTypes
typedef enum FfxmSurfaceFormat {

    FFXM_SURFACE_FORMAT_UNKNOWN,                     ///< Unknown format
    FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS,       ///< 32 bit per channel, 4 channel typeless format
    FFXM_SURFACE_FORMAT_R32G32B32A32_UINT,           ///< 32 bit per channel, 4 channel uint format
    FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT,          ///< 32 bit per channel, 4 channel float format
    FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT,          ///< 16 bit per channel, 4 channel float format
    FFXM_SURFACE_FORMAT_R32G32_FLOAT,                ///< 32 bit per channel, 2 channel float format
    FFXM_SURFACE_FORMAT_R8_UINT,                     ///< 8 bit per channel, 1 channel float format
    FFXM_SURFACE_FORMAT_R32_UINT,                    ///< 32 bit per channel, 1 channel float format
    FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS,           ///<  8 bit per channel, 4 channel float format
    FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM,              ///<  8 bit per channel, 4 channel unsigned normalized format
    FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM,              ///<  8 bit per channel, 4 channel signed normalized format
    FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB,               ///<  8 bit per channel, 4 channel srgb normalized
    FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS,           ///<  8 bit per channel, 4 channel typeless format (BGRA channel order)
    FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM,              ///<  8 bit per channel, 4 channel unsigned normalized format (BGRA channel order)
    FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB,               ///<  8 bit per channel, 4 channel srgb normalized (BGRA channel order)
    FFXM_SURFACE_FORMAT_R11G11B10_FLOAT,             ///< 32 bit 3 channel float format
    FFXM_SURFACE_FORMAT_R16G16_FLOAT,                ///< 16 bit per channel, 2 channel float format
    FFXM_SURFACE_FORMAT_R16G16_UINT,                 ///< 16 bit per channel, 2 channel unsigned int format
    FFXM_SURFACE_FORMAT_R16_FLOAT,                   ///< 16 bit per channel, 1 channel float format
    FFXM_SURFACE_FORMAT_R16_UINT,                    ///< 16 bit per channel, 1 channel unsigned int format
    FFXM_SURFACE_FORMAT_R16_UNORM,                   ///< 16 bit per channel, 1 channel unsigned normalized format
    FFXM_SURFACE_FORMAT_R16_SNORM,                   ///< 16 bit per channel, 1 channel signed normalized format
    FFXM_SURFACE_FORMAT_R8_UNORM,                    ///<  8 bit per channel, 1 channel unsigned normalized format
	FFXM_SURFACE_FORMAT_R8_SNORM,					 ///<  8 bit per channel, 1 channel signed normalized format
    FFXM_SURFACE_FORMAT_R8G8_UNORM,                  ///<  8 bit per channel, 2 channel unsigned normalized format
    FFXM_SURFACE_FORMAT_R32_FLOAT                    ///< 32 bit per channel, 1 channel float format
} FfxmSurfaceFormat;

/// An enumeration of resource usage.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceUsage {

    FFXM_RESOURCE_USAGE_READ_ONLY = 0,               ///< No usage flags indicate a resource is read only.
    FFXM_RESOURCE_USAGE_RENDERTARGET = (1<<0),       ///< Indicates a resource will be used as render target.
    FFXM_RESOURCE_USAGE_UAV = (1<<1),                ///< Indicates a resource will be used as UAV.
    FFXM_RESOURCE_USAGE_DEPTHTARGET = (1<<2),        ///< Indicates a resource will be used as depth target.
    FFXM_RESOURCE_USAGE_INDIRECT = (1<<3),           ///< Indicates a resource will be used as indirect argument buffer
    FFXM_RESOURCE_USAGE_ARRAYVIEW = (1<<4),          ///< Indicates a resource that will generate array views. Works on 2D and cubemap textures
} FfxmResourceUsage;

/// An enumeration of resource states.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceStates {

    FFXM_RESOURCE_STATE_UNORDERED_ACCESS = (1<<0),   ///< Indicates a resource is in the state to be used as UAV.
    FFXM_RESOURCE_STATE_COMPUTE_READ = (1 << 1),     ///< Indicates a resource is in the state to be read by compute shaders.
    FFXM_RESOURCE_STATE_PIXEL_READ = (1 << 2),       ///< Indicates a resource is in the state to be read by pixel shaders.
    FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ = (FFXM_RESOURCE_STATE_PIXEL_READ | FFXM_RESOURCE_STATE_COMPUTE_READ), ///< Indicates a resource is in the state to be read by pixel or compute shaders.
    FFXM_RESOURCE_STATE_COPY_SRC = (1 << 3),         ///< Indicates a resource is in the state to be used as source in a copy command.
    FFXM_RESOURCE_STATE_COPY_DEST = (1 << 4),        ///< Indicates a resource is in the state to be used as destination in a copy command.
    FFXM_RESOURCE_STATE_GENERIC_READ = (FFXM_RESOURCE_STATE_COPY_SRC | FFXM_RESOURCE_STATE_COMPUTE_READ),  ///< Indicates a resource is in generic (slow) read state.
    FFXM_RESOURCE_STATE_INDIRECT_ARGUMENT = (1 << 5),///< Indicates a resource is in the state to be used as an indirect command argument
	FFXM_RESOURCE_STATE_PIXEL_WRITE = (1 << 6), ///< Indicates a resource is in the state to be used as a RenderTarget
} FfxmResourceStates;

/// An enumeration of surface dimensions.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceDimension {

    FFXM_RESOURCE_DIMENSION_TEXTURE_1D,              ///< A resource with a single dimension.
    FFXM_RESOURCE_DIMENSION_TEXTURE_2D,              ///< A resource with two dimensions.
} FfxmResourceDimension;

/// An enumeration of resource view dimensions.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceViewDimension
{
    FFXM_RESOURCE_VIEW_DIMENSION_BUFFER,             ///< A resource view on a buffer.
    FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_1D,         ///< A resource view on a single dimension.
    FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_1D_ARRAY,   ///< A resource view on a single dimensional array.
    FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_2D,         ///< A resource view on two dimensions.
    FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_2D_ARRAY,  ///< A resource view on two dimensional array.
    FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_3D,         ///< A resource view on three dimensions.
} FfxmResourceViewDimension;

/// An enumeration of surface dimensions.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceFlags {

    FFXM_RESOURCE_FLAGS_NONE             = 0,            ///< No flags.
    FFXM_RESOURCE_FLAGS_ALIASABLE        = (1 << 0),     ///< A bit indicating a resource does not need to persist across frames.
    FFXM_RESOURCE_FLAGS_UNDEFINED        = (1 << 1),     ///< Special case flag used internally when importing resources that require additional setup
} FfxmResourceFlags;

/// An enumeration of all resource view types.
///
/// @ingroup SDKTypes
typedef enum FfxmResourceViewType {

    FFXM_RESOURCE_VIEW_UNORDERED_ACCESS,             ///< The resource view is an unordered access view (UAV).
    FFXM_RESOURCE_VIEW_SHADER_READ,                  ///< The resource view is a shader resource view (SRV).
} FfxmResourceViewType;

/// The type of filtering to perform when reading a texture.
///
/// @ingroup SDKTypes
typedef enum FfxmFilterType {

    FFXM_FILTER_TYPE_MINMAGMIP_POINT,        ///< Point sampling.
    FFXM_FILTER_TYPE_MINMAGMIP_LINEAR,       ///< Sampling with interpolation.
    FFXM_FILTER_TYPE_MINMAGLINEARMIP_POINT,  ///< Use linear interpolation for minification and magnification; use point sampling for mip-level sampling.
} FfxmFilterType;

/// The address mode used when reading a texture.
///
/// @ingroup SDKTypes
typedef enum FfxmAddressMode {

    FFXM_ADDRESS_MODE_WRAP,                  ///< Wrap when reading texture.
    FFXM_ADDRESS_MODE_MIRROR,                ///< Mirror when reading texture.
    FFXM_ADDRESS_MODE_CLAMP,                 ///< Clamp when reading texture.
    FFXM_ADDRESS_MODE_BORDER,                ///< Border color when reading texture.
    FFXM_ADDRESS_MODE_MIRROR_ONCE,           ///< Mirror once when reading texture.
} FfxmAddressMode;

/// An enumeration of all supported shader models.
///
/// @ingroup SDKTypes
typedef enum FfxmShaderModel {

    FFXM_SHADER_MODEL_5_1,                           ///< Shader model 5.1.
    FFXM_SHADER_MODEL_6_0,                           ///< Shader model 6.0.
    FFXM_SHADER_MODEL_6_1,                           ///< Shader model 6.1.
    FFXM_SHADER_MODEL_6_2,                           ///< Shader model 6.2.
    FFXM_SHADER_MODEL_6_3,                           ///< Shader model 6.3.
    FFXM_SHADER_MODEL_6_4,                           ///< Shader model 6.4.
    FFXM_SHADER_MODEL_6_5,                           ///< Shader model 6.5.
    FFXM_SHADER_MODEL_6_6,                           ///< Shader model 6.6.
    FFXM_SHADER_MODEL_6_7,                           ///< Shader model 6.7.
} FfxmShaderModel;

// An enumeration for different resource types
///
/// @ingroup SDKTypes
typedef enum FfxmResourceType {

    FFXM_RESOURCE_TYPE_BUFFER,                       ///< The resource is a buffer.
    FFXM_RESOURCE_TYPE_TEXTURE1D,                    ///< The resource is a 1-dimensional texture.
    FFXM_RESOURCE_TYPE_TEXTURE2D,                    ///< The resource is a 2-dimensional texture.
    FFXM_RESOURCE_TYPE_TEXTURE_CUBE,                 ///< The resource is a cube map.
    FFXM_RESOURCE_TYPE_TEXTURE3D,                    ///< The resource is a 3-dimensional texture.
} FfxmResourceType;

/// An enumeration for different heap types
///
/// @ingroup SDKTypes
typedef enum FfxmHeapType {

    FFXM_HEAP_TYPE_DEFAULT = 0,                      ///< Local memory.
    FFXM_HEAP_TYPE_UPLOAD                            ///< Heap used for uploading resources.
} FfxmHeapType;

/// An enumeration for different render job types
///
/// @ingroup SDKTypes
typedef enum FfxmGpuJobType {

    FFXM_GPU_JOB_CLEAR_FLOAT = 0,                 ///< The GPU job is performing a floating-point clear.
    FFXM_GPU_JOB_COPY = 1,                        ///< The GPU job is performing a copy.
    FFXM_GPU_JOB_COMPUTE = 2,                     ///< The GPU job is performing a compute dispatch.
	FFXM_GPU_JOB_FRAGMENT = 3,					 ///< The GPU job is performing a fragment pass.
} FfxmGpuJobType;

/// An enumeration for various descriptor types
///
/// @ingroup SDKTypes
typedef enum FfxmDescriptorType {

    //FFXM_DESCRIPTOR_CBV = 0,   // All CBVs currently mapped to root signature
    //FFXM_DESCRIPTOR_SAMPLER,   // All samplers currently static
    FFXM_DESCRIPTOR_TEXTURE_SRV = 0,
    FFXM_DESCRIPTOR_BUFFER_SRV,
    FFXM_DESCRIPTOR_TEXTURE_UAV,
    FFXM_DESCRIPTOR_BUFFER_UAV,
} FfxmDescriptiorType;

/// An enumeration for view binding stages
///
/// @ingroup SDKTypes
typedef enum FfxmBindStage {

    FFXM_BIND_PIXEL_SHADER_STAGE = 1 << 0,
    FFXM_BIND_VERTEX_SHADER_STAGE = 1 << 1,
    FFXM_BIND_COMPUTE_SHADER_STAGE = 1 << 2,

} FfxmBindStage;

/// An enumeration for barrier types
///
/// @ingroup SDKTypes
typedef enum FfxmBarrierType
{
    FFXM_BARRIER_TYPE_TRANSITION = 0,
    FFXM_BARRIER_TYPE_UAV,
} FfxmBarrierType;

/// An enumeration for message types that can be passed
///
/// @ingroup SDKTypes
typedef enum FfxmMsgType {
    FFXM_MESSAGE_TYPE_ERROR      = 0,
    FFXM_MESSAGE_TYPE_WARNING    = 1,
    FFXM_MESSAGE_TYPE_COUNT
} FfxmMsgType;

/// A typedef representing the graphics device.
///
/// @ingroup SDKTypes
typedef void* FfxmDevice;

/// A typedef representing a command list or command buffer.
///
/// @ingroup SDKTypes
typedef void* FfxmCommandList;

/// A typedef for a root signature.
///
/// @ingroup SDKTypes
typedef void* FfxmRootSignature;

/// A typedef for a command signature, used for indirect workloads
///
/// @ingroup SDKTypes
typedef void* FfxmCommandSignature;

/// A typedef for a pipeline state object.
///
/// @ingroup SDKTypes
typedef void* FfxmPipeline;

/// A structure encapsulating a collection of device capabilities.
///
/// @ingroup SDKTypes
typedef struct FfxmDeviceCapabilities {

    FfxmShaderModel                  minimumSupportedShaderModel;            ///< The minimum shader model supported by the device.
    uint32_t                        waveLaneCountMin;                       ///< The minimum supported wavefront width.
    uint32_t                        waveLaneCountMax;                       ///< The maximum supported wavefront width.
    bool                            fp16Supported;                          ///< The device supports FP16 in hardware.
    bool                            raytracingSupported;                    ///< The device supports ray tracing.
} FfxmDeviceCapabilities;

/// A structure encapsulating a 2-dimensional point, using 32bit unsigned integers.
///
/// @ingroup SDKTypes
typedef struct FfxmDimensions2D {

    uint32_t                        width;                                  ///< The width of a 2-dimensional range.
    uint32_t                        height;                                 ///< The height of a 2-dimensional range.
} FfxmDimensions2D;

/// A structure encapsulating a 2-dimensional point.
///
/// @ingroup SDKTypes
typedef struct FfxmIntCoords2D {

    int32_t                         x;                                      ///< The x coordinate of a 2-dimensional point.
    int32_t                         y;                                      ///< The y coordinate of a 2-dimensional point.
} FfxmIntCoords2D;

/// A structure encapsulating a 2-dimensional set of floating point coordinates.
///
/// @ingroup SDKTypes
typedef struct FfxmFloatCoords2D {

    float                           x;                                      ///< The x coordinate of a 2-dimensional point.
    float                           y;                                      ///< The y coordinate of a 2-dimensional point.
} FfxmFloatCoords2D;

/// A structure describing a resource.
///
/// @ingroup SDKTypes
typedef struct FfxmResourceDescription {

    FfxmResourceType                 type;                                   ///< The type of the resource.
    FfxmSurfaceFormat                format;                                 ///< The surface format.
    union {
        uint32_t                    width;                                  ///< The width of the texture resource.
        uint32_t                    size;                                   ///< The size of the buffer resource.
    };

    union {
        uint32_t                    height;                                 ///< The height of the texture resource.
        uint32_t                    stride;                                 ///< The stride of the buffer resource.
    };

    union {
        uint32_t                    depth;                                  ///< The depth of the texture resource.
        uint32_t                    alignment;                              ///< The alignment of the buffer resource.
    };

    uint32_t                        mipCount;                               ///< Number of mips (or 0 for full mipchain).
    FfxmResourceFlags                flags;                                  ///< A set of <c><i>FfxmResourceFlags</i></c> flags.
    FfxmResourceUsage                usage;                                  ///< Resource usage flags.
} FfxmResourceDescription;

/// An outward facing structure containing a resource
///
/// @ingroup SDKTypes
typedef struct FfxmResource {
    void*                           resource;                               ///< pointer to the resource.
    FfxmResourceDescription          description;
    FfxmResourceStates               state;
    wchar_t                         name[FFXM_RESOURCE_NAME_SIZE];           ///< (optional) Resource name.
} FfxmResource;

/// An internal structure containing a handle to a resource and resource views
///
/// @ingroup SDKTypes
typedef struct FfxmResourceInternal {
    int32_t                         internalIndex;                          ///< The index of the resource.
} FfxmResourceInternal;

/// An internal structure housing all that is needed for backend resource descriptions
///
/// @ingroup SDKTypes
typedef struct FfxmInternalResourceDescription {

    uint32_t                    id;
    const wchar_t* name;
    FfxmResourceType             type;
    FfxmResourceUsage            usage;
    FfxmSurfaceFormat            format;
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    mipCount;
    FfxmResourceFlags            flags;
    uint32_t                    initDataSize;
    void* initData;
} FfxmInternalResourceDescription;

/// A structure defining the view to create
///
/// @ingroup SDKTypes
typedef struct FfxmViewDescription
{
    bool                        uavView;                    ///< Indicates that the view is a UAV.
    FfxmResourceViewDimension    viewDimension;              ///< The view dimension to map
    union {
        int32_t mipLevel;                                   ///< The mip level of the view, (-1) for default
        int32_t firstElement;                               ///< The first element of a buffer view, (-1) for default
    };

    union {
        int32_t arraySize;                                  ///< The array size of the view, (-1) for full depth/array size
        int32_t elementCount;                               ///< The number of elements in a buffer view, (-1) for full depth/array size
    };

    int32_t                     firstSlice;                 ///< The first slice to map to, (-1) for default first slice
    wchar_t name[64];
} FfxmViewDescription;

static FfxmViewDescription s_FfxmViewDescInit = { false, FFXM_RESOURCE_VIEW_DIMENSION_TEXTURE_2D, -1, -1, -1, L"" };

/// A structure defining a resource bind point
///
/// @ingroup SDKTypes
typedef struct FfxmResourceBinding
{
    uint32_t    slotIndex;
    uint32_t    resourceIdentifier;
    uint32_t    bindCount;
    uint32_t    bindSet;
    wchar_t     name[64];
}FfxmResourceBinding;

/// A structure encapsulating a single pass of an algorithm.
///
/// @ingroup SDKTypes
typedef struct FfxmPipelineState {

    FfxmRootSignature                rootSignature;                                      ///< The pipelines rootSignature
    FfxmCommandSignature             cmdSignature;                                       ///< The command signature used for indirect workloads
    FfxmPipeline                     pipeline;                                           ///< The pipeline object
    uint32_t                        uavTextureCount;                                    ///< Count of Texture UAVs used in this pipeline
    uint32_t                        srvTextureCount;                                    ///< Count of Texture SRVs used in this pipeline
    uint32_t                        srvBufferCount;                                     ///< Count of Buffer SRV used in this pipeline
    uint32_t                        uavBufferCount;                                     ///< Count of Buffer UAVs used in this pipeline
    uint32_t                        constCount;                                         ///< Count of constant buffers used in this pipeline
    uint32_t                        rtCount;
    uint32_t                        descriptorSetCount;                                 ///< Count of descriptor set used in this pipeline

    FfxmResourceBinding              uavTextureBindings[FFXM_MAX_NUM_UAVS];               ///< Array of ResourceIdentifiers bound as texture UAVs
    FfxmResourceBinding              srvTextureBindings[FFXM_MAX_NUM_SRVS];               ///< Array of ResourceIdentifiers bound as texture SRVs
    FfxmResourceBinding              srvBufferBindings[FFXM_MAX_NUM_SRVS];                ///< Array of ResourceIdentifiers bound as buffer SRVs
    FfxmResourceBinding              uavBufferBindings[FFXM_MAX_NUM_UAVS];                ///< Array of ResourceIdentifiers bound as buffer UAVs
    FfxmResourceBinding              constantBufferBindings[FFXM_MAX_NUM_CONST_BUFFERS];  ///< Array of ResourceIdentifiers bound as CBs
	FfxmResourceBinding				rtBindings[FFXM_MAX_NUM_RTS];						///< Array of ResourceIdentifiers bound as RenderTargets
} FfxmPipelineState;

/// A structure containing the data required to create a resource.
///
/// @ingroup SDKTypes
typedef struct FfxmCreateResourceDescription {

    FfxmHeapType                     heapType;                               ///< The heap type to hold the resource, typically <c><i>FFXM_HEAP_TYPE_DEFAULT</i></c>.
    FfxmResourceDescription          resourceDescription;                    ///< A resource description.
    FfxmResourceStates               initalState;                            ///< The initial resource state.
    uint32_t                        initDataSize;                           ///< Size of initial data buffer.
    void*                           initData;                               ///< Buffer containing data to fill the resource.
    const wchar_t*                  name;                                   ///< Name of the resource.
    uint32_t                        id;                                     ///< Internal resource ID.
} FfxmCreateResourceDescription;

/// A structure containing the data required to create sampler mappings
///
/// @ingroup SDKTypes
typedef struct FfxmSamplerDescription {

    uint32_t         binding;
    FfxmFilterType   filter;
    FfxmAddressMode  addressModeU;
    FfxmAddressMode  addressModeV;
    FfxmAddressMode  addressModeW;
    FfxmBindStage    stage;
} FfxmSamplerDescription;

/// A structure containing the data required to create root constant buffer mappings
///
/// @ingroup SDKTypes
typedef struct FfxmRootConstantDescription
{
    uint32_t      size;
    FfxmBindStage  stage;
} FfxmRootConstantDescription;

/// A structure containing the description used to create a
/// <c><i>FfxmPipeline</i></c> structure.
///
/// A pipeline is the name given to a shader and the collection of state that
/// is required to dispatch it. In the context of the FidelityFX SDK and its architecture
/// this means that a <c><i>FfxmPipelineDescription</i></c> will map to either a
/// monolithic object in an explicit API (such as a
/// <c><i>PipelineStateObject</i></c> in DirectX 12). Or a shader and some
/// ancillary API objects (in something like DirectX 11).
///
/// The <c><i>contextFlags</i></c> field contains a copy of the flags passed
/// to <c><i>ffxmContextCreate</i></c> via the <c><i>flags</i></c> field of
/// the <c><i>Ffxm<Effect>InitializationParams</i></c> structure. These flags are
/// used to determine which permutation of a pipeline for a specific
/// <c><i>Ffxm<Effect>Pass</i></c> should be used to implement the features required
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
typedef struct FfxmPipelineDescription {

    uint32_t                            contextFlags;                   ///< A collection of <c><i>FfxmInitializationFlagBits</i></c> which were passed to the context.
    const FfxmSamplerDescription*        samplers;                       ///< A collection of samplers to use when building the root signature for the pipeline
    size_t                              samplerCount;                   ///< Number of samplers to create for the pipeline
    const FfxmRootConstantDescription*   rootConstants;                  ///< A collection of root constant descriptions to use when building the root signature for the pipeline
    uint32_t                            rootConstantBufferCount;        ///< Number of root constant buffers to create for the pipeline
    wchar_t                             name[64];                       ///< Pipeline name with which to name the pipeline object
    FfxmBindStage                        stage;                          ///< The stage(s) for which this pipeline is being built
    uint32_t                            indirectWorkload;               ///< Whether this pipeline has an indirect workload
} FfxmPipelineDescription;

/// A structure containing the data required to create a barrier
///
/// @ingroup SDKTypes
typedef struct FfxmBarrierDescription
{
    FfxmBarrierType    barrierType;
    FfxmResourceStates currentState;
    FfxmResourceStates newState;
    uint32_t          subResourceID;
} FfxmBarrierDescription;


/// A structure containing a constant buffer.
///
/// @ingroup SDKTypes
typedef struct FfxmConstantBuffer {

    uint32_t                        num32BitEntries;                        ///< The size (expressed in 32-bit chunks) stored in data.
    uint32_t                        data[FFXM_MAX_CONST_SIZE];               ///< Constant buffer data
}FfxmConstantBuffer;

/// A structure describing a clear render job.
///
/// @ingroup SDKTypes
typedef struct FfxmClearFloatJobDescription {

    float                           color[4];                               ///< The clear color of the resource.
    FfxmResourceInternal             target;                                 ///< The resource to be cleared.
} FfxmClearFloatJobDescription;

/// A structure describing a compute render job.
///
/// @ingroup SDKTypes
typedef struct FfxmComputeJobDescription {

    FfxmPipelineState                pipeline;                               ///< Compute pipeline for the render job.
    uint32_t                        dimensions[3];                          ///< Dispatch dimensions.
    FfxmResourceInternal             cmdArgument;                            ///< Dispatch indirect cmd argument buffer
    uint32_t                        cmdArgumentOffset;                      ///< Dispatch indirect offset within the cmd argument buffer
    FfxmResourceInternal             srvTextures[FFXM_MAX_NUM_SRVS];          ///< SRV texture resources to be bound in the compute job.
    wchar_t                         srvTextureNames[FFXM_MAX_NUM_SRVS][64];
    FfxmResourceInternal             uavTextures[FFXM_MAX_NUM_UAVS];          ///< UAV texture resources to be bound in the compute job.
    uint32_t                        uavTextureMips[FFXM_MAX_NUM_UAVS];       ///< Mip level of UAV texture resources to be bound in the compute job.
    wchar_t                         uavTextureNames[FFXM_MAX_NUM_UAVS][64];
    FfxmResourceInternal             srvBuffers[FFXM_MAX_NUM_SRVS];           ///< SRV buffer resources to be bound in the compute job.
    wchar_t                         srvBufferNames[FFXM_MAX_NUM_SRVS][64];
    FfxmResourceInternal             uavBuffers[FFXM_MAX_NUM_UAVS];           ///< UAV buffer resources to be bound in the compute job.
    wchar_t                         uavBufferNames[FFXM_MAX_NUM_UAVS][64];
    FfxmConstantBuffer               cbs[FFXM_MAX_NUM_CONST_BUFFERS];         ///< Constant buffers to be bound in the compute job.
    wchar_t                         cbNames[FFXM_MAX_NUM_CONST_BUFFERS][64];
} FfxmComputeJobDescription;

/// A structure describing a fragment render job.
///
/// @ingroup SDKTypes
typedef struct FfxmFragmentJobDescription
{
	FfxmPipelineState *pipeline;								///< Fragment pipeline for the render job.
	uint32_t viewport[2];									///< Viewport dimensions.
	FfxmResourceInternal srvTextures[FFXM_MAX_NUM_SRVS];		///< SRV texture resources to be bound in the fragment job.
	wchar_t srvTextureNames[FFXM_MAX_NUM_SRVS][64];
	FfxmResourceInternal uavTextures[FFXM_MAX_NUM_UAVS];		///< UAV texture resources to be bound in the fragment job.
	uint32_t uavTextureMips[FFXM_MAX_NUM_UAVS];				///< Mip level of UAV texture resources to be bound in the fragment job.
	wchar_t uavTextureNames[FFXM_MAX_NUM_UAVS][64];
	FfxmResourceInternal rtTextures[FFXM_MAX_NUM_RTS];		///< RenderTargets to be bound in the fragment job
	wchar_t rtTextureNames[FFXM_MAX_NUM_RTS][64];
	FfxmConstantBuffer cbs[FFXM_MAX_NUM_CONST_BUFFERS];		///< Constant buffers to be bound in the fragment job.
	wchar_t cbNames[FFXM_MAX_NUM_CONST_BUFFERS][64];
} FfxmFragmentJobDescription;

/// A structure describing a copy render job.
///
/// @ingroup SDKTypes
typedef struct FfxmCopyJobDescription
{
    FfxmResourceInternal                     src;                                    ///< Source resource for the copy.
    FfxmResourceInternal                     dst;                                    ///< Destination resource for the copy.
} FfxmCopyJobDescription;

/// A structure describing a single render job.
///
/// @ingroup SDKTypes
typedef struct FfxmGpuJobDescription{

    FfxmGpuJobType                jobType;                                    ///< Type of the job.
    union {
        FfxmClearFloatJobDescription clearJobDescriptor;                     ///< Clear job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFXM_RENDER_JOB_CLEAR_FLOAT</i></c>.
        FfxmCopyJobDescription       copyJobDescriptor;                      ///< Copy job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFXM_RENDER_JOB_COPY</i></c>.
        FfxmComputeJobDescription    computeJobDescriptor;                   ///< Compute job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFXM_RENDER_JOB_COMPUTE</i></c>.
		FfxmFragmentJobDescription   fragmentJobDescription;					///< Fragment job descriptor. Valid when <c><i>jobType</i></c> is <c><i>FFXM_RENDER_JOB_FRAGMENT</i></c>.
	};
} FfxmGpuJobDescription;

#if defined(POPULATE_SHADER_BLOB_FFX)
#undef POPULATE_SHADER_BLOB_FFX
#endif // #if defined(POPULATE_SHADER_BLOB_FFX)

/// Macro definition to copy header shader blob information into its SDK structural representation
///
/// @ingroup SDKTypes
#define POPULATE_SHADER_BLOB_FFX(info, index)                                                                                                              \
    {                                                                                                                                                      \
        info[index].blobData, info[index].blobSize, info[index].numConstantBuffers, info[index].numSRVTextures, info[index].numUAVTextures,                \
            info[index].numSRVBuffers, info[index].numUAVBuffers, info[index].numSamplers, info[index].numRTAccelerationStructures, info[index].numRTTextures, \
            info[index].constantBufferNames, info[index].constantBufferBindings, info[index].constantBufferCounts, info[index].constantBufferSpaces,       \
            info[index].srvTextureNames, info[index].srvTextureBindings, info[index].srvTextureCounts, info[index].srvTextureSpaces,                       \
            info[index].uavTextureNames, info[index].uavTextureBindings, info[index].uavTextureCounts, info[index].uavTextureSpaces,                       \
            info[index].srvBufferNames, info[index].srvBufferBindings, info[index].srvBufferCounts, info[index].srvBufferSpaces,                           \
            info[index].uavBufferNames, info[index].uavBufferBindings, info[index].uavBufferCounts, info[index].uavBufferSpaces,                           \
            info[index].samplerNames, info[index].samplerBindings, info[index].samplerCounts, info[index].samplerSpaces,                                   \
            info[index].rtAccelerationStructureNames, info[index].rtAccelerationStructureBindings, info[index].rtAccelerationStructureCounts,              \
            info[index].rtAccelerationStructureSpaces, info[index].rtTextureNames, info[index].rtTextureBindings, info[index].rtTextureCounts,             \
            info[index].rtTextureSpaces                                                                                                                    \
    }

// A single shader blob and a description of its resources.
///
/// @ingroup SDKTypes
typedef struct FfxmShaderBlob {

    const uint8_t* data;                                // A pointer to the blob
    const uint32_t  size;                               // Size in bytes.

    const uint32_t  cbvCount;                           // Number of CBs.
    const uint32_t  srvTextureCount;                    // Number of SRV Textures.
    const uint32_t  uavTextureCount;                    // Number of UAV Textures.
    const uint32_t  srvBufferCount;                     // Number of SRV Buffers.
    const uint32_t  uavBufferCount;                     // Number of UAV Buffers.
    const uint32_t  samplerCount;                       // Number of Samplers.
    const uint32_t  rtAccelStructCount;                 // Number of RT Acceleration structures.
    const uint32_t  rtTextureCount;						// Number of RenderTarget textures.

    // constant buffers
    const char** boundConstantBufferNames;
    const uint32_t* boundConstantBuffers;               // Pointer to an array of bound ConstantBuffers.
    const uint32_t* boundConstantBufferCounts;          // Pointer to an array of bound ConstantBuffer resource counts
    const uint32_t* boundConstantBufferSets;            // Pointer to an array of bound ConstantBuffer resource descriptor sets

    // srv textures
    const char** boundSRVTextureNames;
    const uint32_t* boundSRVTextures;                   // Pointer to an array of bound SRV resources.
    const uint32_t* boundSRVTextureCounts;              // Pointer to an array of bound SRV resource counts
    const uint32_t* boundSRVTextureSets;                // Pointer to an array of bound SRV resource descriptor sets

    // uav textures
    const char** boundUAVTextureNames;
    const uint32_t* boundUAVTextures;                   // Pointer to an array of bound UAV texture resources.
    const uint32_t* boundUAVTextureCounts;              // Pointer to an array of bound UAV texture resource counts
    const uint32_t* boundUAVTextureSets;                // Pointer to an array of bound UAV texture resource descriptor sets

    // srv buffers
    const char** boundSRVBufferNames;
    const uint32_t* boundSRVBuffers;                    // Pointer to an array of bound SRV buffer resources.
    const uint32_t* boundSRVBufferCounts;               // Pointer to an array of bound SRV buffer resource counts
    const uint32_t* boundSRVBufferSets;                 // Pointer to an array of bound SRV buffer resource descriptor sets

    // uav buffers
    const char** boundUAVBufferNames;
    const uint32_t* boundUAVBuffers;                    // Pointer to an array of bound UAV buffer resources.
    const uint32_t* boundUAVBufferCounts;               // Pointer to an array of bound UAV buffer resource counts
    const uint32_t* boundUAVBufferSets;                 // Pointer to an array of bound UAV buffer resource descriptor sets

    // samplers
    const char** boundSamplerNames;
    const uint32_t* boundSamplers;                      // Pointer to an array of bound sampler resources.
    const uint32_t* boundSamplerCounts;                 // Pointer to an array of bound sampler resource counts
    const uint32_t* boundSamplerSets;                   // Pointer to an array of bound sampler resource descriptor sets

    // rt acceleration structures
    const char** boundRTAccelerationStructureNames;
    const uint32_t* boundRTAccelerationStructures;      // Pointer to an array of bound UAV buffer resources.
    const uint32_t* boundRTAccelerationStructureCounts; // Pointer to an array of bound UAV buffer resource counts
    const uint32_t* boundRTAccelerationStructureSets;   // Pointer to an array of bound UAV buffer resource descriptor sets

    // rt textures
    const char** boundRTTextureNames;
    const uint32_t* boundRTTextures;                    // Pointer to an array of bound rt texture resources.
    const uint32_t* boundRTTextureCounts;               // Pointer to an array of bound rt texture resource counts
    const uint32_t* boundRTTextureSets;                 // Pointer to an array of bound rt texture resource descriptor sets

} FfxmShaderBlob;

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

} // namespace arm
