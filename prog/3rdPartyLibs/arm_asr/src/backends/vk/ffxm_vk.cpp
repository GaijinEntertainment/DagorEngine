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

#include <host/ffxm_interface.h>
#include <host/ffxm_util.h>
#include <host/ffxm_assert.h>
#include <host/backends/vk/ffxm_vk.h>
#include <ffxm_shader_blobs.h>
#include <codecvt>
#include <string.h>
#include <math.h>
#include <array>
#include <ffxm_hash.h>
#include <locale>

namespace arm
{

// prototypes for functions in the interface
FfxmUInt32              GetSDKVersionVK(FfxmInterface* backendInterface);
FfxmErrorCode           CreateBackendContextVK(FfxmInterface* backendInterface, FfxmUInt32* effectContextId);
FfxmErrorCode           GetDeviceCapabilitiesVK(FfxmInterface* backendInterface, FfxmDeviceCapabilities* deviceCapabilities);
FfxmErrorCode           DestroyBackendContextVK(FfxmInterface* backendInterface, FfxmUInt32 effectContextId);
FfxmErrorCode           CreateResourceVK(FfxmInterface* backendInterface, const FfxmCreateResourceDescription* desc, FfxmUInt32 effectContextId, FfxmResourceInternal* outTexture);
FfxmErrorCode           DestroyResourceVK(FfxmInterface* backendInterface, FfxmResourceInternal resource);
FfxmErrorCode           RegisterResourceVK(FfxmInterface* backendInterface, const FfxmResource* inResource, FfxmUInt32 effectContextId, FfxmResourceInternal* outResourceInternal);
FfxmResource            GetResourceVK(FfxmInterface* backendInterface, FfxmResourceInternal resource);
FfxmErrorCode           UnregisterResourcesVK(FfxmInterface* backendInterface, FfxmCommandList commandList, FfxmUInt32 effectContextId);
FfxmResourceDescription GetResourceDescriptionVK(FfxmInterface* backendInterface, FfxmResourceInternal resource);
FfxmErrorCode		   CreateComputePipelineVK(FfxmInterface* backendInterface, FfxmEffect effect, FfxmPass passId, FfxmShaderQuality qualityPreset, FfxmUInt32 permutationOptions, const FfxmPipelineDescription* desc, FfxmUInt32 effectContextId, FfxmPipelineState* outPass);
FfxmErrorCode		   CreateGraphicsPipelineVK(FfxmInterface* backendInterface, FfxmEffect effect, FfxmPass passId, FfxmShaderQuality qualityPreset, FfxmUInt32 permutationOptions, const FfxmPipelineDescription* desc, FfxmUInt32 effectContextId, FfxmPipelineState* outPass);
FfxmErrorCode           DestroyPipelineVK(FfxmInterface* backendInterface, FfxmPipelineState* pipeline, FfxmUInt32 effectContextId);
FfxmErrorCode           ScheduleGpuJobVK(FfxmInterface* backendInterface, const FfxmGpuJobDescription* job);
FfxmErrorCode           ExecuteGpuJobsVK(FfxmInterface* backendInterface, FfxmCommandList commandList);

static VkDeviceContext sVkDeviceContext = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

#define MAX_DESCRIPTOR_SET_LAYOUTS      (32)
#define MAX_DESCRIPTOR_SETS             (2)
#define MAX_RENDER_PASS_COUNT           (FFXM_MAX_QUEUED_FRAMES)
#define MAX_IMAGE_VIEW_COUNT            (FFXM_MAX_QUEUED_FRAMES*4)
#define MAX_FRAME_BUFFER_COUNT          (FFXM_MAX_QUEUED_FRAMES)
#define MAX_GRAPHICS_PIPELINE_COUNT     (FFXM_MAX_QUEUED_FRAMES)

// Redefine offsets for compilation purposes
#define BINDING_SHIFT(name, shift)                       \
constexpr FfxmUInt32 name##_BINDING_SHIFT     = shift; \
constexpr wchar_t* name##_BINDING_SHIFT_STR = L#shift;

// put it there for now
// BINDING_SHIFT(TEXTURE, 0);
// BINDING_SHIFT(SAMPLER, 1000);
// BINDING_SHIFT(UNORDERED_ACCESS_VIEW, 2000);
// BINDING_SHIFT(CONSTANT_BUFFER, 3000);

// To track parallel effect context usage
static FfxmUInt32 s_BackendRefCount = 0;
static FfxmUInt32 s_MaxEffectContexts = 0;

typedef struct ObjectBase_VK {
    uint64_t hash;
	FfxmUInt32 visitedFlag;
} ObjectBase_VK;

typedef struct GraphicPipeline_VK : ObjectBase_VK {
    VkPipeline handle;
} GraphicPipeline_VK;

typedef struct FrameBuffer_VK : ObjectBase_VK {
    VkFramebuffer handle;
} FrameBuffer_VK;

typedef struct ImageView_VK : ObjectBase_VK {
    VkImageView handle;
} ImageView_VK;

typedef struct RenderPass_VK : ObjectBase_VK {
    VkRenderPass handle;
} RenderPass_VK;

typedef struct BackendContext_VK {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        char                    resourceName[64] = {};
#endif
        union {
            VkImage             imageResource;
            VkBuffer            bufferResource;
        };

        FfxmResourceDescription  resourceDescription;
        FfxmResourceStates       initialState;
        FfxmResourceStates       currentState;
        int32_t                 srvViewIndex;
        int32_t                 uavViewIndex;
        FfxmUInt32                uavViewCount;

        VkDeviceMemory          deviceMemory;
        VkMemoryPropertyFlags   memoryProperties;

        bool                    undefined;
        bool                    dynamic;

    } Resource;

    typedef struct UniformBuffer
    {
        VkBuffer bufferResource;
        uint8_t* pData;
    } UniformBuffer;

    typedef struct PipelineLayout {

        VkSampler               samplers[FFXM_MAX_SAMPLERS];
        VkDescriptorSetLayout   descriptorSetLayout[MAX_DESCRIPTOR_SETS];
        VkDescriptorSet         descriptorSets[FFXM_MAX_QUEUED_FRAMES][MAX_DESCRIPTOR_SETS];
        FfxmUInt32                descriptorSetIndex;
        VkPipelineLayout        pipelineLayout;

        // Only used by graphics pipeline
        VkShaderModule          fragShaderModule;
        VkShaderModule          vertShaderModule;

        RenderPass_VK           renderPass[MAX_RENDER_PASS_COUNT];
        FfxmUInt32                renderPassIndex;

        ImageView_VK            imageView[MAX_IMAGE_VIEW_COUNT];
        FfxmUInt32                imageViewIndex;

        FrameBuffer_VK          frameBuffer[MAX_FRAME_BUFFER_COUNT];
        FfxmUInt32                frameBufferIndex;

        GraphicPipeline_VK      graphicsPipeline[MAX_GRAPHICS_PIPELINE_COUNT];
        FfxmUInt32                graphicsPipelineIndex;

        wchar_t                 name[64];
        FfxmUInt32               effectContextId;
    } PipelineLayout;

    typedef struct VKFunctionTable
    {
        PFN_vkGetDeviceProcAddr                  vkGetDeviceProcAddr = 0;
        PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = 0;
        PFN_vkGetPhysicalDeviceMemoryProperties  vkGetPhysicalDeviceMemoryProperties = 0;
        PFN_vkGetPhysicalDeviceProperties2       vkGetPhysicalDeviceProperties2 = 0;
        PFN_vkGetPhysicalDeviceFeatures2         vkGetPhysicalDeviceFeatures2 = 0;
        PFN_vkCmdSetViewport                     vkCmdSetViewport = 0;
        PFN_vkCmdSetScissor                      vkCmdSetScissor = 0;
        PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT = 0;
        PFN_vkCreateDescriptorPool          vkCreateDescriptorPool = 0;
        PFN_vkCreateSampler                 vkCreateSampler = 0;
        PFN_vkCreateDescriptorSetLayout     vkCreateDescriptorSetLayout = 0;
        PFN_vkCreateBuffer                  vkCreateBuffer = 0;
        PFN_vkCreateImage                   vkCreateImage = 0;
        PFN_vkCreateImageView               vkCreateImageView = 0;
        PFN_vkCreateShaderModule            vkCreateShaderModule = 0;
        PFN_vkCreatePipelineLayout          vkCreatePipelineLayout = 0;
        PFN_vkCreateComputePipelines        vkCreateComputePipelines = 0;
        PFN_vkCreateGraphicsPipelines       vkCreateGraphicsPipelines = 0;
        PFN_vkCreateRenderPass              vkCreateRenderPass = 0;
        PFN_vkCreateFramebuffer             vkCreateFramebuffer = 0;
        PFN_vkDestroyPipelineLayout         vkDestroyPipelineLayout = 0;
        PFN_vkDestroyPipeline               vkDestroyPipeline = 0;
        PFN_vkDestroyRenderPass             vkDestroyRenderPass = 0;
        PFN_vkDestroyFramebuffer            vkDestroyFramebuffer = 0;
        PFN_vkDestroyImage                  vkDestroyImage = 0;
        PFN_vkDestroyImageView              vkDestroyImageView = 0;
        PFN_vkDestroyBuffer                 vkDestroyBuffer = 0;
        PFN_vkDestroyDescriptorSetLayout    vkDestroyDescriptorSetLayout = 0;
        PFN_vkDestroyDescriptorPool         vkDestroyDescriptorPool = 0;
        PFN_vkDestroySampler                vkDestroySampler = 0;
        PFN_vkDestroyShaderModule           vkDestroyShaderModule = 0;
        PFN_vkGetBufferMemoryRequirements   vkGetBufferMemoryRequirements = 0;
        PFN_vkGetImageMemoryRequirements    vkGetImageMemoryRequirements = 0;
        PFN_vkAllocateDescriptorSets        vkAllocateDescriptorSets = 0;
        PFN_vkAllocateMemory                vkAllocateMemory = 0;
        PFN_vkFreeMemory                    vkFreeMemory = 0;
        PFN_vkMapMemory                     vkMapMemory = 0;
        PFN_vkUnmapMemory                   vkUnmapMemory = 0;
        PFN_vkBindBufferMemory              vkBindBufferMemory = 0;
        PFN_vkBindImageMemory               vkBindImageMemory = 0;
        PFN_vkUpdateDescriptorSets          vkUpdateDescriptorSets = 0;
        PFN_vkFlushMappedMemoryRanges       vkFlushMappedMemoryRanges = 0;
        PFN_vkCmdPipelineBarrier            vkCmdPipelineBarrier = 0;
        PFN_vkCmdBindPipeline               vkCmdBindPipeline = 0;
        PFN_vkCmdBindDescriptorSets         vkCmdBindDescriptorSets = 0;
        PFN_vkCmdDispatch                   vkCmdDispatch = 0;
        PFN_vkCmdDispatchIndirect           vkCmdDispatchIndirect = 0;
        PFN_vkCmdCopyBuffer                 vkCmdCopyBuffer = 0;
        PFN_vkCmdCopyImage                  vkCmdCopyImage = 0;
        PFN_vkCmdCopyBufferToImage          vkCmdCopyBufferToImage = 0;
        PFN_vkCmdClearColorImage            vkCmdClearColorImage = 0;
        PFN_vkCmdFillBuffer                 vkCmdFillBuffer = 0;
        PFN_vkCmdBeginRenderPass            vkCmdBeginRenderPass = 0;
        PFN_vkCmdEndRenderPass              vkCmdEndRenderPass = 0;
        PFN_vkCmdDraw                       vkCmdDraw = 0;
    } VkFunctionTable;

    VkDevice                device = nullptr;
    VkPhysicalDevice        physicalDevice = nullptr;
    VkFunctionTable         vkFunctionTable = {};

    FfxmGpuJobDescription*   pGpuJobs;
    FfxmUInt32                gpuJobCount = 0;

    typedef struct VkResourceView {
        VkImageView imageView;
    } VkResourceView;
    VkResourceView*         pResourceViews;

    VkDeviceMemory          ringBufferMemory = nullptr;
    VkMemoryPropertyFlags   ringBufferMemoryProperties = 0;
    UniformBuffer*          pRingBuffer;
    FfxmUInt32                ringBufferBase = 0;

    PipelineLayout*         pPipelineLayouts;

    VkDescriptorPool        descriptorPool;

    VkImageMemoryBarrier    imageMemoryBarriers[FFXM_MAX_BARRIERS] = {};
    VkBufferMemoryBarrier   bufferMemoryBarriers[FFXM_MAX_BARRIERS] = {};
    FfxmUInt32               scheduledImageBarrierCount = 0;
    FfxmUInt32               scheduledBufferBarrierCount = 0;
    VkPipelineStageFlags    srcStageMask = 0;
    VkPipelineStageFlags    dstStageMask = 0;

    typedef struct alignas(32) EffectContext {

        // Resource allocation
        FfxmUInt32              nextStaticResource;
        FfxmUInt32              nextDynamicResource;

        // UAV offsets
        FfxmUInt32              nextStaticResourceView;
        FfxmUInt32              nextDynamicResourceView[FFXM_MAX_QUEUED_FRAMES];

        // Pipeline layout
		FfxmUInt32			   nextPipelineLayout;

        // the frame index for the context
        FfxmUInt32              frameIndex;

        // Usage
        bool                  active;

        bool                  isEndOfUpscaler;
    } EffectContext;

    Resource*               pResources;
    EffectContext*          pEffectContexts;

    FfxmUInt32               numDeviceExtensions = 0;
    VkExtensionProperties*  extensionProperties = nullptr;

} BackendContext_VK;

FFXM_API size_t ffxmGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts, PFN_vkEnumerateDeviceExtensionProperties pfnEnumDeviceExtProps)
{
    FfxmUInt32 numExtensions = 0;

    if (physicalDevice && pfnEnumDeviceExtProps)
        pfnEnumDeviceExtProps(physicalDevice, nullptr, &numExtensions, nullptr);

    FfxmUInt32 extensionPropArraySize = sizeof(VkExtensionProperties) * numExtensions;
    FfxmUInt32 gpuJobDescArraySize = FFXM_ALIGN_UP(maxContexts * FFXM_MAX_GPU_JOBS * sizeof(FfxmGpuJobDescription), sizeof(FfxmUInt32));
    FfxmUInt32 resourceViewArraySize = FFXM_ALIGN_UP(maxContexts * FFXM_MAX_QUEUED_FRAMES * FFXM_MAX_RESOURCE_COUNT * 2 * sizeof(BackendContext_VK::VkResourceView), sizeof(FfxmUInt32));
    FfxmUInt32 ringBufferArraySize = FFXM_ALIGN_UP(maxContexts * FFXM_RING_BUFFER_SIZE * sizeof(BackendContext_VK::UniformBuffer), sizeof(FfxmUInt32));
    FfxmUInt32 pipelineArraySize = FFXM_ALIGN_UP(maxContexts * FFXM_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(FfxmUInt32));
    FfxmUInt32 resourceArraySize = FFXM_ALIGN_UP(maxContexts * FFXM_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(FfxmUInt32));
    FfxmUInt32 contextArraySize = FFXM_ALIGN_UP(maxContexts * sizeof(BackendContext_VK::EffectContext), sizeof(FfxmUInt32));

    return FFXM_ALIGN_UP(sizeof(BackendContext_VK) + extensionPropArraySize + gpuJobDescArraySize + resourceArraySize + ringBufferArraySize +
        pipelineArraySize + resourceArraySize + contextArraySize, sizeof(uint64_t));
}

// Create a FfxmDevice from a VkDevice
FfxmDevice ffxmGetDeviceVK(VkDeviceContext* vkDeviceContext)
{
    sVkDeviceContext = *vkDeviceContext;
    return reinterpret_cast<FfxmDevice>(&sVkDeviceContext);
}

