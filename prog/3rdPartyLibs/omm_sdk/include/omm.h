/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef INCLUDE_OMM_SDK_C
#define INCLUDE_OMM_SDK_C

#include <stdint.h>
#include <stddef.h>

#define OMM_VERSION_MAJOR 1
#define OMM_VERSION_MINOR 6
#define OMM_VERSION_BUILD 1

#define OMM_MAX_TRANSIENT_POOL_BUFFERS 8

#define OMM_GRAPHICS_PIPELINE_DESC_VERSION 3

#if defined(_MSC_VER)
    #define OMM_CALL __fastcall
#elif !defined(__aarch64__) && !defined(__x86_64) && (defined(__GNUC__)  || defined (__clang__))
    #define OMM_CALL __attribute__((fastcall))
#else
    #define OMM_CALL 
#endif

#ifndef OMM_API 
    #ifdef __cplusplus
        #define OMM_API extern "C"
    #else
        #define OMM_API
    #endif
#endif

#ifdef DEFINE_ENUM_FLAG_OPERATORS
    #define OMM_DEFINE_ENUM_FLAG_OPERATORS(x) DEFINE_ENUM_FLAG_OPERATORS(x)
#else
    #define OMM_DEFINE_ENUM_FLAG_OPERATORS(x)
#endif

#ifndef OMM_DEPRECATED_MSG
    #if defined(__has_cpp_attribute) && __has_cpp_attribute(deprecated) && !defined(__GNUC__)
        #define OMM_DEPRECATED_MSG(msg) [[deprecated( msg )]]
    #else
        #define OMM_DEPRECATED_MSG(msg)
    #endif
#endif

typedef uint8_t ommBool;

typedef struct _ommBaker _ommBaker;
typedef _ommBaker* ommBaker;

typedef struct _ommCpuBakeResult _ommCpuBakeResult;
typedef _ommCpuBakeResult* ommCpuBakeResult;

typedef struct _ommCpuTexture _ommCpuTexture;
typedef _ommCpuTexture* ommCpuTexture;

typedef struct _ommCpuSerializedResult _ommCpuSerializedResult;
typedef _ommCpuSerializedResult* ommCpuSerializedResult;

typedef struct _ommCpuDeserializedResult _ommCpuDeserializedResult;
typedef _ommCpuDeserializedResult* ommCpuDeserializedResult;

typedef void* (*ommAllocate)(void* userArg, size_t size, size_t alignment);

typedef void* (*ommReallocate)(void* userArg, void* memory, size_t size, size_t alignment);

typedef void (*ommFree)(void* userArg, void* memory);

typedef enum ommResult
{
   ommResult_SUCCESS,
   ommResult_FAILURE,
   ommResult_INVALID_ARGUMENT,
   ommResult_INSUFFICIENT_SCRATCH_MEMORY,
   ommResult_NOT_IMPLEMENTED,
   ommResult_WORKLOAD_TOO_BIG,
   ommResult_MAX_NUM,
} ommResult;

typedef enum ommMessageSeverity
{
    ommMessageSeverity_Info,
    ommMessageSeverity_PerfWarning,
    ommMessageSeverity_Error,
    ommMessageSeverity_Fatal,
    ommMessageSeverity_MAX_NUM,
} ommMessageSeverity;

typedef enum ommOpacityState
{
   ommOpacityState_Transparent,
   ommOpacityState_Opaque,
   ommOpacityState_UnknownTransparent,
   ommOpacityState_UnknownOpaque,
} ommOpacityState;

typedef enum ommSpecialIndex
{
   ommSpecialIndex_FullyTransparent        = -1,
   ommSpecialIndex_FullyOpaque             = -2,
   ommSpecialIndex_FullyUnknownTransparent = -3,
   ommSpecialIndex_FullyUnknownOpaque      = -4,
} ommSpecialIndex;

typedef enum ommFormat
{
   ommFormat_INVALID,
   // Value maps to DX/VK spec.
   ommFormat_OC1_2_State = 1,
   // Value maps to DX/VK spec.
   ommFormat_OC1_4_State = 2,
   ommFormat_MAX_NUM     = 3,
} ommFormat;

typedef enum ommUnknownStatePromotion
{
   // Will either be UO or UT depending on the coverage. If the micro-triangle is "mostly" opaque it will be UO (4-state) or O
   // (2-state). If the micro-triangle is "mostly" transparent it will be UT (4-state) or T (2-state)
   ommUnknownStatePromotion_Nearest,
   // All unknown states get promoted to O in 2-state mode, or UO in 4-state mode
   ommUnknownStatePromotion_ForceOpaque,
   // All unknown states get promoted to T in 2-state mode, or UT in 4-state mode
   ommUnknownStatePromotion_ForceTransparent,
   ommUnknownStatePromotion_MAX_NUM,
} ommUnknownStatePromotion;

typedef enum ommBakerType
{
   ommBakerType_GPU,
   ommBakerType_CPU,
   ommBakerType_MAX_NUM,
} ommBakerType;

typedef enum ommTexCoordFormat
{
   ommTexCoordFormat_UV16_UNORM,
   ommTexCoordFormat_UV16_FLOAT,
   ommTexCoordFormat_UV32_FLOAT,
   ommTexCoordFormat_MAX_NUM,
} ommTexCoordFormat;

typedef enum ommIndexFormat
{
   ommIndexFormat_UINT_16,
   ommIndexFormat_UINT_32,
   ommIndexFormat_I16_UINT OMM_DEPRECATED_MSG("ommIndexFormat_I16_UINT is deprecated, please use ommIndexFormat_UINT_16 instead") = ommIndexFormat_UINT_16,
   ommIndexFormat_I32_UINT OMM_DEPRECATED_MSG("ommIndexFormat_I32_UINT is deprecated, please use ommIndexFormat_UINT_32 instead") = ommIndexFormat_UINT_32,
   ommIndexFormat_MAX_NUM,
} ommIndexFormat;

typedef enum ommTextureAddressMode
{
   ommTextureAddressMode_Wrap,
   ommTextureAddressMode_Mirror,
   ommTextureAddressMode_Clamp,
   ommTextureAddressMode_Border,
   ommTextureAddressMode_MirrorOnce,
   ommTextureAddressMode_MAX_NUM,
} ommTextureAddressMode;

typedef enum ommTextureFilterMode
{
   ommTextureFilterMode_Nearest,
   ommTextureFilterMode_Linear,
   ommTextureFilterMode_MAX_NUM,
} ommTextureFilterMode;

