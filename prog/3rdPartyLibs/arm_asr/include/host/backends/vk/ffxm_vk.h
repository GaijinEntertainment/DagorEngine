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

/// @defgroup VKBackend Vulkan Backend
/// FidelityFX SDK native backend implementation for Vulkan.
///
/// @ingroup Backends

#pragma once

#ifdef FFXM_VKLOADER_VOLK
#if defined(__linux__)
#	define VK_USE_PLATFORM_XCB_KHR 1
#	define VK_USE_PLATFORM_XLIB_KHR 1
#elif defined(_WIN32)
#	define VK_USE_PLATFORM_WIN32_KHR 1
#elif defined(__ANDROID__)
#	define VK_USE_PLATFORM_ANDROID_KHR 1
#else
#	error Not implemented
#endif
#include <Volk/volk.h>
#else // FFXM_VKLOADER_VOLK
#include <vulkan/vulkan.h>
#endif

#include <host/ffxm_interface.h>

namespace arm
{

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

/// Query how much memory is required for the Vulkan backend's scratch buffer.
///
/// @param [in] physicalDevice              A pointer to the VkPhysicalDevice device.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
/// @param [in] pfnEnumDeviceExtProps       Caller-provided vkEnumerateDeviceExtensionProperties pointer. Required when the host
///                                         loads Vulkan dynamically (no vulkan.h prototypes linked); used only for sizing.
///
/// @returns
/// The size (in bytes) of the required scratch memory buffer for the VK backend.
///
/// @ingroup VKBackend
FFXM_API size_t ffxmGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts, PFN_vkEnumerateDeviceExtensionProperties pfnEnumDeviceExtProps);

/// Convenience structure to hold all VK-related device information
typedef struct VkDeviceContext {
    VkDevice                  vkDevice;             /// The Vulkan device
    VkPhysicalDevice          vkPhysicalDevice;     /// The Vulkan physical device
    PFN_vkGetDeviceProcAddr   vkDeviceProcAddr;     /// The device's function address table
    VkInstance                vkInstance;           /// The Vulkan instance (optional, needed when vkGetInstanceProcAddr is set)
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;/// The instance proc-addr loader (optional, falls back to vkDeviceProcAddr when null)
} VkDeviceContext;

/// Create a <c><i>FfxmDevice</i></c> from a <c><i>VkDevice</i></c>.
///
/// @param [in] vkDeviceContext             A pointer to a VKDeviceContext that holds all needed information
///
/// @returns
/// An abstract FidelityFX device.
///
/// @ingroup VKBackend
FFXM_API FfxmDevice ffxmGetDeviceVK(VkDeviceContext* vkDeviceContext);

/// Populate an interface with pointers for the VK backend.
///
/// @param [out] backendInterface           A pointer to a <c><i>FfxmInterface</i></c> structure to populate with pointers.
/// @param [in] device                      A pointer to the VkDevice device.
/// @param [in] scratchBuffer               A pointer to a buffer of memory which can be used by the DirectX(R)12 backend.
/// @param [in] scratchBufferSize           The size (in bytes) of the buffer pointed to by <c><i>scratchBuffer</i></c>.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @retval
/// FFXM_OK                                  The operation completed successfully.
/// @retval
/// FFXM_ERROR_CODE_INVALID_POINTER          The <c><i>interface</i></c> pointer was <c><i>NULL</i></c>.
///
/// @ingroup VKBackend
FFXM_API FfxmErrorCode ffxmGetInterfaceVK(
    FfxmInterface* backendInterface,
    FfxmDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts);

/// Create a <c><i>FfxmCommandList</i></c> from a <c><i>VkCommandBuffer</i></c>.
///
/// @param [in] cmdBuf                      A pointer to the Vulkan command buffer.
///
/// @returns
/// An abstract FidelityFX command list.
///
/// @ingroup VKBackend
FFXM_API FfxmCommandList ffxmGetCommandListVK(VkCommandBuffer cmdBuf);

/// Fetch a <c><i>FfxmResource</i></c> from a <c><i>GPUResource</i></c>.
///
/// @param [in] vkResource                  A pointer to the (agnostic) VK resource.
/// @param [in] ffxmResDescription           An <c><i>FfxmResourceDescription</i></c> for the resource representation.
/// @param [in] ffxmResName                  (optional) A name string to identify the resource in debug mode.
/// @param [in] state                       The state the resource is currently in.
///
/// @returns
/// An abstract FidelityFX resources.
///
/// @ingroup VKBackend
FFXM_API FfxmResource ffxmGetResourceVK(void*  vkResource,
    FfxmResourceDescription                  ffxmResDescription,
    wchar_t*                                ffxmResName,
    FfxmResourceStates                       state = FFXM_RESOURCE_STATE_COMPUTE_READ);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)

} // namespace arm