FfxmErrorCode ffxmGetInterfaceVK(
    FfxmInterface* backendInterface,
    FfxmDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts)
{
    FFXM_RETURN_ON_ERROR(
        !s_BackendRefCount,
        FFXM_ERROR_BACKEND_API_ERROR);
    FFXM_RETURN_ON_ERROR(
        backendInterface,
        FFXM_ERROR_INVALID_POINTER);
    FFXM_RETURN_ON_ERROR(
        scratchBuffer,
        FFXM_ERROR_INVALID_POINTER);
    VkDeviceContext* devCtxForSize = (VkDeviceContext*)device;
    PFN_vkEnumerateDeviceExtensionProperties pfnEnumExtProps = nullptr;
    if (devCtxForSize->vkGetInstanceProcAddr && devCtxForSize->vkInstance)
        pfnEnumExtProps = (PFN_vkEnumerateDeviceExtensionProperties)devCtxForSize->vkGetInstanceProcAddr(devCtxForSize->vkInstance, "vkEnumerateDeviceExtensionProperties");
    FFXM_RETURN_ON_ERROR(
        scratchBufferSize >= ffxmGetScratchMemorySizeVK(devCtxForSize->vkPhysicalDevice, maxContexts, pfnEnumExtProps),
        FFXM_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetSDKVersion = GetSDKVersionVK;
    backendInterface->fpCreateBackendContext = CreateBackendContextVK;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesVK;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextVK;
    backendInterface->fpCreateResource = CreateResourceVK;
    backendInterface->fpDestroyResource = DestroyResourceVK;
    backendInterface->fpRegisterResource = RegisterResourceVK;
    backendInterface->fpGetResource = GetResourceVK;
    backendInterface->fpUnregisterResources = UnregisterResourcesVK;
    backendInterface->fpGetResourceDescription = GetResourceDescriptionVK;
    backendInterface->fpCreateComputePipeline = CreateComputePipelineVK;
    backendInterface->fpCreateGraphicsPipeline = CreateGraphicsPipelineVK;
    backendInterface->fpDestroyPipeline = DestroyPipelineVK;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobVK;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsVK;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    // Map the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    s_MaxEffectContexts = static_cast<FfxmUInt32>(maxContexts);

    return FFXM_OK;
}

FfxmCommandList ffxmGetCommandListVK(VkCommandBuffer cmdBuf)
{
    FFXM_ASSERT(NULL != cmdBuf);
    return reinterpret_cast<FfxmCommandList>(cmdBuf);
}

FfxmResource ffxmGetResourceVK(void* vkResource,
    FfxmResourceDescription          ffxmResDescription,
    [[maybe_unused]] wchar_t* ffxmResName,
    FfxmResourceStates               state /*=FFXM_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxmResource resource = {};
    resource.resource = vkResource;
    resource.state = state;
    resource.description = ffxmResDescription;

#ifdef _DEBUG
    if (ffxmResName) {
        wcscpy(resource.name, ffxmResName);
    }
#endif

    return resource;
}

FfxmUInt32 findMemoryTypeIndex(BackendContext_VK* backendContext, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags requestedProperties, VkMemoryPropertyFlags& outProperties)
{
    FFXM_ASSERT(NULL != backendContext);
    FFXM_ASSERT(NULL != backendContext->physicalDevice);

    VkPhysicalDeviceMemoryProperties memProperties;
    backendContext->vkFunctionTable.vkGetPhysicalDeviceMemoryProperties(backendContext->physicalDevice, &memProperties);

    FfxmUInt32 bestCandidate = UINT32_MAX;

    for (FfxmUInt32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & requestedProperties)) {

#ifndef __ANDROID__
            // if just device-local memory is requested, make sure this is the invisible heap to prevent over-subscribing the local heap
            if (requestedProperties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
                continue;
#endif

            bestCandidate = i;
            outProperties = memProperties.memoryTypes[i].propertyFlags;

            // if host-visible memory is requested, check for host coherency as well and if available, return immediately
            if ((requestedProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                return bestCandidate;
        }
    }

    return bestCandidate;
}

VkBufferUsageFlags ffxmGetVKBufferUsageFlagsFromResourceUsage(FfxmResourceUsage flags)
{
    VkBufferUsageFlags indirectBit = 0;

    if (flags & FFXM_RESOURCE_USAGE_INDIRECT)
        indirectBit = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (flags & FFXM_RESOURCE_USAGE_UAV)
        return indirectBit | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        return indirectBit | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VkImageType ffxmGetVKImageTypeFromResourceType(FfxmResourceType type)
{
    switch (type) {
    case(FFXM_RESOURCE_TYPE_TEXTURE1D):
        return VK_IMAGE_TYPE_1D;
    case(FFXM_RESOURCE_TYPE_TEXTURE2D):
        return VK_IMAGE_TYPE_2D;
    case(FFXM_RESOURCE_TYPE_TEXTURE_CUBE):
    case(FFXM_RESOURCE_TYPE_TEXTURE3D):
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

bool ffxmIsSurfaceFormatSRGB(FfxmSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB):
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return true;
    case (FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
    case (FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT):
    case (FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT):
    case (FFXM_SURFACE_FORMAT_R32G32_FLOAT):
    case (FFXM_SURFACE_FORMAT_R32_UINT):
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM):
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM):
    case (FFXM_SURFACE_FORMAT_R11G11B10_FLOAT):
    case (FFXM_SURFACE_FORMAT_R16G16_FLOAT):
    case (FFXM_SURFACE_FORMAT_R16G16_UINT):
    case (FFXM_SURFACE_FORMAT_R16_FLOAT):
    case (FFXM_SURFACE_FORMAT_R16_UINT):
    case (FFXM_SURFACE_FORMAT_R16_UNORM):
    case (FFXM_SURFACE_FORMAT_R16_SNORM):
    case (FFXM_SURFACE_FORMAT_R8_UNORM):
	case (FFXM_SURFACE_FORMAT_R8_SNORM):
    case (FFXM_SURFACE_FORMAT_R8_UINT):
    case (FFXM_SURFACE_FORMAT_R8G8_UNORM):
    case (FFXM_SURFACE_FORMAT_R32_FLOAT):
    case (FFXM_SURFACE_FORMAT_UNKNOWN):
        return false;
    default:
        FFXM_ASSERT_MESSAGE(false, "Format not yet supported");
        return false;
    }
}

FfxmSurfaceFormat ffxmGetSurfaceFormatFromGamma(FfxmSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case (FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case (FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case (FFXM_SURFACE_FORMAT_R32G32_FLOAT):
        return FFXM_SURFACE_FORMAT_R32G32_FLOAT;
    case (FFXM_SURFACE_FORMAT_R32_UINT):
        return FFXM_SURFACE_FORMAT_R32_UINT;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
        return FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS;
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_R11G11B10_FLOAT):
        return FFXM_SURFACE_FORMAT_R11G11B10_FLOAT;
    case (FFXM_SURFACE_FORMAT_R16G16_FLOAT):
        return FFXM_SURFACE_FORMAT_R16G16_FLOAT;
    case (FFXM_SURFACE_FORMAT_R16G16_UINT):
        return FFXM_SURFACE_FORMAT_R16G16_UINT;
    case (FFXM_SURFACE_FORMAT_R16_FLOAT):
        return FFXM_SURFACE_FORMAT_R16_FLOAT;
    case (FFXM_SURFACE_FORMAT_R16_UINT):
        return FFXM_SURFACE_FORMAT_R16_UINT;
    case (FFXM_SURFACE_FORMAT_R16_UNORM):
        return FFXM_SURFACE_FORMAT_R16_UNORM;
    case (FFXM_SURFACE_FORMAT_R16_SNORM):
        return FFXM_SURFACE_FORMAT_R16_SNORM;
    case (FFXM_SURFACE_FORMAT_R8_UNORM):
        return FFXM_SURFACE_FORMAT_R8_UNORM;
	case(FFXM_SURFACE_FORMAT_R8_SNORM):
		return FFXM_SURFACE_FORMAT_R8_SNORM;
    case (FFXM_SURFACE_FORMAT_R8G8_UNORM):
        return FFXM_SURFACE_FORMAT_R8G8_UNORM;
    case (FFXM_SURFACE_FORMAT_R32_FLOAT):
        return FFXM_SURFACE_FORMAT_R32_FLOAT;
    case (FFXM_SURFACE_FORMAT_UNKNOWN):
        return FFXM_SURFACE_FORMAT_UNKNOWN;

    default:
        FFXM_ASSERT_MESSAGE(false, "Format not yet supported");
        return FFXM_SURFACE_FORMAT_UNKNOWN;
    }
}

VkFormat ffxmGetVKSurfaceFormatFromSurfaceFormat(FfxmSurfaceFormat fmt)
{
    switch (fmt) {

    case(FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_SRGB;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_SRGB;
    case(FFXM_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case(FFXM_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFXM_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFXM_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFXM_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFXM_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
	case(FFXM_SURFACE_FORMAT_R8_SNORM):
		return VK_FORMAT_R8_SNORM;
    case(FFXM_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFXM_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFXM_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;

    default:
        FFXM_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkFormat ffxmGetVKUAVFormatFromSurfaceFormat(FfxmSurfaceFormat fmt)
{
    switch (fmt) {

    case(FFXM_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFXM_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case(FFXM_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFXM_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFXM_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFXM_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFXM_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFXM_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
	case(FFXM_SURFACE_FORMAT_R8_SNORM):
		return VK_FORMAT_R8_SNORM;
    case(FFXM_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFXM_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFXM_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFXM_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;

    default:
        FFXM_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkImageUsageFlags getVKImageUsageFlagsFromResourceUsage(FfxmResourceUsage flags)
{
    VkImageUsageFlags ret = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (flags & FFXM_RESOURCE_USAGE_RENDERTARGET) ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (flags & FFXM_RESOURCE_USAGE_UAV) ret |= (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    return ret;
}

FfxmErrorCode allocateDeviceMemory(BackendContext_VK* backendContext, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags requiredMemoryProperties, BackendContext_VK::Resource* backendResource)
{
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext, memRequirements, requiredMemoryProperties, backendResource->memoryProperties);

    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendResource->deviceMemory);

    if (result != VK_SUCCESS) {
        switch (result) {
        case(VK_ERROR_OUT_OF_HOST_MEMORY):
        case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
            return FFXM_ERROR_OUT_OF_MEMORY;
        default:
            return FFXM_ERROR_BACKEND_API_ERROR;
        }
    }

    return FFXM_OK;
}

void setVKObjectName(BackendContext_VK::VKFunctionTable& vkFunctionTable, VkDevice device, VkObjectType objectType, uint64_t object, char* name)
{
    VkDebugUtilsObjectNameInfoEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, object, name };

    if (vkFunctionTable.vkSetDebugUtilsObjectNameEXT)
        vkFunctionTable.vkSetDebugUtilsObjectNameEXT(device, &s);
}

FfxmUInt32 getDynamicResourcesStartIndex(FfxmUInt32 effectContextId)
{
    // dynamic resources are tracked from the max index
    return (effectContextId * FFXM_MAX_RESOURCE_COUNT) + FFXM_MAX_RESOURCE_COUNT - 1;
}
FfxmUInt32 getDynamicResourceViewsStartIndex(FfxmUInt32 effectContextId, FfxmUInt32 frameIndex)
{
    // dynamic resource views are tracked from the max index
    return (effectContextId * FFXM_MAX_QUEUED_FRAMES * FFXM_MAX_RESOURCE_COUNT * 2) + (frameIndex * FFXM_MAX_RESOURCE_COUNT * 2) + (FFXM_MAX_RESOURCE_COUNT * 2) - 1;
}

void destroyDynamicViews(BackendContext_VK* backendContext, FfxmUInt32 effectContextId, FfxmUInt32 frameIndex)
{
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Release image views for dynamic resources
    const FfxmUInt32 dynamicResourceViewIndexStart = getDynamicResourceViewsStartIndex(effectContextId, frameIndex);
    for (FfxmUInt32 dynamicViewIndex = effectContext.nextDynamicResourceView[frameIndex] + 1; dynamicViewIndex <= dynamicResourceViewIndexStart;
         ++dynamicViewIndex)
    {
        backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[dynamicViewIndex].imageView, VK_NULL_HANDLE);
        backendContext->pResourceViews[dynamicViewIndex].imageView = VK_NULL_HANDLE;
    }
    effectContext.nextDynamicResourceView[frameIndex] = dynamicResourceViewIndexStart;
}

VkAccessFlags getVKAccessFlagsFromResourceState(FfxmResourceStates state)
{
    switch (state) {

    case(FFXM_RESOURCE_STATE_GENERIC_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case(FFXM_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case(FFXM_RESOURCE_STATE_COMPUTE_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case(FFXM_RESOURCE_STATE_PIXEL_WRITE):
        return VK_ACCESS_SHADER_WRITE_BIT;
    case FFXM_RESOURCE_STATE_COPY_SRC:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case FFXM_RESOURCE_STATE_COPY_DEST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case FFXM_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    default:
        FFXM_ASSERT_MESSAGE(false, "State flag not yet supported");
        return VK_ACCESS_SHADER_READ_BIT;
    }
}

VkPipelineStageFlags getVKPipelineStageFlagsFromResourceState(FfxmResourceStates state)
{
    switch (state) {

    case(FFXM_RESOURCE_STATE_GENERIC_READ):
    case(FFXM_RESOURCE_STATE_UNORDERED_ACCESS):
    case(FFXM_RESOURCE_STATE_COMPUTE_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case(FFXM_RESOURCE_STATE_INDIRECT_ARGUMENT):
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    case FFXM_RESOURCE_STATE_COPY_SRC:
    case FFXM_RESOURCE_STATE_COPY_DEST:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case FFXM_RESOURCE_STATE_PIXEL_WRITE:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    default:
        FFXM_ASSERT_MESSAGE(false, "Pipeline stage flag not yet supported");
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

VkImageLayout getVKImageLayoutFromResourceState(FfxmResourceStates state)
{
    switch (state) {

    case(FFXM_RESOURCE_STATE_GENERIC_READ):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFXM_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFXM_RESOURCE_STATE_COMPUTE_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ):
    case(FFXM_RESOURCE_STATE_PIXEL_READ):
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case(FFXM_RESOURCE_STATE_PIXEL_WRITE):
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case FFXM_RESOURCE_STATE_COPY_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case FFXM_RESOURCE_STATE_COPY_DEST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    default:
        FFXM_ASSERT_MESSAGE(false, "Image layout flag not yet supported");
        return VK_IMAGE_LAYOUT_GENERAL;
    }
}

void addMutableViewForSRV(VkImageViewCreateInfo& imageViewCreateInfo, VkImageViewUsageCreateInfo& imageViewUsageCreateInfo, FfxmResourceDescription resourceDescription)
{
    if (ffxmIsSurfaceFormatSRGB(resourceDescription.format) && (resourceDescription.usage & FFXM_RESOURCE_USAGE_UAV) != 0)
    {
        // mutable is only for sRGB textures that will need a storage
        imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        imageViewUsageCreateInfo.pNext = nullptr;
        imageViewUsageCreateInfo.usage = getVKImageUsageFlagsFromResourceUsage(FFXM_RESOURCE_USAGE_READ_ONLY);  // we can assume that SRV is enough
        imageViewCreateInfo.pNext      = &imageViewUsageCreateInfo;
    }
}

void copyResourceState(BackendContext_VK::Resource* backendResource, const FfxmResource* inFfxmResource)
{
    FfxmResourceStates state = inFfxmResource->state;

    // copy the new states
    backendResource->initialState = state;
    backendResource->currentState = state;
    backendResource->undefined    = false;
    backendResource->dynamic      = true;

    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so tag the resource as undefined so we can transition it accordingly.
    if (backendResource->resourceDescription.flags & FFXM_RESOURCE_FLAGS_UNDEFINED)
    {
        backendResource->undefined                 = true;
        backendResource->resourceDescription.flags = (FfxmResourceFlags)((int)backendResource->resourceDescription.flags & ~FFXM_RESOURCE_FLAGS_UNDEFINED);
    }
}

void addBarrier(BackendContext_VK* backendContext, FfxmResourceInternal* resource, FfxmResourceStates newState)
{
    FFXM_ASSERT(NULL != backendContext);
    FFXM_ASSERT(NULL != resource);

    BackendContext_VK::Resource& ffxmResource = backendContext->pResources[resource->internalIndex];

    if(ffxmResource.currentState == newState && !ffxmResource.undefined)
    {
        return;
    }

    if (ffxmResource.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResource = ffxmResource.bufferResource;
        VkBufferMemoryBarrier* barrier = &backendContext->bufferMemoryBarriers[backendContext->scheduledBufferBarrierCount];

        FfxmResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->buffer = vkResource;
        barrier->offset = 0;
        barrier->size = VK_WHOLE_SIZE;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledBufferBarrierCount;
    }
    else
    {
        VkImage vkResource = ffxmResource.imageResource;
        VkImageMemoryBarrier* barrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];

        FfxmResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        VkImageSubresourceRange range;
        range.aspectMask = ffxmResource.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->oldLayout = ffxmResource.undefined ? VK_IMAGE_LAYOUT_UNDEFINED : ffxmResource.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : getVKImageLayoutFromResourceState(curState);
        barrier->newLayout = ffxmResource.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : getVKImageLayoutFromResourceState(newState);
        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->image = vkResource;
        barrier->subresourceRange = range;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledImageBarrierCount;
    }

    if (ffxmResource.undefined)
        ffxmResource.undefined = false;
}

void flushBarriers(BackendContext_VK* backendContext, VkCommandBuffer vkCommandBuffer)
{
    FFXM_ASSERT(NULL != backendContext);
    FFXM_ASSERT(NULL != vkCommandBuffer);

    if (backendContext->scheduledImageBarrierCount > 0 || backendContext->scheduledBufferBarrierCount > 0)
    {
        backendContext->vkFunctionTable.vkCmdPipelineBarrier(vkCommandBuffer, backendContext->srcStageMask, backendContext->dstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, backendContext->scheduledBufferBarrierCount, backendContext->bufferMemoryBarriers, backendContext->scheduledImageBarrierCount, backendContext->imageMemoryBarriers);
        backendContext->scheduledImageBarrierCount = 0;
        backendContext->scheduledBufferBarrierCount = 0;
        backendContext->srcStageMask = 0;
        backendContext->dstStageMask = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// VK back end implementation

FfxmUInt32 GetSDKVersionVK([[maybe_unused]] FfxmInterface* backendInterface)
{
    return FFXM_SDK_MAKE_VERSION(FFXM_SDK_VERSION_MAJOR, FFXM_SDK_VERSION_MINOR, FFXM_SDK_VERSION_PATCH);
}

FfxmErrorCode CreateBackendContextVK(FfxmInterface* backendInterface, FfxmUInt32* effectContextId)
{
    VkDeviceContext* vkDeviceContext = reinterpret_cast<VkDeviceContext*>(backendInterface->device);

    FFXM_ASSERT(NULL != backendInterface);
    FFXM_ASSERT(NULL != vkDeviceContext);
    FFXM_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkDevice);
    FFXM_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkPhysicalDevice);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!s_BackendRefCount) {

        // clear out mem prior to initializing
        memset(backendContext, 0, sizeof(BackendContext_VK));

        // Map all of our pointers
        FfxmUInt32 gpuJobDescArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * FFXM_MAX_GPU_JOBS * sizeof(FfxmGpuJobDescription), sizeof(FfxmUInt32));
        FfxmUInt32 resourceViewArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * FFXM_MAX_QUEUED_FRAMES * FFXM_MAX_RESOURCE_COUNT * 2 * sizeof(BackendContext_VK::VkResourceView), sizeof(FfxmUInt32));
        FfxmUInt32 ringBufferArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * FFXM_RING_BUFFER_SIZE * sizeof(BackendContext_VK::UniformBuffer), sizeof(FfxmUInt32));
        FfxmUInt32 pipelineArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * FFXM_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(FfxmUInt32));
        FfxmUInt32 resourceArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(FfxmUInt32));
        FfxmUInt32 contextArraySize = FFXM_ALIGN_UP(s_MaxEffectContexts * sizeof(BackendContext_VK::EffectContext), sizeof(FfxmUInt32));
        uint8_t* pMem = (uint8_t*)((BackendContext_VK*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxmGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resource view array
        backendContext->pResourceViews = (BackendContext_VK::VkResourceView*)(pMem);
        memset(backendContext->pResourceViews, 0, resourceViewArraySize);
        pMem += resourceViewArraySize;

        // Map the ring buffer array
        backendContext->pRingBuffer = (BackendContext_VK::UniformBuffer*)pMem;
        memset(backendContext->pRingBuffer, 0, ringBufferArraySize);
        pMem += ringBufferArraySize;

        // Map pipeline array
        backendContext->pPipelineLayouts = (BackendContext_VK::PipelineLayout*)pMem;
        memset(backendContext->pPipelineLayouts, 0, pipelineArraySize);
        pMem += pipelineArraySize;

        // Map resource array
        backendContext->pResources = (BackendContext_VK::Resource*)pMem;
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Clear out all resource mappings
        for (FfxmUInt32 i = 0; i < s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT; ++i) {
            backendContext->pResources[i].uavViewIndex = backendContext->pResources[i].srvViewIndex = -1;
        }

        // Map context array
        backendContext->pEffectContexts = (BackendContext_VK::EffectContext*)pMem;
        memset(backendContext->pEffectContexts, 0, contextArraySize);
        pMem += contextArraySize;

        // Map extension array
        backendContext->extensionProperties = (VkExtensionProperties*)pMem;


        // Host must provide vkDeviceProcAddr; we do not link against vulkan-loader prototypes.
        FFXM_ASSERT(vkDeviceContext->vkDeviceProcAddr != NULL);

        if (vkDeviceContext->vkDevice != VK_NULL_HANDLE) {
            backendContext->device = vkDeviceContext->vkDevice;
        }

        if (vkDeviceContext->vkPhysicalDevice != VK_NULL_HANDLE) {
            backendContext->physicalDevice = vkDeviceContext->vkPhysicalDevice;
        }

#ifdef FFXM_VKLOADER_VOLK
        backendContext->vkFunctionTable.vkEnumerateDeviceExtensionProperties = vkEnumerateDeviceExtensionProperties;
        backendContext->vkFunctionTable.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        backendContext->vkFunctionTable.vkGetPhysicalDeviceProperties2 = vkGetPhysicalDeviceProperties2;
        backendContext->vkFunctionTable.vkGetPhysicalDeviceFeatures2 = vkGetPhysicalDeviceFeatures2;
        backendContext->vkFunctionTable.vkCmdSetViewport = vkCmdSetViewport;
        backendContext->vkFunctionTable.vkCmdSetScissor = vkCmdSetScissor;
        backendContext->vkFunctionTable.vkSetDebugUtilsObjectNameEXT = vkSetDebugUtilsObjectNameEXT;
        backendContext->vkFunctionTable.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        backendContext->vkFunctionTable.vkCreateDescriptorPool = vkCreateDescriptorPool;
        backendContext->vkFunctionTable.vkCreateSampler = vkCreateSampler;
        backendContext->vkFunctionTable.vkCreateDescriptorSetLayout = vkCreateDescriptorSetLayout;
        backendContext->vkFunctionTable.vkCreateBuffer = vkCreateBuffer;
        backendContext->vkFunctionTable.vkCreateImage = vkCreateImage;
        backendContext->vkFunctionTable.vkCreateImageView = vkCreateImageView;
        backendContext->vkFunctionTable.vkCreateShaderModule = vkCreateShaderModule;
        backendContext->vkFunctionTable.vkCreatePipelineLayout = vkCreatePipelineLayout;
        backendContext->vkFunctionTable.vkCreateComputePipelines = vkCreateComputePipelines;
        backendContext->vkFunctionTable.vkCreateGraphicsPipelines = vkCreateGraphicsPipelines;
        backendContext->vkFunctionTable.vkCreateRenderPass = vkCreateRenderPass;
        backendContext->vkFunctionTable.vkCreateFramebuffer = vkCreateFramebuffer;
        backendContext->vkFunctionTable.vkDestroyPipelineLayout = vkDestroyPipelineLayout;
        backendContext->vkFunctionTable.vkDestroyPipeline = vkDestroyPipeline;
        backendContext->vkFunctionTable.vkDestroyRenderPass = vkDestroyRenderPass;
        backendContext->vkFunctionTable.vkDestroyFramebuffer = vkDestroyFramebuffer;
        backendContext->vkFunctionTable.vkDestroyImage = vkDestroyImage;
        backendContext->vkFunctionTable.vkDestroyImageView = vkDestroyImageView;
        backendContext->vkFunctionTable.vkDestroyBuffer = vkDestroyBuffer;
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout = vkDestroyDescriptorSetLayout;
        backendContext->vkFunctionTable.vkDestroyDescriptorPool = vkDestroyDescriptorPool;
        backendContext->vkFunctionTable.vkDestroySampler = vkDestroySampler;
        backendContext->vkFunctionTable.vkDestroyShaderModule = vkDestroyShaderModule;
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        backendContext->vkFunctionTable.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        backendContext->vkFunctionTable.vkAllocateDescriptorSets = vkAllocateDescriptorSets;
        backendContext->vkFunctionTable.vkAllocateMemory = vkAllocateMemory;
        backendContext->vkFunctionTable.vkFreeMemory = vkFreeMemory;
        backendContext->vkFunctionTable.vkMapMemory = vkMapMemory;
        backendContext->vkFunctionTable.vkUnmapMemory = vkUnmapMemory;
        backendContext->vkFunctionTable.vkBindBufferMemory = vkBindBufferMemory;
        backendContext->vkFunctionTable.vkBindImageMemory = vkBindImageMemory;
        backendContext->vkFunctionTable.vkUpdateDescriptorSets = vkUpdateDescriptorSets;
        backendContext->vkFunctionTable.vkCmdPipelineBarrier = vkCmdPipelineBarrier;
        backendContext->vkFunctionTable.vkCmdBindPipeline = vkCmdBindPipeline;
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets = vkCmdBindDescriptorSets;
        backendContext->vkFunctionTable.vkCmdDispatch = vkCmdDispatch;
        backendContext->vkFunctionTable.vkCmdDispatchIndirect = vkCmdDispatchIndirect;
        backendContext->vkFunctionTable.vkCmdCopyBuffer = vkCmdCopyBuffer;
        backendContext->vkFunctionTable.vkCmdCopyImage = vkCmdCopyImage;
        backendContext->vkFunctionTable.vkCmdCopyBufferToImage = vkCmdCopyBufferToImage;
        backendContext->vkFunctionTable.vkCmdClearColorImage = vkCmdClearColorImage;
        backendContext->vkFunctionTable.vkCmdFillBuffer = vkCmdFillBuffer;
        backendContext->vkFunctionTable.vkCmdBeginRenderPass = vkCmdBeginRenderPass;
        backendContext->vkFunctionTable.vkCmdEndRenderPass = vkCmdEndRenderPass;
        backendContext->vkFunctionTable.vkCmdDraw = vkCmdDraw;
#else
        // load vulkan functions
        // Instance-level functions: load via vkGetInstanceProcAddr (with fallback to vkDeviceProcAddr if not provided).
        auto loadInstanceFn = [&](const char* name) -> PFN_vkVoidFunction {
            if (vkDeviceContext->vkGetInstanceProcAddr && vkDeviceContext->vkInstance)
                return vkDeviceContext->vkGetInstanceProcAddr(vkDeviceContext->vkInstance, name);
            return vkDeviceContext->vkDeviceProcAddr(backendContext->device, name);
        };
        backendContext->vkFunctionTable.vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)loadInstanceFn("vkEnumerateDeviceExtensionProperties");
        backendContext->vkFunctionTable.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)loadInstanceFn("vkGetPhysicalDeviceMemoryProperties");
        backendContext->vkFunctionTable.vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)loadInstanceFn("vkGetPhysicalDeviceProperties2");
        backendContext->vkFunctionTable.vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)loadInstanceFn("vkGetPhysicalDeviceFeatures2");
        backendContext->vkFunctionTable.vkCmdSetViewport = (PFN_vkCmdSetViewport)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdSetViewport");
        backendContext->vkFunctionTable.vkCmdSetScissor = (PFN_vkCmdSetScissor)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdSetScissor");
        backendContext->vkFunctionTable.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkSetDebugUtilsObjectNameEXT");
        backendContext->vkFunctionTable.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFlushMappedMemoryRanges");
        backendContext->vkFunctionTable.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorPool");
        backendContext->vkFunctionTable.vkCreateSampler = (PFN_vkCreateSampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateSampler");
        backendContext->vkFunctionTable.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorSetLayout");
        backendContext->vkFunctionTable.vkCreateBuffer = (PFN_vkCreateBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateBuffer");
        backendContext->vkFunctionTable.vkCreateImage = (PFN_vkCreateImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImage");
        backendContext->vkFunctionTable.vkCreateImageView = (PFN_vkCreateImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImageView");
        backendContext->vkFunctionTable.vkCreateShaderModule = (PFN_vkCreateShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateShaderModule");
        backendContext->vkFunctionTable.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreatePipelineLayout");
        backendContext->vkFunctionTable.vkCreateComputePipelines = (PFN_vkCreateComputePipelines)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateComputePipelines");
        backendContext->vkFunctionTable.vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateGraphicsPipelines");
        backendContext->vkFunctionTable.vkCreateRenderPass = (PFN_vkCreateRenderPass)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateRenderPass");
        backendContext->vkFunctionTable.vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateFramebuffer");
        backendContext->vkFunctionTable.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipelineLayout");
        backendContext->vkFunctionTable.vkDestroyPipeline = (PFN_vkDestroyPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipeline");
        backendContext->vkFunctionTable.vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyRenderPass");
        backendContext->vkFunctionTable.vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyFramebuffer");
        backendContext->vkFunctionTable.vkDestroyImage = (PFN_vkDestroyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImage");
        backendContext->vkFunctionTable.vkDestroyImageView = (PFN_vkDestroyImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImageView");
        backendContext->vkFunctionTable.vkDestroyBuffer = (PFN_vkDestroyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyBuffer");
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorSetLayout");
        backendContext->vkFunctionTable.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorPool");
        backendContext->vkFunctionTable.vkDestroySampler = (PFN_vkDestroySampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroySampler");
        backendContext->vkFunctionTable.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyShaderModule");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetBufferMemoryRequirements");
        backendContext->vkFunctionTable.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetImageMemoryRequirements");
        backendContext->vkFunctionTable.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateDescriptorSets");
        backendContext->vkFunctionTable.vkAllocateMemory = (PFN_vkAllocateMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateMemory");
        backendContext->vkFunctionTable.vkFreeMemory = (PFN_vkFreeMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFreeMemory");
        backendContext->vkFunctionTable.vkMapMemory = (PFN_vkMapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkMapMemory");
        backendContext->vkFunctionTable.vkUnmapMemory = (PFN_vkUnmapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUnmapMemory");
        backendContext->vkFunctionTable.vkBindBufferMemory = (PFN_vkBindBufferMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindBufferMemory");
        backendContext->vkFunctionTable.vkBindImageMemory = (PFN_vkBindImageMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindImageMemory");
        backendContext->vkFunctionTable.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUpdateDescriptorSets");
        backendContext->vkFunctionTable.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdPipelineBarrier");
        backendContext->vkFunctionTable.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindPipeline");
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindDescriptorSets");
        backendContext->vkFunctionTable.vkCmdDispatch = (PFN_vkCmdDispatch)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatch");
        backendContext->vkFunctionTable.vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatchIndirect");
        backendContext->vkFunctionTable.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBuffer");
        backendContext->vkFunctionTable.vkCmdCopyImage = (PFN_vkCmdCopyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyImage");
        backendContext->vkFunctionTable.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBufferToImage");
        backendContext->vkFunctionTable.vkCmdClearColorImage = (PFN_vkCmdClearColorImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdClearColorImage");
        backendContext->vkFunctionTable.vkCmdFillBuffer = (PFN_vkCmdFillBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdFillBuffer");
        backendContext->vkFunctionTable.vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBeginRenderPass");
        backendContext->vkFunctionTable.vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdEndRenderPass");
        backendContext->vkFunctionTable.vkCmdDraw = (PFN_vkCmdDraw)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDraw");
#endif // FFXM_VKLOADER_VOLK

        // enumerate all the device extensions
        backendContext->numDeviceExtensions = 0;
        backendContext->vkFunctionTable.vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, nullptr);
        backendContext->vkFunctionTable.vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, backendContext->extensionProperties);

        // create a global descriptor pool to hold all descriptors we'll need
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES },
            { VK_DESCRIPTOR_TYPE_SAMPLER, s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, s_MaxEffectContexts * FFXM_MAX_RESOURCE_COUNT * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES },
        };

        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.poolSizeCount = 5;
        descriptorPoolCreateInfo.pPoolSizes = poolSizes;
        descriptorPoolCreateInfo.maxSets = s_MaxEffectContexts * FFXM_MAX_PASS_COUNT * FFXM_MAX_QUEUED_FRAMES;

        if (backendContext->vkFunctionTable.vkCreateDescriptorPool(backendContext->device, &descriptorPoolCreateInfo, nullptr, &backendContext->descriptorPool) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

        // allocate ring buffer of uniform buffers
        {
            for (FfxmUInt32 i = 0; i < FFXM_RING_BUFFER_SIZE * s_MaxEffectContexts; i++)
            {
                BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = FFXM_BUFFER_SIZE;
                bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &uBuffer.bufferResource) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
            }

            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendContext->pRingBuffer[0].bufferResource, &memRequirements);

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = s_MaxEffectContexts * FFXM_RING_BUFFER_MEM_BLOCK_SIZE;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext, memRequirements, requiredMemoryProperties, backendContext->ringBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX) {
                requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(backendContext, memRequirements, requiredMemoryProperties, backendContext->ringBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
            }

            VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendContext->ringBufferMemory);

            if (result != VK_SUCCESS) {
                switch (result) {
                case(VK_ERROR_OUT_OF_HOST_MEMORY):
                case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
                    return FFXM_ERROR_OUT_OF_MEMORY;
                default:
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
            }

            // map the memory block
            uint8_t* pData = nullptr;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendContext->ringBufferMemory, 0, s_MaxEffectContexts * FFXM_RING_BUFFER_MEM_BLOCK_SIZE, 0, reinterpret_cast<void**>(&pData)) != VK_SUCCESS) {
                return FFXM_ERROR_BACKEND_API_ERROR;
            }

            // bind each FFXM_BUFFER_SIZE-byte block to the ubos
            for (FfxmUInt32 i = 0; i < s_MaxEffectContexts * FFXM_RING_BUFFER_SIZE; i++)
            {
                BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];

                // get the buffer memory requirements for each buffer object to silence validation errors
                VkMemoryRequirements memRequirements = {};
                backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, uBuffer.bufferResource, &memRequirements);

                uBuffer.pData = pData + FFXM_BUFFER_SIZE * i;

                if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, uBuffer.bufferResource, backendContext->ringBufferMemory, FFXM_BUFFER_SIZE * i) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
            }
        }
    }

    // Increment the ref count
    ++s_BackendRefCount;

    // Get an available context id
	for(FfxmUInt32 i = 0; i < s_MaxEffectContexts; ++i)
	{
        if (!backendContext->pEffectContexts[i].active)
		{
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.nextStaticResource = (i * FFXM_MAX_RESOURCE_COUNT);
            effectContext.nextDynamicResource = getDynamicResourcesStartIndex(i);
            effectContext.nextStaticResourceView = (i * FFXM_MAX_QUEUED_FRAMES * FFXM_MAX_RESOURCE_COUNT * 2);
            for (FfxmUInt32 frameIndex = 0; frameIndex < FFXM_MAX_QUEUED_FRAMES; ++frameIndex)
            {
                effectContext.nextDynamicResourceView[frameIndex] = getDynamicResourceViewsStartIndex(i, frameIndex);
            }
            effectContext.nextPipelineLayout = (i * FFXM_MAX_PASS_COUNT);
            effectContext.frameIndex = 0;
            break;
        }
    }

    return FFXM_OK;
}

FfxmErrorCode GetDeviceCapabilitiesVK(FfxmInterface* backendInterface, FfxmDeviceCapabilities* deviceCapabilities)
{
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // no shader model in vulkan so assume the minimum
    deviceCapabilities->minimumSupportedShaderModel = FFXM_SHADER_MODEL_5_1;
    deviceCapabilities->waveLaneCountMin = 32;
    deviceCapabilities->waveLaneCountMax = 32;
    deviceCapabilities->fp16Supported = false;
    deviceCapabilities->raytracingSupported = false;

    BackendContext_VK* context = (BackendContext_VK*)backendInterface->scratchBuffer;

    // check if extensions are enabled

    for (FfxmUInt32 i = 0; i < backendContext->numDeviceExtensions; i++)
    {
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME) == 0)
        {
            // check if we the max subgroup size allows us to use wave64
            VkPhysicalDeviceSubgroupSizeControlProperties subgroupSizeControlProperties = {};
            subgroupSizeControlProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;

            VkPhysicalDeviceProperties2 deviceProperties2 = {};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            deviceProperties2.pNext = &subgroupSizeControlProperties;
            backendContext->vkFunctionTable.vkGetPhysicalDeviceProperties2(context->physicalDevice, &deviceProperties2);

            deviceCapabilities->waveLaneCountMin = subgroupSizeControlProperties.minSubgroupSize;
            deviceCapabilities->waveLaneCountMax = subgroupSizeControlProperties.maxSubgroupSize;
        }
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) == 0)
        {
            // check for fp16 support
            VkPhysicalDeviceShaderFloat16Int8Features shaderFloat18Int8Features = {};
            shaderFloat18Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &shaderFloat18Int8Features;

            backendContext->vkFunctionTable.vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->fp16Supported = (bool)shaderFloat18Int8Features.shaderFloat16;
        }
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
        {
            // check for ray tracing support
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &accelerationStructureFeatures;

            backendContext->vkFunctionTable.vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->raytracingSupported = (bool)accelerationStructureFeatures.accelerationStructure;
        }
    }

    return FFXM_OK;
}

FfxmErrorCode DestroyBackendContextVK(FfxmInterface* backendInterface, FfxmUInt32 effectContextId)
{
    FFXM_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Delete any resources allocated by this context
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
	for(FfxmInt32 currentStaticResourceIndex = FfxmInt32(effectContextId * FFXM_MAX_RESOURCE_COUNT); currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex)
	{

        if (backendContext->pResources[currentStaticResourceIndex].imageResource != VK_NULL_HANDLE) {
            FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxmResourceInternal internalResource = { currentStaticResourceIndex };
            DestroyResourceVK(backendInterface, internalResource);
        }
    }

    for (FfxmUInt32 frameIndex = 0; frameIndex < FFXM_MAX_QUEUED_FRAMES; ++frameIndex)
        destroyDynamicViews(backendContext, effectContextId, frameIndex);

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    effectContext.isEndOfUpscaler = false;

    // Decrement ref count
    --s_BackendRefCount;

    if (!s_BackendRefCount) {

        // clean up descriptor pool
        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, backendContext->descriptorPool, VK_NULL_HANDLE);
        backendContext->descriptorPool = VK_NULL_HANDLE;

        // clean up ring buffer & memory
        for (FfxmUInt32 i = 0; i < s_MaxEffectContexts * FFXM_RING_BUFFER_SIZE; i++) {
            BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[i];
            backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, uBuffer.bufferResource, VK_NULL_HANDLE);
            uBuffer.bufferResource = VK_NULL_HANDLE;
            uBuffer.pData = VK_NULL_HANDLE;
        }

        backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendContext->ringBufferMemory);
        backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backendContext->ringBufferMemory, VK_NULL_HANDLE);
        backendContext->ringBufferMemory = VK_NULL_HANDLE;

        if (backendContext->device != VK_NULL_HANDLE)
            backendContext->device = VK_NULL_HANDLE;
    }

    return FFXM_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxmErrorCode CreateResourceVK(
    FfxmInterface* backendInterface,
    const FfxmCreateResourceDescription* createResourceDescription,
    FfxmUInt32 effectContextId,
    FfxmResourceInternal* outResource)
{
    FFXM_ASSERT(NULL != backendInterface);
    FFXM_ASSERT(NULL != createResourceDescription);
    FFXM_ASSERT(NULL != outResource);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    VkDevice vkDevice = backendContext->device;

    FFXM_ASSERT(VK_NULL_HANDLE != vkDevice);

    VkMemoryPropertyFlags requiredMemoryProperties;
    if (createResourceDescription->heapType == FFXM_HEAP_TYPE_UPLOAD)
        requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    else
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Setup the resource description
    FfxmResourceDescription resourceDesc = createResourceDescription->resourceDescription;

    if (resourceDesc.mipCount == 0) {
        resourceDesc.mipCount = (FfxmUInt32)(1 + floor(log2(FFXM_MAXIMUM(FFXM_MAXIMUM(createResourceDescription->resourceDescription.width,
            createResourceDescription->resourceDescription.height), createResourceDescription->resourceDescription.depth))));
    }

    FFXM_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);
    outResource->internalIndex = effectContext.nextStaticResource++;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outResource->internalIndex];
    backendResource->undefined = true;  // A flag to make sure the first barrier for this image resource always uses an src layout of undefined
    backendResource->dynamic = false;   // Not a dynamic resource (need to track them separately for image views)
    backendResource->resourceDescription = resourceDesc;

    const FfxmResourceStates resourceState = (createResourceDescription->initData && (createResourceDescription->heapType != FFXM_HEAP_TYPE_UPLOAD)) ? FFXM_RESOURCE_STATE_COPY_DEST : createResourceDescription->initalState;
    backendResource->initialState = resourceState;
    backendResource->currentState = resourceState;

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), createResourceDescription->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    VkMemoryRequirements memRequirements = {};

    switch (createResourceDescription->resourceDescription.type)
    {
    case FFXM_RESOURCE_TYPE_BUFFER:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = createResourceDescription->resourceDescription.width;
        bufferInfo.usage = ffxmGetVKBufferUsageFlagsFromResourceUsage(resourceDesc.usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (createResourceDescription->initData)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendResource->bufferResource) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)backendResource->bufferResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendResource->bufferResource, &memRequirements);

        // allocate the memory
        FfxmErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFXM_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, backendResource->bufferResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

        // if this is an upload buffer (currently only support upload buffers), copy the data and return
        if (createResourceDescription->heapType == FFXM_HEAP_TYPE_UPLOAD)
        {
            // only allow copies directly into mapped memory for buffer resources since all texture resources are in optimal tiling
            void* data = NULL;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendResource->deviceMemory, 0, createResourceDescription->initDataSize, 0, &data) != VK_SUCCESS) {
                return FFXM_ERROR_BACKEND_API_ERROR;
            }

            memcpy(data, createResourceDescription->initData, createResourceDescription->initDataSize);

            // flush mapped range if memory type is not coherant
            if ((backendResource->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendResource->deviceMemory;
                memoryRange.size = createResourceDescription->initDataSize;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }

            backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendResource->deviceMemory);
            return FFXM_OK;
        }

        break;
    }
    case FFXM_RESOURCE_TYPE_TEXTURE1D:
    case FFXM_RESOURCE_TYPE_TEXTURE2D:
    case FFXM_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFXM_RESOURCE_TYPE_TEXTURE3D:
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = ffxmGetVKImageTypeFromResourceType(createResourceDescription->resourceDescription.type);
        imageInfo.extent.width = createResourceDescription->resourceDescription.width;
        imageInfo.extent.height = createResourceDescription->resourceDescription.type == FFXM_RESOURCE_TYPE_TEXTURE1D ? 1 : createResourceDescription->resourceDescription.height;
        imageInfo.extent.depth = ( createResourceDescription->resourceDescription.type == FFXM_RESOURCE_TYPE_TEXTURE3D || createResourceDescription->resourceDescription.type == FFXM_RESOURCE_TYPE_TEXTURE_CUBE ) ?
            createResourceDescription->resourceDescription.depth : 1;
        imageInfo.mipLevels = backendResource->resourceDescription.mipCount;
        imageInfo.arrayLayers = (createResourceDescription->resourceDescription.type ==
                                FFXM_RESOURCE_TYPE_TEXTURE1D || createResourceDescription->resourceDescription.type == FFXM_RESOURCE_TYPE_TEXTURE2D)
            ? createResourceDescription->resourceDescription.depth : 1;
        imageInfo.format = ((resourceDesc.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) != 0) ? VK_FORMAT_D32_SFLOAT : ffxmGetVKSurfaceFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = getVKImageUsageFlagsFromResourceUsage(resourceDesc.usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ((resourceDesc.usage & FFXM_RESOURCE_USAGE_UAV) != 0 && ffxmIsSurfaceFormatSRGB(createResourceDescription->resourceDescription.format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = ffxmGetVKSurfaceFormatFromSurfaceFormat(ffxmGetSurfaceFormatFromGamma(createResourceDescription->resourceDescription.format));
        }

        if (backendContext->vkFunctionTable.vkCreateImage(backendContext->device, &imageInfo, nullptr, &backendResource->imageResource) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)backendResource->imageResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetImageMemoryRequirements(backendContext->device, backendResource->imageResource, &memRequirements);

        // allocate the memory
        FfxmErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFXM_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindImageMemory(backendContext->device, backendResource->imageResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

        break;
    }
    default:
        FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: Unsupported resource type creation requested.");
        break;
    }

    // Create SRVs and UAVs
    switch (createResourceDescription->resourceDescription.type)
    {
    case FFXM_RESOURCE_TYPE_BUFFER:
        break;
    case FFXM_RESOURCE_TYPE_TEXTURE1D:
    case FFXM_RESOURCE_TYPE_TEXTURE2D:
    case FFXM_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFXM_RESOURCE_TYPE_TEXTURE3D:
    {
        FFXM_ASSERT_MESSAGE(effectContext.nextStaticResourceView + 1 < effectContext.nextDynamicResourceView[0],
            "ffxmInterface: Vulkan: We've run out of resource views. Please increase the size.");
        backendResource->srvViewIndex = effectContext.nextStaticResourceView++;

        FfxmResourceType type = createResourceDescription->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFXM_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFXM_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFXM_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFXM_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFXM_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFXM_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        bool isDepth = ((backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) != 0);
        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = isDepth ? VK_FORMAT_D32_SFLOAT : ffxmGetVKSurfaceFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        // create an image view containing all mip levels for use as an srv
        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_UAV)
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFXM_ASSERT(effectContext.nextStaticResourceView + uavResourceViewCount < effectContext.nextDynamicResourceView[0]);

            backendResource->uavViewIndex = effectContext.nextStaticResourceView;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = (backendResource->resourceDescription.usage == FFXM_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxmGetVKUAVFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);

            for (FfxmUInt32 mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }

            effectContext.nextStaticResourceView += uavResourceViewCount;
        }
        break;
    }
    default:
        FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    // create upload resource and upload job if needed
    if (createResourceDescription->initData)
    {
        FfxmResourceInternal copySrc;
        FfxmCreateResourceDescription uploadDesc = { *createResourceDescription };
        uploadDesc.heapType = FFXM_HEAP_TYPE_UPLOAD;
        uploadDesc.resourceDescription.type = FFXM_RESOURCE_TYPE_BUFFER;
        uploadDesc.resourceDescription.width = createResourceDescription->initDataSize;
        uploadDesc.resourceDescription.usage = FFXM_RESOURCE_USAGE_READ_ONLY;
        uploadDesc.initalState = FFXM_RESOURCE_STATE_GENERIC_READ;
        uploadDesc.initData = createResourceDescription->initData;
        uploadDesc.initDataSize = createResourceDescription->initDataSize;

        backendInterface->fpCreateResource(backendInterface, &uploadDesc, effectContextId, &copySrc);

        // setup the upload job
        FfxmGpuJobDescription copyJob =
        {
            FFXM_GPU_JOB_COPY
        };
        copyJob.copyJobDescriptor.src = copySrc;
        copyJob.copyJobDescriptor.dst = *outResource;

        backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
    }

    return FFXM_OK;
}

FfxmErrorCode DestroyResourceVK(FfxmInterface* backendInterface, FfxmResourceInternal resource)
{
    FFXM_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

     if (resource.internalIndex != -1)
     {
         BackendContext_VK::Resource& backgroundResource = backendContext->pResources[resource.internalIndex];

         if (backgroundResource.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER)
         {
             // Destroy the resource
             if (backgroundResource.bufferResource != VK_NULL_HANDLE)
             {
                 backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, backgroundResource.bufferResource, nullptr);
                 backgroundResource.bufferResource = VK_NULL_HANDLE;
             }
         }
         else
         {
             // Destroy SRV
             backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView, nullptr);
             backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView = VK_NULL_HANDLE;
             backgroundResource.srvViewIndex = 0;

             // And UAVs
             if (backgroundResource.resourceDescription.usage & FFXM_RESOURCE_USAGE_UAV)
             {
                 for (FfxmUInt32 i = 0; i < backgroundResource.uavViewCount; ++i)
                 {
                     if (backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView != VK_NULL_HANDLE)
                     {
                         backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView, nullptr);
                         backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView = VK_NULL_HANDLE;
                     }
                 }
             }

             // Reset indices to resource views
             backgroundResource.uavViewIndex = backgroundResource.srvViewIndex = -1;
             backgroundResource.uavViewCount = 0;

             // Destroy the resource
             if (backgroundResource.imageResource != VK_NULL_HANDLE)
             {
                 backendContext->vkFunctionTable.vkDestroyImage(backendContext->device, backgroundResource.imageResource, nullptr);
                 backgroundResource.imageResource = VK_NULL_HANDLE;
             }
         }

         if (backgroundResource.deviceMemory)
         {
             backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backgroundResource.deviceMemory, nullptr);
             backgroundResource.deviceMemory = VK_NULL_HANDLE;
         }
     }

    return FFXM_OK;
}

FfxmErrorCode RegisterResourceVK(
    FfxmInterface* backendInterface,
    const FfxmResource* inFfxmResource,
    FfxmUInt32 effectContextId,
    FfxmResourceInternal* outFfxmResourceInternal
)
{
    FFXM_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if (inFfxmResource->resource == nullptr) {

        outFfxmResourceInternal->internalIndex = 0; // Always maps to FFXM_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFXM_OK;
    }

    // In vulkan we need to treat dynamic resources a little differently due to needing views to live as long as the GPU needs them.
    // We will treat them more like static resources and use the nextDynamicResource as a "hint" for where it should be.
    // Failure to find the pre-existing resource at the expected location will force a search until the resource is found.
    // If it is not found, a new entry will be created
    FFXM_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outFfxmResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    //bool setupDynamicResource = false;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outFfxmResourceInternal->internalIndex];

    // Start by seeing if this entry is empty, as that triggers an automatic setup of a new dynamic resource
    /*if (backendResource->uavViewIndex < 0 && backendResource->srvViewIndex < 0)
    {
        setupDynamicResource = true;
    }

    // If not a new resource, does it match what's current slotted for this dynamic resource
    if (!setupDynamicResource)
    {
        // If this is us, just return as everything is setup as needed
        if ((backendResource->resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxmResource->resource) ||
            (backendResource->resourceDescription.type != FFXM_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxmResource->resource))
            return FFXM_OK;

        // If this isn't us, search until we either find our entry or an empty resource
        outFfxmResourceInternal->internalIndex = (effectContextId * FFXM_MAX_RESOURCE_COUNT) + FFXM_MAX_RESOURCE_COUNT - 1;
        while (!setupDynamicResource)
        {
            FFXM_ASSERT(outFfxmResourceInternal->internalIndex > effectContext.nextStaticResource); // Safety check while iterating
            backendResource = &backendContext->pResources[outFfxmResourceInternal->internalIndex];

            // Is this us?
            if ((backendResource->resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxmResource->resource) ||
                (backendResource->resourceDescription.type != FFXM_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxmResource->resource))
            {
                 copyResourceState(backendResource, inFfxmResource);
                 return FFXM_OK;
            }

            // Empty?
            if (backendResource->uavViewIndex == -1 && backendResource->srvViewIndex == -1)
            {
                setupDynamicResource = true;
                break;
            }

            --outFfxmResourceInternal->internalIndex;
        }
    }*/

    // If we got here, we are setting up a new dynamic entry
    backendResource->resourceDescription = inFfxmResource->description;
    if (inFfxmResource->description.type == FFXM_RESOURCE_TYPE_BUFFER)
        backendResource->bufferResource = reinterpret_cast<VkBuffer>(inFfxmResource->resource);
    else
        backendResource->imageResource = reinterpret_cast<VkImage>(inFfxmResource->resource);

    copyResourceState(backendResource, inFfxmResource);

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), inFfxmResource->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    // the first call of RegisterResource can be identified because


    //////////////////////////////////////////////////////////////////////////
    // Create SRVs and UAVs
    switch (backendResource->resourceDescription.type)
    {
    case FFXM_RESOURCE_TYPE_BUFFER:
        break;
    case FFXM_RESOURCE_TYPE_TEXTURE1D:
    case FFXM_RESOURCE_TYPE_TEXTURE2D:
    case FFXM_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFXM_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxmResourceType type = backendResource->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFXM_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFXM_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFXM_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFXM_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFXM_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFXM_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = (backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxmGetVKSurfaceFormatFromSurfaceFormat(backendResource->resourceDescription.format);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = (backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // create an image view containing all mip levels for use as an srv
        FFXM_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
        backendResource->srvViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex]--;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_UAV)
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFXM_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1 > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
            backendResource->uavViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = (backendResource->resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxmGetVKUAVFormatFromSurfaceFormat(backendResource->resourceDescription.format);
            imageViewCreateInfo.pNext  = nullptr;

            for (FfxmUInt32 mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }
            effectContext.nextDynamicResourceView[effectContext.frameIndex] -= uavResourceViewCount;
        }
        break;
    }
    default:
        FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    return FFXM_OK;
}