typedef enum ommAlphaMode
{
   ommAlphaMode_Test,
   ommAlphaMode_Blend,
   ommAlphaMode_MAX_NUM,
} ommAlphaMode;

typedef enum ommCpuSerializeFlags
{
    ommCpuSerializeFlags_None,
    ommCpuSerializeFlags_Compress,
} ommCpuSerializeFlags;

typedef struct ommLibraryDesc
{
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t versionBuild;
} ommLibraryDesc;

typedef struct ommSamplerDesc
{
   ommTextureAddressMode addressingMode;
   ommTextureFilterMode  filter;
   float                 borderAlpha;
} ommSamplerDesc;

inline ommSamplerDesc ommSamplerDescDefault()
{
   ommSamplerDesc v;
   v.addressingMode  = ommTextureAddressMode_MAX_NUM;
   v.filter          = ommTextureFilterMode_MAX_NUM;
   v.borderAlpha     = 0;
   return v;
}

typedef struct ommMemoryAllocatorInterface
{
   union {
       ommAllocate                                                                              allocate;
       ommAllocate      OMM_DEPRECATED_MSG("Allocate is deprecated, use allocate instead")      Allocate;
   };
   union {
       ommReallocate                                                                            reallocate;
       ommReallocate    OMM_DEPRECATED_MSG("Reallocate is deprecated, use reallocate instead")  Reallocate;
   };
   union {
       ommFree                                                                                  free;
       ommFree          OMM_DEPRECATED_MSG("Free is deprecated, use free instead")              Free;
   };
   union {
       void*                                                                                    userArg;
       void*            OMM_DEPRECATED_MSG("UserArg is deprecated, use userArg instead")        UserArg;
   };
} ommMemoryAllocatorInterface;

inline ommMemoryAllocatorInterface ommMemoryAllocatorInterfaceDefault()
{
   ommMemoryAllocatorInterface v;
   v.allocate    = NULL;
   v.reallocate  = NULL;
   v.free        = NULL;
   v.userArg     = NULL;
   return v;
}

typedef void(*ommMessageCallback)(ommMessageSeverity severity, const char* message, void* userArg);

typedef struct ommMessageInterface
{
    ommMessageCallback   messageCallback;
    void*                userArg;
} ommMessageInterface;

inline ommMessageInterface ommMessageInterfaceDefault()
{
    ommMessageInterface v;
    v.messageCallback   = NULL;
    v.userArg           = NULL;
    return v;
}

typedef struct ommBakerCreationDesc
{
   ommBakerType                type;
   ommMemoryAllocatorInterface memoryAllocatorInterface;
   ommMessageInterface         messageInterface;
} ommBakerCreationDesc;

inline ommBakerCreationDesc ommBakerCreationDescDefault()
{
   ommBakerCreationDesc v;
   v.type                      = ommBakerType_MAX_NUM;
   v.memoryAllocatorInterface  = ommMemoryAllocatorInterfaceDefault();
   v.messageInterface          = ommMessageInterfaceDefault();
   return v;
}

OMM_API ommLibraryDesc ommGetLibraryDesc();

OMM_API ommResult ommCreateBaker(const ommBakerCreationDesc* bakeCreationDesc, ommBaker* outBaker);

OMM_API ommResult ommDestroyBaker(ommBaker baker);

typedef enum ommCpuTextureFormat
{
   ommCpuTextureFormat_UNORM8,
   ommCpuTextureFormat_FP32,
   ommCpuTextureFormat_MAX_NUM,
} ommCpuTextureFormat;

typedef enum ommCpuTextureFlags
{
   ommCpuTextureFlags_None,
   // Controls the internal memory layout of the texture. does not change the expected input format, it does affect the baking
   // performance and memory footprint of the texture object.
   ommCpuTextureFlags_DisableZOrder = 1u << 0,
} ommCpuTextureFlags;
OMM_DEFINE_ENUM_FLAG_OPERATORS(ommCpuTextureFlags);

typedef enum ommCpuBakeFlags
{
   ommCpuBakeFlags_None,

   // Baker will use internal threads to run the baking process in parallel.
   ommCpuBakeFlags_EnableInternalThreads        = 1u << 0,

   // Will disable the use of special indices in case the OMM-state is uniform, Only set this flag for debug purposes.
   // Note: This prevents promotion of fully known OMMs to use special indices, however for invalid & degenerate UV triangles
   // special indices may still be set.
   ommCpuBakeFlags_DisableSpecialIndices        = 1u << 1,

   // Force 32-bit index format in ommIndexFormat
   ommCpuBakeFlags_Force32BitIndices            = 1u << 2,

   // Will disable reuse of OMMs and instead produce duplicates omm-array data. Generally only needed for debug purposes.
   ommCpuBakeFlags_DisableDuplicateDetection    = 1u << 3,

   // This enables merging of "similar" OMMs where similarity is measured using hamming distance.
   // UT and UO are considered identical.
   // Pros: normally reduces resulting OMM size drastically, especially when there's overlapping UVs.
   // Cons: The merging comes at the cost of coverage.
   // The resulting OMM Arrays will have lower fraction of known states. For large working sets it can be quite CPU heavy to
   // compute. Note: can not be used if DisableDuplicateDetection is set.
   ommCpuBakeFlags_EnableNearDuplicateDetection = 1u << 4,

   // Enable additional validation, when enabled additional processing is performed to validate quality and sanity of input data
   // which may help diagnose omm bake result or longer than expected bake times.
   // *** NOTE messageInterface must be set when using this flag *** 
   ommCpuBakeFlags_EnableValidation             = 1u << 5,

   ommCpuBakeFlags_EnableWorkloadValidation OMM_DEPRECATED_MSG("EnableWorkloadValidation is deprecated, use EnableValidation instead") = 1u << 5,

} ommCpuBakeFlags;
OMM_DEFINE_ENUM_FLAG_OPERATORS(ommCpuBakeFlags);

// The baker supports conservativle baking from a MIP array when the runtime wants to pick freely between texture levels at
// runtime without the need to update the OMM data. _However_ baking from mip level 0 only is recommended in the general
// case for best performance the integration guide contains more in depth discussion on the topic
typedef struct ommCpuTextureMipDesc
{
   uint32_t    width;
   uint32_t    height;
   uint32_t    rowPitch;
   const void* textureData;
} ommCpuTextureMipDesc;

inline ommCpuTextureMipDesc ommCpuTextureMipDescDefault()
{
   ommCpuTextureMipDesc v;
   v.width        = 0;
   v.height       = 0;
   v.rowPitch     = 0;
   v.textureData  = NULL;
   return v;
}

