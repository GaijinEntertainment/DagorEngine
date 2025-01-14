//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_adjpow2.h>

// texture creation flags
enum
{
  // type of texture - exactly one must be specified:
  TEXCF_RGB = 0x00000000U,     // same as TEXFMT_A8R8G8B8
  TEXCF_RTARGET = 0x00000001U, // render target format
  TEXCF_TYPEMASK = 0x00000001U,
  // size/quality hints for type - no more than one must be specified:
  TEXCF_BEST = 0x00000000U,  // obsolete
  TEXCF_ABEST = 0x00000000U, // obsolete

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476335(v=vs.85).aspx#Unordered_Access
  TEXCF_UNORDERED = 0x00000002U, // for compute shaders

  // Indicates usage as VRS texture, enforces the following rules:
  // - Format must be TEXFMT_R8UI
  // - 2D Texture only
  // - no arrayed texture
  // - no mipmaps
  // - no multisampling
  // - no usage as render target (use UAV instead)
  // Rate to per pixel value is as follows:
  // rate 1 = 0
  // rate 2 = 1
  // rate 4 = 2
  // combined rate: rate x << 2 | rate y
  // Which maps to
  // 1x1 = 0
  // 1x2 = 1
  // 2x1 = 4
  // 2x2 = 5
  // 2x4 = 6
  // 4x2 = 9
  // 4x4 = 10
  TEXCF_VARIABLE_RATE = 0x00000004U,

  // Uses of the texture methods updateSubRegion and update require this, TEXCF_RTARGET or
  // TEXCF_UNORDERED usage flag to be set.
  TEXCF_UPDATE_DESTINATION = 0x00004000U,

  TEXCF_SYSTEXCOPY = 0x00000010U, // make copy in system memory

  // creation options

  TEXCF_DYNAMIC = 0x00000100U,  // changes frequently //D3DUSAGE_CPU_CACHED_MEMORY, XALLOC_MEMPROTECT_READWRITE
  TEXCF_READABLE = 0x00000200U, // can only be read, D3DLOCK_READONLY
  TEXCF_READONLY = 0,
  TEXCF_WRITEONLY = 0x00000008U,     // cpu can write (TEXLOCK_WRITE)
  TEXCF_LOADONCE = 0x00000400U,      // texture will be loaded only once - don't use with dynamic
  TEXCF_MAYBELOST = 0x00000800U,     // contents of the texture may be safely lost - they will be regenerated before using it
  TEXCF_STREAMING = 0x00000000U,     // should be deleted shortly, obsolete
  TEXCF_SYSMEM = 0x00010000U,        // texture is allocated in system memory and used only as staging texture
                                     // TEXCF_SYSMEM|TEXCF_WRITEONLY is allocated in WC memory on PS4
  TEXCF_SAMPLECOUNT_2 = 0x00020000U, //  =
  TEXCF_SAMPLECOUNT_4 = 0x00040000U, //  | multisampled render target formats
  TEXCF_SAMPLECOUNT_8 = 0x00060000U, //  =
  TEXCF_SAMPLECOUNT_MAX = TEXCF_SAMPLECOUNT_8,
  TEXCF_SAMPLECOUNT_MASK = 0x00060000U,
  TEXCF_SAMPLECOUNT_OFFSET = 17,

#if _TARGET_C1 | _TARGET_C2






#elif _TARGET_XBOX
  TEXCF_CPU_CACHED_MEMORY = 0x00008000U, // todo: implement allocation in onion instead of garlic mem
  TEXCF_LINEAR_LAYOUT = 0x00100000U,     // todo: implement without tiling
  TEXCF_ESRAM_ONLY = 0x00000020U,        // always reside in ESRAM
  TEXCF_MOVABLE_ESRAM = 0x00000040U,     // Create copy in DDR
  TEXCF_TC_COMPATIBLE = 0,
  TEXCF_RT_COMPRESSED = 0,
#else
  TEXCF_CPU_CACHED_MEMORY = 0,
  TEXCF_LINEAR_LAYOUT = 0,
  TEXCF_ESRAM_ONLY = 0, // make copy in system memory
  TEXCF_MOVABLE_ESRAM = 0,
  TEXCF_TC_COMPATIBLE = 0,
  TEXCF_RT_COMPRESSED = 0,
#endif

  // Indicates that a color render target or uav texture may be used by the 3d and async compute queue at the same time.
  // This directly maps to DX12's D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag and on Vulkan this maps directly
  // to VK_SHARING_MODE_CONCURRENT for all device queues used.
  // This flag removes the need to transfer ownership of the resources between execution queues
  // (eg. RB_FLAG_MOVE_PIPELINE_OWNERSHIP between different GpuPipeline).
  // Required to be paired with TEXCF_RTARGET and/or TEXCF_UNORDERED.
  // It is invalid to apply this flag to resources with depth/stencil formats.
  TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE = 0x00080000U,