FfxmResource GetResourceVK(FfxmInterface* backendInterface, FfxmResourceInternal inResource)
{
    FFXM_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxmResourceDescription ffxmResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxmResource resource = {};
    resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].imageResource);
    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so we will flag it as such to finish initializing it later
    // before it is used.
    if (backendContext->pResources[inResource.internalIndex].undefined) {
        ffxmResDescription.flags = (FfxmResourceFlags)((int)ffxmResDescription.flags | FFXM_RESOURCE_FLAGS_UNDEFINED);
        // Flag it as no longer being undefined as it will no longer be after workload
        // execution
        backendContext->pResources[inResource.internalIndex].undefined = false;
    }
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxmResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        mbstowcs(resource.name, backendContext->pResources[inResource.internalIndex].resourceName, strlen(backendContext->pResources[inResource.internalIndex].resourceName));
    }
#endif

    return resource;
}

// dispose dynamic resources: This should be called at the end of the frame
FfxmErrorCode UnregisterResourcesVK(FfxmInterface* backendInterface, FfxmCommandList commandList, FfxmUInt32 effectContextId)
{
    FFXM_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    const FfxmUInt32 dynamicResourceIndexStart = getDynamicResourcesStartIndex(effectContextId);
    for (FfxmUInt32 resourceIndex = ++effectContext.nextDynamicResource; resourceIndex <= dynamicResourceIndexStart; ++resourceIndex)
    {
        FfxmResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;

        BackendContext_VK::Resource* backendResource = &backendContext->pResources[resourceIndex];

        // Also clear out their srv/uav indices so they are regenerated each frame
        backendResource->uavViewIndex = -1;
        backendResource->srvViewIndex = -1;

        // Add the barrier
        addBarrier(backendContext, &internalResource, backendResource->initialState);
    }

    FFXM_ASSERT(nullptr != commandList);
    VkCommandBuffer pCmdList = reinterpret_cast<VkCommandBuffer>(commandList);

    flushBarriers(backendContext, pCmdList);

    // Just reset the dynamic resource index, but leave the images views.
    // They will be deleted in the first pipeline destroy call as they need to live until then
    effectContext.nextDynamicResource = dynamicResourceIndexStart;

    // destroy the views of the next frame
    if (effectContext.isEndOfUpscaler)
    {
        effectContext.frameIndex = (effectContext.frameIndex + 1) % FFXM_MAX_QUEUED_FRAMES;
        destroyDynamicViews(backendContext, effectContextId, effectContext.frameIndex);
    }

    return FFXM_OK;
}