typedef struct ommCpuTextureDesc
{
   ommCpuTextureFormat         format;
   ommCpuTextureFlags          flags;
   const ommCpuTextureMipDesc* mips;
   uint32_t                    mipCount;
   // Setting the alphaCutoff [0,1] allows the alpha cutoff to be embeded in the texture object which may accelerate the
   // baking operation in some circumstances. Note: if set it must match the alphaCutoff in the bake desc exactly.
   float                       alphaCutoff;
} ommCpuTextureDesc;

inline ommCpuTextureDesc ommCpuTextureDescDefault()
{
   ommCpuTextureDesc v;
   v.format       = ommCpuTextureFormat_MAX_NUM;
   v.flags        = ommCpuTextureFlags_None;
   v.mips         = NULL;
   v.mipCount     = 0;
   v.alphaCutoff  = -1.f;
   return v;
}

typedef struct ommCpuBakeInputDesc
{
   ommCpuBakeFlags          bakeFlags;
   ommCpuTexture            texture;
   // RuntimeSamplerDesc should match the sampler type used at runtime
   ommSamplerDesc           runtimeSamplerDesc;
   ommAlphaMode             alphaMode;
   ommTexCoordFormat        texCoordFormat;
   const void*              texCoords;
   // texCoordStrideInBytes: If zero, packed aligment is assumed
   uint32_t                 texCoordStrideInBytes;
   ommIndexFormat           indexFormat;
   const void*              indexBuffer;
   uint32_t                 indexCount;
   // Configure the target resolution when running dynamic subdivision level.
   // <= 0: disabled.
   // > 0: The subdivision level be chosen such that a single micro-triangle covers approximatley a dynamicSubdivisionScale *
   // dynamicSubdivisionScale texel area.
   float                    dynamicSubdivisionScale;
   // Rejection threshold [0,1]. Unless OMMs achive a rate of at least rejectionThreshold known states OMMs will be discarded
   // for the primitive. Use this to weed out "poor" OMMs.
   float                    rejectionThreshold;
   // The alpha cutoff value. By default it's Texel Opacity = texture > alphaCutoff ? Opaque : Transparent
   float                    alphaCutoff;
   // alphaCutoffGT and alphaCutoffLE allows dynamic configuring of the alpha values in the texture in the following way:
   // Texel opacity = texture > alphaCutoff ? alphaCutoffGT : alphaCutoffLE
   // This can be used to construct different pairings such as transparent and unknown opaque which is useful 
   // for applications requiring partial accumulated opacity, like smoke and particle effects
   union
   {
       OMM_DEPRECATED_MSG("alphaCutoffLE has been deprecated, please use alphaCutoffLessEqual")
       ommOpacityState      alphaCutoffLE;
       ommOpacityState      alphaCutoffLessEqual;
   };
   union
   {
       OMM_DEPRECATED_MSG("alphaCutoffGT has been deprecated, please use alphaCutoffGreater instead")
       ommOpacityState      alphaCutoffGT;
       ommOpacityState      alphaCutoffGreater;
   };
   // The global Format. May be overriden by the per-triangle subdivision level setting.
   ommFormat                format;
   // Use Formats to control format on a per triangle granularity. If Format is set to Format::INVALID the global setting will
   // be used instead.
   const ommFormat*         formats;
   // Determines how to promote mixed states
   ommUnknownStatePromotion unknownStatePromotion;
   // Determines the state of unresolvable(nan/inf UV-triangles) and disabled triangles. Note that degenerate triangles (points/lines) will be resolved correctly.
   union
   {
       OMM_DEPRECATED_MSG("unresolvedTriState has been deprecated, please use unresolvedTriState instead")
       ommSpecialIndex     degenTriState;
       ommSpecialIndex     unresolvedTriState;
   };
   // Micro triangle count is 4^N, where N is the subdivision level.
   // maxSubdivisionLevel level must be in range [0, 12].
   // When dynamicSubdivisionScale is enabled maxSubdivisionLevel is the max subdivision level allowed.
   // When dynamicSubdivisionScale is disabled maxSubdivisionLevel is the subdivision level applied uniformly to all
   // triangles.
   uint8_t                  maxSubdivisionLevel;
   // [optional] Use subdivisionLevels to control subdivision on a per triangle granularity.
   // +14 - reserved for future use.
   // 13 - use global value specified in 'subdivisionLevel.
   // [0,12] - per triangle subdivision level'
   const uint8_t*           subdivisionLevels;
   // [optional] Use maxWorkloadSize to cancel baking when the workload (# micro-triangle / texel tests) increase a certain threshold.
   // The baker will either reduce the baking quality to fit within this computational budget, or fail completely by returning the error code ommResult_WORKLOAD_TOO_BIG
   // This value correlates to the amount of processing required in the OMM bake call.
   // Factors that influence this value is:
   // * Number of unique UVs
   // * Size of the UVs
   // * Resolution of the underlying alpha texture
   // * Subdivision level of the OMMs.
   // Configure this value when experiencing long bake times, a starting point might be maxWorkloadSize = 1 << 28 (~ processing a total of 256 1k textures)
   uint64_t                 maxWorkloadSize;
} ommCpuBakeInputDesc;

inline ommCpuBakeInputDesc ommCpuBakeInputDescDefault()
{
   ommCpuBakeInputDesc v;
   v.bakeFlags                     = ommCpuBakeFlags_None;
   v.texture                       = 0;
   v.runtimeSamplerDesc            = ommSamplerDescDefault();
   v.alphaMode                     = ommAlphaMode_MAX_NUM;
   v.texCoordFormat                = ommTexCoordFormat_MAX_NUM;
   v.texCoords                     = NULL;
   v.texCoordStrideInBytes         = 0;
   v.indexFormat                   = ommIndexFormat_MAX_NUM;
   v.indexBuffer                   = NULL;
   v.indexCount                    = 0;
   v.dynamicSubdivisionScale       = 2;
   v.rejectionThreshold            = 0;
   v.alphaCutoff                   = 0.5f;
   v.alphaCutoffLessEqual          = ommOpacityState_Transparent;
   v.alphaCutoffGreater            = ommOpacityState_Opaque;
   v.format                        = ommFormat_OC1_4_State;
   v.formats                       = NULL;
   v.unknownStatePromotion         = ommUnknownStatePromotion_ForceOpaque;
   v.unresolvedTriState            = ommSpecialIndex_FullyUnknownOpaque;
   v.maxSubdivisionLevel           = 8;
   v.subdivisionLevels             = NULL;
   v.maxWorkloadSize               = 0xFFFFFFFFFFFFFFFF;
   return v;
}