  TEXCF_SRGBWRITE = 0x00200000U,    // perform srgb conversion when render to this texture. Works only on 8 bit 4 components textures
  TEXCF_SRGBREAD = 0x00400000U,     // perform srgb conversion when sample from texture
  TEXCF_GENERATEMIPS = 0x00800000U, // generateMips will be called
  TEXCF_CLEAR_ON_CREATE = 0X00002000U, // will fill texture with zeroes

  TEXCF_TILED_RESOURCE = 0x00000080U, // The texture is not backed by GPU memory. It needs to be backed manually by heaps.

  // Transient texture: content and access is valid only inside render pass
  // uses lazy memory allocation if possible (TBDR framebuffer/tile memory backing)
  TEXCF_TRANSIENT = 0x00001000U,

  // exact texture format
  TEXFMT_DEFAULT = 0U, // argb8!
  TEXFMT_A8R8G8B8 = 0x00000000U,
  TEXFMT_A2R10G10B10 = 0x01000000U,
  TEXFMT_A2B10G10R10 = 0x02000000U,
  TEXFMT_A16B16G16R16 = 0x03000000U, // unsigned
  TEXFMT_A16B16G16R16F = 0x04000000U,
  TEXFMT_A32B32G32R32F = 0x05000000U,
  TEXFMT_G16R16 = 0x06000000U,
  TEXFMT_G16R16F = 0x07000000U,
  TEXFMT_G32R32F = 0x08000000U,
  TEXFMT_R16F = 0x09000000U,
  TEXFMT_R32F = 0x0A000000U,
  TEXFMT_DXT1 = 0x0B000000U,
  TEXFMT_DXT3 = 0x0C000000U,
  TEXFMT_DXT5 = 0x0D000000U,
  TEXFMT_R32G32UI = 0x0E000000U,
  TEXFMT_L16 = 0x10000000U,
  TEXFMT_A8 = 0x11000000U,
  TEXFMT_R8 = 0x12000000U,
  TEXFMT_L8 = TEXFMT_R8, // legacy, to be removed
  TEXFMT_A1R5G5B5 = 0x13000000U,
  TEXFMT_A4R4G4B4 = 0x14000000U,
  TEXFMT_R5G6B5 = 0x15000000U,
  // 0x16000000U currently unused
  TEXFMT_A16B16G16R16S = 0x17000000U,  // signed, d3d11
  TEXFMT_A16B16G16R16UI = 0x18000000U, // unsigned, unorm
  TEXFMT_A32B32G32R32UI = 0x19000000U, // unsigned, unorm
  TEXFMT_ATI1N = 0x1A000000U,          // BC4 in dx10
  TEXFMT_ATI2N = 0x1B000000U,          // BC5 in dx10
  TEXFMT_R8G8B8A8 = 0x1C000000U,       // unorm
  TEXFMT_R32UI = 0x1D000000U,          // unsigned int
  TEXFMT_R32SI = 0x1E000000U,          // signed int
  TEXFMT_R11G11B10F = 0x1F000000U,     // float
  TEXFMT_R9G9B9E5 = 0x20000000U,
  TEXFMT_R8G8 = 0x21000000U,  // unorm, dx10+, OGL+
  TEXFMT_R8G8S = 0x22000000U, // snorm, dx10+, OGL+
  TEXFMT_BC6H = 0x23000000U,
  TEXFMT_BC7 = 0x24000000U,
  TEXFMT_R8UI = 0x25000000U,
  TEXFMT_R16UI = 0x26000000U,

  // should go in a row
  TEXFMT_DEPTH24 = 0x27000000U,
  TEXFMT_DEPTH16 = 0x28000000U,
  TEXFMT_DEPTH32 = 0x29000000U,
  TEXFMT_DEPTH32_S8 = 0x2A000000U,

  // mobile formats:
  TEXFMT_ASTC4 = 0x2E000000U,
  TEXFMT_ASTC8 = 0x2F000000U,
  TEXFMT_ASTC12 = 0x30000000U,

  TEXFMT_ETC2_RG = 0x31000000U,
  TEXFMT_ETC2_RGBA = 0x32000000U,

  TEXFMT_PSSR_TARGET = 0x33000000U, // PS5 only

  TEXFMT_FIRST_DEPTH = TEXFMT_DEPTH24,
  TEXFMT_LAST_DEPTH = TEXFMT_DEPTH32_S8,

  TEXFMT_MASK = 0xFF000000U, // TEXFMT_               =0x__000000,
};

__forceinline int get_sample_count(int flags) { return 1 << ((flags & TEXCF_SAMPLECOUNT_MASK) >> TEXCF_SAMPLECOUNT_OFFSET); }

__forceinline int make_sample_count_flag(int sample_count)
{
  return (get_log2i(sample_count) << TEXCF_SAMPLECOUNT_OFFSET) & TEXCF_SAMPLECOUNT_MASK;
}