FfxmResourceDescription GetResourceDescriptionVK(FfxmInterface* backendInterface, FfxmResourceInternal resource)
{
    FFXM_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxmResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

VkSamplerAddressMode FfxmGetAddressModeVK(const FfxmAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFXM_ADDRESS_MODE_WRAP:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case FFXM_ADDRESS_MODE_MIRROR:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case FFXM_ADDRESS_MODE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case FFXM_ADDRESS_MODE_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case FFXM_ADDRESS_MODE_MIRROR_ONCE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        FFXM_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    }
}

FfxmErrorCode CreateComputePipelineVK(FfxmInterface* backendInterface,
    FfxmEffect effect,
    FfxmPass pass,
	FfxmShaderQuality qualityPreset,
    FfxmUInt32 permutationOptions,
    const FfxmPipelineDescription* pipelineDescription,
    FfxmUInt32 effectContextId,
    FfxmPipelineState* outPipeline)
{
    FFXM_ASSERT(NULL != backendInterface);
    FFXM_ASSERT(NULL != pipelineDescription);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // start by fetching the shader blob
    FfxmShaderBlob shaderBlob = { };
    ffxmGetPermutationBlobByIndex(effect, pass, permutationOptions, &shaderBlob);
    FFXM_ASSERT(shaderBlob.data && shaderBlob.size);

    //////////////////////////////////////////////////////////////////////////
    // One root signature (or pipeline layout) per pipeline
    FFXM_ASSERT_MESSAGE(effectContext.nextPipelineLayout < (effectContextId * FFXM_MAX_PASS_COUNT) + FFXM_MAX_PASS_COUNT, "ffxmInterface: Vulkan: Ran out of pipeline layouts. Please increase FFXM_MAX_PASS_COUNT");
    BackendContext_VK::PipelineLayout* pPipelineLayout = &backendContext->pPipelineLayouts[effectContext.nextPipelineLayout++];

    // Start by creating samplers
    FFXM_ASSERT(pipelineDescription->samplerCount <= FFXM_MAX_SAMPLERS);
    const size_t samplerCount = pipelineDescription->samplerCount;
    for (FfxmUInt32 currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.minLod = 0.f;
        createInfo.maxLod = VK_LOD_CLAMP_NONE;
        createInfo.anisotropyEnable = false; // TODO: Do a check for anisotropy once it's an available filter option
        createInfo.compareEnable = false;
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.addressModeU = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
        createInfo.addressModeV = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
        createInfo.addressModeW = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

        // Set the right filter
        switch (pipelineDescription->samplers[currentSamplerIndex].filter)
        {
        case FFXM_FILTER_TYPE_MINMAGMIP_POINT:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case FFXM_FILTER_TYPE_MINMAGMIP_LINEAR:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case FFXM_FILTER_TYPE_MINMAGLINEARMIP_POINT:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        default:
            FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: Unsupported filter type requested. Please implement");
            break;
        }

        // Create the sampler only if it is needed
        const FfxmUInt32 curBinding = pipelineDescription->samplers[currentSamplerIndex].binding;
        for(FfxmUInt32 curBlobSamplerIdx = 0; curBlobSamplerIdx < shaderBlob.samplerCount; ++curBlobSamplerIdx)
        {
            if(curBinding == shaderBlob.boundSamplers[curBlobSamplerIdx])
            {
                if (backendContext->vkFunctionTable.vkCreateSampler(backendContext->device, &createInfo, nullptr, &pPipelineLayout->samplers[curBinding]) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
                break;
            }
        }
    }

    // Setup descriptor sets
    VkDescriptorSetLayoutBinding layoutBindings[MAX_DESCRIPTOR_SETS][MAX_DESCRIPTOR_SET_LAYOUTS];
    FfxmUInt32 numLayoutBindings[MAX_DESCRIPTOR_SETS] = { 0 };

    // Support more when needed
    VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Samplers - just the static ones for now
    for (FfxmUInt32 currentSamplerIndex = 0; currentSamplerIndex < shaderBlob.samplerCount; ++currentSamplerIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSamplerSets[currentSamplerIndex];
        FfxmUInt32 binding = shaderBlob.boundSamplers[currentSamplerIndex];
        layoutBindings[set][numLayoutBindings[set]++] =
            { binding, VK_DESCRIPTOR_TYPE_SAMPLER, shaderBlob.boundSamplerCounts[currentSamplerIndex], shaderStageFlags, &pPipelineLayout->samplers[binding] };
    }

    // Texture SRVs
    for (FfxmUInt32 srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSRVTextureSets[srvIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundSRVTextures[srvIndex], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            shaderBlob.boundSRVTextureCounts[srvIndex], shaderStageFlags, nullptr };
    }

    // Buffer SRVs
    for (FfxmUInt32 srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSRVBufferSets[srvIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundSRVBuffers[srvIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundSRVBufferCounts[srvIndex], shaderStageFlags, nullptr};
    }

    // Texture UAVs
    for (FfxmUInt32 uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        FfxmUInt32 set = shaderBlob.boundUAVTextureSets[uavIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundUAVTextures[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            shaderBlob.boundUAVTextureCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Buffer UAVs
    for (FfxmUInt32 uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        FfxmUInt32 set = shaderBlob.boundUAVBufferSets[uavIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundUAVBuffers[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundUAVBufferCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Constant buffers (uniforms)
    for (FfxmUInt32 cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        FfxmUInt32 set = shaderBlob.boundConstantBufferSets[cbIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundConstantBuffers[cbIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            shaderBlob.boundConstantBufferCounts[cbIndex], shaderStageFlags, nullptr };
    }

    // Create the descriptor layout

    FfxmUInt32 numDescriptorSets = 0;

    for (FfxmUInt32 setIndex = 0; setIndex < MAX_DESCRIPTOR_SETS; ++setIndex)
    {
        if (numLayoutBindings[setIndex] != 0)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = numLayoutBindings[setIndex];
            layoutInfo.pBindings = layoutBindings[setIndex];

            if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(backendContext->device, &layoutInfo, nullptr, &pPipelineLayout->descriptorSetLayout[setIndex]) != VK_SUCCESS) {
                return FFXM_ERROR_BACKEND_API_ERROR;
            }
            numDescriptorSets++;
        }
    }

    // allocate descriptor sets
    pPipelineLayout->descriptorSetIndex = 0;
    for (FfxmUInt32 i = 0; i < FFXM_MAX_QUEUED_FRAMES; i++)
    {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = backendContext->descriptorPool;
        allocateInfo.descriptorSetCount = numDescriptorSets;
        allocateInfo.pSetLayouts = pPipelineLayout->descriptorSetLayout;

        backendContext->vkFunctionTable.vkAllocateDescriptorSets(backendContext->device, &allocateInfo, pPipelineLayout->descriptorSets[i]);
    }

    // create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = numDescriptorSets;
    pipelineLayoutInfo.pSetLayouts = pPipelineLayout->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (backendContext->vkFunctionTable.vkCreatePipelineLayout(backendContext->device, &pipelineLayoutInfo, nullptr, &pPipelineLayout->pipelineLayout) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    wcscpy(pPipelineLayout->name, pipelineDescription->name);
    pPipelineLayout->effectContextId = effectContextId;

    // set the root signature to pipeline
    outPipeline->rootSignature = reinterpret_cast<FfxmRootSignature>(pPipelineLayout);

    // Only set the command signature if this is setup as an indirect workload
    if (pipelineDescription->indirectWorkload)
    {
        // Only need the stride ahead of time in Vulkan
        outPipeline->cmdSignature = reinterpret_cast<FfxmCommandSignature>(sizeof(VkDispatchIndirectCommand));
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    // populate the pipeline information for this pass
    outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
    outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
    outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
    outPipeline->uavBufferCount = shaderBlob.uavBufferCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    outPipeline->constCount = shaderBlob.cbvCount;

    outPipeline->descriptorSetCount = numDescriptorSets;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (FfxmUInt32 srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
    {
        outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindSet = shaderBlob.boundSRVTextureSets[srvIndex];
        wcscpy(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
    }
    for (FfxmUInt32 srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
    {
        outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindSet = shaderBlob.boundSRVBufferSets[srvIndex];
        wcscpy(outPipeline->srvBufferBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
    }
    for (FfxmUInt32 uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
    {
        outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindSet = shaderBlob.boundUAVTextureSets[uavIndex];
        wcscpy(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
    }
    for (FfxmUInt32 uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
    {
        outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindSet = shaderBlob.boundUAVBufferSets[uavIndex];
        wcscpy(outPipeline->uavBufferBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
    }
    for (FfxmUInt32 cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindSet = shaderBlob.boundConstantBufferSets[cbIndex];
        wcscpy(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
    }

    //////////////////////////////////////////////////////////////////////////
    // pipeline creation
    FfxmDeviceCapabilities capabilities;
    backendInterface->fpGetDeviceCapabilities(backendInterface, &capabilities);

    // shader module
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (FfxmUInt32*)shaderBlob.data;
    shaderModuleCreateInfo.codeSize = shaderBlob.size;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    // fill out shader stage create info
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.module = shaderModule;

    // check if wave64 is requested
    bool isWave64 = false;
    ffxmIsWave64(effect, permutationOptions, isWave64);
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroupSizeCreateInfo = {};
    if (isWave64 && (capabilities.waveLaneCountMin == 32 && capabilities.waveLaneCountMax == 64))
    {
        subgroupSizeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT;
        subgroupSizeCreateInfo.requiredSubgroupSize = 64;
        shaderStageCreateInfo.pNext = &subgroupSizeCreateInfo;
    }

    // create the compute pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = pPipelineLayout->pipelineLayout;

    VkPipeline computePipeline = VK_NULL_HANDLE;
    if (backendContext->vkFunctionTable.vkCreateComputePipelines(backendContext->device, nullptr, 1, &pipelineCreateInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    // done with shader module, so clean up
    backendContext->vkFunctionTable.vkDestroyShaderModule(backendContext->device, shaderModule, nullptr);

    // set the pipeline
    outPipeline->pipeline = reinterpret_cast<FfxmPipeline>(computePipeline);

    return FFXM_OK;
}

FfxmErrorCode CreateGraphicsPipelineVK(FfxmInterface* backendInterface,
    FfxmEffect effect,
    FfxmPass pass,
	FfxmShaderQuality qualityPreset,
    FfxmUInt32 permutationOptions,
    const FfxmPipelineDescription* pipelineDescription,
    FfxmUInt32 effectContextId,
    FfxmPipelineState* outPipeline)
{
    FFXM_ASSERT(NULL != backendInterface);
    FFXM_ASSERT(NULL != pipelineDescription);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // start by fetching the shader blob
    FfxmShaderBlob shaderBlob = { }, vertShaderBlob = { };
    ffxmGetPermutationBlobByIndex(effect, pass, permutationOptions, &shaderBlob, &vertShaderBlob);
    FFXM_ASSERT(shaderBlob.data && shaderBlob.size);
    FFXM_ASSERT(vertShaderBlob.data && vertShaderBlob.size);

    //////////////////////////////////////////////////////////////////////////
    // One root signature (or pipeline layout) per pipeline
    FFXM_ASSERT_MESSAGE(effectContext.nextPipelineLayout < (effectContextId * FFXM_MAX_PASS_COUNT) + FFXM_MAX_PASS_COUNT, "ffxmInterface: Vulkan: Ran out of pipeline layouts. Please increase FFXM_MAX_PASS_COUNT");
    BackendContext_VK::PipelineLayout* pPipelineLayout = &backendContext->pPipelineLayouts[effectContext.nextPipelineLayout++];

    // Start by creating samplers
    FFXM_ASSERT(pipelineDescription->samplerCount <= FFXM_MAX_SAMPLERS);

    for (FfxmUInt32 currentSamplerIndex = 0; currentSamplerIndex < pipelineDescription->samplerCount; ++currentSamplerIndex)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.minLod = 0.f;
        createInfo.maxLod = VK_LOD_CLAMP_NONE;
        createInfo.anisotropyEnable = false; // TODO: Do a check for anisotropy once it's an available filter option
        createInfo.compareEnable = false;
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.addressModeU = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
        createInfo.addressModeV = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
        createInfo.addressModeW = FfxmGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

        // Set the right filter
        switch (pipelineDescription->samplers[currentSamplerIndex].filter)
        {
        case FFXM_FILTER_TYPE_MINMAGMIP_POINT:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case FFXM_FILTER_TYPE_MINMAGMIP_LINEAR:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case FFXM_FILTER_TYPE_MINMAGLINEARMIP_POINT:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        default:
            FFXM_ASSERT_MESSAGE(false, "ffxmInterface: Vulkan: Unsupported filter type requested. Please implement");
            break;
        }

        // Create the sampler only if is needed
        const FfxmUInt32 curBinding = pipelineDescription->samplers[currentSamplerIndex].binding;
        for(FfxmUInt32 curBlobSamplerIdx = 0; curBlobSamplerIdx < shaderBlob.samplerCount; ++curBlobSamplerIdx)
        {
            if(curBinding == shaderBlob.boundSamplers[curBlobSamplerIdx])
            {
                if (backendContext->vkFunctionTable.vkCreateSampler(backendContext->device, &createInfo, nullptr, &pPipelineLayout->samplers[curBinding]) != VK_SUCCESS) {
                    return FFXM_ERROR_BACKEND_API_ERROR;
                }
                break;
            }
        }

    }

    // Setup descriptor sets
    VkDescriptorSetLayoutBinding layoutBindings[MAX_DESCRIPTOR_SETS][MAX_DESCRIPTOR_SET_LAYOUTS];
    FfxmUInt32 numLayoutBindings[MAX_DESCRIPTOR_SETS] = { 0 };

    // Support more when needed
    VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Samplers - just the static ones for now
    for (FfxmUInt32 currentSamplerIndex = 0; currentSamplerIndex < shaderBlob.samplerCount; ++currentSamplerIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSamplerSets[currentSamplerIndex];
        FfxmUInt32 binding = shaderBlob.boundSamplers[currentSamplerIndex];
        layoutBindings[set][numLayoutBindings[set]++] =
            { binding, VK_DESCRIPTOR_TYPE_SAMPLER, shaderBlob.boundSamplerCounts[currentSamplerIndex], shaderStageFlags, &pPipelineLayout->samplers[binding] };
    }

    // Texture SRVs
    for (FfxmUInt32 srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSRVTextureSets[srvIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundSRVTextures[srvIndex], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            shaderBlob.boundSRVTextureCounts[srvIndex], shaderStageFlags, nullptr };
    }

    // Buffer SRVs
    for (FfxmUInt32 srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        FfxmUInt32 set = shaderBlob.boundSRVBufferSets[srvIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundSRVBuffers[srvIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundSRVBufferCounts[srvIndex], shaderStageFlags, nullptr};
    }

    // Texture UAVs
    for (FfxmUInt32 uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        FfxmUInt32 set = shaderBlob.boundUAVTextureSets[uavIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundUAVTextures[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            shaderBlob.boundUAVTextureCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Buffer UAVs
    for (FfxmUInt32 uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        FfxmUInt32 set = shaderBlob.boundUAVBufferSets[uavIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundUAVBuffers[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundUAVBufferCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Constant buffers (uniforms)
    for (FfxmUInt32 cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        FfxmUInt32 set = shaderBlob.boundConstantBufferSets[cbIndex];
        layoutBindings[set][numLayoutBindings[set]++] = { shaderBlob.boundConstantBuffers[cbIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            shaderBlob.boundConstantBufferCounts[cbIndex], shaderStageFlags, nullptr };
    }

    // Create the descriptor layout

    FfxmUInt32 numDescriptorSets = 0;

    for (FfxmUInt32 setIndex = 0; setIndex < MAX_DESCRIPTOR_SETS; ++setIndex)
    {
        if (numLayoutBindings[setIndex] != 0)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = numLayoutBindings[setIndex];
            layoutInfo.pBindings = layoutBindings[setIndex];

            if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(backendContext->device, &layoutInfo, nullptr, &pPipelineLayout->descriptorSetLayout[setIndex]) != VK_SUCCESS) {
                return FFXM_ERROR_BACKEND_API_ERROR;
            }
            numDescriptorSets++;
        }
    }

    // allocate descriptor sets
    pPipelineLayout->descriptorSetIndex = 0;
    for (FfxmUInt32 i = 0; i < FFXM_MAX_QUEUED_FRAMES; i++)
    {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = backendContext->descriptorPool;
        allocateInfo.descriptorSetCount = numDescriptorSets;
        allocateInfo.pSetLayouts = pPipelineLayout->descriptorSetLayout;

        backendContext->vkFunctionTable.vkAllocateDescriptorSets(backendContext->device, &allocateInfo, pPipelineLayout->descriptorSets[i]);
    }

    // create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = numDescriptorSets;
    pipelineLayoutInfo.pSetLayouts = pPipelineLayout->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (backendContext->vkFunctionTable.vkCreatePipelineLayout(backendContext->device, &pipelineLayoutInfo, nullptr, &pPipelineLayout->pipelineLayout) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    wcscpy(pPipelineLayout->name, pipelineDescription->name);
    pPipelineLayout->effectContextId = effectContextId;

    // set the root signature to pipeline
    outPipeline->rootSignature = reinterpret_cast<FfxmRootSignature>(pPipelineLayout);

    // Only set the command signature if this is setup as an indirect workload
    if (pipelineDescription->indirectWorkload)
    {
        // Only need the stride ahead of time in Vulkan
        outPipeline->cmdSignature = reinterpret_cast<FfxmCommandSignature>(sizeof(VkDispatchIndirectCommand));
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    // populate the pipeline information for this pass
    outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
    outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
    outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
    outPipeline->uavBufferCount = shaderBlob.uavBufferCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    outPipeline->constCount = shaderBlob.cbvCount;
    outPipeline->rtCount = shaderBlob.rtTextureCount;

    outPipeline->descriptorSetCount = numDescriptorSets;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (FfxmUInt32 srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
    {
        outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindSet = shaderBlob.boundSRVTextureSets[srvIndex];
        wcscpy(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
    }
    for (FfxmUInt32 srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
    {
        outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindSet = shaderBlob.boundSRVBufferSets[srvIndex];
        wcscpy(outPipeline->srvBufferBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
    }
    for (FfxmUInt32 uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
    {
        outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindSet = shaderBlob.boundUAVTextureSets[uavIndex];
        wcscpy(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
    }
    for (FfxmUInt32 uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
    {
        outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindSet = shaderBlob.boundUAVBufferSets[uavIndex];
        wcscpy(outPipeline->uavBufferBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
    }
	for(FfxmUInt32 rtIndex = 0; rtIndex < outPipeline->rtCount; ++rtIndex)
	{
		outPipeline->rtBindings[rtIndex].slotIndex = shaderBlob.boundRTTextures[rtIndex];
		wcscpy(outPipeline->rtBindings[rtIndex].name, converter.from_bytes(shaderBlob.boundRTTextureNames[rtIndex]).c_str());
	}
    for (FfxmUInt32 cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindSet = shaderBlob.boundConstantBufferSets[cbIndex];
        wcscpy(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
    }

    //////////////////////////////////////////////////////////////////////////
    // pipeline creation
    FfxmDeviceCapabilities capabilities;
    backendInterface->fpGetDeviceCapabilities(backendInterface, &capabilities);

    // shader module
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (FfxmUInt32*)shaderBlob.data;
    shaderModuleCreateInfo.codeSize = shaderBlob.size;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &pPipelineLayout->fragShaderModule) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (FfxmUInt32*)vertShaderBlob.data;
    shaderModuleCreateInfo.codeSize = vertShaderBlob.size;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &pPipelineLayout->vertShaderModule) != VK_SUCCESS) {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    // Defer renderpass, framebuffer, graphics pipeline creation to dispatch
    return FFXM_OK;
}

template<size_t SIZE, typename T> int8_t findObject(T (&objs)[SIZE], uint64_t hash)
{
    int8_t idx = -1;

    for (int8_t i = 0; i < SIZE; i++)
    {
        objs[i].visitedFlag++;

        if (objs[i].hash == hash)
        {
            objs[i].visitedFlag = 0;
            idx = i;
        }
    }

    return idx;
}

template<size_t SIZE, typename T> uint8_t getLRUIndex(T (&objs)[SIZE])
{
    FfxmUInt32 max = 0;
    uint8_t idx = 0;

    for (uint8_t i = 0; i < SIZE; i++)
    {
        if (objs[i].visitedFlag > max)
        {
            max = objs[i].visitedFlag;
            idx = i;
        }
    }

    objs[idx].visitedFlag = 0;

    return idx;
}

FfxmErrorCode getOrCreateFrameBuffer(BackendContext_VK* backendContext, FfxmGpuJobDescription* job)
{
    FFXM_ASSERT(NULL != backendContext);
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->fragmentJobDescription.pipeline->rootSignature);
    FfxmPipelineState* pipeline = job->fragmentJobDescription.pipeline;

    std::array<VkImageView, FFXM_MAX_NUM_RTS> attachments;
    for(FfxmUInt32 rtIndex = 0; rtIndex < pipeline->rtCount; ++rtIndex)
    {
        const FfxmUInt32 resourceIndex = job->fragmentJobDescription.rtTextures[rtIndex].internalIndex;
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = backendContext->pResources[resourceIndex].imageResource;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = (backendContext->pResources[resourceIndex].resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET)
                            ? VK_FORMAT_D32_SFLOAT : ffxmGetVKSurfaceFormatFromSurfaceFormat(backendContext->pResources[resourceIndex].resourceDescription.format);
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = (backendContext->pResources[resourceIndex].resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET)
                                                 ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        uint64_t hash = computeHash(&createInfo, sizeof(createInfo));

        int8_t idx = findObject(pipelineLayout->imageView, hash);

        // find image view
        if (idx != -1)
        {
            pipelineLayout->imageViewIndex = idx;
        }
        else
        {
            // not find, get lru index in the array
            pipelineLayout->imageViewIndex = getLRUIndex(pipelineLayout->imageView);

            if (pipelineLayout->imageView[pipelineLayout->imageViewIndex].handle != VK_NULL_HANDLE)
            {
                backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, pipelineLayout->imageView[pipelineLayout->imageViewIndex].handle, VK_NULL_HANDLE);
            }

            if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &createInfo, nullptr, &pipelineLayout->imageView[pipelineLayout->imageViewIndex].handle) != VK_SUCCESS)
            {
                return FFXM_ERROR_BACKEND_API_ERROR;
            }

            pipelineLayout->imageView[pipelineLayout->imageViewIndex].hash = hash;
        }

        attachments[rtIndex] = pipelineLayout->imageView[pipelineLayout->imageViewIndex].handle;
    }

    uint64_t hash = computeHash(attachments.data(), pipeline->rtCount*sizeof(attachments[0]));

    VkFramebufferCreateInfo fbufCreateInfo = { } ;
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle;
    fbufCreateInfo.attachmentCount = pipeline->rtCount;
    fbufCreateInfo.width = job->fragmentJobDescription.viewport[0];
    fbufCreateInfo.height = job->fragmentJobDescription.viewport[1];
    fbufCreateInfo.layers = 1;

    hash = appendHash(&fbufCreateInfo, sizeof(fbufCreateInfo), hash);

    fbufCreateInfo.pAttachments = attachments.data();

    int8_t idx = findObject(pipelineLayout->frameBuffer, hash);

    // find frame buffer
    if (idx != -1)
    {
        pipelineLayout->frameBufferIndex = idx;
    }
    else
    {
        // not find, get lru index in the array
        pipelineLayout->frameBufferIndex = getLRUIndex(pipelineLayout->frameBuffer);

        if (pipelineLayout->frameBuffer[pipelineLayout->frameBufferIndex].handle != VK_NULL_HANDLE)
        {
            backendContext->vkFunctionTable.vkDestroyFramebuffer(backendContext->device, pipelineLayout->frameBuffer[pipelineLayout->frameBufferIndex].handle, VK_NULL_HANDLE);
        }

        if (backendContext->vkFunctionTable.vkCreateFramebuffer(backendContext->device, &fbufCreateInfo, nullptr, &pipelineLayout->frameBuffer[pipelineLayout->frameBufferIndex].handle) != VK_SUCCESS)
        {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

        pipelineLayout->frameBuffer[pipelineLayout->frameBufferIndex].hash = hash;
    }

    return FFXM_OK;
}

FfxmErrorCode getOrCreateRenderPass(BackendContext_VK* backendContext, FfxmGpuJobDescription* job)
{
    FFXM_ASSERT(NULL != backendContext);
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->fragmentJobDescription.pipeline->rootSignature);
    FfxmPipelineState* pipeline = job->fragmentJobDescription.pipeline;

    std::array<VkAttachmentDescription, FFXM_MAX_NUM_RTS> attachmentDescriptions;
    std::array<VkAttachmentReference, FFXM_MAX_NUM_RTS> colorAttachmentReferences;

    for(FfxmUInt32 rtIndex = 0; rtIndex < pipeline->rtCount; ++rtIndex)
    {
        const FfxmUInt32 resourceIndex = job->fragmentJobDescription.rtTextures[rtIndex].internalIndex;
        VkAttachmentDescription attachmentDescription = { };
        attachmentDescription.format = (backendContext->pResources[resourceIndex].resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET)
                                        ? VK_FORMAT_D32_SFLOAT : ffxmGetVKSurfaceFormatFromSurfaceFormat(backendContext->pResources[resourceIndex].resourceDescription.format);
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentDescriptions[rtIndex] = attachmentDescription;

        VkAttachmentReference colorAttachmentReference = { };
        colorAttachmentReference.attachment = rtIndex;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachmentReferences[rtIndex] = colorAttachmentReference;
    }

    uint64_t hash = computeHash(attachmentDescriptions.data(), pipeline->rtCount*sizeof(attachmentDescriptions[0]));
    hash = appendHash(colorAttachmentReferences.data(), pipeline->rtCount*sizeof(colorAttachmentReferences[0]), hash);

    // Note: this is a description of how the attachments of the render pass will be used in this sub pass
    // e.g. if they will be read in shaders and/or drawn to
    VkSubpassDescription subPassDescription = {};
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.colorAttachmentCount = pipeline->rtCount;
    subPassDescription.pColorAttachments = colorAttachmentReferences.data();

    // Create the render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = pipeline->rtCount;
    renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subPassDescription;

    int8_t idx = findObject(pipelineLayout->renderPass, hash);

    // find render pass
    if (idx != -1)
    {
        pipelineLayout->renderPassIndex = idx;
    }
    else
    {
        // not find, get lru index in the array
        pipelineLayout->renderPassIndex = getLRUIndex(pipelineLayout->renderPass);

        if (pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle != VK_NULL_HANDLE)
        {
            backendContext->vkFunctionTable.vkDestroyRenderPass(backendContext->device, pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle, VK_NULL_HANDLE);
        }

        if (backendContext->vkFunctionTable.vkCreateRenderPass(backendContext->device, &renderPassCreateInfo, nullptr, &pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle) != VK_SUCCESS)
        {
            return FFXM_ERROR_BACKEND_API_ERROR;
        }

        pipelineLayout->renderPass[pipelineLayout->renderPassIndex].hash = hash;
    }

    return FFXM_OK;
}

FfxmErrorCode getOrCreateGraphicsPipeline(BackendContext_VK* backendContext, FfxmGpuJobDescription* job)
{
    FFXM_ASSERT(NULL != backendContext);
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->fragmentJobDescription.pipeline->rootSignature);
    FfxmPipelineState* pipeline = job->fragmentJobDescription.pipeline;

    // pipeline only depends on render pass, so compute hash first
    uint64_t hash = computeHash(&pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle, sizeof(pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle));

    int8_t idx = findObject(pipelineLayout->graphicsPipeline, hash);

    // find graphics pipeline
    if (idx != -1)
    {
        pipelineLayout->graphicsPipelineIndex = idx;

        // set the pipeline
        pipeline->pipeline = reinterpret_cast<FfxmPipeline>(pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].handle);

        return FFXM_OK;
    }

    // fill out shader stage create info
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.pName = "main";
    fragShaderStageCreateInfo.module = pipelineLayout->fragShaderModule;

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCreateInfo.pName = "main";
    vertShaderStageCreateInfo.module = pipelineLayout->vertShaderModule;

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    // Describe vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Describe input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.scissorCount = 1;

    VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicState;

    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f; // 0.f
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    std::array<VkPipelineColorBlendAttachmentState, FFXM_MAX_NUM_RTS> colorBlendAttachmentStates;
    for(FfxmUInt32 rtIndex = 0; rtIndex < pipeline->rtCount; ++rtIndex)
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachmentStates[rtIndex] = colorBlendAttachmentState;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = pipeline->rtCount;
    colorBlendCreateInfo.pAttachments = colorBlendAttachmentStates.data();
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout->pipelineLayout;
    pipelineCreateInfo.renderPass = pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    // not find, get lru index in the array
    pipelineLayout->graphicsPipelineIndex = getLRUIndex(pipelineLayout->graphicsPipeline);

    if (pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].handle != VK_NULL_HANDLE)
    {
        backendContext->vkFunctionTable.vkDestroyPipeline(backendContext->device, pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].handle, VK_NULL_HANDLE);
    }

    if (backendContext->vkFunctionTable.vkCreateGraphicsPipelines(backendContext->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].handle) != VK_SUCCESS)
    {
        return FFXM_ERROR_BACKEND_API_ERROR;
    }

    pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].hash = hash;

    // set the pipeline
    pipeline->pipeline = reinterpret_cast<FfxmPipeline>(pipelineLayout->graphicsPipeline[pipelineLayout->graphicsPipelineIndex].handle);

    return FFXM_OK;
}

FfxmErrorCode DestroyPipelineVK(FfxmInterface* backendInterface, FfxmPipelineState* pipeline, [[maybe_unused]] FfxmUInt32 effectContextId)
{
    FFXM_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    if (!pipeline)
        return FFXM_OK;

    // Destroy the pipeline
    VkPipeline vkPipeline = reinterpret_cast<VkPipeline>(pipeline->pipeline);

    BackendContext_VK::PipelineLayout* pPipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(pipeline->rootSignature);

    if (vkPipeline != VK_NULL_HANDLE && vkPipeline != pPipelineLayout->graphicsPipeline[pPipelineLayout->graphicsPipelineIndex].handle) {
        backendContext->vkFunctionTable.vkDestroyPipeline(backendContext->device, vkPipeline, VK_NULL_HANDLE);
        pipeline->pipeline = VK_NULL_HANDLE;
    }

    // Zero out the cmd signature
    pipeline->cmdSignature = nullptr;

    // Destroy the pipeline layout
    if (pPipelineLayout)
    {
        // Descriptor set layout
        if (pPipelineLayout->pipelineLayout != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyPipelineLayout(backendContext->device, pPipelineLayout->pipelineLayout, VK_NULL_HANDLE);
            pPipelineLayout->pipelineLayout = VK_NULL_HANDLE;
        }

        // Descriptor sets
        for (FfxmUInt32 i = 0; i < FFXM_MAX_QUEUED_FRAMES; i++)
            for (FfxmUInt32 j = 0; j < MAX_DESCRIPTOR_SETS; j++)
                pPipelineLayout->descriptorSets[i][j] = VK_NULL_HANDLE;

        // Descriptor set layout
        for (FfxmUInt32 i = 0; i < MAX_DESCRIPTOR_SETS; i++)
        {
            if (pPipelineLayout->descriptorSetLayout[i] != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(backendContext->device, pPipelineLayout->descriptorSetLayout[i], VK_NULL_HANDLE);
                pPipelineLayout->descriptorSetLayout[i] = VK_NULL_HANDLE;
            }
        }

        // Samplers
        for (FfxmUInt32 currentSamplerIndex = 0; currentSamplerIndex < FFXM_MAX_SAMPLERS; ++currentSamplerIndex)
        {
            if (pPipelineLayout->samplers[currentSamplerIndex] != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroySampler(backendContext->device, pPipelineLayout->samplers[currentSamplerIndex], VK_NULL_HANDLE);
                pPipelineLayout->samplers[currentSamplerIndex] = VK_NULL_HANDLE;
            }
        }

        for (FfxmUInt32 i = 0; i < MAX_IMAGE_VIEW_COUNT; i++)
        {
            if (pPipelineLayout->imageView[i].handle != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, pPipelineLayout->imageView[i].handle, VK_NULL_HANDLE);
            }
        }

        for (FfxmUInt32 i = 0; i < MAX_FRAME_BUFFER_COUNT; i++)
        {
            if (pPipelineLayout->frameBuffer[i].handle != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroyFramebuffer(backendContext->device, pPipelineLayout->frameBuffer[i].handle, VK_NULL_HANDLE);
            }
        }

        for (FfxmUInt32 i = 0; i < MAX_RENDER_PASS_COUNT; i++)
        {
            if (pPipelineLayout->renderPass[i].handle != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroyRenderPass(backendContext->device, pPipelineLayout->renderPass[i].handle, VK_NULL_HANDLE);
            }
        }

        for (FfxmUInt32 i = 0; i < MAX_GRAPHICS_PIPELINE_COUNT; i++)
        {
            if (pPipelineLayout->graphicsPipeline[i].handle != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroyPipeline(backendContext->device, pPipelineLayout->graphicsPipeline[i].handle, VK_NULL_HANDLE);
            }
        }

        if (pPipelineLayout->fragShaderModule != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyShaderModule(backendContext->device, pPipelineLayout->fragShaderModule, nullptr);
        }

        if (pPipelineLayout->vertShaderModule != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyShaderModule(backendContext->device, pPipelineLayout->vertShaderModule, nullptr);
        }
    }

    return FFXM_OK;
}

FfxmErrorCode ScheduleGpuJobVK(FfxmInterface* backendInterface, const FfxmGpuJobDescription* job)
{
    FFXM_ASSERT(NULL != backendInterface);
    FFXM_ASSERT(NULL != job);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFXM_ASSERT(backendContext->gpuJobCount < FFXM_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;

    if (job->jobType == FFXM_GPU_JOB_COMPUTE) {

        // needs to copy SRVs and UAVs in case they are on the stack only
        FfxmComputeJobDescription* computeJob = &backendContext->pGpuJobs[backendContext->gpuJobCount].computeJobDescriptor;
        const FfxmUInt32 numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
        for (FfxmUInt32 currentRootConstantIndex = 0; currentRootConstantIndex < numConstBuffers; ++currentRootConstantIndex)
        {
            computeJob->cbs[currentRootConstantIndex].num32BitEntries = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries;
            memcpy(computeJob->cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, computeJob->cbs[currentRootConstantIndex].num32BitEntries * sizeof(FfxmUInt32));
        }
    }

	if(job->jobType == FFXM_GPU_JOB_FRAGMENT)
	{

		// needs to copy SRVs and UAVs in case they are on the stack only
		FfxmFragmentJobDescription* fragmentJob = &backendContext->pGpuJobs[backendContext->gpuJobCount].fragmentJobDescription;
		const FfxmUInt32 numConstBuffers = job->fragmentJobDescription.pipeline->constCount;
		for(FfxmUInt32 currentRootConstantIndex = 0; currentRootConstantIndex < numConstBuffers; ++currentRootConstantIndex)
		{
			fragmentJob->cbs[currentRootConstantIndex].num32BitEntries = job->fragmentJobDescription.cbs[currentRootConstantIndex].num32BitEntries;
			memcpy(fragmentJob->cbs[currentRootConstantIndex].data, job->fragmentJobDescription.cbs[currentRootConstantIndex].data,
				   fragmentJob->cbs[currentRootConstantIndex].num32BitEntries * sizeof(FfxmUInt32));
		}
	}

    backendContext->gpuJobCount++;

    return FFXM_OK;
}

static FfxmErrorCode executeGpuJobCompute(BackendContext_VK* backendContext, FfxmGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->computeJobDescriptor.pipeline.rootSignature);

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    FfxmUInt32               descriptorWriteIndex = 0;
    VkWriteDescriptorSet   writeDescriptorSets[FFXM_MAX_RESOURCE_COUNT];

    // These MUST be initialized
    FfxmUInt32               imageDescriptorIndex = 0;
    VkDescriptorImageInfo  imageDescriptorInfos[FFXM_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFXM_MAX_RESOURCE_COUNT; ++i)
        imageDescriptorInfos[i] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    // These MUST be initialized
    FfxmUInt32               bufferDescriptorIndex = 0;
    VkDescriptorBufferInfo bufferDescriptorInfos[FFXM_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFXM_MAX_RESOURCE_COUNT; ++i)
        bufferDescriptorInfos[i] = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    // bind texture UAVs
    for (FfxmUInt32 currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.uavTextures[currentPipelineUavIndex], FFXM_RESOURCE_STATE_UNORDERED_ACCESS);

        // where to bind it
        const FfxmUInt32 currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex;

        for (FfxmUInt32 uavEntry = 0; uavEntry < job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount; ++uavEntry, ++imageDescriptorIndex, ++descriptorWriteIndex)
        {
            // source: UAV of resource to bind
            const FfxmUInt32 resourceIndex = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].internalIndex;
            const FfxmUInt32 uavViewIndex = backendContext->pResources[resourceIndex].uavViewIndex + job->computeJobDescriptor.uavTextureMips[imageDescriptorIndex];

            writeDescriptorSets[descriptorWriteIndex] = {};
            writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindSet];
            writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
            writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[descriptorWriteIndex].pImageInfo = &imageDescriptorInfos[imageDescriptorIndex];
            writeDescriptorSets[descriptorWriteIndex].dstBinding = currentUavResourceIndex;
            writeDescriptorSets[descriptorWriteIndex].dstArrayElement = uavEntry;

            imageDescriptorInfos[imageDescriptorIndex] = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageDescriptorInfos[imageDescriptorIndex].imageView = backendContext->pResourceViews[uavViewIndex].imageView;
        }
    }

    // bind buffer UAVs
    for (FfxmUInt32 currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex) {

        addBarrier(backendContext, &job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex], FFXM_RESOURCE_STATE_UNORDERED_ACCESS);

        // source: UAV of buffer to bind
        const FfxmUInt32 resourceIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].internalIndex;

        // where to bind it
        const FfxmUInt32 currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding = currentUavResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex] = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range = VK_WHOLE_SIZE;
    }

    // bind texture SRVs
    for (FfxmUInt32 currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount;
         ++currentPipelineSrvIndex, ++descriptorWriteIndex)
    {

        // where to bind it
        const FfxmUInt32 currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = currentSrvResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        for(FfxmUInt32 i = 0; i < job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount; ++i, ++imageDescriptorIndex)
        {
            addBarrier(backendContext, &job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex + i], FFXM_RESOURCE_STATE_COMPUTE_READ);

            const FfxmUInt32 resourceIndex = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex + i].internalIndex;
            const FfxmUInt32 srvViewIndex  = backendContext->pResources[resourceIndex].srvViewIndex;

            imageDescriptorInfos[imageDescriptorIndex]             = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[srvViewIndex].imageView;
        }
    }

    // bind buffer SRVs
    for (FfxmUInt32 currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount;
         ++currentPipelineSrvIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex], FFXM_RESOURCE_STATE_COMPUTE_READ);

        // source: SRV of buffer to bind
        const FfxmUInt32 resourceIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].internalIndex;

        // where to bind it
        const FfxmUInt32 currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = currentSrvResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex]        = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = VK_WHOLE_SIZE;
    }

    // update uniform buffers
    for (FfxmUInt32 currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex)
    {
        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->computeJobDescriptor.pipeline.constantBufferBindings[currentRootConstantIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding = job->computeJobDescriptor.pipeline.constantBufferBindings[currentRootConstantIndex].slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        // the ubo ring buffer is pre-populated with VkBuffer objects of FFXM_BUFFER_SIZE-bytes to prevent creating buffers at runtime
        FfxmUInt32 dataSize = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(FfxmUInt32);
        FFXM_ASSERT(dataSize <= FFXM_MAX_CONST_SIZE * sizeof(FfxmUInt32));

        BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[backendContext->ringBufferBase];

        bufferDescriptorInfos[bufferDescriptorIndex].buffer = uBuffer.bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range = dataSize;

        if (job->computeJobDescriptor.cbs[currentRootConstantIndex].data != nullptr)
        {
            memcpy(uBuffer.pData, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);

            // flush mapped range if memory type is not coherent
            if ((backendContext->ringBufferMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange;
                memset(&memoryRange, 0, sizeof(memoryRange));

                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendContext->ringBufferMemory;
                memoryRange.offset = FFXM_BUFFER_SIZE * backendContext->ringBufferBase;
                memoryRange.size = dataSize;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }
        }

        // Increment the base pointer in the ring buffer
        ++backendContext->ringBufferBase;
        if (backendContext->ringBufferBase >= FFXM_RING_BUFFER_SIZE * s_MaxEffectContexts)
            backendContext->ringBufferBase = 0;
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFXM_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    // insert all the barriers
    flushBarriers(backendContext, vkCommandBuffer);

    // update all uavs and srvs
    backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, descriptorWriteIndex, writeDescriptorSets, 0, nullptr);

    // bind pipeline
    backendContext->vkFunctionTable.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reinterpret_cast<VkPipeline>(job->computeJobDescriptor.pipeline.pipeline));

    // bind descriptor sets
    backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, 0, job->computeJobDescriptor.pipeline.descriptorSetCount, pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex], 0, nullptr);

    // Dispatch (or dispatch indirect)
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        const FfxmUInt32 resourceIndex = job->computeJobDescriptor.cmdArgument.internalIndex;
        VkBuffer buffer = backendContext->pResources[resourceIndex].bufferResource;
        backendContext->vkFunctionTable.vkCmdDispatchIndirect(vkCommandBuffer, buffer, job->computeJobDescriptor.cmdArgumentOffset);
    }
    else
    {
        backendContext->vkFunctionTable.vkCmdDispatch(vkCommandBuffer, job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    }

    // move to another descriptor set for the next compute render job so that we don't overwrite descriptors in-use
    ++pipelineLayout->descriptorSetIndex;
    if (pipelineLayout->descriptorSetIndex >= FFXM_MAX_QUEUED_FRAMES)
        pipelineLayout->descriptorSetIndex = 0;

    return FFXM_OK;
}

static FfxmErrorCode executeGpuJobFragment(BackendContext_VK* backendContext, FfxmGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->fragmentJobDescription.pipeline->rootSignature);

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    FfxmUInt32               descriptorWriteIndex = 0;
    VkWriteDescriptorSet   writeDescriptorSets[FFXM_MAX_RESOURCE_COUNT];

    // These MUST be initialized
    FfxmUInt32               imageDescriptorIndex = 0;
    VkDescriptorImageInfo  imageDescriptorInfos[FFXM_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFXM_MAX_RESOURCE_COUNT; ++i)
        imageDescriptorInfos[i] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    // These MUST be initialized
    FfxmUInt32               bufferDescriptorIndex = 0;
    VkDescriptorBufferInfo bufferDescriptorInfos[FFXM_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFXM_MAX_RESOURCE_COUNT; ++i)
        bufferDescriptorInfos[i] = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    // bind texture UAVs
    for (FfxmUInt32 currentPipelineUavIndex = 0; currentPipelineUavIndex < job->fragmentJobDescription.pipeline->uavTextureCount; ++currentPipelineUavIndex)
    {
        addBarrier(backendContext, &job->fragmentJobDescription.uavTextures[currentPipelineUavIndex], FFXM_RESOURCE_STATE_UNORDERED_ACCESS);

        // where to bind it
        const FfxmUInt32 currentUavResourceIndex = job->fragmentJobDescription.pipeline->uavTextureBindings[currentPipelineUavIndex].slotIndex;

        for (FfxmUInt32 uavEntry = 0; uavEntry < job->fragmentJobDescription.pipeline->uavTextureBindings[currentPipelineUavIndex].bindCount; ++uavEntry, ++imageDescriptorIndex, ++descriptorWriteIndex)
        {
            // source: UAV of resource to bind
            const FfxmUInt32 resourceIndex = job->fragmentJobDescription.uavTextures[currentPipelineUavIndex].internalIndex;
            const FfxmUInt32 uavViewIndex = backendContext->pResources[resourceIndex].uavViewIndex + job->fragmentJobDescription.uavTextureMips[imageDescriptorIndex];

            writeDescriptorSets[descriptorWriteIndex] = {};
            writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->fragmentJobDescription.pipeline->uavTextureBindings[currentPipelineUavIndex].bindSet];
            writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
            writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[descriptorWriteIndex].pImageInfo = &imageDescriptorInfos[imageDescriptorIndex];
            writeDescriptorSets[descriptorWriteIndex].dstBinding = currentUavResourceIndex;
            writeDescriptorSets[descriptorWriteIndex].dstArrayElement = uavEntry;

            imageDescriptorInfos[imageDescriptorIndex] = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageDescriptorInfos[imageDescriptorIndex].imageView = backendContext->pResourceViews[uavViewIndex].imageView;
        }
    }

    // bind texture SRVs
    for (FfxmUInt32 currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->fragmentJobDescription.pipeline->srvTextureCount;
         ++currentPipelineSrvIndex, ++descriptorWriteIndex)
    {

        // where to bind it
        const FfxmUInt32 currentSrvResourceIndex = job->fragmentJobDescription.pipeline->srvTextureBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->fragmentJobDescription.pipeline->srvTextureBindings[currentPipelineSrvIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = job->fragmentJobDescription.pipeline->srvTextureBindings[currentPipelineSrvIndex].bindCount;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = currentSrvResourceIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        for(FfxmUInt32 i = 0; i < job->fragmentJobDescription.pipeline->srvTextureBindings[currentPipelineSrvIndex].bindCount; ++i, ++imageDescriptorIndex)
        {
            addBarrier(backendContext, &job->fragmentJobDescription.srvTextures[currentPipelineSrvIndex + i], FFXM_RESOURCE_STATE_PIXEL_READ);

            const FfxmUInt32 resourceIndex = job->fragmentJobDescription.srvTextures[currentPipelineSrvIndex + i].internalIndex;
            const FfxmUInt32 srvViewIndex  = backendContext->pResources[resourceIndex].srvViewIndex;

            imageDescriptorInfos[imageDescriptorIndex]             = {};
            imageDescriptorInfos[imageDescriptorIndex].imageLayout = backendContext->pResources[resourceIndex].resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : getVKImageLayoutFromResourceState(backendContext->pResources[resourceIndex].currentState);
            imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[srvViewIndex].imageView;
        }
    }

    // update uniform buffers
    for (FfxmUInt32 currentRootConstantIndex = 0; currentRootConstantIndex < job->fragmentJobDescription.pipeline->constCount; ++currentRootConstantIndex, ++bufferDescriptorIndex, ++descriptorWriteIndex)
    {
        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex][job->fragmentJobDescription.pipeline->constantBufferBindings[currentRootConstantIndex].bindSet];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding = job->fragmentJobDescription.pipeline->constantBufferBindings[currentRootConstantIndex].slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        // the ubo ring buffer is pre-populated with VkBuffer objects of FFXM_BUFFER_SIZE-bytes to prevent creating buffers at runtime
        FfxmUInt32 dataSize = job->fragmentJobDescription.cbs[currentRootConstantIndex].num32BitEntries * sizeof(FfxmUInt32);
        FFXM_ASSERT(dataSize <= FFXM_MAX_CONST_SIZE * sizeof(FfxmUInt32));

        BackendContext_VK::UniformBuffer& uBuffer = backendContext->pRingBuffer[backendContext->ringBufferBase];

        bufferDescriptorInfos[bufferDescriptorIndex].buffer = uBuffer.bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = 0;
        bufferDescriptorInfos[bufferDescriptorIndex].range = dataSize;

        if (job->fragmentJobDescription.cbs[currentRootConstantIndex].data)
        {
            memcpy(uBuffer.pData, job->fragmentJobDescription.cbs[currentRootConstantIndex].data, dataSize);

            // flush mapped range if memory type is not coherent
            if ((backendContext->ringBufferMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange;
                memset(&memoryRange, 0, sizeof(memoryRange));

                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendContext->ringBufferMemory;
                memoryRange.offset = FFXM_BUFFER_SIZE * backendContext->ringBufferBase;
                memoryRange.size = dataSize;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }
        }

        // Increment the base pointer in the ring buffer
        ++backendContext->ringBufferBase;
        if (backendContext->ringBufferBase >= FFXM_RING_BUFFER_SIZE * s_MaxEffectContexts)
            backendContext->ringBufferBase = 0;
    }

	// Tansit RTs
	for(FfxmUInt32 rt = 0; rt < job->fragmentJobDescription.pipeline->rtCount; ++rt)
	{
		addBarrier(backendContext, &job->fragmentJobDescription.rtTextures[rt], FFXM_RESOURCE_STATE_PIXEL_WRITE);
	}

    // insert all the barriers
    flushBarriers(backendContext, vkCommandBuffer);

    // update all uavs and srvs
    backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, descriptorWriteIndex, writeDescriptorSets, 0, nullptr);

    getOrCreateRenderPass(backendContext, job);

    getOrCreateFrameBuffer(backendContext, job);

    getOrCreateGraphicsPipeline(backendContext, job);

    VkClearValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    VkExtent2D extent = { job->fragmentJobDescription.viewport[0], job->fragmentJobDescription.viewport[1] };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = pipelineLayout->renderPass[pipelineLayout->renderPassIndex].handle;
    renderPassBeginInfo.framebuffer = pipelineLayout->frameBuffer[pipelineLayout->frameBufferIndex].handle;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = extent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    backendContext->vkFunctionTable.vkCmdBeginRenderPass(vkCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // bind pipeline
    backendContext->vkFunctionTable.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, reinterpret_cast<VkPipeline>(job->fragmentJobDescription.pipeline->pipeline));

    // bind descriptor sets
    backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->pipelineLayout, 0, job->fragmentJobDescription.pipeline->descriptorSetCount, pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex], 0, nullptr);

    VkViewport viewport = { 0.0f, 0.0f, (float)job->fragmentJobDescription.viewport[0], (float)job->fragmentJobDescription.viewport[1] };
    backendContext->vkFunctionTable.vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { { 0, 0 }, { job->fragmentJobDescription.viewport[0], job->fragmentJobDescription.viewport[1] } };
    backendContext->vkFunctionTable.vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);

    backendContext->vkFunctionTable.vkCmdDraw(vkCommandBuffer, 3, 1, 0, 0);

    backendContext->vkFunctionTable.vkCmdEndRenderPass(vkCommandBuffer);

    // move to another descriptor set for the next compute render job so that we don't overwrite descriptors in-use
    ++pipelineLayout->descriptorSetIndex;
    if (pipelineLayout->descriptorSetIndex >= FFXM_MAX_QUEUED_FRAMES)
        pipelineLayout->descriptorSetIndex = 0;

    backendContext->pEffectContexts[pipelineLayout->effectContextId].isEndOfUpscaler = !!wcscmp(pipelineLayout->name, L"FSR2-GEN_REACTIVE");

    return FFXM_OK;
}

static FfxmErrorCode executeGpuJobCopy(BackendContext_VK* backendContext, FfxmGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::Resource ffxmResourceSrc = backendContext->pResources[job->copyJobDescriptor.src.internalIndex];
    BackendContext_VK::Resource ffxmResourceDst = backendContext->pResources[job->copyJobDescriptor.dst.internalIndex];

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFXM_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFXM_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, vkCommandBuffer);

    if (ffxmResourceSrc.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER && ffxmResourceDst.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxmResourceSrc.bufferResource;
        VkBuffer vkResourceDst = ffxmResourceDst.bufferResource;

        VkBufferCopy bufferCopy = {};

        bufferCopy.dstOffset = 0;
        bufferCopy.srcOffset = 0;
        bufferCopy.size = ffxmResourceSrc.resourceDescription.width;

        backendContext->vkFunctionTable.vkCmdCopyBuffer(vkCommandBuffer, vkResourceSrc, vkResourceDst, 1, &bufferCopy);
    }
    else if (ffxmResourceSrc.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER && ffxmResourceDst.resourceDescription.type != FFXM_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxmResourceSrc.bufferResource;
        VkImage vkResourceDst = ffxmResourceDst.imageResource;

        VkImageSubresourceLayers subresourceLayers = {};

        subresourceLayers.aspectMask = ((ffxmResourceDst.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) != 0) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceLayers.baseArrayLayer = 0;
        subresourceLayers.layerCount = 1;
        subresourceLayers.mipLevel = 0;

        VkOffset3D offset = {};

        offset.x = 0;
        offset.y = 0;
        offset.z = 0;

        VkExtent3D extent = {};

        extent.width = ffxmResourceDst.resourceDescription.width;
        extent.height = ffxmResourceDst.resourceDescription.height;
        extent.depth = ffxmResourceDst.resourceDescription.depth;

        VkBufferImageCopy bufferImageCopy = {};

        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource = subresourceLayers;
        bufferImageCopy.imageOffset = offset;
        bufferImageCopy.imageExtent = extent;

        backendContext->vkFunctionTable.vkCmdCopyBufferToImage(vkCommandBuffer, vkResourceSrc, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
    }
    else
    {
        bool isSrcDepth = ((ffxmResourceSrc.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) != 0);
        bool isDstDepth = ((ffxmResourceDst.resourceDescription.usage & FFXM_RESOURCE_USAGE_DEPTHTARGET) != 0);
        FFXM_ASSERT_MESSAGE(isSrcDepth == isDstDepth, "Copy operations aren't allowed between depth and color textures in the vulkan backend of the FFX SDK.");

        #define FFXM_MAX_IMAGE_COPY_MIPS 14 // Will handle 4k down to 1x1
        VkImageCopy             imageCopies[FFXM_MAX_IMAGE_COPY_MIPS];
        VkImage vkResourceSrc = ffxmResourceSrc.imageResource;
        VkImage vkResourceDst = ffxmResourceDst.imageResource;

        FfxmUInt32 numMipsToCopy = FFXM_MINIMUM(ffxmResourceSrc.resourceDescription.mipCount, ffxmResourceDst.resourceDescription.mipCount);

        for (FfxmUInt32 mip = 0; mip < numMipsToCopy; mip++)
        {
            VkImageSubresourceLayers srcSubresourceLayers = {};

            srcSubresourceLayers.aspectMask     = isSrcDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            srcSubresourceLayers.baseArrayLayer = 0;
            srcSubresourceLayers.layerCount     = 1;
            srcSubresourceLayers.mipLevel       = mip;

            VkImageSubresourceLayers dstSubresourceLayers = {};

            dstSubresourceLayers.aspectMask     = isDstDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            dstSubresourceLayers.baseArrayLayer = 0;
            dstSubresourceLayers.layerCount     = 1;
            dstSubresourceLayers.mipLevel       = mip;

            VkOffset3D offset = {};

            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};

            extent.width = ffxmResourceSrc.resourceDescription.width / (mip + 1);
            extent.height = ffxmResourceSrc.resourceDescription.height / (mip + 1);
            extent.depth = ffxmResourceSrc.resourceDescription.depth / (mip + 1);

            VkImageCopy& copyRegion = imageCopies[mip];

            copyRegion.srcSubresource = srcSubresourceLayers;
            copyRegion.srcOffset = offset;
            copyRegion.dstSubresource = dstSubresourceLayers;
            copyRegion.dstOffset = offset;
            copyRegion.extent = extent;
        }

        backendContext->vkFunctionTable.vkCmdCopyImage(vkCommandBuffer, vkResourceSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMipsToCopy, imageCopies);
    }

    return FFXM_OK;
}

static FfxmErrorCode executeGpuJobClearFloat(BackendContext_VK* backendContext, FfxmGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    FfxmUInt32 idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_VK::Resource ffxmResource = backendContext->pResources[idx];

    if (ffxmResource.resourceDescription.type == FFXM_RESOURCE_TYPE_BUFFER)
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFXM_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkBuffer vkResource = ffxmResource.bufferResource;

        backendContext->vkFunctionTable.vkCmdFillBuffer(vkCommandBuffer, vkResource, 0, VK_WHOLE_SIZE, (FfxmUInt32)job->clearJobDescriptor.color[0]);
    }
    else
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFXM_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkImage vkResource = ffxmResource.imageResource;

        VkClearColorValue clearColorValue = {};

        clearColorValue.float32[0] = job->clearJobDescriptor.color[0];
        clearColorValue.float32[1] = job->clearJobDescriptor.color[1];
        clearColorValue.float32[2] = job->clearJobDescriptor.color[2];
        clearColorValue.float32[3] = job->clearJobDescriptor.color[3];

        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = ffxmResource.resourceDescription.mipCount;
        range.baseArrayLayer = 0;
        range.layerCount = ffxmResource.resourceDescription.depth;

        backendContext->vkFunctionTable.vkCmdClearColorImage(vkCommandBuffer, vkResource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &range);
    }

    return FFXM_OK;
}

FfxmErrorCode ExecuteGpuJobsVK(FfxmInterface* backendInterface, FfxmCommandList commandList)
{
    FFXM_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxmErrorCode errorCode = FFXM_OK;

    // execute all renderjobs
    for (FfxmUInt32 i = 0; i < backendContext->gpuJobCount; ++i)
    {
        FfxmGpuJobDescription* gpuJob = &backendContext->pGpuJobs[i];
        VkCommandBuffer vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList);

        switch (gpuJob->jobType)
        {
        case FFXM_GPU_JOB_CLEAR_FLOAT:
        {
            errorCode = executeGpuJobClearFloat(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFXM_GPU_JOB_COPY:
        {
            errorCode = executeGpuJobCopy(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFXM_GPU_JOB_COMPUTE:
        {
            errorCode = executeGpuJobCompute(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFXM_GPU_JOB_FRAGMENT:
        {
            errorCode = executeGpuJobFragment(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        default:;
        }
    }

    // check the execute function returned cleanly.
    FFXM_RETURN_ON_ERROR(
        errorCode == FFXM_OK,
        FFXM_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFXM_OK;
}

} // namespace arm