typedef struct ommCpuOpacityMicromapDesc
{
   // Byte offset into the opacity micromap map array.
   uint32_t offset;
   // Micro triangle count is 4^N, where N is the subdivision level.
   uint16_t subdivisionLevel;
   // OMM input format.
   uint16_t format;
} ommCpuOpacityMicromapDesc;

typedef struct ommCpuOpacityMicromapUsageCount
{
   // Number of OMMs with the specified subdivision level and format.
   uint32_t count;
   // Micro triangle count is 4^N, where N is the subdivision level.
   uint16_t subdivisionLevel;
   // OMM input format.
   uint16_t format;
} ommCpuOpacityMicromapUsageCount;

typedef struct ommCpuBakeResultDesc
{
   // Below is used as OMM array build input DX/VK.
   const void*                            arrayData;
   uint32_t                               arrayDataSize;
   const ommCpuOpacityMicromapDesc*       descArray;
   uint32_t                               descArrayCount;
   // The histogram of all omm data referenced by 'ommDescArray', can be used as 'pOMMUsageCounts' for the OMM build in D3D12
   const ommCpuOpacityMicromapUsageCount* descArrayHistogram;
   uint32_t                               descArrayHistogramCount;
   // Below is used for BLAS build input in DX/VK
   const void*                            indexBuffer;
   uint32_t                               indexCount;
   ommIndexFormat                         indexFormat;
   // Same as ommDescArrayHistogram but usage count equals the number of references by ommIndexBuffer. Can be used as
   // 'pOMMUsageCounts' for the BLAS OMM attachment in D3D12
   const ommCpuOpacityMicromapUsageCount* indexHistogram;
   uint32_t                               indexHistogramCount;
} ommCpuBakeResultDesc;

typedef struct ommCpuBlobDesc
{
    void*       data;
    uint64_t    size;
} ommCpuBlobDesc;

inline ommCpuBlobDesc ommCpuBlobDescDefault()
{
    ommCpuBlobDesc v;
    v.data = nullptr;
    v.size = 0;
    return v;
}

typedef struct ommCpuDeserializedDesc
{
    ommCpuSerializeFlags        flags;
    // Optional
    int                         numInputDescs;
    const ommCpuBakeInputDesc*  inputDescs;
    // Optional
    int                         numResultDescs;
    const ommCpuBakeResultDesc* resultDescs;
} ommCpuDeserializedDesc;

inline ommCpuDeserializedDesc ommCpuDeserializedDescDefault()
{
    ommCpuDeserializedDesc v;
    v.flags             = ommCpuSerializeFlags_None;
    v.numInputDescs     = 0;
    v.inputDescs        = nullptr;
    v.numResultDescs    = 0;
    v.resultDescs       = nullptr;
    return v;
}

OMM_API ommResult ommCpuCreateTexture(ommBaker baker, const ommCpuTextureDesc* desc, ommCpuTexture* outTexture);

OMM_API ommResult ommCpuDestroyTexture(ommBaker baker, ommCpuTexture texture);

OMM_API ommResult ommCpuBake(ommBaker baker, const ommCpuBakeInputDesc* bakeInputDesc, ommCpuBakeResult* outBakeResult);

OMM_API ommResult ommCpuDestroyBakeResult(ommCpuBakeResult bakeResult);

OMM_API ommResult ommCpuGetBakeResultDesc(ommCpuBakeResult bakeResult, const ommCpuBakeResultDesc** desc);

// Serialization API useful to distribute input and /or output data for debugging& visualization purposes

// Serialization
OMM_API ommResult ommCpuSerialize(ommBaker baker, const ommCpuDeserializedDesc& desc, ommCpuSerializedResult* outResult);

OMM_API ommResult ommCpuGetSerializedResultDesc(ommCpuSerializedResult result, const ommCpuBlobDesc** desc);

OMM_API ommResult ommCpuDestroySerializedResult(ommCpuSerializedResult result);

// Deserialization
OMM_API ommResult ommCpuDeserialize(ommBaker baker, const ommCpuBlobDesc& desc, ommCpuDeserializedResult* outResult);

OMM_API ommResult ommCpuGetDeserializedDesc(ommCpuDeserializedResult result, const ommCpuDeserializedDesc** desc);

OMM_API ommResult ommCpuDestroyDeserializedResult(ommCpuDeserializedResult result);

typedef struct _ommGpuPipeline _ommGpuPipeline;
typedef _ommGpuPipeline* ommGpuPipeline;

typedef enum ommGpuDescriptorType
{
   ommGpuDescriptorType_TextureRead,
   ommGpuDescriptorType_BufferRead,
   ommGpuDescriptorType_RawBufferRead,
   ommGpuDescriptorType_RawBufferWrite,
   ommGpuDescriptorType_MAX_NUM,
} ommGpuDescriptorType;

typedef enum ommGpuResourceType
{
   // 1-4 channels, any format.
   ommGpuResourceType_IN_ALPHA_TEXTURE,
   ommGpuResourceType_IN_TEXCOORD_BUFFER,
   ommGpuResourceType_IN_INDEX_BUFFER,
   // (Optional) R8, Values must be in range [-2, 12].
   // Positive values to enforce specific subdibision level for the primtive.
   // -1 to use global subdivision level.
   // -2 to use automatic subduvision level based on tunable texel-area heuristic
   ommGpuResourceType_IN_SUBDIVISION_LEVEL_BUFFER,
   // Used directly as argument for OMM build in DX/VK
   ommGpuResourceType_OUT_OMM_ARRAY_DATA,
   // Used directly as argument for OMM build in DX/VK
   ommGpuResourceType_OUT_OMM_DESC_ARRAY,
   // Used directly as argument for OMM build in DX/VK. (Read back to CPU to query memory requirements during OMM Array build)
   ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM,
   // Used directly as argument for OMM build in DX/VK
   ommGpuResourceType_OUT_OMM_INDEX_BUFFER,
   // Used directly as argument for OMM build in DX/VK. (Read back to CPU to query memory requirements during OMM Blas build)
   ommGpuResourceType_OUT_OMM_INDEX_HISTOGRAM,
   // Read back the PostDispatchInfo struct containing the actual sizes of ARRAY_DATA and DESC_ARRAY.
   ommGpuResourceType_OUT_POST_DISPATCH_INFO,
   // Can be reused after baking
   ommGpuResourceType_TRANSIENT_POOL_BUFFER,
   // Initialize on startup. Read-only.
   ommGpuResourceType_STATIC_VERTEX_BUFFER,
   // Initialize on startup. Read-only.
   ommGpuResourceType_STATIC_INDEX_BUFFER,
   ommGpuResourceType_MAX_NUM,
} ommGpuResourceType;

typedef enum ommGpuPrimitiveTopology
{
   ommGpuPrimitiveTopology_TriangleList,
   ommGpuPrimitiveTopology_MAX_NUM,
} ommGpuPrimitiveTopology;

typedef enum ommGpuPipelineType
{
   ommGpuPipelineType_Compute,
   ommGpuPipelineType_Graphics,
   ommGpuPipelineType_MAX_NUM,
} ommGpuPipelineType;

typedef enum ommGpuDispatchType
{
   ommGpuDispatchType_Compute,
   ommGpuDispatchType_ComputeIndirect,
   ommGpuDispatchType_DrawIndexedIndirect,
   ommGpuDispatchType_BeginLabel,
   ommGpuDispatchType_EndLabel,
   ommGpuDispatchType_MAX_NUM,
} ommGpuDispatchType;

typedef enum ommGpuBufferFormat
{
   ommGpuBufferFormat_R32_UINT,
   ommGpuBufferFormat_MAX_NUM,
} ommGpuBufferFormat;

typedef enum ommGpuRasterCullMode
{
   ommGpuRasterCullMode_None,
} ommGpuRasterCullMode;

typedef enum ommGpuRenderAPI
{
   ommGpuRenderAPI_DX12,
   ommGpuRenderAPI_Vulkan,
   ommGpuRenderAPI_MAX_NUM,
} ommGpuRenderAPI;

typedef enum ommGpuScratchMemoryBudget
{
   ommGpuScratchMemoryBudget_Undefined,
   ommGpuScratchMemoryBudget_MB_4      = 4ull << 20ull,
   ommGpuScratchMemoryBudget_MB_32     = 32ull << 20ull,
   ommGpuScratchMemoryBudget_MB_64     = 64ull << 20ull,
   ommGpuScratchMemoryBudget_MB_128    = 128ull << 20ull,
   ommGpuScratchMemoryBudget_MB_256    = 256ull << 20ull,
   ommGpuScratchMemoryBudget_MB_512    = 512ull << 20ull,
   ommGpuScratchMemoryBudget_MB_1024   = 1024ull << 20ull,
   ommGpuScratchMemoryBudget_Default   = 256ull << 20ull,
} ommGpuScratchMemoryBudget;

typedef enum ommGpuBakeFlags
{
   // Either PerformSetup, PerformBake (or both simultaneously) must be set.
   ommGpuBakeFlags_Invalid                      = 0,

   // (Default) OUT_OMM_DESC_ARRAY_HISTOGRAM, OUT_OMM_INDEX_HISTOGRAM, OUT_OMM_INDEX_BUFFER, OUT_OMM_DESC_ARRAY and
   // OUT_POST_DISPATCH_INFO will be updated.
   ommGpuBakeFlags_PerformSetup                 = 1u << 0,

   // (Default) OUT_OMM_INDEX_HISTOGRAM, OUT_OMM_INDEX_BUFFER, OUT_OMM_ARRAY_DATA and OUT_POST_DISPATCH_INFO (if stats
   // enabled) will be updated. If special indices are detected OUT_OMM_INDEX_BUFFER may also be modified.
   // If PerformBuild is not used with this flag, OUT_OMM_DESC_ARRAY_HISTOGRAM, OUT_OMM_INDEX_HISTOGRAM, OUT_OMM_INDEX_BUFFER,
   // OUT_OMM_DESC_ARRAY must contain valid data from a prior PerformSetup pass.
   ommGpuBakeFlags_PerformBake                  = 1u << 1,

   // Alias for (PerformSetup | PerformBake)
   ommGpuBakeFlags_PerformSetupAndBake          = 3u,

   // Baking will only be done using compute shaders and no gfx involvement (drawIndirect or graphics PSOs). (Beta)
   // Will become default mode in the future.
   // + Useful for async workloads
   // + Less memory hungry
   // + Faster baking on low texel ratio to micro-triangle ratio (=rasterizing small triangles)
   // - May looses efficency when resampling large triangles (tail-effect). Potential mitigation is to batch multiple bake
   // jobs. However this is generally not a big problem.
   ommGpuBakeFlags_ComputeOnly                  = 1u << 2,

   // Must be used together with EnablePostDispatchInfo. If set baking (PerformBake) will fill the stats data of
   // OUT_POST_DISPATCH_INFO.
   ommGpuBakeFlags_EnablePostDispatchInfoStats  = 1u << 3,

   // Will disable the use of special indices in case the OMM-state is uniform. Only set this flag for debug purposes.
   ommGpuBakeFlags_DisableSpecialIndices        = 1u << 4,

   // If texture coordinates are known to be unique tex cooord deduplication can be disabled to save processing time and free
   // up scratch memory.
   ommGpuBakeFlags_DisableTexCoordDeduplication = 1u << 5,

   // Force 32-bit indices in OUT_OMM_INDEX_BUFFER
   ommGpuBakeFlags_Force32BitIndices            = 1u << 6,

   // Use only for debug purposes. Level Line Intersection method is vastly superior in 4-state mode.
   ommGpuBakeFlags_DisableLevelLineIntersection = 1u << 7,

   // Slightly modifies the dispatch to aid frame capture debugging.
   ommGpuBakeFlags_EnableNsightDebugMode        = 1u << 8,
} ommGpuBakeFlags;
OMM_DEFINE_ENUM_FLAG_OPERATORS(ommGpuBakeFlags);

typedef struct ommGpuResource
{
   ommGpuDescriptorType stateNeeded;
   ommGpuResourceType   type;
   uint16_t             indexInPool;
   uint16_t             mipOffset;
   uint16_t             mipNum;
} ommGpuResource;

typedef struct ommGpuDescriptorRangeDesc
{
   ommGpuDescriptorType descriptorType;
   uint32_t             baseRegisterIndex;
   uint32_t             descriptorNum;
} ommGpuDescriptorRangeDesc;

typedef struct ommGpuBufferDesc
{
   size_t bufferSize;
} ommGpuBufferDesc;

typedef struct ommGpuShaderBytecode
{
   const void* data;
   size_t      size;
} ommGpuShaderBytecode;

typedef struct ommGpuComputePipelineDesc
{
   ommGpuShaderBytecode             computeShader;
   const char*                      shaderFileName;
   const char*                      shaderEntryPointName;
   const ommGpuDescriptorRangeDesc* descriptorRanges;
   uint32_t                         descriptorRangeNum;
   // if "true" all constant buffers share same "ConstantBufferDesc" description. if "false" this pipeline doesn't have a
   // constant buffer
   ommBool                          hasConstantData;
} ommGpuComputePipelineDesc;

typedef struct ommGpuGraphicsPipelineInputElementDesc
{
   const char*        semanticName;
   ommGpuBufferFormat format;
   uint32_t           inputSlot;
   uint32_t           semanticIndex;
   ommBool            isPerInstanced;
} ommGpuGraphicsPipelineInputElementDesc;

inline ommGpuGraphicsPipelineInputElementDesc ommGpuGraphicsPipelineInputElementDescDefault()
{
   ommGpuGraphicsPipelineInputElementDesc v;
   v.semanticName    = "POSITION";
   v.format          = ommGpuBufferFormat_R32_UINT;
   v.inputSlot       = 0;
   v.semanticIndex   = 0;
   v.isPerInstanced  = 0;
   return v;
}

// Config specification not declared in the GraphicsPipelineDesc is implied, but may become explicit in future versions.
// Stenci state = disabled
// BlendState = disabled
// Primitive topology = triangle list
// Input element = count 1, see GraphicsPipelineInputElementDesc
// Fill mode = solid
// Track any changes via OMM_GRAPHICS_PIPELINE_DESC_VERSION
typedef struct ommGpuGraphicsPipelineDesc
{
   ommGpuShaderBytecode             vertexShader;
   const char*                      vertexShaderFileName;
   const char*                      vertexShaderEntryPointName;
   ommGpuShaderBytecode             geometryShader;
   const char*                      geometryShaderFileName;
   const char*                      geometryShaderEntryPointName;
   ommGpuShaderBytecode             pixelShader;
   const char*                      pixelShaderFileName;
   const char*                      pixelShaderEntryPointName;
   ommBool                          conservativeRasterization;
   const ommGpuDescriptorRangeDesc* descriptorRanges;
   uint32_t                         descriptorRangeNum;
   // if NumRenderTargets = 0 a null RTV is implied.
   uint32_t                         numRenderTargets;
   // if "true" all constant buffers share same "ConstantBufferDesc" description. if "false" this pipeline doesn't have a
   // constant buffer
   ommBool                          hasConstantData;
} ommGpuGraphicsPipelineDesc;

typedef struct ommGpuPipelineDesc
{
   ommGpuPipelineType type;

   union
   {
      ommGpuComputePipelineDesc  compute;
      ommGpuGraphicsPipelineDesc graphics;
   };
} ommGpuPipelineDesc;

typedef struct ommGpuDescriptorSetDesc
{
   uint32_t constantBufferMaxNum;
   uint32_t storageBufferMaxNum;
   uint32_t descriptorRangeMaxNumPerPipeline;
} ommGpuDescriptorSetDesc;

typedef struct ommGpuConstantBufferDesc
{
   uint32_t registerIndex;
   uint32_t maxDataSize;
} ommGpuConstantBufferDesc;

typedef struct ommGpuViewport
{
   float minWidth;
   float minHeight;
   float maxWidth;
   float maxHeight;
} ommGpuViewport;

typedef struct ommGpuComputeDesc
{
   const char*           name;
   // concatenated resources for all "DescriptorRangeDesc" descriptions in DenoiserDesc::pipelines[ pipelineIndex ]
   const ommGpuResource* resources;
   uint32_t              resourceNum;
   // "root constants" in DX12
   const uint8_t*        localConstantBufferData;
   uint32_t              localConstantBufferDataSize;
   uint16_t              pipelineIndex;
   uint32_t              gridWidth;
   uint32_t              gridHeight;
} ommGpuComputeDesc;

typedef struct ommGpuComputeIndirectDesc
{
   const char*           name;
   // concatenated resources for all "DescriptorRangeDesc" descriptions in DenoiserDesc::pipelines[ pipelineIndex ]
   const ommGpuResource* resources;
   uint32_t              resourceNum;
   // "root constants" in DX12
   const uint8_t*        localConstantBufferData;
   uint32_t              localConstantBufferDataSize;
   uint16_t              pipelineIndex;
   ommGpuResource        indirectArg;
   size_t                indirectArgByteOffset;
} ommGpuComputeIndirectDesc;

typedef struct ommGpuDrawIndexedIndirectDesc
{
   const char*           name;
   // concatenated resources for all "DescriptorRangeDesc" descriptions in DenoiserDesc::pipelines[ pipelineIndex ]
   const ommGpuResource* resources;
   uint32_t              resourceNum;
   // "root constants" in DX12
   const uint8_t*        localConstantBufferData;
   uint32_t              localConstantBufferDataSize;
   uint16_t              pipelineIndex;
   ommGpuResource        indirectArg;
   size_t                indirectArgByteOffset;
   ommGpuViewport        viewport;
   ommGpuResource        indexBuffer;
   uint32_t              indexBufferOffset;
   ommGpuResource        vertexBuffer;
   uint32_t              vertexBufferOffset;
} ommGpuDrawIndexedIndirectDesc;

typedef struct ommGpuBeginLabelDesc
{
   const char* debugName;
} ommGpuBeginLabelDesc;

typedef struct ommGpuDispatchDesc
{
   ommGpuDispatchType type;

   union
   {
      ommGpuComputeDesc             compute;
      ommGpuComputeIndirectDesc     computeIndirect;
      ommGpuDrawIndexedIndirectDesc drawIndexedIndirect;
      ommGpuBeginLabelDesc          beginLabel;
   };
} ommGpuDispatchDesc;

typedef struct ommGpuStaticSamplerDesc
{
   ommSamplerDesc desc;
   uint32_t       registerIndex;
} ommGpuStaticSamplerDesc;

typedef struct ommGpuSPIRVBindingOffsets
{
   uint32_t samplerOffset;
   uint32_t textureOffset;
   uint32_t constantBufferOffset;
   uint32_t storageTextureAndBufferOffset;
} ommGpuSPIRVBindingOffsets;

typedef struct ommGpuPipelineConfigDesc
{
   // API is required to make sure indirect buffers are written to in suitable format
   ommGpuRenderAPI renderAPI;
} ommGpuPipelineConfigDesc;

inline ommGpuPipelineConfigDesc ommGpuPipelineConfigDescDefault()
{
   ommGpuPipelineConfigDesc v;
   v.renderAPI  = ommGpuRenderAPI_DX12;
   return v;
}

// Note: sizes may return size zero, this means the buffer will not be used in the dispatch.
typedef struct ommGpuPreDispatchInfo
{
   // Format of outOmmIndexBuffer
   ommIndexFormat outOmmIndexBufferFormat;
   uint32_t       outOmmIndexCount;
   // Min required size of OUT_OMM_ARRAY_DATA. GetPreDispatchInfo returns most conservative estimation while less conservative
   // number can be obtained via BakePrepass
   uint32_t       outOmmArraySizeInBytes;
   // Min required size of OUT_OMM_DESC_ARRAY. GetPreDispatchInfo returns most conservative estimation while less conservative
   // number can be obtained via BakePrepass
   uint32_t       outOmmDescSizeInBytes;
   // Min required size of OUT_OMM_INDEX_BUFFER
   uint32_t       outOmmIndexBufferSizeInBytes;
   // Min required size of OUT_OMM_ARRAY_HISTOGRAM
   uint32_t       outOmmArrayHistogramSizeInBytes;
   // Min required size of OUT_OMM_INDEX_HISTOGRAM
   uint32_t       outOmmIndexHistogramSizeInBytes;
   // Min required size of OUT_POST_DISPATCH_INFO
   uint32_t       outOmmPostDispatchInfoSizeInBytes;
   // Min required sizes of TRANSIENT_POOL_BUFFERs
   uint32_t       transientPoolBufferSizeInBytes[OMM_MAX_TRANSIENT_POOL_BUFFERS];
   uint32_t       numTransientPoolBuffers;
} ommGpuPreDispatchInfo;

inline ommGpuPreDispatchInfo ommGpuPreDispatchInfoDefault()
{
   ommGpuPreDispatchInfo v;
   v.outOmmIndexBufferFormat                                         = ommIndexFormat_MAX_NUM;
   v.outOmmIndexCount                                                = 0xFFFFFFFF;
   v.outOmmArraySizeInBytes                                          = 0xFFFFFFFF;
   v.outOmmDescSizeInBytes                                           = 0xFFFFFFFF;
   v.outOmmIndexBufferSizeInBytes                                    = 0xFFFFFFFF;
   v.outOmmArrayHistogramSizeInBytes                                 = 0xFFFFFFFF;
   v.outOmmIndexHistogramSizeInBytes                                 = 0xFFFFFFFF;
   v.outOmmPostDispatchInfoSizeInBytes                               = 0xFFFFFFFF;
   v.numTransientPoolBuffers                                         = 0;
   return v;
}

typedef struct ommGpuDispatchConfigDesc
{
   ommGpuBakeFlags           bakeFlags;
   // RuntimeSamplerDesc describes the texture sampler that will be used in the runtime alpha test shader code.
   ommSamplerDesc            runtimeSamplerDesc;
   ommAlphaMode              alphaMode;
   //  The texture dimensions of IN_ALPHA_TEXTURE
   uint32_t                  alphaTextureWidth;
   //  The texture dimensions of IN_ALPHA_TEXTURE
   uint32_t                  alphaTextureHeight;
   // The channel in IN_ALPHA_TEXTURE containing opacity values
   uint32_t                  alphaTextureChannel;
   ommTexCoordFormat         texCoordFormat;
   uint32_t                  texCoordOffsetInBytes;
   // texCoordStrideInBytes: If zero, packed aligment is assumed
   uint32_t                  texCoordStrideInBytes;
   ommIndexFormat            indexFormat;
   // The actual number of indices can be lower.
   uint32_t                  indexCount;
   // If zero packed aligment is assumed.
   uint32_t                  indexStrideInBytes;
   // The alpha cutoff value. By default it's Texel Opacity = texture > alphaCutoff ? Opaque : Transparent
   float                     alphaCutoff;
   // alphaCutoffGT and alphaCutoffLE allows dynamic configuring of the alpha values in the texture in the following way:
   // Texel opacity = texture > alphaCutoff ? alphaCutoffGT : alphaCutoffLE
   // This can be used to construct different pairings such as transparent and unknown opaque which is useful 
   // for applications requiring partial accumulated opacity, like smoke and particle effects
   union
   {
       OMM_DEPRECATED_MSG("alphaCutoffLE has been deprecated, please use alphaCutoffLessEqual")
       ommOpacityState      alphaCutoffLE;
       ommOpacityState      alphaCutoffLessEqual;
   };
   union
   {
       OMM_DEPRECATED_MSG("alphaCutoffGT has been deprecated, please use alphaCutoffGreater instead")
       ommOpacityState      alphaCutoffGT;
       ommOpacityState      alphaCutoffGreater;
   };
   // Configure the target resolution when running dynamic subdivision level. <= 0: disabled. > 0: The subdivision level be
   // chosen such that a single micro-triangle covers approximatley a dynamicSubdivisionScale * dynamicSubdivisionScale texel
   // area.
   float                     dynamicSubdivisionScale;
   // The global Format. May be overriden by the per-triangle config.
   ommFormat                 globalFormat;
   uint8_t                   maxSubdivisionLevel;
   ommBool                   enableSubdivisionLevelBuffer;
   // The SDK will try to limit the omm array size of PreDispatchInfo::outOmmArraySizeInBytes and
   // PostDispatchInfo::outOmmArraySizeInBytes.
   // Currently a greedy algorithm is implemented with a first come-first serve order.
   // The SDK may (or may not) apply more sophisticated heuristics in the future.
   // If no memory is available to allocate an OMM Array Block the state will default to Unknown Opaque (ignoring any bake
   // flags do disable special indices).
   uint32_t                  maxOutOmmArraySize;
   // Target scratch memory budget, The SDK will try adjust the sum of the transient pool buffers to match this value. Higher
   // budget more efficiently executes the baking operation. May return INSUFFICIENT_SCRATCH_MEMORY if set too low.
   ommGpuScratchMemoryBudget maxScratchMemorySize;
} ommGpuDispatchConfigDesc;

inline ommGpuDispatchConfigDesc ommGpuDispatchConfigDescDefault()
{
   ommGpuDispatchConfigDesc v;
   v.bakeFlags                     = ommGpuBakeFlags_PerformSetupAndBake;
   v.runtimeSamplerDesc            = ommSamplerDescDefault();
   v.alphaMode                     = ommAlphaMode_MAX_NUM;
   v.alphaTextureWidth             = 0;
   v.alphaTextureHeight            = 0;
   v.alphaTextureChannel           = 3;
   v.texCoordFormat                = ommTexCoordFormat_MAX_NUM;
   v.texCoordOffsetInBytes         = 0;
   v.texCoordStrideInBytes         = 0;
   v.indexFormat                   = ommIndexFormat_MAX_NUM;
   v.indexCount                    = 0;
   v.indexStrideInBytes            = 0;
   v.alphaCutoff                   = 0.5f;
   v.alphaCutoffLessEqual          = ommOpacityState_Transparent;
   v.alphaCutoffGreater            = ommOpacityState_Opaque;
   v.dynamicSubdivisionScale       = 2;
   v.globalFormat                  = ommFormat_OC1_4_State;
   v.maxSubdivisionLevel           = 8;
   v.enableSubdivisionLevelBuffer  = 0;
   v.maxOutOmmArraySize            = 0xFFFFFFFF;
   v.maxScratchMemorySize          = ommGpuScratchMemoryBudget_Default;
   return v;
}

typedef struct ommGpuPipelineInfoDesc
{
   ommGpuSPIRVBindingOffsets      spirvBindingOffsets;
   const ommGpuPipelineDesc*      pipelines;
   uint32_t                       pipelineNum;
   ommGpuConstantBufferDesc       globalConstantBufferDesc;
   ommGpuConstantBufferDesc       localConstantBufferDesc;
   ommGpuDescriptorSetDesc        descriptorSetDesc;
   const ommGpuStaticSamplerDesc* staticSamplers;
   uint32_t                       staticSamplersNum;
} ommGpuPipelineInfoDesc;

// Format of OUT_POST_DISPATCH_INFO
typedef struct ommGpuPostDispatchInfo
{
   uint32_t outOmmArraySizeInBytes;
   uint32_t outOmmDescSizeInBytes;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalOpaqueCount;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalTransparentCount;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalUnknownCount;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalFullyOpaqueCount;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalFullyTransparentCount;
   // Will be populated if EnablePostDispatchInfoStats is set.
   uint32_t outStatsTotalFullyStatsUnknownCount;
} ommGpuPostDispatchInfo;

typedef struct ommGpuDispatchChain
{
   const ommGpuDispatchDesc* dispatches;
   uint32_t                  numDispatches;
   const uint8_t*            globalCBufferData;
   uint32_t                  globalCBufferDataSize;
} ommGpuDispatchChain;

// Global immutable resources. These contain the static immutable resources being shared acroess all bake calls.  Currently
// it's the specific IB and VB that represents a tesselated triangle arranged in bird curve order, for different
// subdivision levels.
OMM_API ommResult ommGpuGetStaticResourceData(ommGpuResourceType resource, uint8_t* data, size_t* outByteSize);

OMM_API ommResult ommGpuCreatePipeline(ommBaker baker, const ommGpuPipelineConfigDesc* pipelineCfg, ommGpuPipeline* outPipeline);

OMM_API ommResult ommGpuDestroyPipeline(ommBaker baker, ommGpuPipeline pipeline);

// Return the required pipelines. Does not depend on per-dispatch settings.
OMM_API ommResult ommGpuGetPipelineDesc(ommGpuPipeline pipeline, const ommGpuPipelineInfoDesc** outPipelineDesc);

// Returns the scratch and output memory requirements of the baking operation.
OMM_API ommResult ommGpuGetPreDispatchInfo(ommGpuPipeline pipeline, const ommGpuDispatchConfigDesc* config, ommGpuPreDispatchInfo* outPreDispatchInfo);

// Returns the dispatch order to perform the baking operation. Once complete the OUT_OMM_* resources will be written to and
// can be consumed by the application.
OMM_API ommResult ommGpuDispatch(ommGpuPipeline pipeline, const ommGpuDispatchConfigDesc* config, const ommGpuDispatchChain** outDispatchDesc);

typedef struct ommDebugSaveImagesDesc
{
   const char* path;
   const char* filePostfix;
   // The default behaviour is to dump the entire alpha texture with the OMM-triangle in it. Enabling detailedCutout will
   // generate cropped version zoomed in on the OMM, and supersampled for detailed analysis
   ommBool     detailedCutout;
   // Only dump index 0.
   ommBool     dumpOnlyFirstOMM;
   // Will draw unknown transparent and unknown opaque in the same color.
   ommBool     monochromeUnknowns;
   // true:Will draw all primitives to the same file. false: will draw each primitive separatley.
   ommBool     oneFile;
} ommDebugSaveImagesDesc;

inline ommDebugSaveImagesDesc ommDebugSaveImagesDescDefault()
{
   ommDebugSaveImagesDesc v;
   v.path                = "";
   v.filePostfix         = "";
   v.detailedCutout      = 0;
   v.dumpOnlyFirstOMM    = 0;
   v.monochromeUnknowns  = 0;
   v.oneFile             = 0;
   return v;
}

typedef struct ommDebugStats
{
   uint64_t totalOpaque;
   uint64_t totalTransparent;
   uint64_t totalUnknownTransparent;
   uint64_t totalUnknownOpaque;
   uint32_t totalFullyOpaque;
   uint32_t totalFullyTransparent;
   uint32_t totalFullyUnknownOpaque;
   uint32_t totalFullyUnknownTransparent;
} ommDebugStats;

inline ommDebugStats ommDebugStatsDefault()
{
   ommDebugStats v;
   v.totalOpaque                   = 0;
   v.totalTransparent              = 0;
   v.totalUnknownTransparent       = 0;
   v.totalUnknownOpaque            = 0;
   v.totalFullyOpaque              = 0;
   v.totalFullyTransparent         = 0;
   v.totalFullyUnknownOpaque       = 0;
   v.totalFullyUnknownTransparent  = 0;
   return v;
}

// Walk each primitive and dumps the corresponding OMM overlay to the alpha textures.
OMM_API ommResult ommDebugSaveAsImages(ommBaker baker, const ommCpuBakeInputDesc* bakeInputDesc, const ommCpuBakeResultDesc* res, const ommDebugSaveImagesDesc* desc);

OMM_API ommResult ommDebugGetStats(ommBaker baker, const ommCpuBakeResultDesc* res, ommDebugStats* out);

OMM_API ommResult ommDebugSaveBinaryToDisk(ommBaker baker, const ommCpuBlobDesc& data, const char* path);

#endif // #ifndef INCLUDE_OMM_SDK_C
